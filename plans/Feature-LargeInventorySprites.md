# Feature: Inventory sprites larger than 2×3

**Status:** Implemented (Jul 2026). Builds clean (Release/Win32); `dx-test`'s advanced HWP engine is
a worked 3×3 example (48×48 PNG) and the mod loads with zero errors. Two caps lifted, no format
change and no new ruleset key.

Raised while building the [modular vehicle](Feature-ModularVehicles.md) demonstrator, where the
advanced engine wanted a 3×3 footprint and had to be cut to 2×3. Legacy DX allowed bigger inventory
sprites; DX should too.

## Audit (Jul 2026) — the cap is not where it looks

The obvious assumption is that `BIGOBS.PCK` frames are hard-locked to 32×48 (2×3 cells), because
`ExtraSprites::loadSurfaceSet` only resizes a surface set that is **empty**:

```cpp
if (set->getTotalFrames() == 0 && (set->getWidth() != surfaceSetX || ...))
```

That is true of the **set's default frame size**, but it is *not* what limits an individual frame.
Three things turn out to already work:

| Path | Behavior | Capped? |
|---|---|---|
| Loading an `extraSprites` file into a frame | `Surface::loadImage` does `*this = Surface(width, height, 0, 0)` ([Surface.cpp:359](../src/Engine/Surface.cpp#L359)) — the frame is **replaced at the image's real dimensions** | **No** |
| Frame storage | `SurfaceSet::_frames` is `std::vector<Surface>`, and each `Surface` carries its own width/height — the set's `_width/_height` is only the default for `addFrame()` | **No** |
| Drawing an item in a slot | `executeBlit(frame, _items, x, y, 0)` blits the whole frame at the slot origin, into a full-screen 320×200 `_items` layer | **No** |

So a 48×48 BIGOBS sprite already loads and already draws correctly in a slot. The real caps are two,
and both are in the *interaction* layer rather than the graphics layer:

**C1 — the dragged-item surface is hardcoded to one hand-box.**
[`Inventory.cpp:115`](../src/Battlescape/Inventory.cpp#L115)

```cpp
_selection = new Surface(RuleInventory::HAND_W * RuleInventory::SLOT_W,
                         RuleInventory::HAND_H * RuleInventory::SLOT_H, x, y);   // 32x48
```

`drawHandSprite()` blits the item's frame into `_selection`, so anything larger than 32×48 is
**clipped while being dragged** — it looks correct sitting in a slot and wrong in the player's hand.

**C2 — hand sprite offsets go negative for oversized items.**
[`RuleItem.cpp:2093`](../src/Mod/RuleItem.cpp#L2093)

```cpp
int RuleItem::getHandSpriteOffX() const
{ return (RuleInventory::HAND_W - getInventoryWidth()) * RuleInventory::SLOT_W / 2; }
```

The offset centers an item inside the 2×3 hand box. For `invWidth: 3` it evaluates to
`(2 - 3) * 16 / 2 = -8`, i.e. the sprite is drawn **8px outside the box**. This is reachable because
`RuleInventory::fitItemInSlot` returns `true` unconditionally for `INV_HAND` — *"hands hold any item
regardless of size"* ([RuleInventory.cpp:273](../src/Mod/RuleInventory.cpp#L273)) — so an oversized
item genuinely can be placed in a hand.

## Plan

1. **C1 — size `_selection` to the largest item the mod defines.** Compute the maximum
   `invWidth`/`invHeight` across all `items:` once in `Mod::loadAll()`, expose it, and size the drag
   surface from it. Floored at the current 32×48 so nothing ever shrinks and stock behavior is
   byte-identical when no mod defines a bigger item. Deriving it beats a hardcoded cap: no arbitrary
   ceiling, and no cost for mods that don't use the feature.
2. **C2 — clamp the hand offsets at 0.** An item too big for the hand box then draws from the box
   origin (overflowing right/down, which is visible and sane) instead of shifted up-left over the
   neighbouring UI.

Explicitly *not* changing: `fitItemInSlot`'s "hands accept anything" rule (long-standing upstream
behavior, and useful — it is what lets a 2×3 rifle sit in a hand at all), and nothing about the
sprite format, `extraSprites` schema, or `SurfaceSet`.

## Notes

- No ruleset surface changes: a modder just declares a bigger `invWidth`/`invHeight` and supplies a
  matching image. Worth documenting in `docs/Ruleset-Items.md` that the two must agree, since a
  sprite larger than its declared footprint will overlap neighbouring slots when drawn.
- Sprites are still anchored top-left to the item's slot, so art must fill its footprint from (0,0).


## What shipped

| Piece | Where |
|---|---|
| `Mod::getMaxItemInvWidth/Height()` — largest item footprint, computed once after items merge | [Mod.h](../src/Mod/Mod.h) / [Mod.cpp](../src/Mod/Mod.cpp) `loadAll` |
| C1 — drag surface sized from that maximum | [Inventory.cpp](../src/Battlescape/Inventory.cpp) ctor |
| C2 — hand sprite offsets clamped at 0 | [RuleItem.cpp](../src/Mod/RuleItem.cpp) `getHandSpriteOffX/Y` |
| Worked 3×3 example | `bin/standard/dx-test/dx-test-vehicles.rul` + `Resources/HwpEngineAdv.png` |

### Notes from implementation

- **The maximum is derived, not configured.** Scanning `items:` once in `loadAll()` avoids inventing
  an arbitrary ceiling *and* costs nothing for mods that never use the feature — the value is floored
  at `HAND_W`/`HAND_H`, so a stock install produces exactly the old 32×48 drag surface.
- `Mod.h` only forward-declares `RuleInventory`, so the member's default is the literal `2`/`3` with
  the floor re-applied against the real constants in `loadAll()`.
- **Both hand offsets needed the clamp**, not just X. A 3-cell-tall item would have been shifted up
  out of the box the same way a 3-cell-wide one was shifted left.
- The `width`/`height` on an `extraSprites` entry are the *set* defaults and are ignored when a whole
  file replaces a single frame — worth knowing, since leaving them at `32`/`48` next to a 48×48 file
  reads like a contradiction but is correct.
