---
name: game-ui-screen
description: >-
  Author or modify OpenXcom DX's UI screens and their interface configuration.
  Use when building a new screen/dialog (a State subclass with widgets), changing
  an existing screen's layout/colors/background, adding a widget to a screen, wiring
  a button or list to push another screen, or editing the interfaces.rul ruleset that
  drives screen palettes and element colors. Covers the Geoscape/Basescape/Battlescape/
  Menu/Ufopaedia UI and the reusable Interface widgets (Text, TextButton, TextList,
  Window, Bar, ComboBox, etc.).
---

# Authoring OpenXcom DX UI screens

A screen in this engine is a **`State` subclass** (one per window/dialog) that composes
**`Interface` widgets** (Surfaces), plus a matching **`interfaces.rul`** entry that supplies
the palette and per-element colors. The C++ code defines *structure and behavior*; the YAML
defines *appearance* (colors, background image, palette, and optionally position/size). Modders
re-skin screens by editing the YAML alone — so keep visual constants out of C++ where the
ruleset already covers them.

Reference implementation to copy from: [BaseInfoState.cpp](../../../src/Basescape/BaseInfoState.cpp)
+ [BaseInfoState.h](../../../src/Basescape/BaseInfoState.h), and its YAML block `baseInfo` in
[bin/standard/xcom1/interfaces.rul](../../../bin/standard/xcom1/interfaces.rul). The `State`
base class is [src/Engine/State.h](../../../src/Engine/State.h); the YAML schema is
[RuleInterface.h](../../../src/Mod/RuleInterface.h) (`struct Element`).

## Decision: which task am I doing?

- **Just changing how an existing screen looks** (a color, the background image, the palette,
  or nudging a widget) → you likely only touch `interfaces.rul`. See *Modifying an existing
  screen*. No rebuild needed for YAML-only changes.
- **Adding/removing a widget, changing data shown, or new behavior** → edit the `State` `.cpp`
  (and `.h`), and add a matching element `id` to the screen's `interfaces.rul` entry. Rebuild.
- **A brand-new screen** → new `State` subclass + new `interfaces.rul` entry + build
  registration (3 files). See *Building a new screen*.

---

## Modifying an existing screen

1. Find the State (e.g. `grep` `src/<Scape>/` for the screen name) and note the string passed
   to `setInterface("...")` — that is the screen's interface `type` in `interfaces.rul`.
2. Find that `type:` block in [bin/standard/xcom1/interfaces.rul](../../../bin/standard/xcom1/interfaces.rul)
   (UFO) and/or `bin/standard/xcom2/interfaces.rul` (TFTD). Each `add(widget, "id", "category")`
   call in the C++ maps to an `- id: <id>` element under that block.
3. Edit the element's `color` / `color2` / `border` (palette indices 0–255), `backgroundImage`,
   `palette`, `music`, or `sound`. To re-skin without touching code, this is all you change.
4. To add a *new* visual element, you must add both the widget in C++ **and** an `- id:` entry
   here; an `id` with no matching `add()` is inert, and an `add()` whose `id` is absent from the
   ruleset just renders with default (white/uncolored) styling.

### What an Element can override (`struct Element` in RuleInterface.h)

```yaml
- id: someWidget
  color:  213   # primary color (palette index)
  color2: 138   # secondary color (e.g. TextList alt rows, Bar second value)
  border: 239   # border/frame color
  x: 10          # OPTIONAL position/size override — only applied when the widget is
  y: 20          #   add()-ed with a non-null `parent` arg (see State::add). Coords are
  w: 100         #   then RELATIVE to the parent's x/y. Most screens hardcode geometry in
  h: 14          #   C++ and only set colors here.
  custom: 0      # free int a State may read for screen-specific tweaks
  TFTDMode: false
```

Special element ids consumed by the engine (not your widgets):
- `palette` — its `color` sets the background palette block (`backPal`) for the whole screen;
  `color2` is used when `setInterface(..., alterPal=true)`.
- Top-level keys on the block (siblings of `elements:`): `palette` (e.g. `PAL_BASESCAPE`,
  `PAL_GEOSCAPE`, `PAL_BATTLESCAPE`), `backgroundImage`, `altBackgroundImage`, `music`, `sound`,
  `parent` (inherit elements/palette from another interface `type`).

---

## Building a new screen

### 1. Header — `src/<Scape>/MyScreenState.h`
Copy the license header from an existing file. Subclass `State`, forward-declare widget types,
declare widget pointers, the constructor, `~`, `init()` (if data refreshes on show), and one
`void handler(Action*)` per interactive widget.

```cpp
#pragma once
#include "../Engine/State.h"
namespace OpenXcom {
class Window; class Text; class TextButton; class TextList;
class MyScreenState : public State
{
private:
	Window *_window;
	Text *_txtTitle;
	TextButton *_btnOk;
	TextList *_lstItems;
public:
	MyScreenState(/* domain pointers, e.g. Base* base */);
	~MyScreenState();
	void init() override;            // optional: refresh data each time shown
	void btnOkClick(Action *action);
};
}
```

### 2. Source — `src/<Scape>/MyScreenState.cpp`
The constructor follows a **fixed order** (this order matters):

1. **Construct** every widget with `new T(width, height, x, y)` (geometry in absolute screen
   coords, 320×200 reference resolution). A full-screen background is `new Window(this,320,200,0,0)`
   or `new Surface(320,200,0,0)`.
