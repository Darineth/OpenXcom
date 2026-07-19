# Ruleset: `globe:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleGlobe`](../src/Mod/RuleGlobe.h) · **Singleton** (one map, not a keyed
list) · **Loader:** [`RuleGlobe::load`](../src/Mod/RuleGlobe.cpp)

The globe rule defines the **physical geoscape**: the landmass polygons, decorative polylines
(country borders), the per-polygon **textures** that map a globe location to battlescape terrain
and deployment, and the globe's label/line colors. It is a singleton — later mods override it
field-by-field (and each of `data`/`polygons`/`polylines` *replaces* the previous set wholesale).

```yaml
globe:
  data: GEODATA/WORLD.DAT          # vanilla polygon set (alternative to polygons:)
  textures:
    - id: 1
      terrain:
        - name: FOREST
        - name: CULTA
          area: [ 0, 360, -10, 10 ]   # this terrain only near the equator
      deployments:
        STR_TERROR_MISSION: 100
    - delete: 7                    # remove an inherited texture
  countryColor: 239
  oceanShading: true
```

## Geometry

| Key | Type | Default | Meaning |
|---|---|---|---|
| `data` | string path | — | Load the landmass polygons from an X-COM `WORLD.DAT` file (replaces all previously loaded polygons). |
| `polygons` | list of lists | — | Explicit polygon set (replaces all previous): each entry is `[textureId, lon1, lat1, lon2, lat2, lon3, lat3, …]` in degrees — usually 3 or 4 vertices (the YAML form accepts any count; the 3–4 limit only applies to `WORLD.DAT` files). |
| `polylines` | list of lists | — | Decorative line strips (replaces all previous): each entry is `[lon1, lat1, lon2, lat2, …]` in degrees. |

`data` and `polygons` are alternatives; whichever appears in the entry replaces the polygon list.

## Colors & shading

Defaults are the engine's UFO values set in [`Mod`'s constructor](../src/Mod/Mod.cpp) (TFTD mods
override them in their rulesets). All are palette indices into the geoscape palette — see the
`palette-color-reference`/`interfaces:` docs for picking values.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `countryColor` | int | 239 | Default color of country labels and `extraGlobeLabels` (per-country `labelColor` overrides it — see [countries](Ruleset-Countries.md)). |
| `cityColor` | int | 138 | Color of city labels. |
| `baseColor` | int | 133 | Color of X-COM base labels. |
| `lineColor` | int | 162 | Color of the drawn `polylines` (borders). |
| `oceanPalette` | int block | block 12 | Palette **block index** (×16) where the 32-color ocean ramp starts; the ocean is drawn/shaded from this ramp. |
| `oceanShading` | bool | true | Whether the day/night shader re-shades pixels inside the ocean ramp (turn off for TFTD-style pre-shaded oceans). |

## Textures

`textures:` is a list that **merges by `id`** (unlike the geometry keys): an entry with an
existing `id` updates that texture, `- delete: <id>` removes one. Each polygon's texture id
selects one of these, and that texture decides **what a battle at that spot looks like**.

Sub-fields (loader: [`Texture::load`](../src/Mod/Texture.cpp)):

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | int | — | Texture id referenced by polygons and by region mission-zone areas. |
| `baseGridSprite` | int sprite | 0 | Index into `BASEBITS.PCK` used to draw the base-grid background when a base sits on this texture. |
| `isOcean` | bool | false | Marks a cosmetic-only ocean texture (globe rendering/landing logic treats it as water). |
| `fakeUnderwater` | bool | false | Marks the texture as "fake underwater" (TFTD-style hybrid mods: underwater bases/battles on this texture). |
| `startingCondition` | string | — | [Starting condition](Ruleset-StartingConditions.md) applied to battles generated on this texture. |
| `deployments` | map name → weight | — | Weighted [alienDeployments](Ruleset-AlienDeployments.md) for missions generated on this texture (e.g. per-texture terror deployments). |
| `terrain` | list of criteria | — | Weighted [terrains](Ruleset-Terrains.md) for battles here; each entry: `name`, `weight` (default 1), optional `area: [lonMin, lonMax, latMin, latMax]` restricting the criterion geographically. |
| `baseTerrain` | list of criteria | — | Same as `terrain`, but used for **base defense** maps of bases built on this texture. |

The engine picks a terrain by summing the weights of all criteria whose `area` contains the
target and rolling among them; likewise for `deployments`.

## See also

- [`regions:`](Ruleset-Regions.md) — mission zones whose point areas carry texture ids
- [`countries:`](Ruleset-Countries.md) — funding nations and extra globe labels drawn on this globe
- [`terrains:`](Ruleset-Terrains.md) / [`alienDeployments:`](Ruleset-AlienDeployments.md) — what the texture criteria select
