# Feature: Soldier Roles (player-authored classification + per-role loadout)

**Status:** In progress (Jul 2026). **Step 1 (data backbone) committed & building clean** — `RuleRole`
seeds + `roles:` node in `Mod`, savegame `Role` (identity + own loadout), `SavedGame` role list with
new-game seeding / accessors / save-load, and `Soldier._roleId` (additive save format). Authored
xcom1 role icons shipped as individual named `singleImage` surfaces (uncommitted).

**Icon model → named icons via named surfaces (Jul 2026):** roles reference an icon by a **stable string
name** (a new `roleIcons`/`RuleRoleIcon` registry), and each icon is an individual **named surface**
(`RoleIcon<Name>` / `RoleIcon<Name>Map`) resolved via `Mod::getSurface`. The identity is a string
end-to-end, so saved role icons survive mod-list / load-order changes with no frame-index / sprite-offset
/ `maxSharedFrames` machinery at all. This **supersedes** step 1's `int _icon` on `RuleRole`/`Role` —
that field becomes a `roleIcons`-name reference; the `roles:`/`Role`/`Soldier._roleId` plumbing already
built is otherwise unchanged. **`RuleRoleIcon` + `roleIcons:` parsing implemented and `_icon` converted
int→name — committed (b13922b87), builds clean.** **Content shipped (uncommitted):** the `roleIcons:`
registry (NONE + 14 roles) and 14 default seed roles + their display strings, all in `roles.rul`
(cross-references validated). **Template-target generalization — already satisfied:**
`InventoryState::_createInventoryTemplate` / `_applyInventoryTemplate` already take a generic
`std::vector<EquipmentLayoutItem*>&` and operate purely on the passed vector + selected unit, so a
role's `getLoadout()` can be passed straight in (no code needed). **Assignment UI shipped (uncommitted,
builds clean):** Soldier Info now shows the assigned role's badge (top-right) + a **Role** button that
opens a `RoleSelectState` picker to set/clear a soldier's role; `SoldierInfoState::init()` refreshes the
badge/label when the picker closes. `roleSelect` interface blocks added in xcom1 + xcom2; strings in
`Language/DX/`. This is the **first icon consumer** — needs an in-game visual check (badge render +
palette). **Next:** the role-management screen (create/rename/icon/colour/delete/edit-loadout), then the
battlescape marker + list columns and per-role armor colours. This is the Phase 7 **"Role Definitions &
Templates"** roadmap item, redesigned from the legacy fixed-list model into a **player-authored** system.

Roadmap items covered/affected:
- **Role Definitions & Templates** — *this pass* (data backbone + minimal assign/manage UI).
- **Role UI & Markers** — *mostly deferred* (battlescape role marker, list columns, tooltip/combat-log
  display). The minimal Soldier-Info assign entry point lands here; the rest is the next item.
- **Per-Role Armor Colors** — *deferred* (each `Role` already carries a `color`, so the data hook is
  laid here; the render wiring is the third item).

## The design pivot (Jul 2026)

The original plan (and the legacy DX fork, `Legacy-DX-Features.md` §5) made roles **mod-authored**: a
fixed `RuleRole` list from `roles.rul`, with the player only *assigning* soldiers to one of the
predefined roles. We are instead making roles **player-authored save data**:

- The engine ships a **set of role icons** (a `SurfaceSet`) the player picks from.
- The player **creates / renames / re-icons / recolors / deletes** roles in-game — no YAML editing.
- A ruleset **seeds** a handful of default roles + declares the icon set, so a fresh game has usable
  starters out of the box. After new-game, roles are fully player-owned savegame data.

**Why the pivot:** it aligns with DX's stated philosophy — *prefer visible, discoverable,
player-facing UI over fixed mod-only config* — and it maps almost perfectly onto infrastructure that
already exists (the 50 named player-authored global loadout templates). A player-created role is
essentially **a named loadout + an icon + a color + soldier assignment**. See the two agreed decisions
below.

### Agreed decisions

- **Q1 — Authorship = hybrid seed + player-editable.** A `roles:` ruleset node seeds a few default
  roles and declares the shared icon `SurfaceSet`; on new-game the seeds are copied into the savegame's
  editable role list. Thereafter the player creates/renames/re-icons/recolors/deletes freely. Works out
  of the box **and** fully player-authored.
- **Q2 — A role owns its own loadout.** Each `Role` carries its own `std::vector<EquipmentLayoutItem*>`
  template (the same type the global template slots use), stored per-role on the savegame. The existing
  50-slot global library stays a separate general-purpose tool — no coupling. Assigning a role can apply
  its loadout to the soldier; re-saving writes the soldier's current loadout back onto the role.

## OXCE / OXCE-Plus + DX audit (Jul 2026)

