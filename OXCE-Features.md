# OpenXcom Extended (OXCE) — Feature Overview

**Branch evaluated:** `oxce-plus` · **Engine:** OpenXcom **Extended 8.6.1** (`OPENXCOM_VERSION_ENGINE "Extended"`, build v2026-05-02)

This document describes what this branch adds on top of vanilla OpenXcom (which itself reimplements the original 1994 *X-COM: UFO Defense* / *TFTD*). OXCE is the "OpenXcom Extended" fork originally driven by Meridian; version 5.0 merged the two historical branches (OXCE and OXCE+) into one engine, and development has continued well beyond the old `Extended.txt` changelog (which only covers the early Yankes-era scripting work up to v5.0).

The single biggest theme: **OXCE turns almost every hardcoded vanilla constant into a moddable rule**, adds several genuinely new game systems, layers a full scripting engine on top, and ships a large suite of player-facing quality-of-life tools.

---

## How to read this document

Every feature is tagged by how you get access to it:

| Tag | Meaning |
|-----|---------|
| **[Player]** | **Directly usable out of the box** — a UI toggle, hotkey, option, or default behavior. Works in any mod (including base X-COM) without authoring anything. |
| **[Mod]** | **Modding-required** — you must author ruleset YAML (or scripts) to leverage it. Invisible unless a mod uses it. |
| **[Both]** | The mechanic must be *defined* by a mod, but the player *experiences or toggles* it in-game (e.g. hunter-killer UFOs, the mana bar, craft shields). |

If you are a player choosing a modpack, the **[Player]** items are what OXCE gives you for free. If you are a modder, the **[Mod]** and **[Both]** items are your toolbox.

---

## 1. Battlescape — Tactical Combat

### 1a. Player-facing tools & QoL — **[Player]**
Accessible in any battle, most via hotkeys or the on-screen **Extended Links** popup menus.

