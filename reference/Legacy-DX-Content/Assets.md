# xcomtd — Custom Assets

This document catalogues the custom assets shipped with the legacy "X-COM: Terror Defense" master mod
(`D:\Code\Projects\OpenXcomDX-Legacy\bin\standard\xcomtd\`, `id: xcomtd`, master on `xcom1`,
authors "Microprose, OpenXCOM, Xusilak, Darineth"). Sources inventoried: the `Resources/` tree
(832 files — 631 GIF, 195 PNG, 5 Paint.NET `.pdn` sources, 1 WAV; ~4.1 MB), the TFTD terrain port in
`MAPS/` / `ROUTES/` / `TERRAIN/`, `GEODATA/PALETTES.DAT`, the `CelebrateDiversity/` name pool,
`Language/en-US.yml`, and the rulesets that consume them (`ExtraSprites.rul`,
`ExtraSprites_Diversity.rul`, `Overhaul.rul`, `Commendations.rul`, `AlienArmor.rul`,
`GlobeTextures.rul`, `Items.rul`, `Terrain.rul`, `Palettes.rul`, `Crafts.rul`, `Vehicles.rul`,
`Soldiers.rul`). The optional third-party packs in `bin\user\mods\` are listed at the end.

Provenance legend used throughout:

- **[custom]** — art made for the mod (much of it has `.pdn` Paint.NET sources alongside).
- **[borrowed]** — taken from TFTD or from community OpenXcom mods.
- **[vanilla]** — vanilla UFO assets reused by index/reference (no file shipped).

---

## 1. Resources/ tree

| Folder | Files | Format | Purpose | Consumed by |
|---|---|---|---|---|
| `AirCombat/` | 4 GIF + 1 `.pdn` | 320×200 screen, 32×32 icons, 155×31 cursor strip | UI for the legacy DX **New Air Combat** screen: `AirCombatScreen`, `AirCombatAimCursor` (5-frame 31×31 strip), `UnknownCraft`/`UnknownUfo` blips. **[custom]** | `ExtraSprites.rul` (`AirCombatScreen`, `AirCombatAimCursor`, `AirCombatUnknownCraft/Ufo` surface types read by the legacy engine) |
| `AlienInventory/` | 14 PNG | 320×200 paperdolls | Inventory-screen paperdolls for every alien race + male/female civilians (sectoid, floater, snakeman, muton, ethereal, chryssalid, reaper, celatid, silacoid, zombie, cyberdisc, sectopod, civilianm/f). **[borrowed]** community alien-inventory paperdoll pack (?) | `AlienArmor.rul` (`spriteInv` singleImage entries per alien armor) |
| `Armor/` | 11 PNG + 1 GIF | paperdolls + 256×1400 sprite sheet | `StealthArmor/`: full battlescape sheet (`StealthArmorSprite.gif`, 32×40 cells) + 8 M/F×4 look paperdolls; `FlyingArmor/FlyingArmorInv.png`; `ScoutArmor.png`, `StealthArmor.png`. **[custom]** recolors of vanilla armor art | Registered in `Overhaul.rul` as `STEALTHARMOR.PCK` + `STEALTHARMORINV{M,F}{0-3}.SPK` — **but `Armors.rul` still points stealth armor at vanilla `XCOM_1.PCK`/`MAN_1`, so all of `Armor/` is effectively unused/WIP** |
| `BulletSprites/` | 2 PNG | 105×33 sheets of 3×3 cells (35 frames × 11 rows) | Full replacement of the vanilla `Projectiles` bullet-sprite sheet, plus a blue-tinted `BulletSprites-Underwater.png` for the legacy DX underwater-battle feature (`UnderwaterProjectiles` surface set). **[custom]** | `ExtraSprites.rul` |
| `Commendations/` | 55 PNG (`medal_100..154`) | 31×8 ribbons | Commendation ribbon strip, folder-loaded from index 100. **[borrowed]** from the community Soldier Diaries/Commendations mod | `Commendations.rul` (`Commendations` surface set) |
| `CommendationDecorations/` | 10 PNG (`award_0..9`) | 31×8 | Ribbon decoration overlays (levels). **[borrowed]** same origin | `Commendations.rul` (`CommendationDecorations`) |
| `Medals/` | 57 PNG (`MEDAL_*.png`) | 320×200 | Full-screen medal images for each commendation's Ufopaedia-style page (56 used + `MEDAL_PLACEHOLDER`). **[borrowed]** same origin | `Commendations.rul` (one singleImage sprite per medal); `MEDAL_MELEE.png` is unreferenced |
| `Craft/` | 8 GIF + 1 `.pdn` | 32×32 combat icons, basescape/dogfight/pedia images | `Skyranger/` + `Interceptor/` combat icons for the air-combat screen; `Tornado/` — complete art set for the new Tornado interceptor: `Tornado.gif` (Ufopaedia image, overrides `UP002B.SPK`), `TornadoBase.gif` (`BASEBITS.PCK` 56), `InterceptorBase.gif` (`BASEBITS.PCK` 57), `TornadoMinimized.gif` (`INTICON.PCK` 23), `TornadoDogfight.gif` (`INTICON.PCK` 34), `TornadoCombat.gif`. **[custom]** (Tornado craft art possibly adapted from a community craft mod ?) | `ExtraSprites.rul` (`AirCombatSprites` 1–3), `Overhaul.rul` (`UP002B.SPK`, `BASEBITS.PCK`, `INTICON.PCK`); `Crafts.rul` `combatSprite:` fields |
| `Flags/` | 40 PNG (`00-USA` … `39-Ukraine`) | small flag icons | Nationality flags matching the 40 CelebrateDiversity name pools. **[borrowed]** community "Celebrate Diversity" flag pack | `ExtraSprites_Diversity.rul` (`Flag0`–`Flag39` singleImage types, shown by OXCE soldier-nationality UI) |
| `GlobeUFO/` | 39 GIF | globe textures (13 textures × 3 zoom levels) | Vanilla UFO geoscape terrain textures extracted to GIF. **[vanilla]** re-encoded | `GlobeTextures.rul` — rebuilds `TEXTURE.DAT` as a 78-entry set |
| `GlobeTFTD/` | 39 GIF (+ 39 in `Originals/`) | globe textures | TFTD ocean-floor geoscape textures, merged with the UFO set so the expanded `Globe.rul` can texture ocean polygons for underwater missions. **[borrowed]** from TFTD. `Originals/` = unedited backups, unreferenced | `GlobeTextures.rul` (`TEXTURE.DAT` indices interleaved with GlobeUFO) |
| `Items/` | ~420 GIF + 1 WAV + 1 `.pdn` in ~60 subfolders | 32×48 bigobs, floorobs, 8-frame handob sets, 3×3 projectile strips | The mod's big weapon/equipment art library — one folder per item family (see §3). **[custom]**, largely edits/recolors of vanilla weapon art | `Overhaul.rul` `extraSprites` (`BIGOBS.PCK` 1000+, `FLOOROB.PCK` 1000+, `HANDOB.PCK` folder-loaded 8-frame sets, `Projectiles` rows 385–525) and `extraSounds` |
| `Roles/` | 6 PNG + 1 `.pdn` | 23×23 + 16×16 icons | Soldier-role icons (MachineGunner, AntiArmor, Marksman; large + "Simple" small variants) for the legacy DX **soldier roles** feature. **[custom]** | `Overhaul.rul` `roles:` + named `extraSprites` types |
| `UFO/` | 8 GIF | 32×32 combat icons | Air-combat screen icons for all 8 UFO classes (Small/Medium/Large Scout, Harvester, Abductor, Supply Ship, Terror Ship, Battleship). **[custom]** | `ExtraSprites.rul` (`AirCombatSprites` 101–108) |
| `Vehicles/` | 1 PNG + 1 GIF + 1 `.pdn` | 320×200 | `Inventory/Tank.png` — HWP inventory-screen image for the legacy DX **HWP loadout/inventory** feature. **[custom]** (`Tank.gif` + `CustomTank.pdn` are working copies, unreferenced) | `Overhaul.rul` (`tankInventoryImage`) |

## 2. Sprite/sound registration (rulesets)

- **`ExtraSprites.rul`** — bullet-sprite sheet replacements (`Projectiles`, `UnderwaterProjectiles`) and the whole New Air Combat surface family (`AirCombatScreen`, `AirCombatBackground`, `AirCombatUnknownCraft/Ufo`, `AirCombatSprites` with craft at 1–3 and UFOs at 101–108, `AirCombatAimCursor`). These are custom surface types consumed by legacy engine code, not by vanilla OpenXcom.
- **`ExtraSprites_Diversity.rul`** — `Flag0`–`Flag39` nationality flags (one singleImage each).
- **`Overhaul.rul` `extraSprites:`** (lines ~620–1121) — the master item-art registry: ~112 `BIGOBS.PCK` entries and ~90 `FLOOROB.PCK` entries at indices 1000–1649 (blocks of 10 per item family), 28 `HANDOB.PCK` folder-loaded 8-frame hand-object sets, 5 extra `Projectiles` rows (indices 385/420/455/490/525 = new bullet types 11–15: sniper bullet, laser-beam-rifle beam, plasma-beam-rifle beam, heavy laser beam, heavy plasma beam), the stealth-armor sheet/paperdolls, Tornado craft art (`UP002B.SPK`, `BASEBITS.PCK` 56–57, `INTICON.PCK` 23/34), the three role icon pairs, and `tankInventoryImage`.
- **`Overhaul.rul` `extraSounds:`** (lines 1122–1125) — exactly **one** custom sound: `BATTLE.CAT` index 55 = `Items/SniperRifle/SniperRifleFire.wav`.
- **`Commendations.rul`** — `Commendations` (folder-load), `CommendationDecorations` (folder-load), plus 57 singleImage medal screens.
- **`AlienArmor.rul`** — 14 alien/civilian `spriteInv` paperdolls.
- **`GlobeTextures.rul`** — replaces `TEXTURE.DAT` with 78 GIFs (39 GlobeUFO + 39 GlobeTFTD).
- Note: `CraftWeapons.rul_NewAirCombat` and `UFOs.rul_NewAirCombat` have a non-`.rul` extension, so they are **not loaded** — parked config for the New Air Combat feature (the air-combat *art* in `ExtraSprites.rul` still loads).

## 3. How new weapons get their look and sound (Items.rul pattern)

- **Big/floor sprites:** ~112 items use custom `bigSprite:` indices ≥1000 (the Overhaul.rul registry above); only ~25 items keep vanilla indices <1000 (grenades, flares, medikit, motion scanner, mind probe, stun rod, corpses, alien artefacts, etc.). Each item family owns a block of 10 indices (e.g. Sniper Rifle 1000–1003, HWP engines 1420–1431), with matching floorob indices.
- **Hand sprites:** custom weapons use folder-loaded 8-frame handob sets at 1000+ (e.g. 1004 SniperRifle, 1144 HeavyPlasma, 1649 GrenadeLauncher); ~20 items with unchanged appearance keep vanilla `handSprite` values (0–104). The X-COM-manufactured plasma line mostly **reuses the alien plasma handobs** (only `XCOMPlasmaMinigun` gets its own set at 1514); the duplicate `HandObjects/` folders under the other `XCOMPlasma*` directories are shipped but never registered.
- **Fire sounds:** almost entirely vanilla `BATTLE.CAT` indices — 18 (plasma, 25 items), 11 (laser, 14 items), 4 (kinetic, 9 items), 51–53 (launchers: Small Launcher, Rocket Launcher, Arc/Blaster/HWP launchers), 12. The only new sound is index 55 (the custom sniper WAV), shared by 5 heavy/precision kinetic weapons (`STR_BASIC_SNIPER`, `STR_BASIC_HEAVY`, HWP light/heavy cannon, HWP artillery cannon).
- **Hit sounds:** 100% vanilla (0, 13, 19, 22, 36, 37).
- **Bullet sprites:** vanilla types 0–10 (redrawn wholesale by `BulletSprites.png`) plus the five new types 11–15 listed in §2. Summary: *the mod redraws sprites liberally but adds only one sound file.*

## 4. TFTD terrain port (MAPS/, ROUTES/, TERRAIN/)

All **[borrowed]** — TFTD terrain data converted for the UFO engine (community TFTD-conversion lineage), used for the legacy DX underwater-mission feature.

| Folder | Contents | Used by |
|---|---|---|
| `MAPS/` (28 files) | `SEABED00–13` (14), `CORAL00–12` (13), `LIGHTNIN.MAP` | `Terrain.rul` SEABED/SEACORAL terrain families (10 terrains: base + `_SUNKEN_UFO/_USO/_GALLEON/_CARGO_SHIP` variants, `depth: [1, 2]`, scripts in `MapScripts.rul`). `LIGHTNIN.MAP` overrides the vanilla **Lightning craft** map (`Crafts.rul`). `SEABED13` and `CORAL12` are not in any mapBlocks list — unused |
| `ROUTES/` (51 files) | RMPs matching the maps, plus `SUNKENPLANE00–20` (21), `SEA.RMP`, `SUBASE_04.RMP` | Routes for the used maps load by name. `SUNKENPLANE*`, `SEA.RMP`, `SUBASE_04.RMP` (and `SEABED13`/`CORAL12` RMPs) have **no matching map/ruleset reference — unused/WIP** (a sunken-plane crash-site terrain that never shipped) |
| `TERRAIN/` (63 files = 21 MCD/PCK/TAB sets) | `BLANKS`, `SEASAND(6)`, `SEAROCKS`, `SEAWEEDS`, `SEACORAL`, `SEADEBRIS(6/6EXT)`, `SEANOM`, `SEABITS`, `SEAURBAN`, `USOBITS`, `SEAPLANE`, `SEAMSUNK1/2`, `SEAORGANIC1–3`, `SEASUNK(6)` | 10 sets referenced (`BLANKS`, `SEASAND6`, `SEAROCKS`, `SEAWEEDS`, `SEADEBRIS6`, `SEANOM`, `SEACORAL`, `USOBITS` by the seabed/coral terrains; `SEAURBAN` + `SEABITS` by ship/port/island terrains). **11 sets unused** (`SEAPLANE`, `SEAMSUNK1/2`, `SEAORGANIC1–3`, `SEASUNK`, `SEASUNK6`, `SEASAND`, `SEADEBRIS`, `SEADEBRIS6EXT`) — staged for unfinished terrains |

Note: `Terrain.rul` also defines ~100 land terrains (AREA51*, MADURBAN, SIBERIA, PORTTFTD, CULTA-farm variants, DAWNURBAN, …) whose map/terrain files are **not shipped in the mod at all** — they come from Hobbes' community *Terrain Pack* and are expected to be present in the user's UFO data folders. Nothing matching (e.g. `AREA51*`) exists anywhere in the legacy repo.

## 5. Palettes

- **`GEODATA/PALETTES.DAT`** (3,870 bytes = the exact vanilla 5-palette format: geoscape/basescape/graphs backgrounds, 774 bytes each) — a **modified replacement** of UFO's palette file. What exactly was recolored is undocumented (?), likely tweaks supporting the new UI screens.
- **`Ruleset/Palettes.rul`** — `customPalettes:` fully redefines four 256-color battlescape palettes: `PAL_BATTLESCAPE_0` (surface) and `PAL_BATTLESCAPE_1/2/3` — TFTD-style **depth palettes** with progressively blue-shifted ramps, selected by the underwater terrains' `depth: [1, 2]`. **[borrowed]** color data derived from TFTD.

## 6. Soldier names and flags

`CelebrateDiversity/` holds 40 `.nam` name pools (`00-American` … `39-Ukrainian`, ~223 KB) from the community **Celebrate Diversity** mod. `Soldiers.rul` wires them in with `soldierNames: [delete, CelebrateDiversity/]` (replacing the vanilla pool), and each nationality's flag renders via the `Flag0`–`Flag39` sprites from §1. **[borrowed]**

## 7. Language

`Language/en-US.yml` (1,359 lines) — a single en-US string file mixing: legacy-DX engine strings (soldier avatar names, PSI inventory display, music track names), rewritten vanilla Ufopaedia text for the rebalanced craft/weapons, and all new TD item/research/facility strings. `Overhaul.rul` also carries an inline `extraStrings:` block (research category names etc.). No other locales are provided.

## 8. Missing and unused assets

**Referenced but missing on disk (1):**
- `Resources/AirCombat/AirCombatBackground.gif` — registered as `AirCombatBackground` in `ExtraSprites.rul` but the file does not exist (the screen presumably draws without it or the entry silently fails).

**Case hazards (works on Windows only):** `Overhaul.rul` references `Resources/items/...` (lowercase) for 4 handob folders and `ElectroLaser*.gif` vs on-disk `Electrolaser*.gif`.

**On disk but unreferenced by any ruleset:**
- All of `Armor/` in practice: `StealthArmor` sheet + 8 paperdolls are registered but no armor uses them; `FlyingArmorInv.png`, `ScoutArmor.png`, `StealthArmor.png` aren't referenced at all (WIP custom armor art).
- `GlobeTFTD/Originals/` (39 backup textures), `Vehicles/Inventory/Tank.gif`, `Items/Addons/Base.gif` (template), `Medals/MEDAL_MELEE.png`.
- The 5 `.pdn` Paint.NET source files (AirCombatScreen, InterceptorCombat, CustomTank, HWPSmartLauncherBig, RoleIconMarksman).
- Duplicate `HandObjects/` sets under `XCOMPlasmaPistol/SubmachineGun/Shotgun/Marksman/Sniper/MachineGun/Heavy` (only the Minigun set is registered).
- Terrain leftovers: maps `SEABED13`/`CORAL12`, routes `SUNKENPLANE00–20`, `SEA.RMP`, `SUBASE_04.RMP`, and 11 of the 21 TERRAIN datasets (§4).
- `Ruleset/Interfaces.rul` is an empty (0-byte) file; `*.rul_NewAirCombat` files are deliberately disabled.

## 9. Optional third-party mods in `bin\user\mods\`

Not xcomtd content — separate audio/visual packs in the legacy configuration ("TD" variants are the same pack cloned to target the xcomtd master):

- **Alternative Smoke** / **Alternative SmokeTD** — Robin's less-distracting smoke/fire animation (Apocalypse-style).
- **Amiga Fonts** — Amiga/PSX-style fonts for UFO Defense.
- **Craft Missile Sounds** — replacement craft-weapon (Stingray/Avalanche) firing sounds.
- **HQSounds** / **HQSoundsTD** — higher-quality remastered battlescape sound effects (~2.8 MB each).
- **PSXMusic** / **PSXMusicTD** — PlayStation-version OGG soundtrack (~181 MB each).
- **PSXSFX** — PlayStation sound effects replacing the PC ones.
- **PSXVideos** — PlayStation animated intro/outro videos replacing slideshows (~43 MB).
- **XCOM2012Music** — music from XCOM: Enemy Unknown (2012) (~15 MB).
- **XCOM2Music** — music from XCOM 2, split into geoscape/ambient/action battlescape sets (~57 MB).