A thorough source audit confirmed roles are **entirely greenfield** — there is **no** `RuleRole`,
`Role`, `soldierRole`, or soldier-"role"/"specialization" concept anywhere in `src/` (the only "role"
hits are unrelated comments). What already exists to **reuse**:

- **Loadout-template plumbing (the reuse target).**
  - `EquipmentLayoutItem` ([src/Savegame/EquipmentLayoutItem.h](../src/Savegame/EquipmentLayoutItem.h))
    — the stored loadout entry (`_itemType`, `_slot`, `_slotX/Y`, `_ammoItem[]`, `_fuseTimer`,
    `_fixed`) with `load`/`save`. A role's loadout is a `std::vector<EquipmentLayoutItem*>` of these.
  - Per-soldier layouts on `Soldier` ([src/Savegame/Soldier.h:83-85](../src/Savegame/Soldier.h#L83-L85)):
    `_equipmentLayout` (game-managed, last-used), `_personalEquipmentLayout` (player-managed),
    `_personalEquipmentArmor`. Accessors + `clearEquipmentLayout()`.
  - Global named library on `SavedGame` (`MAX_EQUIPMENT_LAYOUT_TEMPLATES = 50`):
    `_globalEquipmentLayout[]` / `…Name[]` / `…Armor[]` with `getGlobalEquipmentLayout(index)` etc.
  - Create/apply/clear operations in
    [src/Battlescape/InventoryState.cpp](../src/Battlescape/InventoryState.cpp):
    `_createInventoryTemplate(vector<EquipmentLayoutItem*>&)`, `_applyInventoryTemplate(...)`,
    `_clearInventoryTemplate(...)`, plus the personal-template variants. Load/save UI in
    `InventoryLoadState`/`InventorySaveState`. **These are the functions the role loadout apply/save
    reuse directly** (a role's vector is just another `vector<EquipmentLayoutItem*>` target).
- **Sprite-index + icon patterns to mirror.** `RuleSoldier` already has `_rankSprite*` and
  `_skillIconSprite` ([src/Mod/RuleSoldier.h](../src/Mod/RuleSoldier.h)); a role icon is the same
  pattern (a frame index into a `SurfaceSet`).
- **Icon `SurfaceSet` shipping mechanism.** `extraSprites:` entries load a PNG strip into a named
  `SurfaceSet` with `subX`/`subY` subframes (e.g. `bin/standard/xcom1/extraSprites.rul`). The role-icon
  palette ships the same way; `Mod::getSurfaceSet(name)` retrieves it.
- **Per-soldier appearance system (for the deferred armor-color item).** `SoldierAvatar` +
  `Soldier::getArmorLayers()` ([src/Savegame/Soldier.cpp:958](../src/Savegame/Soldier.cpp#L958)) resolve
  layered armor sprites by gender/look/variant — but there is **no color/recolor field today**. The
  Per-Role Armor Colors item will add a tint hook there; out of scope for this pass beyond storing the
  `color` on `Role`.

**No existing docs mention DX roles** beyond the roadmap checklist and the legacy historical spec
(`Legacy-DX-Features.md` §5), whose class names (`RuleRole`, `Role`, `RoleMenuState`,
`RoleChangeState`) describe the *old fork's fixed-list* implementation and do **not** exist in the
current tree.

## Data model

### Ruleset (mod-authored: seeds + icon set)

A top-level `roles:` node plus a companion `roleIcons` key naming the icon `SurfaceSet`. Parsed by
`Mod` into a lightweight **seed** definition (`RuleRole`) — the *only* role rule data, used to populate
a fresh save's role list and to register the icon set:

Each role icon is shipped as an **individual named `singleImage` ExtraSprite** (not a numbered
SurfaceSet), and a `roleIcons:` registry bundles a badge + map-marker pair under a stable name:

```yaml
# Icons are named surfaces (extraSprites): RoleIcon<Name> (23x23 badge) and
# RoleIcon<Name>Map (small battlescape marker). See xcom1/roles.rul.
roleIcons:                 # registry: bundles a badge+marker pair under a stable name
  - name: INFANTRY         # stable string id (this is what a Role stores)
    sprite: RoleIconInfantry       # badge surface name (Mod::getSurface)
    mapSprite: RoleIconInfantryMap  # map-marker surface name
  - name: SNIPER
    sprite: RoleIconSniper
    mapSprite: RoleIconSniperMap
  - name: ENGINEER         # a mod just declares its own named surfaces + a registry entry
    sprite: MYMOD_RoleIconEngineer
    mapSprite: MYMOD_RoleIconEngineerMap

roles:                     # seed roles copied into a NEW save's editable role list
  - name: STR_ROLE_INFANTRY
    icon: INFANTRY         # references a roleIcons entry BY NAME
    color: 132            # the role's UNIT ARMOR colour (per-role armor recolour; deferred)
  - name: STR_ROLE_SNIPER
    icon: SNIPER
  # ...one seed per starting role
```

- `RuleRoleIcon` ([src/Mod/RuleRoleIcon.h/.cpp], new) — the named icon definition: `name` (stable
  string id) + `sprite` + `mapSprite` (**surface names**, resolved via `Mod::getSurface`). Parsed from a
  top-level `roleIcons:` node into `std::map<std::string, RuleRoleIcon*>`; `Mod::getRoleIcon(name)` /
  `getRoleIconsList()`. **Names are the identity** — a role stores the registry name, and the icon stores
  surface names; nothing stores a frame index. No `color` — the icon is shared across roles, so it can't
  own a per-role colour (see below).
- `RuleRole` ([src/Mod/RuleRole.h/.cpp], seed) — fields: `name` (STR id), `icon` (a **`roleIcons` name**,
  not an int), `color` (the role's **unit armor colour** — palette index; see the note below).
  Deliberately minimal; legacy's `smallIconSprite`/`isBlank` dropped (the badge/map surface pair covers
  the two sizes).
- `Mod` ([src/Mod/Mod.cpp](../src/Mod/Mod.cpp)) — parse `roleIcons:` and `roles:`.

**Why named surfaces (not a numbered SurfaceSet):** the icon identity is a **string** end-to-end
(role → `roleIcons` name → surface names), so it survives mod-list / load-order changes with zero
special handling — `getSurface(name)` re-resolves every load. Using a SurfaceSet would make the
identity a frame *index*, which drags in per-mod sprite offsets, a reserved `maxSharedFrames` boundary
(settable only in engine code — `ExtraSprites::load` has no such key), and silent index-collision
between mods. Named surfaces avoid all of it: a mod adds an icon by declaring more named surfaces (a
namespaced name like `MYMOD_...` avoids collisions) plus a `roleIcons:` entry. The only cost is a more
verbose ruleset (~2 small blocks per icon), which we already pay via the registry.

### Savegame (player-authored: the live, editable roles)

- `Role` ([src/Savegame/Role.h/.cpp], new) — the mutable per-save role:
  - `int _id` (stable unique id, minted from a savegame counter like soldiers' `_id`)
  - `std::string _name` (player-editable display/STR text)
  - `std::string _icon` (a **`roleIcons` name**, resolved to `RuleRoleIcon`/sprites at runtime — the
    save-stable identity; empty = the `None`/no-badge default)
  - `int _color` (the role's **unit armor colour** — a palette index used to recolour the assigned
    soldier's armor; the Phase 7 "Per-Role Armor Colors" item. Stored now, rendered by that later item.
    *May* optionally also tint the role icon, but the icon is not its purpose.)
  - `std::vector<EquipmentLayoutItem*> _loadout` (+ `const Armor* _loadoutArmor`, matching the global
    template's name/armor pairing)
  - `load`/`save`.
- `SavedGame` ([src/Savegame/SavedGame.h/.cpp](../src/Savegame/SavedGame.h)) — owns
  `std::vector<Role*> _roles` + `int _roleId` counter. On new-game, seed `_roles` from
  `Mod::getRoleSeeds()`. Accessors: `getRoles()`, `getRole(int id)`, `createRole()`, `removeRole(id)`
  (also clears the id from any soldier holding it). `save`/`load` a `roles:` sequence.
- `Soldier` ([src/Savegame/Soldier.h/.cpp](../src/Savegame/Soldier.h)) — add `int _roleId` (0/`-1` =
  none), `getRoleId()/setRoleId()`, resolved to `Role*` via `SavedGame::getRole()` where needed.
  Save/load the id. Referencing by **stable id** (not name) keeps assignment intact across renames; a
  deleted role clears the soldier's id.

**Save-format impact:** additive only — a new top-level `roles:` block and a per-soldier `roleId`.
Old saves load with no roles and every soldier role-less (backward compatible); the seed list only
populates on **new** games.

## UI (this pass = minimal, but functional)

1. **Role management screen** (working title `RoleMenuState`, `src/Basescape/`) — lists the save's
   roles with icon + name; **New / Rename / Icon / Color / Delete**; and **Edit Loadout** (opens the
   inventory in template-edit mode, reading/writing `Role::_loadout` via the existing
   `_createInventoryTemplate`/`_applyInventoryTemplate` helpers, generalized to accept an arbitrary
   `vector<EquipmentLayoutItem*>&` target). Entry point: a button on a suitable Basescape screen
   (candidate: Soldiers list, or the equipment/templates area — TBD in implementation).
2. **Minimal soldier assignment** — on `SoldierInfoState`, a **Role** control (button opening a small
   role picker, or cycle) that sets the soldier's `_roleId`. On assign, offer to **apply the role's
   loadout** to the soldier's personal equipment layout (reusing `_applyInventoryTemplate`). The role
   name/icon shows on the Soldier Info screen.

Deferred to **Role UI & Markers** / **Per-Role Armor Colors**: the bobbing battlescape selected-unit
role marker, the craft/inventory soldier-list role columns + Show Stats/Show Roles toggle, tooltip &
combat-log role text, and the armor tint render.

## Icon art — RESOLVED (Jul 2026): authored xcom1 icons shipped

The procedural-placeholder plan is dropped: a full set of **authored** role icons now ships under
[bin/standard/xcom1/Resources/Roles/](../bin/standard/xcom1/Resources/Roles/) — one PNG per role
(Infantry, Sniper, Scout, Rocketeer, Assault, Heavy, Grenadier, Medic, Psionics, Demolitions,
Specialist, plus Marksman / MachineGunner / AntiArmor extras), a `RoleIconNone` blank, `*Map`
small map-marker variants, and the `RoleIconTemplate.pdn` Paint.NET source. **These are the starting icons.**

Known limitation: the art currently lives under **xcom1 only** — it is not yet a shared/cross-ruleset
resource, so TFTD (xcom2) and other rulesets have no role icons yet. A later **sprite-management** pass
will address shipping/sharing the icon set across rulesets. For now the icons are committed and wired
under xcom1.

### Icon surfaces (done — xcom1 `roles.rul`)

Each role icon is an **individual named `singleImage` ExtraSprite** in the dedicated
[bin/standard/xcom1/roles.rul](../bin/standard/xcom1/roles.rul) (which will also hold the `roleIcons:`
registry and the `roles:` seed list), two per role:

- **`RoleIcon<Name>`** — the full 23×23 badge (inventory + battlescape stats bar). All 15 roles
  (including `RoleIconNone`) have one.
- **`RoleIcon<Name>Map`** — the small variable-size glyph that replaces the bobbing down-arrow
  selected-unit indicator in the live battlescape. 14 of them — **no `RoleIconNoneMap`** (the `None`
  role keeps the default arrow).

Shipped names: `None`, `Infantry`, `Marksman`, `Sniper`, `Scout`, `Assault`, `MachineGunner`, `Heavy`,
`AntiArmor`, `Rocketeer`, `Grenadier`, `Demolitions`, `Medic`, `Psionics`, `Specialist`. Declared with
`typeSingle:` + `fileSingle:` (no `width`/`height` — `Surface::loadImage` reallocates each surface to
its PNG's native size). They are **lazy-loaded on first access**, so nothing loads until a consumer (a
later rendering step) draws them via `Mod::getSurface(name)`.

A `roleIcons:` registry entry pairs a badge + map surface under a stable name (`INFANTRY → sprite
RoleIconInfantry / mapSprite RoleIconInfantryMap`); roles reference the registry name. There is **no
frame index, no per-mod offset, and no `maxSharedFrames` boundary** — the named-surface model makes the
whole identity a string (see "Why named surfaces" above).

## Scope for this pass

**In:** `RuleRole` (seed) + `roles:`/`roleIcons` parsing in `Mod`; `Role` savegame class + `_roles`
list/counter/accessors on `SavedGame` with new-game seeding and save/load; `Soldier::_roleId` +
save/load; the role-management screen (create/rename/icon/color/delete/edit-loadout); the minimal
Soldier-Info assign entry point with apply-loadout; the role-icon `SurfaceSet` (procedural placeholder
+ ruleset hook); default seed roles in `dx-test.rul`; DX language strings in `Language/DX/`.

**Out (later roadmap items):** battlescape role marker, soldier-list role columns + Stats/Roles toggle,
tooltip/combat-log role text (Role UI & Markers); armor tint rendering (Per-Role Armor Colors); any
role-driven stat weighting/bonuses (not planned — roles are loadout + identity, not stat profiles;
`RuleSoldierBonus` already covers stat layering if ever wanted).

## Open questions

- **Icon art — RESOLVED (Jul 2026):** authored xcom1 role icons shipped (see above); no procedural
  placeholder. Cross-ruleset sharing deferred to a later sprite-management pass.
- **Management-screen entry point** — which Basescape screen hosts the "Roles" button (Soldiers list is
  the leading candidate).
- **Apply-on-assign — RESOLVED (Jul 2026): prompt/optional.** Assigning a role tags the soldier but
  does not auto-overwrite their loadout; applying the role's template is a separate explicit action (or
  a yes/no prompt), so picking a role never stomps a hand-tuned loadout.
- **`color` = unit armor colour (RESOLVED, Jul 2026).** The role's `color` is primarily the assigned
  soldier's **armor recolour** (Phase 7 "Per-Role Armor Colors"), stored now and rendered by that later
  item. It *may* also tint the role icon if that turns out to work cleanly, but the armor is the point —
  it does not live on the shared `RuleRoleIcon`.
