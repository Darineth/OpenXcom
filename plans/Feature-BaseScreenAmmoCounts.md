# Feature: Base-Screen Ammo Counts

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 6 (Ammo & Reloading), last
item. Re-derived from the legacy DX fork (Legacy-DX-Features.md §4: "ammo items display the count they
contain per purchase/manufacture on base screens"). Implemented on Buy / Sell / Transfer / Stores via
the shared `STR_DX_AMMO_ROUND_COUNT` suffix; Manufacture deferred.

## Motivation

On the base item screens (Buy / Sell / Transfer / Stores) an ammo stack shows only its name and how
many *clips* you have — not how many **rounds** each clip holds. When clips differ in capacity (a
20-round rifle clip vs a 50-round cannon drum) you can't tell at a glance how much ammunition a
purchase or a stockpile actually represents. DX appends the round count to ammo rows.

## Audit — the delta

- The Buy/Sell/Transfer screens already **detect ammo** and indent its name by two spaces
  (`ammo = BT_AMMO || (BT_NONE && clipSize>0)` → `name.insert(0, "  ")`):
  [PurchaseState.cpp:732](../src/Basescape/PurchaseState.cpp#L732),
  [SellState.cpp:626](../src/Basescape/SellState.cpp#L626),
  [TransferItemsState.cpp:501](../src/Basescape/TransferItemsState.cpp#L501).
  `StoresState` builds its row name at [:327](../src/Basescape/StoresState.cpp#L327).
- **None of them show the round count.** `getClipSize()` is only used for the ammo *classification*,
  never displayed (the pedia's Stats-for-Nerds shows `clipSize`, but not the base list screens).
  So this is a pure display addition at those four sites.

## Design

- Append a compact round-count suffix to the displayed name of an ammo row when the clip bundles more
  than one round: shown when the item is classified as ammo **and** `getClipSize() > 1`.
  - Single-shot ammo (`clipSize == 1`, e.g. rockets) → no suffix (nothing useful to show), matching
    the inventory ammo-badge rule that skips `capacity <= 1`.
  - `battleClipSize` ammo (`clipSize == 0`, stocked one round at a time) → no suffix, which correctly
    conveys its per-round economy.
- Format via a new shared DX string `STR_DX_AMMO_ROUND_COUNT: "{0} (x{1})"` (`{0}` = the already
  indented name, `{1}` = the round count), so translators control the layout and the four screens
  stay consistent. Example row: `  Rifle Clip (x20)`.
- Applied at the base item screens: **Buy, Sell, Transfer, Stores, and Craft Equipment** (the
  pre-battle craft loadout list, which shares the same ammo-indent pattern). (Manufacture output is a
  separate structure — deferred unless wanted; the round count matters most where you stock/move/equip
  stacks.)

## Touch points

- `src/Basescape/PurchaseState.cpp`, `SellState.cpp`, `TransferItemsState.cpp` — after the existing
  `name.insert(0, "  ")`, reformat the name through `STR_DX_AMMO_ROUND_COUNT` when `getClipSize() > 1`.
- `src/Basescape/StoresState.cpp` — at row-name construction ([:327](../src/Basescape/StoresState.cpp#L327)),
  add the same suffix for ammo rules.
- `src/Basescape/CraftEquipmentState.cpp` — after the BT_AMMO name indent
  ([:507](../src/Basescape/CraftEquipmentState.cpp#L507)), add the same suffix.
- `bin/common/Language/DX/en-US.yml` — `STR_DX_AMMO_ROUND_COUNT`.
- Docs: `DX-Features.md`, this doc, `DX-Roadmap.md` (tick the last Phase 6 box).

## Testing plan

- With dx-test active, open Buy / Sell / Transfer / Stores and confirm multi-round clips show
  `(xN)` (e.g. the shotgun **buckshot** clip `clipSize: 8` → `(x8)`), single-shot ammo shows nothing,
  and the `battleClipSize` **slug** shows nothing (per-round economy).
- Non-ammo items are unchanged.
- Build clean with `-Wall -Wextra`.

## Deferred

- Manufacture screens (produced-ammo round count) — only if wanted; different code path.
