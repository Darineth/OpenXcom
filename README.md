# OpenXcom [![Workflow Status][workflow-badge]][actions-url]

[workflow-badge]: https://github.com/OpenXcom/OpenXcom/workflows/ci/badge.svg
[actions-url]: https://github.com/OpenXcom/OpenXcom/actions

OpenXcom is an open-source clone of the popular "UFO: Enemy Unknown" ("X-COM:
UFO Defense" in the USA release) and "X-COM: Terror From the Deep" videogames
by Microprose, licensed under the GPL and written in C++ / SDL.

See more info at the [website](https://openxcom.org)
and the [wiki](https://www.ufopaedia.org/index.php/OpenXcom).

Uses modified code from SDL\_gfx (LGPL) with permission from author.

## Installation

OpenXcom requires a vanilla copy of the X-COM resources -- from either or both
of the original games.  If you own the games on Steam, the Windows installer
will automatically detect it and copy the resources over for you.

If you want to copy things over manually, you can find the Steam game folders
at:

    UFO: "Steam\SteamApps\common\XCom UFO Defense\XCOM"
    TFTD: "Steam\SteamApps\common\X-COM Terror from the Deep\TFD"

Do not use modded versions (e.g. with XcomUtil) as they may cause bugs and
crashes.  Copy the UFO subfolders to the UFO subdirectory in OpenXcom's data
or user folder and/or the TFTD subfolders to the TFTD subdirectory in OpenXcom's
data or user folder (see below for folder locations).

## Mods

Mods are an important and exciting part of the game.  OpenXcom comes with a set
of standard mods based on traditional XcomUtil and UFOExtender functionality.
There is also a [mod portal website](https://openxcom.mod.io/) with a thriving
mod community with hundreds of innovative mods to choose from.

To install a mod, go to the mods subdirectory in your user directory (see below
for folder locations).  Extract the mod into a new subdirectory.  WinZip has an
"Extract to" option that creates a directory whose name is based on the archive
name.  It doesn't really matter what the directory name is as long as it is
unique.  Some mods are packed with extra directories at the top, so you may
need to move files around inside the new mod directory to get things straighted
out.  For example, if you extract a mod to mods/LulzMod and you see something
like:

    mods/LulzMod/data/TERRAIN/
    mods/LulzMod/data/Rulesets/

and so on, just move everything up a level so it looks like:

    mods/LulzMod/TERRAIN/
    mods/LulzMod/Rulesets/

and you're good to go!  Enable your new mod on the Options -> Mods page in-game.

## Directory Locations

OpenXcom has three directory locations that it searches for user and game files:

<table>
  <tr>
    <th>Folder Type</th>
    <th>Folder Contents</th>
  </tr>
  <tr>
    <td>user</td>
    <td>mods, savegames, screenshots</td>
  </tr>
  <tr>
    <td>config</td>
    <td>game configuration</td>
  </tr>
  <tr>
    <td>data</td>
    <td>UFO and TFTD data files, standard mods, common resources</td>
  </tr>
</table>

Each of these default to different paths on different operating systems (shown
below).  For the user and config directories, OpenXcom will search a list of
directories and use the first one that already exists.  If none exist, it will
create a directory and use that.  When searching for files in the data
directory, OpenXcom will search through all of the named directories, so some
files can be installed in one directory and others in another.  This gives
you some flexibility in case you can't copy UFO or TFTD resource files to some
system locations.  You can also specify your own path for each of these by
passing a commandline argument when running OpenXcom.  For example:

    openxcom -data "$HOME/bin/OpenXcom/usr/share/openxcom"

or, if you have a fully self-contained installation:

    openxcom -data "$HOME/games/openxcom/data" -user "$HOME/games/openxcom/user" -config "$HOME/games/openxcom/config"

### Windows

User and Config folder:
- C:\Documents and Settings\\\<user\>\My Documents\OpenXcom (Windows 2000/XP)
- C:\Users\\\<user\>\Documents\OpenXcom (Windows Vista/7)
- \<game directory\>\user
- .\user

Data folders:
- C:\Documents and Settings\\\<user\>\My Documents\OpenXcom\data (Windows 2000/XP)
- DATADIR build flag
- C:\Users\\\<user\>\Documents\OpenXcom\data (Windows Vista/7/8)
- \<game directory\>
- . (the current directory)

### Mac OS X

User and Config folder:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/Library/Application Support/OpenXcom
- $HOME/.openxcom
- ./user

Data folders:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/Library/Application Support/OpenXcom (if $XDG\_DATA\_HOME is not defined)
- DATADIR build flag
- $XDG\_DATA\_DIRS/openxcom (for each directory in $XDG\_DATA\_DIRS if $XDG\_DATA\_DIRS is defined)
- /Users/Shared/OpenXcom (if $XDG\_DATA\_DIRS is not defined or is empty)
- . (the current directory)

### Linux

User folder:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/.local/share/openxcom (if $XDG\_DATA\_HOME is not defined)
- $HOME/.openxcom
- ./user

Config folder:
- $XDG\_CONFIG\_HOME/openxcom (if $XDG\_CONFIG\_HOME is defined)
- $HOME/.config/openxcom (if $XDG\_CONFIG\_HOME is not defined)

Data folders:
- $XDG\_DATA\_HOME/openxcom (if $XDG\_DATA\_HOME is defined)
- $HOME/.local/share/openxcom (if $XDG\_DATA\_HOME is not defined)
- DATADIR build flag
- $XDG\_DATA\_DIRS/openxcom (for each directory in $XDG\_DATA\_DIRS if $XDG\_DATA\_DIRS is defined)
- /usr/local/share/openxcom (if $XDG\_DATA\_DIRS is not defined or is empty)
- /usr/share/openxcom (if $XDG\_DATA\_DIRS is not defined or is empty)
- the directory data files were installed to
- . (the current directory)

## Configuration

OpenXcom has a variety of game settings and extras that can be customized, both
in-game and out-game. These options are global and affect any old or new
savegame.

For more details please check the [wiki](https://ufopaedia.org/index.php/Options_(OpenXcom)).

## Development

OpenXcom requires the following developer libraries:

- [SDL](https://www.libsdl.org) (libsdl1.2)
- [SDL\_mixer](https://www.libsdl.org/projects/SDL_mixer/) (libsdl-mixer1.2)
- [SDL\_gfx](https://www.ferzkopp.net/wordpress/2016/01/02/sdl_gfx-sdl2_gfx/) (libsdl-gfx1.2), version 2.0.22 or later
- [SDL\_image](https://www.libsdl.org/projects/SDL_image/) (libsdl-image1.2)

The source code includes files for the following build tools:

- Microsoft Visual C++ 2010 or newer
- Xcode
- Make (see Makefile.simple)
- CMake

It's also been tested on a variety of other tools on Windows/Mac/Linux. More
detailed compiling instructions are available at the
[wiki](https://ufopaedia.org/index.php/Compiling_(OpenXcom)), along with
pre-compiled dependency packages.

## OpenXcom DX

This fork ("OpenXcom DX") extends OXCE-Plus with gameplay systems and quality-of-life work of its
own. [DX-Features.md](DX-Features.md) documents everything in detail (options, ruleset keys,
formulas); [DX-Roadmap.md](DX-Roadmap.md) tracks the plan. The summary below separates what is
**always on** out of the box from what is **moddable / opt-in** capability.

### Always-on gameplay & UI

*Battlescape combat*

- **Concurrent projectiles & explosions** — volleys fly and resolve independently (auto/burst fire
  walks around the target, shotgun pellets are real projectiles, impact explosions animate while
  rounds are still in the air), with explosion visuals/sound scaled by blast radius.
- **Dual-fire** — fire both hands' weapons at once at the same target.
- **Combat log** — floating battle event feed (turns, hits, casualties, reactions…; toggleable,
  default on).
- **On-map overlays** — hovered unit name, primed-grenade indicators, status glyphs (bleeding /
  fire / shock), motion-scanner blips on scanned tiles, and fog-of-war dimming of unseen tiles
  (each toggleable, default on).
- **Revamped action menu** — compact layout, numeric hotkeys, TU/ammo availability and shot counts,
  and a visible **Reload** entry.
- **Aim trajectory preview** — tracer line / throw arc while aiming (toggleable, default on;
  Alt shows a sampled impact-dot cloud for cone weapons).
- **Sprint & Sneak** — OXCE's hidden Ctrl/Alt movement modes surfaced properly: colored path
  previews, faster/slower animation, sprint commits to its path, and movement modes change the
  mover's reaction-fire *evasion* (a new defensive stat split from reactions).
- **Sneak light gate** — units emitting light (personal light, carried lit flares/torches, burning)
  can't creep; the personal-light key now toggles the **selected unit** (Ctrl+key = whole squad),
  making "go dark to sneak" a real tactical choice.
- **Bleedout** — soldiers dropping below 0 HP bleed out instead of dying outright (buffer wounds,
  on-map dying indicators, HP-bar wound marks); stabilized soldiers are out for the mission but
  recoverable. Medikit target-status readout included.
- **Soldier Roles** — player-authored role system (create/rename/re-icon/recolor/delete in-game),
  with icon badges on Soldier Info and the battle inventory, rank-line abbreviations (`MRK-Rookie`),
  a role map-marker replacing the selected-unit arrow, per-role equipment kits (save/apply), and
  per-role armor colors. Default roles + icons ship for both UFO and TFTD.
- **Overwatch cancellation on turn** — explicit turn orders drop overwatch (reaction pivots don't).

*Strategy layer & UI*

- **Geoscape activity display** — last-month score/economy readouts (toggleable, default on).
- **Maximized info screens** — every info/detail screen can drop to 320×200 (extends the OXCE
  option to all screens).
- **Inventory QoL** — ammo-count badges on weapons/clips, a contextual item info panel (stats,
  shot modes, medikit charges), the full unit stat block, mousewheel ground scrolling, and pre-battle
  move-TU costs.
- **Basescape QoL** — craft stats on Craft Info, soldier debriefing outcomes (`KIA/MIA/WND/OK` +
  recovery days), per-clip round counts on Buy/Sell/Transfer/Stores screens, visible craft-loadout
  Save/Load buttons, and a reworked Soldier Info screen (craft assign toggle, direct inventory
  access).
- **New Battle QoL** — edit soldier inventories before starting, one-click craft Fill, and loadout
  templates/roles persist across sessions.

### Moddable / opt-in capabilities

These ship in the engine but activate via ruleset keys or (off-by-default) options — the stock game
is unchanged until a mod (or the bundled `dx-test` testing mod) enables them:

- **Aim-cone firing model** (`baseAccuracy` per weapon) — physical 3D deflection cones for soldier
  and weapon error replacing the native scatter, with kneel/two-handed/exhaustion/smoke modifiers,
  a cover-aware hover hit-% readout, and effective-range display in the action menu.
- **Burst fire** (`tuBurst` per weapon) — a fourth firing mode alongside snap/aimed/auto.
- **Overwatch** (`overwatchRange`/`overwatchConeAngle`/… per weapon, or mod-wide
  `overwatchDefaults:`) — reserved-TU held fire over a directional cone, with trigger-tile markers
  and one-free-shot arming.
- **Blast radius dropoff** (`blastDropoff` per item) — center-weighted explosion falloff.
- **Armor degradation** (`ToArmor*` damage-type knobs) — armor wear from blocked and penetrating
  hits, tunable per damage type.
- **Editable base damage types** (`damageTypes:` node) — retune any built-in damage type mod-wide.
- **Item stat systems** — items/armor granting flat stats or percentage modifiers
  (`stats`/`statModifiers`), and items contributing per-side armor (`frontArmor`/`sideArmor`/…).
- **Inventory architecture** — per-armor inventory layouts (`inventoryLayouts:`), typed slots
  (battle-type filters, combat-locked slots, stat gating, move-cost allow-lists), single-item
  utility/equipment sockets (with an in-combat use hotkey), and configurable hand slots.
- **Per-round ammo economy** (`battleClipSize` per item) — rounds tracked individually, magazines
  packed at battle generation.
- **Light equipment** — torches/lanterns are plain ruleset flares (held lit items light the
  carrier, including utility-slot-worn lights), plus directional cone lights (`glowConeAngle`) that
  beam along the carrier's facing; sneak gate threshold tunable via `sneakDefaults:`.
- **Mod-wide tuning nodes** — default armor move costs (`moveCostDefaults:`), movement-mode evasion
  (`evasionDefaults:`), bleedout parameters (`bleedoutDefaults:`), proportional wound recovery with
  a Field Surgery research gate (`health:`), and soldier armor colors (`soldierArmorBaseColors:`).
- **Off-by-default options** — realistic physical throwing deviation (`battleRealisticThrowing`)
  and weight-based reload costs (`battleWeightBasedReloadCost`).
