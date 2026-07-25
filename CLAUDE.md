# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

This is **OpenXcom DX**, based on **OXCE-Plus** (the `oxce-plus` branch of Darineth's fork of
OpenXcom) — an open-source C++/SDL reimplementation of the 1994 *UFO: Enemy Unknown* / *X-COM:
Terror From the Deep*. The lineage is: base **OpenXcom** → **OpenXcom Extended (OXCE)** → the
**OXCE-Plus** fork → **OpenXcom DX** (this project). OXCE is a heavily extended engine on top
of base OpenXcom, adding a large modding surface and a built-in scripting language; OXCE-Plus
layers further changes on top (see `Extended.txt` for the running changelog of OXCE features by
version, and `CHANGELOG.txt` for upstream OpenXcom history).

Features added by **OpenXcom DX** specifically (on top of OXCE-Plus) are tracked in
`DX-Features.md`. Consult it to see what DX has changed, and add an entry there whenever you
implement a new DX feature.

- Language: **C++17** (enforced in CMake; build fails on older compilers).
- Rendering/audio/input: **SDL 1.2** (`SDL`, `SDL_mixer`, `SDL_gfx`, `SDL_image`) + optional OpenGL.
- Version is defined in `src/version.h` (`OPENXCOM_VERSION_*`) — that file is the source of truth.
  DX uses **dual-track versioning**: `OPENXCOM_VERSION_NUMBER` is the **OXCE base** DX last synced to
  (bump only on upstream merges; it drives the `Extended` compatibility name so OXCE mods still load),
  and `OPENXCOM_VERSION_NUMBER_DX` is **DX's own** feature-set version (bump when DX ships
  ruleset-visible changes a mod might gate on; it drives the `Extended DX` name). UI shows the compact
  `Extended DX <dx>` (`OPENXCOM_VERSION_SHORT`); logs/`--version`/save headers show the full
  `Extended DX <dx> (OXCE <base>)` (`OPENXCOM_VERSION_SHORT_OXCE`).
  `MIN_REQUIRED_RULESET_VERSION_NUMBER` there gates which mods load.

The game requires the original game's data files (UFO/TFTD resources) to actually run — they
are not in this repo. See `README.md` for data/user/config folder locations per OS.

## Building

There is **no separate lint step and no automated test suite** (see "Testing" below). The
build itself, with `-Wall -Wextra` (and `-Wsuggest-override`, `-Wshadow`, etc. on GCC), is the
primary correctness gate. Use `-DFATAL_WARNING=ON` to promote warnings to errors as CI-equivalent strictness.

### Windows (primary dev platform here)
Two supported paths:

