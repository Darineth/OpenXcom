# Feature: Ufopaedia fallback articles for unconfigured rules

**Status:** ✅ Implemented. Builds clean; verified against stock xcom1 (54 articles generated, both
ruleset flags confirmed parsing and taking effect). **Not yet play-tested** — opening a generated
article of each type via middle-click still needs an in-game pass.

**Roadmap:** "Ufopaedia fallback stats page for unconfigured items" (DX-Roadmap.md, Phase list).

## Motivation

Middle-clicking almost anything in OpenXcom opens its Ufopaedia article — an item in the
inventory, a soldier's armor, a craft weapon, a base facility. That article carries the *stats
block* (accuracy, damage, TU costs, weight, armor values, capacities), which is genuinely
useful reference material quite apart from the prose.

But the article only exists if the modder authored a `ufopaedia:` entry for that rule. When
they didn't, middle-click **silently does nothing**. There is no feedback, no stats, no
indication that the interaction was even attempted. For mods that add a lot of content without
writing pedia prose for all of it — which is most of them — large parts of the arsenal are
simply not inspectable.

The prose is the expensive part to author; the stats block is derived entirely from the rule
and needs no authoring at all. DX should generate the latter when the former is absent.

## OXCE / OXCE-Plus audit

Checked against the current engine and `Extended.txt`. **No auto-generation feature exists.**
What OXCE does provide, and why none of it solves this:

- **`ufopediaType:` redirection** (note the one-`a` spelling) — `RuleItem::getUfopediaType()`
  ([RuleItem.cpp:947](../src/Mod/RuleItem.cpp#L947)), and the same accessor on `Armor`
  ([Armor.cpp:440](../src/Mod/Armor.cpp#L440)), `RuleCraftWeapon`
  ([RuleCraftWeapon.cpp:148](../src/Mod/RuleCraftWeapon.cpp#L148)) and `RuleBaseFacility`
  ([RuleBaseFacility.cpp:279](../src/Mod/RuleBaseFacility.cpp#L279)). This lets a rule point at
  a *differently named* article; it does not create one. Falls back to `_type`, so an item with
  no article resolves to an id that isn't in the pedia.
- **`section: STR_NOT_AVAILABLE`** (`UFOPAEDIA_NOT_AVAILABLE`) — an article that exists but is
  kept out of the index. Historically used for ammo. Such articles are filtered out of
  `articleList` at [Ufopaedia.cpp:371](../src/Ufopaedia/Ufopaedia.cpp#L371), so **middle-click
  on them does nothing today either** — a second, smaller instance of the same problem.
- **`StatsForNerdsState`** — the closest existing thing: it renders raw rule data for a topic
  with *no* `ArticleDefinition` at all, driven purely by `_typeId` + `_topicId`
  ([StatsForNerdsState.cpp:191](../src/Ufopaedia/StatsForNerdsState.cpp#L191)). But it's a
  debug/reference dump of every field, not the curated stats block, and it's only reachable
  from an already-open article's INFO button — which is exactly what we don't have.
- The player-toggled `PEDIA_STATUS_HIDDEN` and `hiddenCommendation` flags are unrelated
  mechanisms that happen to share the word "hidden".

So this is a true delta, not a re-implementation.

## Design

### The chokepoint

Every one of the ~45 middle-click call sites funnels into
`Ufopaedia::openArticle(Game*, const std::string&)`
([Ufopaedia.cpp:184](../src/Ufopaedia/Ufopaedia.cpp#L184)), whose entire failure mode is one
guard:

```cpp
state->current_index = getArticleIndex(state->articleList, article_id);
if (state->current_index != ArticleCommonState::invalid)
    game->pushState(createArticleState(std::move(state)));
```

Rather than adding a fallback branch *there* — which would need the caller's type context that
every call site currently throws away — the cleaner approach is to **make the lookup succeed**:
synthesize the missing `ArticleDefinition` objects at mod-load time so they are already in
`Mod::_ufopaediaArticles` by the time anything looks them up. Every call site then lights up at
once, with zero changes to any of them, and `getArticleIndex`'s existing `_UC`/`_requires`
fallback passes keep working.

### Where to generate

`Mod::loadAll()` calls `sortLists()` at [Mod.cpp:2559](../src/Mod/Mod.cpp#L2559), immediately
after all mods are loaded and cross-linked. **Generate immediately before that call.**
`sortLists()` then registers the synthesized articles' sections into `_ufopaediaSections` /
`_ufopaediaCatIndex` and sorts `_ufopaediaIndex` normally. Generating *after* `sortLists()`
would leave the articles out of the index entirely.

Generation mirrors the YAML path at [Mod.cpp:3234-3281](../src/Mod/Mod.cpp#L3234): heap-allocate
the right `ArticleDefinition` subclass, set `id`, set `_pages[0].title` / `.text`, insert into
`_ufopaediaArticles[id]`, `push_back` onto `_ufopaediaIndex`, bump `_ufopaediaListOrder`. The
`Mod` destructor already deletes everything in `_ufopaediaArticles`
([Mod.cpp:732](../src/Mod/Mod.cpp#L732)), so ownership needs no special handling.

Only generate for a rule when `_ufopaediaArticles` has no entry under that rule's
`getUfopediaType()` (falling back to its type name) — never overwrite authored content, and
never shadow a `weapon:`-redirected article.

`ArticleDefinition::_pages` and friends are `protected`, so this needs either a small setter or
friend access on `ArticleDefinition`. A `static ArticleDefinition::makeGenerated(...)`-style
factory per subclass is probably the tidiest.

### Marking generated articles

Add `bool _generated = false;` to `ArticleDefinition` (default false, **not serialized** — it's
mod-load-derived, not save state). This is what the index flag filters on, and it's also the
hook for any future "auto-generated" visual treatment.

### The two flags

The browsable index and the middle-click path are genuinely separate code paths — the index is
fed by `Ufopaedia::list()` (via `UfopaediaSelectState::loadSelectionList`,
[UfopaediaSelectState.cpp:215](../src/Ufopaedia/UfopaediaSelectState.cpp#L215)), while
middle-click navigation is fed by `createCommonArticleState()`
([Ufopaedia.cpp:365](../src/Ufopaedia/Ufopaedia.cpp#L365)). So the two behaviors can be flagged
independently with no awkwardness:

| Ruleset key | Default | Effect |
|---|---|---|
| `generateMissingPediaArticles` | *TBD — see open questions* | Master switch. Synthesizes the articles at all; makes middle-click open them. |
| `listGeneratedPediaArticles` | *TBD* | Whether generated articles also appear in the browsable pedia index. Requires the above. |

Both follow the existing global-scalar pattern: declare near
[Mod.h:288-295](../src/Mod/Mod.h#L288), init in the ctor list near
[Mod.cpp:465](../src/Mod/Mod.cpp#L465), parse with `reader.tryRead(...)` in the run at
[Mod.cpp:3460-3496](../src/Mod/Mod.cpp#L3460), inline getter near
[Mod.h:1024-1042](../src/Mod/Mod.h#L1024).

When `listGeneratedPediaArticles` is off, filter `_generated` articles out inside
`Ufopaedia::list()` — one predicate, and it keeps them fully functional for middle-click.

### Research gating

Generated articles get their `_requires` populated from the underlying rule, so the **existing**
`Ufopaedia::isArticleAvailable` gate ([Ufopaedia.cpp:59](../src/Ufopaedia/Ufopaedia.cpp#L59) →
`SavedGame::isResearched`) applies unchanged. No new gating code. An unresearched alien weapon
therefore stays uninspectable, exactly as an authored article would.

Requirement accessors differ per type and need normalizing to `std::vector<std::string>`
(`ArticleDefinition::_requires`' type):

| Rule | Accessor | Returns |
|---|---|---|
| `RuleItem` | `getRequirements()` ([RuleItem.cpp:988](../src/Mod/RuleItem.cpp#L988)) | `vector<const RuleResearch*>` — needs `->getName()` |
| `Armor` | `getRequiredResearch()` ([Armor.cpp:582](../src/Mod/Armor.cpp#L582)) | **single** `const RuleResearch*` |
| `RuleCraft` | `getRequirements()` ([RuleCraft.cpp:256](../src/Mod/RuleCraft.cpp#L256)) | `vector<string>` |
| `RuleBaseFacility` | `getRequirements()` ([RuleBaseFacility.cpp:303](../src/Mod/RuleBaseFacility.cpp#L303)) | `vector<string>` |
| `RuleSoldier` | `getRequirements()` ([RuleSoldier.cpp:282](../src/Mod/RuleSoldier.cpp#L282)) | `vector<string>` |
| `RuleCraftWeapon` | **none** | gate via `getLauncherItem()`/`getClipItem()` → that item's requirements |
| `Unit`, `RuleUfo` | **none** | ungated (an empty `_requires` returns `true` from `isResearched`) |

### Per-type safety

This is the part that needs care. The `ArticleState` subclasses were written assuming a
hand-authored definition, and several dereference `Mod::getSurface(image_id)` **unguarded**.
`Mod::getRule` returns `0` for an empty name rather than throwing
([Mod.cpp:866](../src/Mod/Mod.cpp#L866)), so an empty `image_id` on those types is a **null
dereference — a hard crash**, not an exception.

| Type | Minimum safe payload | Hazard if defaulted |
|---|---|---|
| Item | `id` + title | none — background is hardcoded `BACK08.SCR` |
| BaseFacility | `id` + title | none — hardcoded `BACK09.SCR` |
| Ufo | `id` + title | none — hardcoded `BACK11.SCR` |
| Vehicle | `id` + title | `image_id` **already** falls back to `BACK10.SCR` ([ArticleStateVehicle.cpp:71](../src/Ufopaedia/ArticleStateVehicle.cpp#L71)); but throws unless the item has a `vehicleUnit` — filter on `getVehicleUnit()` |
| Armor | `id` + title | throws if the armor's `spriteInv` resolves to nothing ([ArticleStateArmor.cpp:124](../src/Ufopaedia/ArticleStateArmor.cpp#L124)) — pre-check |
| Craft | + `image_id`, `rect_text`, `rect_stats` | **null-deref crash** on empty `image_id` ([ArticleStateCraft.cpp:59](../src/Ufopaedia/ArticleStateCraft.cpp#L59)); zero rects → invisible body |
| CraftWeapon | + `image_id` | **null-deref crash** ([ArticleStateCraftWeapon.cpp:81](../src/Ufopaedia/ArticleStateCraftWeapon.cpp#L81)) |
| Unit | + `image_id`, rects | **null-deref crash** ([ArticleStateUnit.cpp:73](../src/Ufopaedia/ArticleStateUnit.cpp#L73)); `rect_armor.height == 0` is legal and hides the armor block |

Two things follow:

1. **Harden the unguarded derefs** in `ArticleStateCraft`, `ArticleStateCraftWeapon` and
   `ArticleStateUnit` to skip the blit when the surface is null. This is a latent crash for
   *hand-authored* articles too (a typo'd `image_id` with a missing sprite crashes rather than
   erroring), so it belongs in **`DX-OXCE-Fixes.md`** as an upstream-behavior fix independent of
   this feature.
2. **Supply a generic background** rather than relying on the guard for a blank screen.
   `BACK10.SCR` is the natural neutral choice — it's what the text and vehicle articles already
   use, and it's reachable by name from `Mod::getSurface`.

Default rects for Craft should mirror the stock articles (text `{5,40,310,60}`, stats
`{5,96,140,60}` or thereabouts); Unit likewise. Exact values to be tuned against a real mod
during implementation.

TFTD article types are a separate family driven by `interfaces.rul` palettes and background
images rather than article fields; `image_id` there is optional and guarded. They need the
matching `article*TFTD` interface to exist. **Deferred** — see open questions.

### Title and text

`ArticleDefinition::load()` sets `_pages[0].title = id` *before* reading `title`
([ArticleDefinition.cpp:59](../src/Mod/ArticleDefinition.cpp#L59)). The C++ generation path must
do the same, or the title bar renders blank (`tr("")` → `""`, no crash).

Using the rule's own name key as the title gives the correct localized display name for free,
since item/armor/craft names are already `STR_*` keys with translations.

For the body text: leaving it empty renders a blank body, which is fine and honest — the stats
block is the point. Setting it to a nonexistent `STR_*` key would render the raw token, which
looks broken. So: **empty text**, and let the stats block carry the page. Per DX convention any
player-facing chrome we *do* add (e.g. a "no description available" line, if we decide we want
one) needs an `STR_DX_*` key in `bin/common/Language/DX/en-US.yml`.

## Resolved decisions

1. **Flag defaults** — `generateMissingPediaArticles` defaults **on**, `listGeneratedPediaArticles`
   defaults **off**. Middle-click quick info works everywhere out of the box; no existing mod's
   authored pedia index changes unless it opts in.
2. **Rule filtering** — filter out obvious non-items rather than a blanket sweep: skip rules with
   no inventory/big sprite, plus the mandatory safety filters (vehicles without `getVehicleUnit()`,
   armor whose `spriteInv` doesn't resolve). Keeps placeholder and internal rules out of the pedia.
3. **TFTD article types** — **included in v1**. Each generator must verify the matching
   `article*TFTD` interface exists (with a `backgroundImage`) before generating that style, and
   skip the rule rather than crash when it doesn't. `text_width` must be set to 157 explicitly,
   since only the YAML path defaults it.

## Open questions

1. **Should generated articles be visually distinguishable** when listed (a color, a suffix)?
   Only matters if a mod turns `listGeneratedPediaArticles` on. Defer until someone does.
2. **`STR_NOT_AVAILABLE` articles.** Should the same treatment un-break middle-click for articles
   that exist but are deliberately kept out of the index? Related but distinct — arguably the
   modder said "not available" on purpose, though the current silent no-op is still bad feedback.
   Handle separately.

## As implemented — deviations from the plan above

### The gating hole, and the derived gate that closed it

The plan claimed research gating "comes free" because a generated article inherits the rule's
requirements. **That was wrong**, and testing showed it plainly: 33 alien unit articles were being
generated for stock xcom1 with *empty* requirements, because `Unit` has no requirements field at all
- and `SavedGame::isResearched({})` returns `true`. Every alien's stats, Ethereal Commander and
Sectopod included, readable on day one. The same held for `RuleUfo`, and for items that simply never
set `requires:` (alien corpses, the built-in terror weapons).

The first response was to cut units and UFOs from scope entirely. That was an overcorrection, and
the question that exposed it was simple: *if `RuleUfo` has no requirements, how are the authored UFO
articles hidden until researched?*

The answer is that **the gate lives on the article, not the rule**, and it follows a consistent
convention - an article requires a research topic named after its own subject:

```yaml
- id: STR_SMALL_SCOUT      # the UFO
  requires:
    - STR_SMALL_SCOUT      # the research topic of the same name
```

That convention is derivable. `derivedGate()` looks for a `RuleResearch` whose name equals the
rule's type; if one exists it is taken as the topic meant to reveal that rule, and the generated
article inherits exactly the gate a modder would have written by hand. It holds broadly in stock:

| Rule | Same-named research |
|---|---|
| `STR_SECTOID_SOLDIER` (unit) | yes - live-alien interrogation |
| `STR_CYBERDISC_TERRORIST` (unit) | yes |
| `STR_SMALL_SCOUT` (UFO) | yes |
| `STR_SECTOID_CORPSE` (item) | yes - autopsy |
| `CELATID_WEAPON` (internal) | no - correctly none |

So units and UFOs are back in scope, properly gated. The resulting policy has two tiers:

- **Player-facing types** (items, armor, craft, craft weapons, facilities, soldiers): rule
  requirements first, derived gate as fallback, ungated allowed - for these, no requirement
  legitimately means "available from the start".
- **Enemy content** (alien units, UFOs): a derivable gate is **mandatory**. No research topic, no
  article. This is what keeps `MALE_CIVILIAN`, `FEMALE_CIVILIAN` and `STR_ZOMBIE` out - none has a
  research topic, so none can be gated, so none is generated.

Two item filters remain, on value rather than secrecy grounds:

- **Fixed weapons the player can never recover** (`isFixed() && !isRecoverable()`) - a unit's innate
  attack rather than equipment, never held or owned, and carrying no research topic either. Player
  HWP weapons are fixed too but *are* recoverable, so the test keeps them.
- **Corpses** (`getBattleType() == BT_CORPSE`) - nothing to inspect but weight and sell price, and
  vanilla already covers them with autopsy articles.

Stock xcom1 + dx-test now generates **37** articles: 30 gated alien unit pages, plus 7 ungated
player-side items, armors and the soldier type.

**Note on UFOs specifically:** stock authors all eight UFO articles, so none is generated there
either way. UFO generation only ever matters for mods that add UFOs without authoring articles.


### Navigation had to be fixed separately

Filtering `Ufopaedia::list()` was not sufficient. The prev/next buttons walk `articleList` directly
(built by `createCommonArticleState`), so with listing off the index correctly hid generated
articles while navigation walked straight into them. Fixed by marking unlisted generated articles in
`articleStatusList`, reusing the engine's existing skip mechanism — which keeps them in `articleList`
so `openArticle` can still find them for middle-click. That in turn made the upstream recursion in
`nextArticle`/`prevArticle` easy to blow up, hardened into bounded loops (see `DX-OXCE-Fixes.md`).

### Other deviations

Everything else landed as designed, plus four things the plan didn't anticipate:

1. **`ufopediaType` redirection had to be handled per type.** The plan assumed generating under the
   id the middle-click handler asks for was always safe. It isn't: `ArticleStateItem` /
   `ArticleStateTFTDItem` resolve their rule from `weapon` first and only fall back to the article
   id, but the armor, craft-weapon, facility, craft, UFO and vehicle screens resolve from
   `defs->id` alone with `error = true` — so generating under a redirected id would throw on open.
   Resolution: items set `weapon` to the item's own type (safe under any id); the id-only types
   skip generation when `ufopediaType != type`, on the reasoning that a modder who redirected the
   id meant to author that article themselves.
2. **`sortLists()` also needed filtering.** Registering a generated article's section there would
   create visibly empty pedia categories when listing is off, so section registration is skipped
   for generated articles unless `listGeneratedPediaArticles` is set.
3. **TFTD interface readiness is checked before generating**, not just assumed — `ArticleStateTFTD`
   blits the interface background unguarded, so a missing `article*TFTD` interface would crash.
   Checked via `getBackgroundImage(this, nullptr)`; a null `SavedGame` is correct at mod-load time
   and yields the base background, which is all the check needs.
4. **Added a load-time log line** reporting the generated count, chosen style, and index setting.
   This is what made the flags verifiable without driving the UI.

The upstream null-deref hardening was done as planned and written up in `DX-OXCE-Fixes.md`, keeping
`error = true` so typo'd sprite names still throw rather than silently blanking.

## Verification plan

No unit test framework exists, so verification is in-game against a mod:

- Middle-click an item with no authored article in the inventory → stats page opens.
- Middle-click armor, a craft weapon, a base facility, a craft → each opens without crashing.
- Confirm an unresearched alien weapon still does *not* open.
- Confirm authored articles are unchanged and are never shadowed by a generated one.
- Toggle each flag and confirm index inclusion / middle-click behave independently.
- Debug build (the `assert()`s are `NDEBUG`-gated) with a mod that has a deliberately broken
  `image_id`, to exercise the hardened blits.
