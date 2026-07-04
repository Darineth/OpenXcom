# Feature: `battleClipSize` — per-round ammo economy, magazine-fed in the field

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 6 (Ammo & Reloading),
first item. Ported/re-derived from the legacy DX fork (Legacy-DX-Features.md §4). Not present in
current OXCE-Plus (audit below). Implemented as designed — the field logic routes through a new
`RuleItem::getBattleMagazineSize()` helper (magazine capacity) while `getClipSize()` stays the
economy/store unit; vehicle/HWP fixed ammo is left on the whole-clip economy (out of scope).
dx-test exercises it via `STR_SHOTGUN_SLUG` (`battleClipSize: 8`, compared against the normal
`clipSize: 8` buckshot).

## Motivation

Some ammo is *expensive per shot* but still *magazine-fed*: blaster bombs, fusion/artillery
shells, and (later) psi-orbs. The stock model forces a single number, `clipSize`, to mean two
different things at once:

- **the economy unit** — how base stores stock / buy / sell / recover the item, and
- **the magazine capacity** — how many shots a loaded weapon holds before it must reload.

For an ordinary rifle clip (20 rounds bought and loaded as a block) that conflation is fine. For a
weapon that fires individually-costed rounds but still holds several per reload, it is not: you'd
have to either buy rounds one-at-a-time *and* reload one-at-a-time (clipSize 1), or buy a whole
magazine as one indivisible purchase (clipSize N). `battleClipSize` **decouples the two**: stock
per round, load as a magazine.

## Audit — what OXCE-Plus already provides (the delta)

- `battleClipSize` / `getBattleClipSize` / `getBattleMagazineSize`: **absent** from `src/`
  (grep finds nothing). Genuine DX work.