2. **`setInterface("myScreen")`** — sets palette from the ruleset. Call before `add()`.
3. **`add(widget, "elementId", "myScreen")`** for each widget, in draw order (later =
   on top). The `"elementId"` must exist under `myScreen`'s `elements:` in `interfaces.rul`.
   Use `add(widget)` (no id) for things that need no ruleset styling.
4. **`centerAllSurfaces()`** — required so the screen centers on non-320×200 displays.
5. **Configure**: `setWindowBackground(_window, "myScreen")` (or blit a `.SCR`),
   `widget->setText(tr("STR_KEY"))`, register handlers
   (`_btnOk->onMouseClick((ActionHandler)&MyScreenState::btnOkClick)`,
   `onKeyboardPress(..., Options::keyCancel)` for ESC), populate lists, etc.

```cpp
MyScreenState::MyScreenState()
{
	_window   = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 8);
	_lstItems = new TextList(280, 128, 8, 40);
	_btnOk    = new TextButton(288, 16, 16, 176);

	setInterface("myScreen");

	add(_window, "window", "myScreen");
	add(_txtTitle, "text", "myScreen");
	add(_lstItems, "list", "myScreen");
	add(_btnOk, "button", "myScreen");

	centerAllSurfaces();

	setWindowBackground(_window, "myScreen");
	_txtTitle->setText(tr("STR_MY_SCREEN_TITLE"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&MyScreenState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&MyScreenState::btnOkClick, Options::keyCancel);
}
```

Open the screen from elsewhere with `_game->pushState(new MyScreenState(...))`; close it with
`_game->popState()` in the OK/cancel handler.

### 3. Ruleset — add a block to `interfaces.rul`
Add under `interfaces:` in `bin/standard/xcom1/interfaces.rul` (and `xcom2` if it should also
exist in TFTD). Every element `id` you passed to `add()` needs an entry:

```yaml
  - type: myScreen
    palette: PAL_GEOSCAPE
    backgroundImage: BACK01.SCR
    elements:
      - id: palette
        color: 0
      - id: window
        color: 133
      - id: text
        color: 138
      - id: list
        color: 133
        color2: 138
      - id: button
        color: 133
```

### 4. Build registration (REQUIRED for new files — no globbing)
Add the new `.cpp`/`.h` in **all three**:
- [src/CMakeLists.txt](../../../src/CMakeLists.txt) — in the appropriate `*_src` list
  (alongside e.g. `Basescape/BaseInfoState.cpp`).
- `src/OpenXcom.2010.vcxproj` — a `<ClCompile Include="...cpp" />` and `<ClInclude Include="...h" />`.
- `src/OpenXcom.2010.vcxproj.filters` — same two entries with a `<Filter>` group.

### 5. Strings
Any `tr("STR_...")` key must be defined in the language files (e.g.
`bin/standard/xcom1/Language/en-US.yml` or the relevant `extraStrings`). Missing keys render as
the raw `STR_...` token.

---

## Widget catalog ([src/Interface/](../../../src/Interface/))

| Widget | Use |
|---|---|
| `Window` | Framed background panel; `setWindowBackground` skins it |
| `Text` | Static/dynamic label; `setBig()`, `setAlign()`, `setWordWrap()` |
| `TextButton` | Clickable button; `setText`, `onMouseClick` |
| `ToggleTextButton` | On/off button |
| `TextEdit` | Editable text field (needs `State*`: `new TextEdit(this, ...)`) |
| `TextList` | Scrollable multi-column list; `addRow`, `onMouseClick`, columns via `setColumns` |
| `ComboBox` | Dropdown (needs `State*`) |
| `Bar` | Horizontal value/max bar; `setMax/setValue/setScale` |
| `ProgressBar` | Progress indicator |
| `Slider`, `ScrollBar`, `ArrowButton` | Range/scroll controls |
| `Frame`, `NumberText`, `ImageButton`, `Surface` | Decoration / numeric / image / raw blit |

## Gotchas (learned from the codebase)

- **Constructor vs `init()`**: build widgets and wire handlers in the constructor; put data that
  must refresh every time the screen (re)appears in `init()` (call `State::init()` first). See
  how `BaseInfoState::init()` re-reads base stats when switching bases.
- **`add()` order is z-order.** Background first, interactive controls last.
- **Palette correctness**: always `setInterface(...)` before `add(...)`; widgets get the state's
  palette at `add()` time. Wrong/missing palette → garbled colors.
- **Position/size overrides from YAML only apply with a parent** — `State::add` ignores `x/y/w/h`
  unless you pass the 4th `parent` arg. Default screens set geometry in C++.
- **Ownership**: surfaces `add()`-ed to a State are tracked and deleted by the State; don't
  `delete` them in `~`. The destructor is usually empty.
- **Two rulesets**: UFO (`xcom1`) and TFTD (`xcom2`) have separate `interfaces.rul`. A screen
  reachable in both needs an entry in both (or a shared `parent`).
- **Keyboard**: bind ESC/cancel via `onKeyboardPress(handler, Options::keyCancel)` and OK via
  `Options::keyOk` for consistency with the rest of the UI.
- **No tests**: verify by building (the `-Wall -Wextra` build is the gate) and running the game
  to the screen. YAML-only edits need only a restart, not a rebuild.
- If this is a **DX-specific** new screen/feature, add an entry to
  [DX-Features.md](../../../DX-Features.md) (project rule in CLAUDE.md).
