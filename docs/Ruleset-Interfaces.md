# Ruleset: `interfaces:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleInterface`](../src/Mod/RuleInterface.h) · **List key:** `type` · **Loader:**
[`RuleInterface::load`](../src/Mod/RuleInterface.cpp)

One `interfaces:` entry describes **one screen** (one `State` in the engine): which palette it uses,
which background image and music it plays, and the color/position/size of each named widget on it.
The engine has no named colors — a screen's C++ asks the mod for `getInterface("geoscape")->
getElement("text")->color` and gets a **raw palette index** back. That is the whole contract: the
`id`s are fixed by the C++, the values are yours.

```yaml
interfaces:
  - type: mainMenu                # the screen id, defined by the C++ State
    backgroundImage: BACK01.SCR
    music: GMSTORY
    elements:
      - id: palette
        color: 0                  # background palette block (see below)
      - id: window
        color: 133                # minty green
      - id: text
        color: 138                # yellow
      - id: button
        color: 133
```

Entries **merge** by `type`, and `refNode:` copies another entry's fields first. Merging is
**per element and per field**: redefining `text`'s `color` in a later mod leaves that element's
`pos`/`size` and every other element alone.

## Interface fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | The screen id. Must be one the engine actually asks for (`mainMenu`, `geoscape`, `basescape`, `soldierList`, `battlescape`, `articleItem`, …) — see the calls to `getInterface(...)` in [`src/`](../src). |
| `palette` | string | inherited/`PAL_GEOSCAPE` | Which palette the screen loads (`PAL_GEOSCAPE`, `PAL_BASESCAPE`, `PAL_UFOPAEDIA`, `PAL_BATTLEPEDIA`, `PAL_BATTLESCAPE`, or a [custom palette](Ruleset-CustomPalettes.md)). |
| `parent` | string | — | Another interface entry to fall back to for the `palette` and the `palette` element when this one doesn't define them. |
| `backgroundImage` | string | — | The screen's background surface (an image from [`extraSprites:`](Ruleset-ExtraSprites.md) or a vanilla SCR). |
| `altBackgroundImage` | string | `TAC00.SCR` | Background used when the screen is drawn in the battlescape theme (a Geoscape screen opened mid-battle). |
| `upgBackgroundImage` | list of `[research, image]` | — | Conditional backgrounds: the first pair whose research is done replaces `backgroundImage`. |
| `music` | string | — | [Music track](Ruleset-Musics.md) to play on this screen. |
| `sound` | int | −1 | Sound (index into `GEO.CAT`) played when the screen opens. |
| `elements` | list | — | The widgets — see below. |
| `refNode` | map | — | Inherit all fields from another node before this entry's own fields apply. |

## Element fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | string | — | The widget name the C++ looks up. Not free-form: it must match what the screen asks for. |
| `pos` | `[x, y]` | unset | Position, **relative to the widget's parent surface** (only applied when the C++ passes a parent). |
| `size` | `[w, h]` | unset | Width and height in pixels (same caveat). |
| `color` | int 0–255 | unset | Primary palette index (`Surface::setColor`) — for most widgets, the color of the widget/text. |
| `color2` | int 0–255 | unset | Secondary palette index (`setSecondaryColor`) — typically the highlight/value color in a list or the alt-row color. |
| `border` | int 0–255 | unset | Border/tertiary palette index (`setBorderColor`) — window and list frames. |
| `custom` | int | 0 | A free integer whose meaning is entirely up to the individual screen (e.g. `debriefing`/`list` uses it as the first column's width; `stats`/`numMaxHealth` uses it as a bitmask). |
| `TFTDMode` | bool | false | Puts the widget into TFTD-style inverted rendering (the click/press inversion behavior of `InteractiveSurface`). |

"Unset" is `INT_MAX` internally: the engine simply does not touch that property, so the widget keeps
whatever the C++ constructed it with. That is why you can override just a color and leave layout alone.

## Colors are palette indices

`color`, `color2` and `border` are **raw indices into the screen's active palette**, not RGB and not
named. The very same number is a different hue in `PAL_GEOSCAPE` than in `PAL_BASESCAPE` — always pick
a value against the palette named by this entry's `palette:` (or its `parent`'s). In the X-COM
palettes each 16-index block is one hue ramp, and **text uses the first color of a block**, so text
colors are normally multiples of 16 (`0`, `16`, … `240`) and shades are offsets within the block.

## Special element ids

- **`palette`** — not a widget. Its `color` is the **background palette block** (`backPal`) the screen
  loads its window backgrounds with; `color2` is the alternate used when the screen requests the
  alternate palette. If absent, the `parent` interface's `palette` element is used.
- **`battlescapeTheme`** (on `mainMenu`) — the colors every screen is recolored to when displayed in
  the battlescape theme: `color` for surfaces, `color2`/`border` for the rest.

## See also

- [`extraSprites:`](Ruleset-ExtraSprites.md) — where `backgroundImage:` names come from
- [`musics:`](Ruleset-Musics.md) / [`extraSounds:`](Ruleset-ExtraSounds.md) — `music:` and `sound:`
- [`customPalettes:`](Ruleset-CustomPalettes.md) — defining a palette to name in `palette:`
- [`ufopaedia:`](Ruleset-Ufopaedia.md) — the `article*` interface entries color the pedia screens