- Existing `clipSize` (`RuleItem::_clipSize`, default 0) already drives everything:
  - `BattleItem` ctor seeds ammo quantity from `getClipSize()` ([BattleItem.cpp:54](../src/Savegame/BattleItem.cpp#L54)); `-1` = unlimited ([BattleItem.cpp:421](../src/Savegame/BattleItem.cpp#L421)).
  - Field spend/refill gate on `getClipSize() > 0` ([BattleItem.cpp:953](../src/Savegame/BattleItem.cpp#L953)); `getAmmoQuantityMax` script binding maps to `getClipSize` ([BattleItem.cpp:1552](../src/Savegame/BattleItem.cpp#L1552)).
  - Battle generation creates **one `BattleItem` per stored unit** at three sites — launching craft ([BattlescapeGenerator.cpp:1243](../src/Battlescape/BattlescapeGenerator.cpp#L1243)), base-defense stores ([:1270](../src/Battlescape/BattlescapeGenerator.cpp#L1270)), docked craft ([:1294](../src/Battlescape/BattlescapeGenerator.cpp#L1294)).
  - Mission recovery accumulates raw rounds into `_rounds[rule]`, then divides by `getClipSize()` to
    whole clips at debrief ([DebriefingState.cpp:2054-2067](../src/Battlescape/DebriefingState.cpp#L2054)).
  - Base-screen ammo classification keys on `BT_AMMO || (BT_NONE && getClipSize() > 0)` (Purchase/Sell/Transfer/Debrief) — economy already tracks stores per *store unit*, whatever a unit means.
- **So the store/buy/sell economy is already per-store-unit.** The trick is: make a store unit = **one
  round** (by forcing `clipSize → 0`) while giving the *field* a separate magazine capacity to load,
  fire, and refill against.

## Design

### Rule model (`RuleItem`)
- Add `int _battleClipSize = 0;` + YAML key `battleClipSize` (parsed next to `clipSize` in
  `RuleItem::load`, [RuleItem.cpp:496](../src/Mod/RuleItem.cpp#L496)).
- Add `int getBattleClipSize() const;`.
- **Mutual exclusion:** at end of `load`, `if (_battleClipSize > 0) _clipSize = 0;` — so an ammo item
  is *either* a normal clip *or* a battleClipSize round-store, never both. Document in the header and
  in the `battleClipSize` value in DX-Features.md.
- Add a single **effective magazine capacity** accessor to route field logic through, e.g.
  `int getBattleMagazineSize() const { return _battleClipSize > 0 ? _battleClipSize : _clipSize; }`.
  This is the "how many shots does a loaded mag hold" number; `getClipSize()` stays the **economy /
  store-unit** number (0 under the battleClipSize model, so stores stay per-round).

### Field side — load / fire / refill honor the magazine size
Route the *capacity* checks (not the economy) through `getBattleMagazineSize()`:
- `BattleItem` ctor initial fill ([:54](../src/Savegame/BattleItem.cpp#L54)) — seed from
  `getBattleMagazineSize()` so a bare-created battleClipSize ammo item comes full (the generator
  overrides for partial last mags; see below).
- `getAmmoQuantity()` unlimited check ([:421](../src/Savegame/BattleItem.cpp#L421)) — unchanged
  (`-1` still means unlimited; battleClipSize is always ≥ 1).
- Spend/refill gate ([:953](../src/Savegame/BattleItem.cpp#L953)) — change `getClipSize() > 0` to
  "finite magazine" i.e. `getBattleMagazineSize() > 0`, else a battleClipSize round is never spent.
- `getAmmoQuantityMax` script binding ([:1552](../src/Savegame/BattleItem.cpp#L1552)) and the
  debug/self-ammo `.../clipSize` displays ([:68](../src/Savegame/BattleItem.cpp#L68), [:1427](../src/Savegame/BattleItem.cpp#L1427), [:1481](../src/Savegame/BattleItem.cpp#L1481)) — use the magazine size so the ammo bar reads `n/battleClipSize`.
- Inventory capacity / self-ammo bars ([Inventory.cpp:70-88](../src/Battlescape/Inventory.cpp#L70), [:614](../src/Battlescape/Inventory.cpp#L614); [InventoryState.cpp:2131](../src/Battlescape/InventoryState.cpp#L2131)) — same substitution so the loaded mag draws correctly.
- Audit the compatibleAmmo divisibility checks ([RuleItem.cpp:820](../src/Mod/RuleItem.cpp#L820), [:1667-1696](../src/Mod/RuleItem.cpp#L1667)) — these compute "how many shots does this weapon+ammo combo hold"; they must see the magazine size, not 0.

### Battle generation — pack loose rounds into magazines
The three per-unit creation loops must, **for `getBattleClipSize() > 0` BT_AMMO**, pack the stored
rounds into magazine `BattleItem`s of `min(remaining, battleClipSize)` instead of creating one
1-round item per stored unit. Introduce a small helper on `BattlescapeGenerator`, e.g.
`createStoredItemsForTile(const RuleItem* rule, int count, Tile* tile)`:
- battleClipSize ammo → `while (count > 0) { int n = min(count, bcs); auto* bi = createItemForTile(rule, tile); bi->setAmmoQuantity(n); count -= n; }`
- everything else → the existing `for (count) createItemForTile(...)` loop (unchanged behavior).

Call it at [:1243](../src/Battlescape/BattlescapeGenerator.cpp#L1243), [:1270](../src/Battlescape/BattlescapeGenerator.cpp#L1270), [:1294](../src/Battlescape/BattlescapeGenerator.cpp#L1294). Base-defense store removal already removes by full count, so it stays correct.

### Recovery — return raw rounds, don't divide by (zero) clipSize
At debrief ([DebriefingState.cpp:2054-2067](../src/Battlescape/DebriefingState.cpp#L2054)):
`total_clips = pair.first->getBattleClipSize() > 0 ? pair.second : pair.second / getClipSize();`
This both (a) returns raw recovered rounds to stores (per the legacy spec) and (b) **guards the
divide-by-zero** that would otherwise happen because battleClipSize forces `clipSize = 0`. Statistical
bullet conservation only applies to the `clipSize` branch. Also confirm the `_rounds[rule] += clipSize`
"restore 100%" paths ([:2436](../src/Battlescape/DebriefingState.cpp#L2436), [:2553](../src/Battlescape/DebriefingState.cpp#L2553)) use the magazine size for battleClipSize mags so a recovered loaded weapon returns its rounds.

### Economy / base screens — mostly free
Purchase/Sell/Transfer/base-store classification already treats these as `BT_AMMO` items counted per
store unit; with `clipSize = 0`, each unit is one round, so buy/sell price and store counts are
per-round automatically. **No economy-specific change is expected** beyond confirming nothing divides
by `getClipSize()` on the store side. (The "ammo items display contained count on base screens"
polish is a *separate* Phase 6 item, not required here.)

## Testing plan
- dx-test mod: give an existing expensive round (or a new test ammo) `battleClipSize: 4`; confirm:
  1. Base stores buy/sell it one round at a time.
  2. Loading a weapon in the pre-battle inventory yields a magazine that reads `4/4` (or `k/4` for a
     partial last mag when stores aren't a multiple of 4).
  3. Firing decrements per shot; weapon reloads from another mag when empty.
  4. After the mission, raw unused + recovered rounds return to base stores (not rounded down to
     whole 4-round clips).
- Confirm a normal `clipSize` weapon is completely unchanged (the helper returns `clipSize` and every
  battleClipSize branch is skipped).
- Build clean with `-Wall -Wextra` (the correctness gate); watch for the divide-by-zero path.

## Open questions
- **Helper naming:** `getBattleMagazineSize()` (field capacity) vs. keeping `getClipSize()` as the
  economy unit — confirm this split reads clearly, or whether to instead special-case each site.
- **Save compatibility:** `_battleClipSize` is a Rule field (not persisted in saves), and magazine
  `BattleItem`s already serialize their `_ammoQuantity`, so no save-format change is expected —
  verify a mid-mission save/load of a partial battleClipSize magazine round-trips.
