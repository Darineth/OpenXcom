# Feature - Geoscape Activity Display

**Status:** Done — fully implemented. Global section shows economy warnings (negative balance, monthly deficit, maintenance spike). Per-base sections cover research, manufacturing, training, construction, craft maintenance/missions, stores, defense readiness, transfers, and wounded soldiers. Jun 2026.

## Overview

The Activity Display provides an at-a-glance overview of all ongoing operations across the player's bases directly on the Geoscape screen, eliminating the need to enter individual base screens just to check progress. It is a text-based overlay rendered on the left side of the Geoscape, showing research progress, manufacturing queues, craft maintenance status, and stored alien fuel — with idle personnel and crafts requiring attention highlighted in reversed colors.

This feature reads data already tracked by the engine (`Base` research/manufacturing progress, `Craft` maintenance timers, fuel stores). The implementation challenge is UI layout: fitting a multi-section text overlay on the Geoscape without obscuring the globe or existing sidebar widgets.

---

## What Already Exists (OXCE-Plus Base)

### Base data structures

| File | Class / Field | Role |
|------|---------------|------|
| `src/Savegame/Base.h` | `_research: std::vector<ResearchProject*>` | Active research projects at the base |
| `src/Savegame/Base.h` | `_productions: std::vector<Production*>` | Manufacturing queues |
| `src/Savegame/Base.h` | `_scientists`, `_engineers: int` | Count of idle vs assigned personnel (computed by `calculateServices`) |
| `src/Savegame/Base.h` | `_crafts: std::vector<Craft*>` | Crafts stationed at the base |

### ResearchProject (`src/Savegame/ResearchProject.*`)

- `_spent: int` — time already spent on project
- `_cost: int` — total time cost
- `step()` — called every new day to compute progress; returns true when finished
- `isFinished()` — whether research is complete
- `getSpent()`, `getCost()` — accessors
- `getRules()->getName(lang)` → project display name (via `RuleResearch`)

### Production (`src/Savegame/Production.*`)

- `_timeSpent: int` — time spent producing so far
- `_amountProduced: int` — units produced
- `_amount: int` — total required (or 0 for infinite)
- `getAmountTotal()`, `getTimeSpent()`, `getAmountProduced()` — accessors
- `getSellItems()` — whether items are marked for sale (`$` indicator)
- `getAssignedEngineers()` — how many engineers assigned
- `step(Base*, SavedGame*, const Mod*, Language*)` — called daily; advances production

### Craft (`src/Savegame/Craft.*`)

- `_status: std::string` — craft status text (e.g. "Repairing", "Refueling", "Rearming")
- `_fuel`, `_damage`, `_shield: int` — current values used to determine if maintenance is needed
- `getRules()->getName(lang)` → craft display name (via `RuleCraft`)

### GeoscapeState (`src/Geoscape/GeoscapeState.*`)

- Constructor creates the right-side sidebar (`_sidebar`, `_btnIntercept`–`_btnFunding`), globe (`_globe`), and time display widgets — all positioned on the **right** side of screen (x ≥ `screenWidth - 64`).
- The **left side** (x = 0..~200) is largely empty: `_bg` background surface, globe centered. This provides clear space for an overlay panel.
- Time advancement: `timeAdvance()` → `time5Seconds()` / `time10Minutes()` / `time30Minutes()` / `time1Hour()` / `time1Day()`. These are the hooks where the display should refresh.
- `_game->getSavedGame()->getBases()` — returns the list of all player bases (iterable).

### Language strings

- DX language file: `bin/common/Language/DX/en-US.yml` — new section for activity display keys (`STR_ACTIVITY_DISPLAY_*`).

---

## Approach

### 1. New member in GeoscapeState

Add a single text widget to render the entire activity panel:

```cpp
// In GeoscapeState.h (private members)
Text *_txtActivity;          // Activity Display overlay
bool _activityDirty;         // true when display needs refresh
int _lastRefreshTimeSpeed;   // track time-speed changes
```

The widget is built in the constructor with a small fixed-size surface (e.g. 200×300 px, scaled for resolution), using a compact monospace-style font (`FONT_GEO_SMALL` or a dedicated activity font). It is added to the state and drawn **after** `_bg` but **before** the globe so it overlays correctly.

### 2. Build the display text

A private method `void buildActivityDisplay()` iterates all bases from `getSavedGame()->getBases()`, collects non-empty sections, and formats them into a single string:

```cpp
void GeoscapeState::buildActivityDisplay()
{
    _activityDirty = false;
    std::string text;
    const auto *bases = _game->getSavedGame()->getBases();

    for (int i = 0; i < bases->size(); ++i)
    {
        const Base *base = (*bases)[i];
        bool hasContent = false;

        // --- Resources: fuel storage ---
        int totalFuel = base->getTotalAlienFuel();  // or iterate items container
        if (totalFuel > 0)
        {
            text += formatLine(i + 1, "Resources",
                std::to_string(totalFuel) + " Fuel");
            hasContent = true;
        }

        // --- Research ---
        const auto *research = base->getResearch();
        bool hasIdleScientists = (base->getNumScientists() > research->size());
        if (!research->empty() || hasIdleScientists)
        {
            for (auto *rp : *research)
            {
                text += formatLine(i + 1, "Research",
                    rp->getRules()->getName(_game->getLanguage()) + ": " +
                    std::to_string(rp->getSpent()));
            }
            if (hasIdleScientists)
            {
                text += formatAlertLine(i + 1, "No Research");
            }
            hasContent = true;
        }

        // --- Manufacturing ---
        const auto *productions = base->getProductions();
        bool hasIdleEngineers = (base->getNumEngineers() > productions->size());
        if (!productions->empty() || hasIdleEngineers)
        {
            for (auto *prod : *productions)
            {
                std::string line = prod->getRules()->getName(_game->getLanguage()) + ": " +
                    std::to_string(prod->getAmountProduced()) + "/" +
                    std::to_string(prod->getAmountTotal());
                if (prod->getSellItems())
                    line += " $";
                // TODO: compute time remaining from step() delta
                text += formatLine(i + 1, "Manufacturing", line);
            }
            if (hasIdleEngineers)
            {
                text += formatAlertLine(i + 1, "No Manufacturing");
            }
            hasContent = true;
        }

        // --- Craft maintenance ---
        const auto *crafts = base->getCrafts();
        for (auto *craft : *crafts)
        {
            if (!craft->isStationedAtBase() || craft->getStatus().empty())
                continue;
            text += formatLine(i + 1, "Craft",
                craft->getRules()->getName(_game->getLanguage()) + ": " +
                craft->getStatus());
            hasContent = true;
        }

        if (hasContent)
            text += "\n";  // separator between bases
    }

    _txtActivity->setText(text);
}
```

### 3. Formatting helpers

Two helper methods handle line formatting with reversed-color support:

```cpp
// Normal progress line: "[N] Section: Details"
std::string formatLine(int baseNum, const std::string &section, const std::string &details);

// Alert/idle line: "[N] ALERT — Message" rendered in reversed colors
std::string formatAlertLine(int baseNum, const std::string &message);
```

The `Text` widget supports per-line color control via palette indices. Reversed colors can be achieved by using the palette's "inverted" block (typically a specific index range) or by drawing text with a negative shade offset. The existing `_txtSlacking` and `_txtTraining` indicators already use reversed-color rendering — reuse that technique.

### 4. Time format

Estimated times for manufacturing:
- Hours (`h`) for durations under one day → `timeRemaining < 24`
- Days (`d`) for longer durations → `timeRemaining >= 24`

Computing time remaining requires knowing the production rate (units per day). This can be derived from `Production::step()`'s effect on `_timeSpent`, or by reading the ruleset's `timePerUnit` field from `RuleManufacture`. For research, remaining = `_cost - _spent` days.

### 5. Refresh triggers

Call `buildActivityDisplay()` in these geoscape time hooks:

| Hook | Frequency | Calls buildActivityDisplay() |
|------|-----------|------------------------------|
| `time5Seconds()` | Every 5 game seconds (at fastest speed) | ✅ Yes |
| `time10Minutes()` | Every 10 game minutes | ✅ Yes |
| `time30Minutes()` | Every 30 game minutes | ✅ Yes |
| `time1Hour()` | Every hour | ✅ Yes |
| `time1Day()` | Every day | ✅ Yes (also triggers `ResearchProject::step()`, `Production::step()`) |

All these hooks are called from `timeAdvance()` which itself is called on every geoscape tick. The display refreshes automatically whenever game time advances, matching the spec's requirement that "update frequency varies with the selected time speed setting."

### 6. Visibility gating

Add a DX option `activityDisplayEnabled` (default **on**) registered in `Options.inc.h` / `Options.cpp`, alongside other geoscape UI toggles like `showFundsOnGeoscape`. The display is hidden when the option is off, and `_txtActivity` is not added to the state.

---

## Implementation Steps

### Step 1: Language strings

Add DX language keys in `bin/common/Language/DX/en-US.yml`:
- `STR_ACTIVITY_DISPLAY_RESOURCES` — "Resources"
- `STR_ACTIVITY_DISPLAY_RESEARCH` — "Research"
- `STR_ACTIVITY_DISPLAY_MANUFACTURING` — "Manufacturing"
- `STR_ACTIVITY_DISPLAY_CRAFT` — "Craft"
- `STR_ACTIVITY_NO_RESEARCH` — "No Research"
- `STR_ACTIVITY_NO_MANUFACTURING` — "No Manufacturing"
- `STR_ACTIVITY_ALERT` — alert prefix (e.g. "⚠")

### Step 2: GeoscapeState members and constructor

Add `_txtActivity`, `_activityDirty`, `_lastRefreshTimeSpeed` to `GeoscapeState.h`. Build the widget in the constructor with appropriate size, font, and alignment. Add it to the state tree.

