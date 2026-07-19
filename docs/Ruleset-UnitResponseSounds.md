# Ruleset: `unitResponseSounds:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** — (raw sound tables on `Mod`) · **List key:** `name` · **Loader:**
[`Mod::loadFile`](../src/Mod/Mod.cpp) (parsed inline; consumed by
[`BattleUnit::prepareUnitResponseSounds`](../src/Savegame/BattleUnit.cpp))

*Unit response sounds* are the voice barks a battlescape unit makes when you select it, order it to
move, pick a weapon, or click it repeatedly. `unitResponseSounds:` attaches a voice bank to an
**individual soldier by name** — the "named voice" feature: give `Jane Doe` her own selection grunts
and she keeps them for the whole campaign.

```yaml
enableUnitResponseSounds: true      # top-level global switch, NOT part of this node

unitResponseSounds:
  - name: Jane Doe                  # the soldier's NAME, not a soldier type
    selectUnitSound: [55, 56, 57]   # indices into BATTLE.CAT; one is picked at random
    startMovingSound: [58, 59]
    selectWeaponSound: [60]
    annoyedSound: [61, 62]
```

Entries merge by `name`.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | The **soldier's individual name** (as displayed in the base roster), not a `type`. |
| `selectUnitSound` | int or list of ints | — | Sound(s) played when the unit is selected. |
| `startMovingSound` | int or list of ints | — | Sound(s) played when the unit is ordered to move. |
| `selectWeaponSound` | int or list of ints | — | Sound(s) played when a weapon/action is chosen for the unit. |
| `annoyedSound` | int or list of ints | — | Sound(s) played when the unit is selected repeatedly ("stop poking me"). |

All four are **sound-offset fields**: the values are indices into `BATTLE.CAT`, and indices at or
above the vanilla sound count are automatically shifted into the current mod's private range — the
same rule that governs [`extraSounds:`](Ruleset-ExtraSounds.md), so numbers you added there work here
unchanged. A field given several sounds picks one **at random** per bark.

## How the banks resolve

At unit setup, `BattleUnit` picks a voice bank with this priority:

1. **`unitResponseSounds:` by soldier name** — if the unit's name matches an entry here, that entry
   wins outright (if it defines *any* of the four fields, none of the lower-priority banks apply).
2. **Armor** — the `selectUnitMale/Female`, `startMovingMale/Female`, `selectWeaponMale/Female`,
   `annoyedMale/Female` fields on the equipped [`armors:`](Ruleset-Armors.md), where non-empty.
3. **Soldier type / unit type** — the same fields on [`soldiers:`](Ruleset-Soldiers.md) (per gender) or
   [`units:`](Ruleset-Units.md).

## Related global knobs

These are separate **top-level ruleset keys** (siblings of `unitResponseSounds:`, not fields inside it):

| Key | Type | Default | Meaning |
|---|---|---|---|
| `enableUnitResponseSounds` | bool | false | Master switch — with it off, **no** unit response sounds play at all and this whole root is inert. |
| `unitResponseSoundsFrequency` | list of 4 ints | `[100, 100, 100, 20]` | Percentage chance per event that a bark actually plays, in the order select / start-moving / select-weapon / annoyed. |

## See also

- [`extraSounds:`](Ruleset-ExtraSounds.md) — getting the audio files into `BATTLE.CAT` in the first place
- [`armors:`](Ruleset-Armors.md) — the per-armor voice banks (priority 2)
- [`soldiers:`](Ruleset-Soldiers.md) / [`units:`](Ruleset-Units.md) — the per-type voice banks (priority 3)
