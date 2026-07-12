# Ruleset: `roles:`, `roleIcons:`, `soldierArmorBaseColors:` **[DX]**

Back to the [ruleset index](Ruleset.md).

**Engine classes:** [`RuleRole`](../src/Mod/RuleRole.h) · [`RuleRoleIcon`](../src/Mod/RuleRoleIcon.h) ·
**List key:** `name` (both) · **Loaders:** [`RuleRole::load`](../src/Mod/RuleRole.cpp) ·
[`RuleRoleIcon::load`](../src/Mod/RuleRoleIcon.cpp) · `soldierArmorBaseColors:` is parsed in
[`Mod::loadFile`](../src/Mod/Mod.cpp)

All three roots are **DX-only** and belong to one feature: **soldier roles** — a classification
(Infantry, Sniper, Medic, …) a soldier carries, shown as an icon badge, driving a saved loadout, a
battlescape map marker, a roster abbreviation, and an armor tint.

The important thing about DX roles: **roles are player-owned savegame data, not rules.** The player
creates, renames, re-icons, recolors and deletes them in-game. A `roles:` entry is only a **seed** —
a starter role copied into the role list of a *new* game so a fresh campaign has usable defaults.
Editing `roles:` therefore has no effect on an existing save. What *is* pure mod data is the icon
registry (`roleIcons:`) and the color palette (`soldierArmorBaseColors:`), which the in-game pickers
enumerate.

See [DX-Features.md](../DX-Features.md#soldier-roles-in-progress) and the design doc
[plans/Feature-SoldierRoles.md](../plans/Feature-SoldierRoles.md); the shipped content lives in
[`bin/standard/xcom1/roles.rul`](../bin/standard/xcom1/roles.rul) (and the TFTD twin in `xcom2/`).

```yaml
# 1. the icon art: ordinary named single-image surfaces
extraSprites:
  - typeSingle: RoleIconSniper
    fileSingle: Resources/DX/Roles/RoleIconSniper.png
  - typeSingle: RoleIconSniperMap
    fileSingle: Resources/DX/Roles/RoleIconSniperMap.png

# 2. the registry: bundles a badge + map marker under a stable name
roleIcons:
  - name: SNIPER
    sprite: RoleIconSniper
    mapSprite: RoleIconSniperMap

# 3. the seed role copied into a new game's editable role list
roles:
  - name: STR_ROLE_SNIPER
    shortName: SNP
    icon: SNIPER            # a roleIcons entry, by name
    color: 15               # a soldierArmorBaseColors value (Black)

# 4. the colour set the in-game picker offers
soldierArmorBaseColors:
  - name: STR_COLOR_BLACK
    color: 15
```

---

## `roles:` — seed roles (`RuleRole`)

Entries merge across mods/files by `name` (`delete: true` supported; **no `refNode:`**).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique id; also the role's display-name string key (`STR_ROLE_*`). |
| `shortName` | string | — | 3-letter abbreviation shown in the roster/craft lists as a rank prefix (`SNP-Rookie`); empty = the UI falls back to the first three letters of the name. |
| `icon` | string | — | The `roleIcons:` entry to use (see below), **by registry name**; empty = no icon. |
| `color` | int | −1 | Armor accent colour for soldiers with this role: a battlescape-palette index from `soldierArmorBaseColors:`. −1 = "Armor Default" (no recolour). |

Seeds carry **identity only** — no loadout, no stats, no equipment. The loadout is filled in by the
player (Save Kit / Apply Kit in the battlescape inventory) and stored in the savegame.

## `roleIcons:` — the icon registry (`RuleRoleIcon`)

Entries merge across mods/files by `name` (`delete: true` supported; **no `refNode:`**).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Stable registry id; this string is what a role (and the savegame) stores. |
| `sprite` | string surface | — | Surface name of the 23×23 badge shown on the Soldier Info and Inventory screens. |
| `mapSprite` | string surface | — | Surface name of the small battlescape marker that replaces the bobbing selected-unit arrow; empty = keep the default arrow. |

Both fields are **surface names**, not SurfaceSet frame indices — declare each icon as a
`typeSingle:`/`fileSingle:` entry in [`extraSprites:`](Ruleset-ExtraSprites.md) and reference it by
that name. This is deliberate: the icon identity is a string end-to-end
(role → registry name → surface name), so a saved role's icon survives mod-list and load-order
changes with no sprite-offset or shared-frame handling. A mod adds icons simply by declaring more
named surfaces plus registry entries (namespace the names to avoid collisions).

Each surface keeps its PNG's native size, so no width/height is needed.

## `soldierArmorBaseColors:` — the role colour palette (singleton)

A **singleton list** (not a keyed rule list): the ordered set of colours the in-game role colour
picker offers. Entries are merged **by name** — a later mod may retune an existing colour's value or
append new ones. Order here is the picker's display order. The engine prepends the "Armor Default"
row (`STR_COLOR_NONE`, value −1) itself.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Display-name string key of the colour (`STR_COLOR_*`); also the merge key. |
| `color` | int | −1 | Battlescape-palette index — the base of the 16-colour block the accent is remapped into. |

Values are **palette-dependent**, which is why they are ruleset data and not code: UFO and TFTD ship
different sets.

### How the colour is applied

A soldier whose role has a `color` gets its armor's **accent block** remapped into that colour's
block — shade-preserving, so shading survives — on both the battlescape sprite and the inventory
paperdoll. The accent block is the OXCE **utile** recolor channel: an armor opts in by declaring
`spriteUtileGroup:` (see [armors](Ruleset-Armors.md#appearance--audio)); armors that don't declare
one render stock.

The colour value's **low nibble selects the recolor mode** (legacy DX semantics):

| Low nibble | Mode |
|---|---|
| `1` | Lighten — shades are compressed/shifted toward the light end of the block. |
| `15` | Darken — shades are compressed into the dark half of the block (so "Black" is a dark ramp, not flat). |
| anything else | Plain offset — `blockBase + shade`, i.e. OXCE's stock utile recolor. |

So `color: 15` (Black) and `color: 1` (White) are the two shaped ramps of the greyscale block, while
e.g. `color: 32` (Red) is a plain block swap.

## See also

- [`armors:`](Ruleset-Armors.md#appearance--audio) — `spriteUtileGroup:`, the accent block roles recolor
- [`extraSprites:`](Ruleset-ExtraSprites.md) — how the `RoleIcon*` named surfaces are declared
- [`soldiers:`](Ruleset-Soldiers.md) — the soldier a role is assigned to
- [DX-Features.md](../DX-Features.md#soldier-roles-in-progress) · [plans/Feature-SoldierRoles.md](../plans/Feature-SoldierRoles.md)