### Step 3: Helper methods

Implement `formatLine()`, `formatAlertLine()` for consistent text formatting. Implement `buildActivityDisplay()` as described above.

### Step 4: Time hooks

Wire `buildActivityDisplay()` into `time5Seconds()`, `time10Minutes()`, `time30Minutes()`, `time1Hour()`, and `time1Day()`. Mark `_activityDirty = true` on each call; only rebuild when dirty (avoids redundant string formatting).

### Step 5: Option toggle

Add `activityDisplayEnabled` to Options, register in advanced options UI, gate the widget creation and refresh behind it.

---

## Key Code Locations

| File | Role |
|------|------|
| `src/Geoscape/GeoscapeState.h` / `.cpp` | New `_txtActivity`, `_activityDirty`; `buildActivityDisplay()`; time-hook wiring |
| `src/Savegame/Base.*` | Read `_research`, `_productions`, `_scientists`, `_engineers`, `_crafts` (no changes needed — all data is already accessible) |
| `src/Savegame/ResearchProject.*` | Read `_spent`, `_cost`; call `getRules()->getName()` for project name |
| `src/Savegame/Production.*` | Read `_timeSpent`, `_amountProduced`, `_amountTotal`, `getSellItems()`; compute remaining time |
| `src/Savegame/Craft.*` | Read `_status`, `_fuel`, `_damage`; check if craft is stationed at base |
| `src/Engine/Options.inc.h` / `Options.cpp` | New `activityDisplayEnabled` option (default on) |
| `bin/common/Language/DX/en-US.yml` | Activity display language strings |

---

## Design Decisions & Open Questions

### 1. Panel width and font choice

The left side of the Geoscape has roughly 200px of clear space before the globe center. A monospace or compact proportional font at `FONT_GEO_SMALL` size should fit ~35–40 characters per line. If base names are needed instead of numbers, this becomes tight — the spec uses `[1]`, `[2]` numbering to keep entries concise.

**Decision:** Use sequential base numbers (`[1]`, `[2]`) as specified. If a user wants base names, they can hover/click the globe marker (existing behavior).

### 2. Time remaining computation for manufacturing

`Production::step()` advances `_timeSpent` daily but doesn't expose a "remaining" method. The ruleset `RuleManufacture` has a `timePerUnit` field that gives days per unit. Remaining = `(total - produced) * timePerUnit`.

**Decision:** Compute remaining from `RuleManufacture::getTimePerUnit()` × `(amountTotal - amountProduced)`. For infinite production, show nothing or "∞".

### 3. Craft maintenance status source

`Craft::_status` is a string set during geoscape processing (e.g. when a craft lands and needs repairs). The exact conditions for setting it are in `GeoscapeState::time1Day()` or related methods. Need to verify which crafts get a non-empty `_status`.

**Decision:** Show any craft with a non-empty `_status` string that indicates maintenance need (repair/refuel/rearm). Filter out status strings like "Flying" or "Patrolling".

### 4. Reversed-color technique

The existing `_txtSlacking` and `_txtTraining` indicators use reversed colors via the interface system's `custom` offset property. The exact mechanism is palette-index manipulation — likely drawing text with a specific color index that maps to an inverted hue in the geoscape palette.

**Decision:** Reuse the same technique as `_txtSlacking`. Check how those widgets are styled in the geoscape interface ruleset and replicate for alert lines.

### 5. Panel scrollability

If there are many bases (e.g., 10+), the display could exceed available vertical space. The spec doesn't mention scrolling.

**Decision:** For v1, cap the display to show only the first N bases (e.g., top 8) with a "..." indicator if more exist. Scrolling can be added later as an enhancement.

---

## Testing Notes

- Verify display updates at each time speed tier (5s, 1min, 5min, 30min, 1hr, 1day).
- Confirm idle scientist/engineer alerts appear correctly when personnel count exceeds active projects/productions.
- Test with zero bases, single base, and many bases to check layout boundaries.
- Verify reversed-color rendering matches the geoscape palette (UFO and TFTD palettes separately).
- Ensure the display doesn't interfere with globe click handling or existing sidebar buttons.

---

## Expansion Checklist (Post-v1)

Per follow-up scope discussion, the activity display can be extended with the items below.

- [x] **Incoming transfers summary** per base (count + nearest ETA).
- [x] **Facility bottleneck warnings** (stores near/full only).
- [x] **Wounded soldier summary** (total wounded; severe subset with recovery time).
- [x] **Active craft operations summary** (craft currently out with detailed status: RETURNING TO BASE, INTERCEPTING UFO, PATROLLING, etc.).
- [x] **Training capacity snapshot** (training/total for martial and psi).
- [x] **Base defense readiness warnings** (disabled defenses and/or unarmed defense facilities).
- [x] **Economy warning line(s)** only for actionable alerts (e.g., projected negative cashflow, maintenance spike).
