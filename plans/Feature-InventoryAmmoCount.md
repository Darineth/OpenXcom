# Feature: Inventory Ammo-Count Badges

**Status:** Implemented (Jun 2026).

## Summary

Show, at a glance, how many rounds a weapon or clip holds, without hovering. A small bordered
number is drawn at the **top-right** of every relevant item in the inventory, colored by how full it
is (good / warning / low). The same badge is shown on the right-side ammo **preview** box, including
when hovering an ammo clip directly.

## OXCE / OXCE-Plus audit

Stock OXCE shows loaded-ammo info only as a transient hover element: the `_selAmmo` sprite preview
plus a `_txtAmmo` "AMMO ROUNDS LEFT n" text in the right column, and only while hovering a loaded
weapon. There is no persistent per-item round count, no state coloring, and hovering a bare clip
shows text but no sprite preview. This feature is a DX addition.

## What shipped

- **Per-item badge** (`Inventory::drawItems`): for each item in a slot/hand and on the ground, a
  bordered round-count is drawn top-right via `Inventory::drawAmmoCount`.
  - `getInventoryAmmoCount` decides the number: an ammo item shows its own remaining rounds; a weapon
    shows its primary loaded ammo's rounds (or self-ammo charge). Unloaded/non-ammo items get none.
  - **Single-shot ammo** (clip size ≤ 1, e.g. a rocket) shows no badge — the count is trivial.
- **State color** (`Inventory::ammoStateColor`): full (`count == capacity`) → `ammoFull`;
  half-or-better → `ammoMid`; below half → `ammoLow`.
- **Configurable colors**: new `inventory` interface elements `ammoFull` / `ammoMid` / `ammoLow`
  in `interfaces.rul` (UFO: 50 / 18 / 34; TFTD: 81 / 161 / 177). Chosen at each palette's
  green / amber / red block **start** so the bordered glyph (which offsets the base color by up to
  +12) stays inside one 16-color block and doesn't fringe into a neighboring hue. The code falls
  back to the two-handed/medikit colors if a mod omits the elements.
- **Preview badge**: the right-side `_selAmmo` preview (`InventoryState::think`) now draws the same
  badge top-right via the public `Inventory::drawAmmoBadge`, and the preview also appears when
  hovering a bare ammo clip (not just a loaded weapon).
- **Removed** the old `_txtAmmo` rounds text (redundant with the badges, and it overlapped the
  right-edge buttons).

## Notes / follow-ups

- Removing `_txtAmmo` also removed the medikit pain/stim/heal quantities it used to show on hover.
  That display is intended to return via the planned "replace the stats panel with weapon shot
  accuracy and medikit quantities on hover" feature.
- The `textAmmo` interface element is now unused (left in `interfaces.rul`, harmless).
