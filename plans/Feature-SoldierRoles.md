# Feature: Soldier Roles (player-authored classification + per-role loadout)

**Status:** In progress (Jul 2026). **Step 1 (data backbone) done & building clean** — `RuleRole`
seeds + `roles:` node in `Mod`, savegame `Role` (identity + own loadout), `SavedGame` role list with
new-game seeding / accessors / save-load, and `Soldier._roleId` (additive save format). Authored
xcom1 role icons shipped as assets. **Next:** template-target generalization, the role-management
screen, and the minimal Soldier-Info assign entry point. This is the Phase 7 **"Role Definitions &
Templates"** roadmap item, redesigned from the legacy fixed-list model into a **player-authored**
system.

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

```yaml
roleIcons: ROLE_ICONS      # SurfaceSet (from extraSprites) the picker cycles through
roles:                     # seed roles copied into a NEW save's editable role list
  - name: STR_ROLE_RIFLEMAN
    icon: 0                # frame index into ROLE_ICONS
    color: 132            # palette index for the role's marker/tint (deferred use)
  - name: STR_ROLE_SNIPER
    icon: 1
    color: 45
  # ...Scout, Rocketeer, Assault, Heavy, Grenadier, Medic, Psionics, Demolitions, Specialist
```

- `RuleRole` ([src/Mod/RuleRole.h/.cpp], new) — fields: `name` (STR id), `icon` (frame index), `color`
  (palette index). Deliberately minimal; legacy's `smallIconSprite`/`isBlank` are dropped unless the UI
  proves it needs them (a single icon set + on-the-fly small draw should suffice).
- `Mod` ([src/Mod/Mod.cpp](../src/Mod/Mod.cpp)) — parse `roles:` into `std::vector<RuleRole*> _roles`
  and store the `_roleIcons` surfaceset name; expose `getRoleSeeds()` and
  `getRoleIconSurfaceSet()`. Registered in the `loadFile` dispatch alongside the other top-level nodes.

### Savegame (player-authored: the live, editable roles)

- `Role` ([src/Savegame/Role.h/.cpp], new) — the mutable per-save role:
  - `int _id` (stable unique id, minted from a savegame counter like soldiers' `_id`)
  - `std::string _name` (player-editable display/STR text)
  - `int _icon` (frame index into the role-icon set)
  - `int _color` (palette index; marker/tint — stored now, rendered by later items)
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
(Rifleman, Sniper, Scout, Rocketeer, Assault, Heavy, Grenadier, Medic, Psionics, Demolitions,
Specialist, plus Marksman / MachineGunner / AntiArmor extras), a `RoleIconNone` blank, `*Simple`
low-detail variants, and the `RoleIconTemplate.pdn` Paint.NET source. **These are the starting icons.**

Known limitation: the art currently lives under **xcom1 only** — it is not yet a shared/cross-ruleset
resource, so TFTD (xcom2) and other rulesets have no role icons yet. A later **sprite-management** pass
will address shipping/sharing the icon set across rulesets (and the `SurfaceSet` wiring: an
`extraSprites:` entry naming the set + a `roleIcons` reference the picker cycles through). For now the
icons are committed as raw assets ahead of that wiring.

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
- **`color` semantics now** — store only (rendered later) vs. also show a colored name/icon on the
  management + Soldier-Info screens this pass (cheap; likely yes for the icon tint).
