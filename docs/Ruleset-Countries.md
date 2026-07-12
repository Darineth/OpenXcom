# Ruleset: `countries:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleCountry`](../src/Mod/RuleCountry.h) · **List key:** `type` · **Loader:**
[`RuleCountry::load`](../src/Mod/RuleCountry.cpp)

A country is a **funding nation**: a set of rectangular areas on the globe, a monthly funding
range, and a label. Countries score X-COM/alien activity inside their areas, adjust funding each
month, and can sign a pact with the aliens (leaving X-COM) when infiltrated.

> **This doc also covers the `extraGlobeLabels:` root.** It loads the *same* `RuleCountry` class
> through a separate list — purely cosmetic text labels on the globe with **no funding, scoring or
> pact logic**. Only the label-related fields matter there, and `zoomLevel` *only* works there
> (see the field table).

```yaml
countries:
  - type: STR_USA                    # unique id; also the label string key
    fundingBase: 600                 # starting funding: 600..1200 (×$1000)
    fundingCap: 3000                 # never exceeds $3,000,000 ($K)
    labelLon: 260.51                 # label position, degrees
    labelLat: -42.02
    areas:
      - [ 189.84, 288.98, -71.01, -50.62 ]   # [lonMin, lonMax, latMin, latMax]
      - [ 189.84, 270.60, -50.62, -48.51 ]

extraGlobeLabels:
  - type: STR_ATLANTIC_OCEAN
    labelLon: 330.0
    labelLat: -30.0
    labelColor: 130
    zoomLevel: 2                     # only drawn at this globe zoom or closer
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the string key of the label / country name. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `fundingBase` | int $K | 0 | Starting monthly funding is rolled as `RNG(fundingBase, fundingBase*2) × $1000`. |
| `fundingCap` | int $K | 0 | Hard ceiling (in thousands of $) the country's monthly funding can never grow past. |
| `labelLon` / `labelLat` | double degrees | 0 / 0 | Globe position of the country's text label (converted to radians on load). |
| `labelColor` | int palette index | 0 | Label color override; 0 = use the globe-wide `countryColor` from [`globe:`](Ruleset-Globe.md). |
| `zoomLevel` | int | 0 | Minimum globe zoom level at which the label is drawn — **works for `extraGlobeLabels:` entries only**, vanilla countries always draw at zoom ≥ 2. |
| `areas` | list of `[lonMin, lonMax, latMin, latMax]` | — | Rectangles (degrees) that make up the country's territory; a point is "inside" if it falls in any rectangle. Rectangles may cross the prime meridian by giving `lonMin > lonMax`. |
| `signedPactEvent` | string event | — | [Geoscape event](Ruleset-Events.md) spawned when the country signs a pact with the aliens (leaves X-COM). |
| `rejoinedXcomEvent` | string event | — | [Geoscape event](Ruleset-Events.md) spawned when the country cancels its pact and rejoins X-COM. |
| `provideBaseFunc` | list of tags | — | Base-function tags an X-COM base located inside this country gains (checked by facilities/research/etc. via `requiresBaseFunc`). |
| `forbiddenBaseFunc` | list of tags | — | Base-function tags forbidden for bases inside this country. |

Notes straight from the loader:

- `areas` entries are appended (there is no `deleteOldAreas` here, unlike
  [`regions:`](Ruleset-Regions.md)); a swapped `latMin`/`latMax` pair is auto-corrected.
- Countries also accept Y-Script hooks (`scripts:` sub-node, `countryScripts`) and custom `tags:`
  — see [Ruleset-Scripting.md](Ruleset-Scripting.md).

## See also

- [`regions:`](Ruleset-Regions.md) — the other globe partition (mission zones, cities, base cost)
- [`globe:`](Ruleset-Globe.md) — polygons/textures and the default label colors (`countryColor`)
- [`events:`](Ruleset-Events.md) — the pact/rejoin events referenced above
- [`facilities:`](Ruleset-Facilities.md) — consumers of the base-function tags
