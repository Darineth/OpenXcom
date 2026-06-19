# Debriefing Soldier Status

Status: implemented in DX.

## Motivation

The post-mission debriefing should show each soldier's outcome directly in the results table,
including whether they were killed in action, missing in action, wounded, or returned safely.
The existing per-soldier stat-gain breakdown already exists; this feature adds the missing outcome
and wound-recovery context on top of that data.

## Approach

- Reuse the battle/debriefing data that already records KIA/MIA and wound-recovery days.
- Extend the debriefing soldier-results table with compact outcome and recovery columns.
- Keep the existing stat-gain columns intact so the debriefing still shows the per-soldier
  improvement breakdown.

## Final behavior

- The debriefing soldier table now shows `KIA`, `MIA`, `WND`, or `OK` for each soldier.
- Wounded soldiers show their recovery time in days.
- The stat-gain columns remain visible on the same view.

## Implementation notes

- Compact 2-character column headers use dedicated DX keys (`STR_STAT_TU`, `STR_STAT_EN`,
  `STR_STAT_HP`, `STR_STAT_BR`, `STR_STAT_RX`, `STR_STAT_FA`, `STR_STAT_TH`, `STR_STAT_ME`,
  `STR_STAT_ST`, `STR_STAT_PS`, `STR_STAT_PK`, `STR_STAT_MP`) defined in
  `bin/common/Language/DX/en-US.yml`, plus `STR_STATUS_SHORT` and `STR_RECOVERY_SHORT`. These are
  kept separate from the shared `STR_*_ABBREVIATION` strings so other screens are unaffected and
  the psi headers are not clobbered by mod extra-strings (which load after the DX language slice).
- The header `Text` widgets and the `TextList` columns share one grid (list x=16..304, width 288):
  name 72 (left-aligned), status 20, recovery 20, then eleven 16px stat columns, all centered.