- **Night vision** — brighten the map in darkness; toggle/hold hotkeys, configurable color (`oxceNightVisionColor`) and an auto-on darkness threshold (`oxceAutoNightVisionThreshold`).
- **Personal-light toggle** and a **brightness / debug-vision cycle**.
- **Accuracy on the crosshair** — live, color-coded firing/throw/psi hit chance at the cursor, accounting for range dropoff and no-line-of-sight penalties (`oxceShowAccuracyOnCrosshair`).
- **Accuracy + TU cost shown in the action menu**, plus a **melee damage preview**.
- **Hit Log** — running log of shots/damage this turn; **Turn Diary** — scrollable per-turn history.
- **Fatal-wounds readout** and **experience overview** (which soldiers gained/haven't gained XP).
- **Skill menu** — trigger soldier special abilities in combat (hotkeys 1–5), with TU/mana cost shown (definitions are modded; see §4/§5).
- **Special-weapon button** for built-in medikits/scanners; **action-item hotkeys 1–5**; **reload** hotkey.
- **Camera & selection**: single-layer view, center-on-enemy 1–10 with HUD counters, colored unit markers, select-without-centering, reopen briefing, **in-battle music jukebox**, mute unit barks.
- **Off-centre shooting** (auto-adjust firing angle when line-of-fire is blocked) and **uniform/tighter shooting spread** options.
- **Reaction-fire threshold** — stop your units wasting reaction shots below a chosen hit chance (`oxceReactionFireThreshold`); pin a **preferred reaction weapon** and disable a hand for reactions via inventory right-click.
- **Inventory QoL**: equipment templates (save/apply named layouts), per-soldier personal equipment, quick-search, ctrl-click auto-move, auto-equip, clear inventory, armor/avatar/Ufopaedia shortcuts, unload fixed weapons.
- **Medikit UI** — click a body part to heal a specific limb; **prime-grenade UI** with a 0–23 fuse grid, primeable from the inventory.
- **Touch buttons** — on-screen Ctrl/Alt/Shift/RMB/MMB modifiers for tablets.

### 1b. Damage & combat model — **[Mod]** / **[Both]**
OXCE replaces the vanilla "power → armor → HP" pipeline with a fully configurable one (`RuleDamageType`, `RuleItem`, `RuleStatBonus`):

- **Multi-channel damage** — a single hit splits power independently into HP / stun / fatal-wounds / energy / morale / TU / mana / item / tile damage (`ToHealth`, `ToStun`, `ToWound`, …). **[Both]**
- **Selectable damage-spread models** — UFO, TFTD, flat, two-dice, etc., with per-channel randomness toggles. **[Mod]**
- **Armor-piercing / partial armor** — `ArmorEffectiveness` (ignore a fraction of armor), pre-armor damage, and a resistance channel decoupled from the visual type. **[Mod]**
- **Power range dropoff** — projectiles/explosions lose power over distance (`powerRangeReduction` + threshold). **[Both]**
- **UFOExtender-style accuracy dropoff** — per-fire-mode min/max range band with accuracy penalty outside it; **no-LOS accuracy penalty**. **[Both]**
- **Stat-driven formulas** — accuracy and damage become polynomials of *any* unit stat (strength, reactions, mana, current HP/energy, rank, wounds…), replacing vanilla's hardcoded math (`RuleStatBonus`). **[Mod]**
- **Overkill / gib propagation** — excess damage past a unit's `overKill` threshold spills into its inventory/corpses. **[Both]**
- **Explosion tuning** — blast radius, per-tile falloff, fire/smoke thresholds, directional vs omnidirectional. **[Mod]**
- **Tunable global combat constants** — a `constants:` block flips behaviors like extended melee reactions, terrain melee, berserk-with-aimed, running cost, experience-award system, etc. **[Mod]**

### 1c. Items & weapons — **[Mod]** / **[Both]**
- **Per-fire-mode config** (`confAimed/Auto/Snap/Melee`) — each mode gets its own accuracy, range, shot count, ammo slot, cost, arcing flag, and custom name. **[Both]**
- **Configurable auto-shot count** (vanilla's fixed 3 becomes tunable). **[Both]**
- **Multi-slot ammo** — up to 4 ammo slots per weapon, each with its own compatible-ammo list and load/unload cost. **[Mod]**
- **Six-dimensional action costs** — using/throwing/priming can spend time, energy, morale, health, stun, and mana; flat vs percentage cost. **[Both]**
- **Shotguns** — pellet count, cone behavior, spread, choke. **[Both]**
- **Spray / auto-walk fire** and **generalized guided/waypoint weapons** (Blaster logic for any weapon). **[Both]**
- **Arcing shots**, tunable bullet/explosion speed, and land/underwater **throw range & dropoff**. **[Both]**
- **Fuse mechanics** — instant/set/timed fuses, throw/proximity trigger & explode events, mines hidden on the minimap. **[Both]**
- **Special / empty-hand weapons** — fixed weapons with an on-HUD button (medikit/scanner style). **[Both]**
- **Spawn-on-use / zombify** — items can spawn units or items, or zombify targets (by armor, gender, or type, with chance/faction). **[Both]**
- **Extended medikit** — heal/stimulant/painkiller modes, target masks (self/immune/ground/standing), mana & morale recovery. **[Both]**
- **Targeting restrictions** — faction target-matrix, LOS-required, underwater/land-only, psi/mana-required, convert-to-civilian. **[Mod]**
- **Experience-training control** — pick exactly which stat each weapon trains (~35 modes). **[Mod]**
- **Vapor trails**, AI use-weighting & delays, melee-with-ranged-weapon (`meleePower`), fire extinguishers, turret types. **[Mod]**

### 1d. Melee, psi & CQB — **[Both]** / **[Mod]**
- **Melee dodge & backstab** — targets dodge based on a stat formula, reduced when hit from the flank/rear. **[Both]**
- **Close-Quarters Combat (CQB)** — automatic reactive melee stab when an enemy steps into an adjacent tile. **[Both]**
- **Melee reactions**, **terrain melee** (attack tiles, never miss), and **multi-hit melee** (AI swings several times). **[Mod]**
- **Psi range limits & psi-vision** (see through walls), plus XP for resisting psi. **[Both]**
- **Throwing** reworked with half-weighted accuracy and separate land/underwater ranges. **[Mod]**

### 1e. AI — **[Player default]** / **[Mod]**
- **Sniper / spotter intel-sharing** — units share sightings so snipers fire at targets their spotters see. **[Mod]**
- **AI self-preservation** — hostiles won't blow themselves up (`explosiveEfficacy`). **[Player default]**
- **Per-unit AI knobs** — intelligence, aggression, Leeroy-Jenkins mode, weapon-pickup behavior, fire avoidance, surrender/auto-surrender, ignored-by-AI, cosmetic units. **[Mod]**
- **Bug Hunt mode** — after a configurable turn/threshold, remaining stragglers are auto-revealed/flagged so mop-up isn't tedious. **[Both]**

### 1f. Vision, lighting & environment — **[Both]** / **[Mod]**
- **New light system** — separate configurable ranges for static (tile/fire) vs dynamic (unit/item) light, and per-faction personal light. **[Both]**
- **Smoke/fire vision attenuation** — line-of-sight traced through voxel-space smoke/fire density; heat-vision armor sees through it; **fire cancels camouflage**. **[Both]**
- **Camouflage/stealth** — day/night camo, anti-camo, psi-camo, always-visible flags on armor. **[Mod]**
- **`RuleEnviroEffects`** — per-terrain environmental damage-over-time (chance/turn, body part, damage type), palette shifts, forced protective-armor swaps on deploy (e.g. space/underwater suits), map background tint. **[Both]**

---

## 2. Geoscape — Strategic Layer

### 2a. Player-facing tools & QoL — **[Player]**
- **UFO Tracker** — one list of every detected UFO / mission site / alien base with size, altitude, heading, speed (hotkey **T**).
- **Tech Tree Viewer** — browsable, color-coded tree of research / manufacture / facilities / items / crafts with prerequisites and availability (hotkey **Q**).
- **Global overviews** across all bases: **Production**, **Research**, **Alien Containment** (hotkeys P / C / J).
- **Research Diary** and **Daily Pilot Experience** screen.
- **Notes** — an in-game notepad saved with your game.
- **Music jukebox** to pick any track.
- **Extended Geoscape Links** hub menu collecting all of the above (optionally replacing the Funding button).
- **Intercept table upgrades** — resizable list, ETA column, maintenance-time (refuel/rearm/repair) column, sort craft by distance to target.
- **Craft auto-patrol**, **ignore-UFO** flag, slacking/training base indicators, UFO-landing alerts, "go to nearest base," configurable time-slowdown near events, and periodic **geoscape autosaves**.

### 2b. Alien missions & campaign scripting — **[Mod]**
- **Mission Scripts / Arc Scripts / Event Scripts** — a unified scheduling system gated by month, difficulty, score, funds, research, items, facilities, soldier types, and X-COM presence in a region/country. Arc scripts drive multi-mission **story arcs**; event scripts fire one-off **geoscape events**.
- **Extended mission objectives** — instant retaliation, supply, plus per-wave placement control (spawn on landing site / on an X-COM base).
- **Alien-base "gen missions"** — bases spawn their own follow-up attacks with their own region/race weights.
- **Retaliation tuning** — skip-scouting, multi-UFO retaliation, ignore base defenses, per-mission odds, research-interrupted missions, and **endless infiltration**.
- **Mission operation bases** — missions physically operate from space / an existing base / a new base / earth / a hunt.

### 2c. Geoscape events — **[Both]**
- **`RuleEvent`** pop-ups deliver score/funds, items (fixed/random/weighted), soldiers, spawned craft or persons, free research, cutscenes, and custom music/background; region- or city-specific; can *remove* items too. The player sees and resolves the dialog; instant-delivery is toggleable.
- **Country pact/rejoin events** — a country can fire an event when infiltrated or when it rejoins X-COM.

### 2d. UFO behavior & interception — **[Both]** / **[Mod]**
- **Hunter-Killer UFOs** — UFOs that actively chase your craft/bases; configurable chase mode, speed, and behavior (flee / kamikaze / random). In a dogfight the roles flip: the UFO is the aggressor. **[Both]**
- **Escort UFOs** — protect other UFOs of the same mission; your own craft can escort each other and speed-match. **[Both]**
- **Tractor beams** — craft weapons that accumulate slowdown on a UFO and can force it down. **[Both]**
- **Missile craft** — one-shot self-destruct interceptors; **missile UFOs** that model incoming strikes. **[Mod]**
- **Craft shields** — a regenerating shield layer over craft HP, with bleed-through and recharge rules. **[Both]**
- **Fake-water landings & splashdown survival**, softlock-breaking for stuck dogfights, per-UFO custom alert sounds/images, and per-race stat bonuses. **[Both]**/**[Mod]**
- **Chance-based radar detection** — facilities contribute a detection *chance* scaled by target visibility, with separate short/long-range and hyperwave handling (rather than guaranteed detection). **[Both]**

### 2e. Regions, countries & globe — **[Mod]**
- Region/country **base-function grants & bans**, **funding caps**, weighted mission zones with per-area terrain/textures, extra globe labels with zoom thresholds, and **texture→terrain/deployment/starting-condition mapping** (including fake-underwater terrain that runs underwater-style battles on land).

---

## 3. Basescape — Bases, Economy, Research & Manufacture

### 3a. Base facilities & the base-function system — **[Mod]** / **[Both]**
- **Base functions** (`provideBaseFunc` / `requiresBaseFunc` / `forbiddenBaseFunc`) — a 128-slot capability system: facilities provide named "services" that research, manufacture, purchases, hiring, and transformations can require or be blocked by. This is the backbone of gated tech trees. **[Mod]**
- **Item build costs** — facilities can cost items (not just money) to build, with per-item refunds. **[Mod]**
- **Facility upgrades / build-over** — build one facility on top of another, upgrade-only facilities, leave-behind-on-sell with rebuild timers. **[Mod]**
- **Multi-tile / non-square facilities**, per-facility **max-allowed** caps, and **right-click custom actions**. **[Mod]**
- **Sick-bay / recovery facilities** — accelerate wound/mana/health recovery per day. **[Both]**
- **Training rooms (gyms)** — physical (non-psi) training capacity. **[Both]**
- **Facility defenses with ammo**, variable mind-shield power, stacking grav-shields, and **destructible base facilities** (UFOs can bombard and damage individual buildings). **[Both]**

### 3b. Research — **[Mod]** (with **[Player]** viewers)
- **Get-one-free** (random or sequential, with protected/conditional pools), **disable/re-enable** other topics, **spawn items or events** on completion, **repeatable** research (interrogations), **needed-item** handling (consumed/returned/held), custom counters, score points, `lookup` article redirection, and `unlockFinalMission`. Player-facing: **Tech Tree Viewer**, **Global Research overview**, **Research Diary**.

### 3c. Manufacture — **[Mod]** (with **[Player]** QoL)
- **Multiple / random produced items**, **produce or consume craft**, **spawn soldiers/engineers/scientists**, base-function requirements, refunds on cancel, per-unit score, completion events, and category filters. **[Mod]**
- **Manufacture shortcuts** — auto-derive a recipe that breaks sub-components down to raw materials. **[Mod]**
- **Player QoL**: infinite production (∞), **auto-sell of output**, fallback queueing, a **manufacture dependency-tree viewer**, and a **Global Manufacture overview**. **[Player]**

### 3d. Purchase / sell / transfer / storage QoL — **[Player]**
- Quick-search and category filters on all buy/sell/transfer lists; **sell-all / sell-all-but-one** hotkeys; sortable rows; **sell directly from the debriefing screen**; an **item-locations viewer** (which base/craft holds an item); overfull-store handling that separates normal from critical overflow.
- Purchase/hiring gating by research, base function, allied country, or **monthly buy limits**. **[Mod]**

### 3e. Alien containment — **[Both]** / **[Player]**
- **Multiple independent prison types** — facilities and live-alien items declare a `prisonType`, so you can have separate containment pools (e.g. small vs large aliens, or aliens vs other captives), each with its own capacity accounting. **[Mod]** to define; **[Player]** per-base and **Global Alien Containment** management screens, with kill/sell/transfer of prisoners and over-capacity warnings.

---

## 4. Soldiers, Careers & Units

This is one of the most heavily expanded areas. Mostly **[Mod]** to configure, with rich **[Player]** management UI.

- **Multiple soldier types/classes** with per-type stat ranges, caps, salaries, and list ordering; **nationalities & generated callsigns**; expanded **avatar/look system** (many more portrait variants than vanilla's four) with an in-game **avatar picker**. **[Both]**
- **Separate training stat caps** and **physical (gym) training** toward them, **anytime psi training**, and **return-to-training-when-healed**. **[Both]**
- **Mana stat** and **soft-wound (health-missing) recovery** pools with facility-scaled daily regen; deployment can be blocked if too much mana/health is missing. **[Both]**
- **Soldier Transformations** (`RuleSoldierTransformation`) — a whole projects system to upgrade/convert soldiers: change class or armor, re-roll or flat/percent-adjust stats, clone, or even **resurrect the dead** and produce items instead of soldiers; gated by research, base functions, items, commendations, rank, stats, and prior transformations. **[Both]** (player runs projects; modder defines them).
- **Soldier Bonuses** (`RuleSoldierBonus`) — permanent stat/armor/vision/regen packages granted by commendations or transformations. **[Mod]**
- **Soldier Skills** (`RuleSkill`) — battlescape-activated special abilities with their own targeting, cost, weapon compatibility, and bonus requirements. **[Both]**
- **Commendations / medals** (`RuleCommendations`) — auto-awarded from an extensive **soldier diary** (kills by rank/race/weapon, missions by region/type, accuracy, months of service, dozens of special counters), with escalating decoration levels that can grant soldier bonuses. Player-visible in the **Diary Performance** UI. **[Both]**
- **Pilots** — craft can require piloting soldiers whose stats and bonuses affect dogfight accuracy/dodge/approach speed; pilots gain **dogfight experience**. In-game pilot-assignment UI. **[Both]**
- **StatStrings** — auto-generated nickname tags based on stat thresholds; **personal equipment layouts** that follow a soldier onto their craft; armor backup/restore across swaps. **[Both]**/**[Player]**
- **Manual vs automatic promotions** toggle. **[Player]**

---

## 5. Craft & Equipment Loadouts

- **Rich craft stats** — additive stat modifiers from mounted weapons/modules (`RuleCraftStats`): shields, hit/avoid/power bonuses, armor, sight/radar. **[Mod]**
- **More weapon slots with typed slotting** (up to 4 slots, 8 allowed types each), fixed/built-in craft weapons, and per-slot UI strings. **[Mod]**
- **Granular capacity limits** — separate caps for small/large soldiers and vehicles, plus item and storage-space limits. **[Mod]**
- **Craft skins** (cosmetic variants), soldier/armor-group boarding restrictions, spacecraft/altitude rules (Mars-capable, water-only), refuel-by-item, radar-invisible craft, auto-patrol, and purchase gating. **[Both]**/**[Mod]**
- **Craft weapon categories** — weapon / tractor-beam / pure-equipment module; unified damage formula; statistical ammo saving. **[Mod]**
- **Craft equipment templates** and **alternate craft-equipment management** (moving soldiers moves their gear). **[Player]**
- **Starting Conditions** (`RuleStartingCondition`) — per-mission environment rules that restrict/replace allowed armor, craft, items, item-categories, vehicles, and soldier types (e.g. force EVA suits, ban tanks), require carried items, or require a commander onboard. **[Mod]**

---

## 6. Modding & Scripting Engine

Almost entirely **[Mod]** — this is the modder's toolbox that makes the rest of OXCE possible.

### 6a. Y-Script scripting engine
A full embedded, statically-typed scripting VM (`src/Engine/Script.*`, hooks in `src/Mod/ModScript.h`). Scripts attach to named hooks in rulesets under `scripts:`. Capabilities:
- Typed local variables and **typed pointers to live game objects** (BattleUnit, BattleItem, Soldier, Craft, Ufo, Mod, SavedGame, RuleItem, Armor…), full arithmetic/logic, RNG, debug logging, and per-type getters/setters.
- **Global "event" scripts** that multiple mods can layer onto the same hook without conflict.

Major hooks (what each controls):
- **Sprites/sound (cosmetic):** `recolorUnitSprite`, `selectUnitSprite`, `selectMoveSoundUnit`, `recolorItemSprite`, `selectItemSprite`, vapor/particle FX.
- **Combat resolution:** `hitUnit`, `damageUnit` (+ `…Ammo` and `damageSpecialUnit` variants), `healUnit`, `awardExperience`.
- **Attack gating:** `tryPsiAttackUnit/Item`, `tryMeleeAttackUnit/Item`, `skillUseUnit` (return 0 blocks the action).
- **Reactions:** `reactionUnitAction`, `reactionUnitReaction`, `reactionWeaponAction`.
- **Lifecycle:** `createUnit`, `newTurnUnit`, `returnFromMissionUnit`, `createItem`, `newTurnItem`.
- **Vision & AI:** `visibilityUnit` (override spotting), `aiCalculateTargetWeight` (AI target scoring).
- **Economy:** `sellCostItem`, `buyCostItem` (dynamic prices); `newMonthCountry` (funding/score).
- **Detection:** `detectUfoFromBase`, `detectUfoFromCraft`.
- **Stat-formula scripts:** `damageBonus`, `meleeBonus`, `accuracyMultiplier`, `meleeMultiplier`, `throwMultiplier`, `closeQuartersMultiplier`, `psiDefence`, `meleeDodge`, per-turn recovery formulas, `applySoldierBonuses`.
- **Ufopaedia:** `statsForNerdsArmor/Item/Ufo/Craft` (inject custom info lines).

### 6b. Custom tags & global variables — **[Mod]**
- **Custom tags** (`ScriptValues<T>`) — declare arbitrary named per-object variables on most rule and save objects (items, units, soldiers, craft, armor…); they persist in saves.
- **Global variables** — mod-wide persistent scripted state; shareable tag-definition files.

### 6c. Ufopaedia enhancements — **[Both]**
- Full **TFTD-style article set** and dedicated **Soldier/Unit** article types.
- **Stats for Nerds** — a deep raw-ruleset dump per item/armor/UFO/craft (scriptable, toggleable).
- Extended item info — resistances, damage types, per-armor modifiers, clip size; custom colors/backgrounds per article.

### 6d. Interface & palettes — **[Mod]** (one **[Player]** fix)
- **`RuleInterface`** — per-screen override of palette, backgrounds, music, sounds, and per-element colors/borders.
- **`CustomPalettes`** — define/replace palettes from external 256-color palette files.
- **Palette flicker fix** and raw screenshots. **[Player]**

### 6e. Mod loading, dependencies & resources — **[Both]** / **[Mod]**
- **Master mods** (total conversions) with sub-mod chaining; **engine/version requirements** (a mod can require "Extended ≥ X"), surfaced to the player as a validation dialog when enabling incompatible mods.
- **`reservedSpace`** — mods reserve index ranges in shared sprite/sound sets so they don't collide.
- **ExtraSprites / ExtraSounds / ExtraStrings** — inject sprites (single images, spritesheets, whole folders, auto-sliced grids), sounds, and translation strings.
- **RuleConverter / Save Converter** — import original 1994 X-COM saves via modder-defined ID mappings. **[Both]**
- **VFS layering**, lazy resource loading, and a configurable **mod-validation strictness** level.

---

## 7. General UI, Options & Persistence — **[Player]**

- **Quick Search** (default **Q**) filter box on most list screens.
- **Multiple / periodic autosave slots**; **insta-save / quick-save-load** hotkeys.
- **OXCE clickable links** between related Ufopaedia / tech-tree / stats screens; **highlight new topics**; **touch/thumb buttons** and "fat-finger" enlarged buttons for tablets.
- **Update check** (online), **recommended-options** prompt on first run.
- A large set of rebindable **hotkeys** across geoscape, basescape, and battlescape (navigation, loadout templates, training add/remove, night vision, action items, music track, graph zoom…).
- Many previously-hidden vanilla options exposed in the OXCE options UI (lazy loading, VSync, etc.).

---

## Quick reference

**What you get for free as a player (works in any modpack, including base X-COM):**
Night vision · crosshair accuracy · reaction-fire threshold & preferred reaction weapon · hit log / turn diary · equipment & craft-loadout templates · quick search · UFO Tracker · Tech Tree Viewer · global research/production/containment overviews · notes · music jukebox · intercept table upgrades · craft auto-patrol · manual/auto promotions · sell-all hotkeys · sell-from-debriefing · item locations · multiple autosaves · touch controls · bug-hunt mode · off-centre shooting.

**What needs a mod to appear (the modder's toolbox):**
The multi-channel damage model, stat-driven accuracy/damage formulas, per-fire-mode weapon configs, multi-slot ammo, shotguns/spray/guided weapons, spawn/zombify, extended medikits, CQB & sniper/spotter AI, the mana stat, soldier skills/bonuses/transformations/commendations, the base-function tech-gating system, research/manufacture extensions, multiple prison types, craft shields/tractor beams/pilots, hunter-killer UFOs, story arcs & geoscape events, environmental effects & starting conditions, the entire Y-Script engine, custom tags, Ufopaedia article types, and interface/palette theming.

**Things that are a mod mechanic but you feel as a player ([Both]):**
Hunter-killer UFOs chasing you · craft shields & tractor beams in dogfights · the mana bar · story events popping up · bug-hunt auto-reveal · multiple containment types · environmental damage forcing protective suits · pilots improving dogfights.

---

*Sources: this overview was assembled by reading the branch source directly — chiefly `src/Mod/Rule*.{h,cpp}` (the ruleset surface), `src/Engine/Options.cpp` (the `OPTION_OXCE`-tagged options), `src/Engine/Script.*` and `src/Mod/ModScript.h` (the scripting engine and hook registrations), the `Extended*LinksState` UI (the in-game feature menus), and the battlescape/geoscape/basescape state classes. `Extended.txt` in the repo root documents only the early Yankes-era scripting work up to mod v5.0 and is far from complete for 8.6.1.*