- **Visual Studio solution** (what CI uses for Windows): open/build `src/OpenXcom.2010.sln`.
  CI invokes it as `msbuild OpenXcom.2010.sln /p:Configuration=... /p:Platform=Win32`.
  Prebuilt SDL dependencies for Windows are vendored under `deps/` (headers in `deps/include`,
  libs/DLLs in `deps/lib/Win32` and `deps/lib/x64`); the CMake build auto-copies the DLLs next
  to the executable.

  **On this dev machine**, the configured way to build is the VS Code task in
  `.vscode/tasks.json` ("Build (Release)" / "Build (Debug)"), which shells out to MSBuild
  directly. **Always use the `run_task` tool to invoke these tasks — do not run MSBuild
  manually in a terminal.** The task IDs are `"shell: Build (Release)"` and
  `"shell: Build (Debug)"`, in the repository root workspace folder.
  Output lands in `bin/Win32/Release/OpenXcom.exe` (or `...\Debug\...`), with the SDL DLLs from
  `deps/` auto-copied next to it. A harmless `LNK4099: PDB 'SDLmain.pdb' not found` warning is
  expected.

  Use the regular **`Release`** / **`Debug`** configs (Win32 or x64) — they use
  `$(DefaultPlatformToolset)` and build as-is on modern Visual Studio (verified on VS2026 /
  toolset v145). **DX does not support the `Release_XP` configs**: they hardcode the
  deprecated `v141_xp` (Windows XP) toolset, which isn't installed on current VS and which DX
  has no intention of targeting. Ignore them — or if the IDE prompts to retarget the solution,
  decline (retargeting only matters for the XP configs we don't use).
- **CMake** (cross-platform): from the repo root,
  ```
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  ```

### Linux
```
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
```
Requires SDL 1.2 dev packages (`libsdl1.2`, `libsdl-mixer1.2`, `libsdl-gfx1.2`, `libsdl-image1.2`)
plus zlib. On UNIX, dependencies are resolved via `pkg-config`.

### macOS
```
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/local -G Ninja -B build
cmake --build build
```

### Useful CMake options (see `CMakeLists.txt`)
- `DEV_BUILD` (default ON) — development build; turn OFF for releases.
- `FATAL_WARNING` (default OFF) — treat warnings as errors.
- `EMBED_ASSETS` — embed `bin/common` + `bin/standard` into the executable.
- `DUMP_CORE` — disable the in-process crash/exception handler (useful for debugging under a debugger).
- `DEPS_DIR` — override the Windows dependencies directory (defaults to `./deps`).

The build output goes to `bin/` alongside the bundled `common`, `standard`, `UFO`, and `TFTD`
asset folders, which are copied next to the executable post-build (unless `EMBED_ASSETS`).

## Testing

There is **no unit test framework**. Correctness checks are compiled-in `assert()`s guarded by
`#ifndef NDEBUG`, so they only run in **debug** builds. Example: the bottom of `src/main.cpp`
exercises `Collections::removeIf` and the vector math in `fmath.h`. To run them, build in Debug
and launch the binary — a failed invariant aborts at startup. There is no "run a single test"
mechanism; verification of gameplay logic is done by running the game against a mod/savegame.

## Architecture (the big picture)

The engine is a **stack-based state machine** driving an SDL surface-blitting renderer. Three
concepts dominate and span many files:

### 1. Game loop + State stack (`src/Engine/Game.*`, `src/Engine/State.*`)
`main()` (`src/main.cpp`) installs crash handlers, calls `Options::init()`, constructs the
single `Game` object, pushes an initial `StartState`, and calls `game->run()`. `Game` owns the
`Screen`, `Cursor`, `Language`, the loaded `Mod`, and the active `SavedGame`. It holds a
`std::list<State*>` — the **state stack**. Each `State` is roughly one full screen/window and
owns a set of `Surface`s (interface widgets). `run()` pumps SDL events to the top state,
`think()`s, and blits. Pushing/popping states (`Game::pushState`/`popState`) is how all screen
transitions and modal dialogs work. When adding UI, you add a `State` subclass plus the
`Surface`/`Interface` widgets it composes.

### 2. The three game "scapes" — each a cluster of States
The gameplay UI is organized into directories that mirror the original game's modes:
- `src/Geoscape/` — the global strategy layer (the spinning globe `Globe`, time passage via
  `GeoscapeState`, dogfights, base building decisions, mission/event generation).
- `src/Basescape/` — base management (research, manufacture, soldiers, stores, transfers).
- `src/Battlescape/` — the turn-based tactical layer. This is the most complex subsystem:
  `BattlescapeGame` (turn/action orchestration), `BattlescapeGenerator` (builds the map from
  terrain + map blocks + deployment), `TileEngine` (line-of-sight, lighting, damage), `Map`
  (rendering), `Pathfinding`, `AIModule` (enemy AI), and a family of `*BState` classes
  (`ProjectileFlyBState`, `UnitWalkBState`, `ExplosionBState`, …) implementing the
  battle-action state sub-machine.
- `src/Menu/` (main menu, options, load/save), `src/Ufopaedia/` (in-game encyclopedia),
  `src/Interface/` (reusable widgets: `Text`, `TextButton`, `Window`, `ComboBox`, `TextList`, …).

### 3. Mod (rules) vs. Savegame (state) — the data backbone
This separation is fundamental:
- **`src/Mod/`** = immutable *rule definitions* loaded from YAML rulesets. `Mod.cpp` is the
  central loader/registry; the `Rule*` classes (`RuleItem`, `RuleCraft`, `RuleResearch`,
  `RuleAlienMission`, `Armor`, `Unit`, `AlienDeployment`, …) each parse one ruleset node type.
  Mods are layered (multiple mods merge), and rulesets drive almost all game content.
- **`src/Savegame/`** = mutable *runtime/persisted state* (`SavedGame`, `SavedBattleGame`,
  `Base`, `Soldier`, `Craft`, `BattleUnit`, `BattleItem`, `Tile`, …). These reference the
  `Rule*` objects from the Mod by pointer/id and are what gets serialized to save files.

  Rule of thumb: if it's defined by the modder and never changes during play, it's in `Mod/`;
  if it changes as the game is played and must survive save/load, it's in `Savegame/`.

### 4. Engine services (`src/Engine/`)
Cross-cutting infrastructure used everywhere: `Options` (global settings), `Language`/
`LocalizedText`/`Unicode` (i18n), `Surface`/`SurfaceSet`/`Palette`/`Font`/`Screen`/`Zoom`
(graphics + scalers like hq2x/xbrz), `Sound`/`Music`/`AdlibMusic` (audio), `FileMap` +
`CrossPlatform` (virtual filesystem and OS abstraction), `RNG`, `Timer`, and `Yaml.*`.

YAML is parsed with the vendored **rapidyaml** library (`libs/rapidyaml`, wrapped by
`src/Engine/Yaml.*`); zip/deflate via vendored **miniz** (`libs/miniz`). Both are compiled
directly into the binary (see `src/CMakeLists.txt`).

### 5. The OXCE scripting engine (`src/Engine/Script.*`)
A distinctive OXCE feature: a small custom bytecode/VM scripting language exposed to mods (the
`Y-Script`/`scripts:` ruleset hooks). `Script.h/.cpp` define the parser, the typed register
model, and the binding macros that expose engine C++ types/fields to scripts. When working on
anything that lets mods react to game events (damage, recolor, stat bonuses via
`RuleStatBonus`, etc.), this is the machinery involved. It is performance-sensitive and
heavily template/macro-based.

## Conventions

- Code style is defined by `.clang-format`, `.astylerc`, and `.editorconfig` (tabs for
  indentation). Match the surrounding file.
- Every `Rule*`/`Savegame` class typically has a `load(const YAML::YamlNodeReader&)` and a
  `save(...)` pair — keep them in sync when adding a field, and remember new persisted fields
  affect save compatibility.
- The codebase is GPLv3; preserve the license header block at the top of source files.
- **Never commit without explicit user permission.** Do not run `git commit` (or any push/reset/force
  operations) unless the user explicitly asks. Stage and show status freely, but stop there.
- **Git commits: do not list AI/Claude as an author or co-author.** Omit any
  `Co-Authored-By: Claude ...` trailer (and similar AI attribution) from commit messages.
- Adding a new source file requires registering it in **`src/CMakeLists.txt`** (the `*_src`
  lists) — there is no glob-based collection — and, for the VS build, in the
  `OpenXcom.2010.vcxproj` project files.
- **DX-specific language strings go in the `Language/DX/` folder** (e.g.
  `bin/common/Language/DX/en-US.yml`), not mixed into the base or `Language/OXCE/` files. This
  mirrors how OXCE isolates its own strings, and the folder is loaded as a dedicated VFS slice in
  `Game::loadLanguages` (`src/Engine/Game.cpp`). Group keys by source file with the existing
  `#=== Section ===` / `#FileName.cpp` comment convention. New `STR_*` keys referenced from C++
  via `tr(...)` must be defined here or they render as the raw token.
- **All player-facing display text must be localized**: do not hard-code UI/output strings in C++.
  Add/extend `STR_*` keys and render them through `tr(...)` (with `.arg(...)` where needed).

# Planning Features

When preparing to build a feature, start by writing a design doc in `plans/` describing the feature, the motivation, and the implementation approach. Link to it from `DX-Roadmap.md` near its checklist item. This helps coordinate development and provides a reference for future maintainers.

If a requested new feature is not clearly specified, create/update the feature plan doc first and ask clarifying questions until there is a concrete implementation plan before writing code.

Before planning or implementing any feature, review current OXCE/OXCE-Plus behavior first (engine code plus `Extended.txt`) to confirm whether the feature already exists fully or partially. Capture that audit result in the design doc so DX only plans true deltas.

If the feature is entirely new and is not in the roadmap, add it to `DX-Roadmap.md` at the bottom in the "New Features" section with a link to the design doc.

## Updating docs before committing

**Update the documentation in the same commit as the code — do not commit a feature with stale docs.** Before committing, make sure these are in sync with what actually shipped:

- **`DX-Features.md`** — add or refresh the feature's entry (including any option names/defaults), matching the final behavior, not the original intent.
- **The design doc in `plans/`** — update its status line and any details that changed during implementation (approaches tried, bugs fixed, final tuning).
- **`DX-Roadmap.md`** — tick the relevant checkbox(es) and note anything discovered (e.g. "already provided by OXCE-Plus").
- **`docs/Ruleset-*.md`** — **if the change adds, renames, or alters ANY ruleset key, document it.** Update the reference page for that root (`docs/Ruleset.md` is the index); tag DX-added keys **[DX]**; a whole new root also needs a row in the index. A key that isn't in the ruleset docs effectively doesn't exist for a modder, so this ships with the code like the rest of the docs — it is not optional cleanup.
- **`DX-OXCE-Fixes.md`** — only if the change alters *pre-existing upstream OXCE behavior* (a bug fix or a deliberate deviation). Not for DX's own features.

Keep the docs honest: describe the behavior that shipped (final colors, formulas, option names), and revise earlier wording if the implementation diverged from the plan.