<!-- Canonical copy: /root/bot-master-plan.md (single source of truth; edits go there, then re-sync) -->

# Tortoise Bot Master Plan — "Best of Both"

**Status:** DRAFT — documentation phase, no implementation started yet
**Date:** 2026-06-27
**Supersedes:** the scope limits in `playerbot-port-plan.md` (that plan's engine-porting detail remains valid history)
**Owner intent (verbatim):** *"I think it's time to remove that limit. I want the best possible bots for tortoise! I'm not precious about which bot system we pull our solutions from — I strongly believe in diversity of sources at all times."*

---

## 1. Vision

Build the best possible bot system for Tortoise-WoW (1.18.1 / 7272) by treating every existing bot codebase as a **code mine**, not a framework to adopt wholesale. Per-subsystem, take the best implementation available from any source, adapted onto our chassis.

**The chassis is non-negotiable:** the AC-mod-playerbots decision engine (3-engine shared context, triggers/strategies/actions/values, NamedObjectContext factories) already ported to `src/game/PlayerBots/` and independently verified correct by the 2026-06-26 critical review (`playerbot-review.md`). Everything else is liftable, replaceable, or deletable.

## 2. Source inventory

| Source | Location | Lineage | What we take from it |
|---|---|---|---|
| **Our AC port** | `/root/tortoise-wow` branch `playerbot-engine-port` (HEAD `c5f57bd`, pushed to avirar) | AzerothCore mod-playerbots | **Chassis**: Engine, Queue, contexts, login/persistence/factory, loot pipeline, packet triggers, item stat engine |
| **Shyalya fork** | git remote `shyalya` in `/root/tortoise-wow` (full 903-commit history fetched 2026-06-27, +52MB) + browsable mirror `/root/shyalya-tortoise-wow` — ⚠ **upstream archived read-only 2026-09-30** | CMaNGOS ike3 (the ancestor of AC's system) | Features to mine: BG tactics (Arathi node capture), AHBot + market policy, taxi/travel system (~15 fixes), ManTech scheduling for 6k bots, diagnostics suite, 5,082-line config surface |
| **AC mod-playerbots reference** | `/root/azerothcore-wotlk/modules/mod-playerbots/` | AC (actively maintained) | Per-class strategies for all 9 classes, ItemUsage semantics, group/dungeon behaviors, quest/travel strategies, raid scripts |
| **mod-ollama-bot-buddy** | `/root/azerothcore-wotlk/modules/mod-ollama-bot-buddy/` | AC module (user's own) | Agent↔bot interface logic: command API, game-state snapshots, chat override (see `agent-bot-interface-plan.md`) |
| **acore-data** | `/root/acore-data` (+ client data at `/root/azerothcore-wotlk/env/dist/bin`, wowgaming v20.0) | AC tooling (user's own) | **Concepts only** → new build `tortoise-data`, a tortoise-native CLI driven by mangos `DBCfmt` strings (see `tortoise-data-plan.md` v2 — fork rejected: AC registry machinery was the cost driver and doesn't apply) |

Key lineage fact: AC mod-playerbots **descends from** ike3/CMaNGOS bots, so Shyalya's module shares conceptual DNA (actions/triggers/strategies/values) with our chassis despite different internals (`ReactionEngine`, `Multiplier`, `ActionBasket`, per-class strategy dirs). Concepts translate even when code doesn't.

## 3. Architecture decisions

| # | Decision | Rationale |
|---|---|---|
| A1 | **Stay in-core** (`src/game/PlayerBots/`) — no module infra | Our base (Penqle main @ `31586bd`) predates the module system Shyalya's tree has. Porting the module loader for one consumer is churn; mining feature logic into our tree is cheaper. Revisit only if we adopt many modules. |
| A2 | **Base sync first** — rebase/merge `playerbot-engine-port` onto current Penqle `main` (we are 251 commits behind) | Every Shyalya lift assumes a newer core (their bot commits touch `World.cpp`, `WorldSession.cpp`, `CharacterHandler.cpp`, `MovementHandler.cpp`, `AuctionHouseHandler.cpp`). Syncing gives us those files' current shape and 251 commits of upstream fixes (quest fixes, vmap guard, movespline clamp, Windows build fixes). |
| A3 | **Class AI = AC per-class strategies**, with `SelectOffensiveSpell` kept as universal baseline | Review confirmed the bridge is now wired (commit `435d129`): all 9 classes cast via `cast spell` default action. AC class strategies layer priorities/rotations on top per class — port them class-by-class like the Warrior pattern. |
| A4 | **Item evaluation = our vanilla stat engine** (now wired, commit `e7603c0`) | Ours is vanilla-correct (raw % hit/crit, ON_EQUIP spell stats, spec-aware weights). AC's `ItemUsageValue` semantics (need/greed/sell/quest decisions) port **on top** of our scorer. |
| A5 | **Shyalya lifts are rewrites onto our chassis**, with source-file citations in commit messages | ike3 code won't drop in (different engine internals, different API names). We port the *logic* (e.g., AB banner capture state machine) as AC-style triggers/actions. |
| A6 | **Diversity-of-sources policy**: every subsystem cites its origin; when two sources solve the same problem, prefer (1) correctness on vanilla 1.12-era mechanics, (2) maintainability on our chassis, (3) feature depth | Explicit user policy. Avoids "not invented here" and "not re-invented here" failure modes. |

## 4. Review reconciliation — `playerbot-review.md` vs HEAD `c5f57bd`

The local-AI review (2026-06-26) was written **before** the 2026-06-27 commit batch. Status at current HEAD:

| Finding | Severity | Status | Where |
|---|---|---|---|
| P0-1 `cast spell` dead code → 8/9 classes never cast | P0 | **FIXED** — registered in `ActionContext.cpp:31`, default action in `CombatStrategy.cpp:15`; Melee/Ranged strategies per class in `PlayerbotAIBase::Initialize` | `435d129` |
| P0-2 `|| true` forces melee auto-attack for ranged | P0 | **FIXED** — `shouldMelee = CanReachWithMeleeAutoAttack \|\| melee-class check` | `435d129` |
| P0-3 StatsWeightCalculator 100% dead code | P0 | **FIXED** — wired into `ItemUsageValue.cpp:71` (plus spec-aware weights, on-hit/on-use spell stats) | `e7603c0` |
| P1-1 Warrior `GetSpecDefaultActions` dead | P1 | **FIXED** — consumed by `GenericWarriorStrategy::getDefaultActions` | `435d129` |
| P1-2 free-repair exploit + no spirit-healer path | P1 | **FIXED** — `DurabilityRepairAll` calls removed; `dead → spirit healer` trigger in `DeadStrategy` | `435d129` |
| P2-1 iteration cap computed once | P2 | **FIXED** — AC refresh-per-iteration pattern | `435d129` |
| P2-2 redundant `SetTargetGuid` | P2 | **FIXED** — removed (SetSelectionGuid does both) | `435d129` |
| P1-3 Warriors hardcoded to Arms; no spec detection | P1 | **FIXED 2026-09-16** — `Util/SpecDetect.h` (shared talent-tab detection, StatsWeightCalculator now delegates); Arms/Fury/Tank chosen by dominant tree at login | pending commit |
| P2-3 `StoreLootAction` mutates `is_looted` directly | P2 | **FIXED 2026-09-16** — direct write removed; opcode handler marks looted server-side (`LootHandler.cpp:225-249`) | pending commit |
| P2-5 `GetName()` (`std::string`) passed to `%s` | P2 | **NON-ISSUE** — tortoise `Player::GetName()` returns `char const*` (`Player.h:2758`) | — |
| P2-6 FFA loot "inverted" | P2 | **FIXED 2026-09-16** — review read wrong: AC also skips FFA by default, gated by config; ported AC semantics (`PlayerBot.FreeMethodLoot=1` opts in) | pending commit |
| P3 batch (static map leak, dead `lastRelevance`, RepopAction tautology) | P3 | **PARTIAL 2026-09-16** — static-map leak fixed (prune >10min entries); stale-target race verified safe (runs before actions); `lastRelevance` + `release` action left (low risk, revisit in R2) | pending commit |

## 5. Roadmap (scope limits removed)

Phases are ordered by dependency, not priority — R4/R6 can interleave once R1 lands.

### R0 — Tooling & docs (this phase, docs only)
- [x] `bot-master-plan.md` (this file)
- [x] `tortoise-data` — **IMPLEMENTED** (v0.1: `tw` CLI at /root/tortoise-data, pushed to github.com/avirar/tortoise-data; DBCfmt-driven DBC reader + read-only SQL, tortoise spell_table/WorldSafeLocs specifics handled)
- [x] `agent-bot-interface-plan.md` — bot-buddy logic port
- [x] Websearch/fetch — RESOLVED via `pi-web-access` package (Exa zero-config fallback chain), custom SearXNG extension spec not needed
- [x] AC client data installed (wowgaming v20.0 → `env/dist/bin`) for acore-data reference runs
- [ ] Update `AGENTS.md` hub, supersede port-plan scope, commit docs

### R1 — Base sync & stabilization (IN PROGRESS — see `r1-base-sync-log.md` for issues & solutions)
- [x] Merge of Penqle `main` onto `playerbot-engine-port` (merge `a473d4f5`; backup branch `playerbot-engine-port-pre-r1`)
- [x] Build fix: WorldSession bot plumbing re-added (`GetBot/SetBot/m_bot`, friend for private `LoginPlayer`)
- [x] DB auto-updater fixed (config key rename + duplicate-key removal; 146 world + 1 char migration reconciled idempotently)
- [x] mangosd boots on new base; 100 bots queued at startup
- [x] **100/100 bots online** — bot login via upstream `HeadlessSessionMgr` (socketless sessions can't live in `World::m_sessions`; `UpdateSessions` purges non-connected sessions) — see `r1-base-sync-log.md` §5
- [x] Fix open review items: P1-3 spec wiring, P2-3, P2-6, P2-5 audit, P3 cleanup (2026-09-16; commits `ceeb728c`, `97ab55cc`)
- [x] **CRITICAL (2026-09-16):** merge also dropped `sPlayerBotMgr.OnPlayerInWorld(this)` from `Player::AddToWorld` → bots online but AI never ran (engines 0/0/0, no movement/casts). Re-added (+ include) — commit `24512ec1`; verified active (engines running, movement/targeting/casts). This invalidated all pre-fix "soaks" — soak clock restarted with AI verified (`r1-base-sync-log.md` §6b)
- [x] Also restored bot pet-taming guard in `Spell.cpp` dropped by merge (commit `f273bb10`)
- [~] Bot login soak ≥30 min with AI active on new base — restarted 2026-09-17 08:45 with spell fix deployed; 100/100 online, 0 real crashes, all 9 classes casting. (Soak clock running.)
- **Exit criteria:** soak passes on new base; review table all-green.

### R2 — Class parity (AC strategies, 8 classes)
- **STATUS (2026-09-17): essentially complete.** 7/9 classes have dedicated combat strategies (Warrior Arms/Fury/Tank full, Mage v1, Warlock, Priest, Shaman, Paladin, Druid). Rogue/Hunter intentionally on the generic fallback (no low-level class CC; offensive table covers their rotation) — deferred until higher-level features. Full warrior spec trees now include all triggers. Verified live: all 9 classes learning + casting class spells, `fear`/`heal self`/polymorph/stances firing, 100/100 online, 0 crashes.
- Port AC per-class strategy trees following the established Warrior pattern (`Ai/Class/<Class>/`): rogue, priest, mage, warlock, hunter, shaman, paladin, druid.
- [x] **Prerequisite: spell auto-learn fixed (2026-09-17, commit `3e5932cf`)** — factory-created bots lack base class skills and `AutoLearnSpellsForLevel`'s `req_skill_value != 0` filter skipped every `skill_line_ability` row, so non-warrior bots had 0 class spells. Now trains the skill to the spell's required value then `LearnSpell`. All 9 classes learn + cast their spells in combat. Without this, no class strategy can function.
- [x] **Class enum is non-vanilla** (`ChrClasses.dbc`): 7=Shaman, 8=Mage, 9=Warlock, 11=Druid. `characters.class` and `skill_line_ability.class_mask` are consistent via `GetClassMask()=1<<(c-1)` — use `tw dbc ChrClasses` for ground truth, never assume vanilla numbering.
- [x] Spec detection: talent-tab detection in `Util/SpecDetect.h`, warrior spec strategy selected at login (P1-3, 2026-09-16) — extend to other classes as their strategy trees land
- [x] **Mage v1 (2026-09-17, commit `3e5932cf`)**: all-spec `GenericMageStrategy` — Polymorph CC action + `can polymorph` trigger, cast-spell nukes via `SelectOffensiveSpell` per-class table, inherits `RangedCombatStrategy` flee-when-close. Blink deferred (positioning risk).
- [x] **5 more class strategies (2026-09-17)**: `GenericWarlockStrategy` (Fear CC), `GenericPriestStrategy` / `GenericShamanStrategy` / `GenericDruidStrategy` (combat self-heal), `GenericPaladinStrategy` (Hammer of Justice CC + self-heal). Shared `SelfHealAction` (`Ai/Base/Actions/`) resolves the per-class heal spell and casts on self, gated by the `low health` trigger (≤30%) on the COMBAT engine (no conflict with the NON_COMBAT food system). Verified live: `fear` + `heal self` actions firing, 100/100 online, 0 crashes.
- [x] **Learnable-spell lesson (2026-09-17)**: CC/self-heal spells must be ones the bots actually learn — `AutoLearnSpellsForLevel` only grants spells present in `skill_line_ability` (by `class_mask`). **Hex (11641) is `spellLevel=30` and ABSENT from `skill_line_ability`** → never learned → warlock CC uses **Fear (5782)** instead. Self-heal base IDs had to match `character_spell` ground truth: Priest **2053** (not 2060), Shaman **332** (not 331), Paladin **639** (not 635), Druid **5186 Healing Touch** (not 3734 Regrowth). `GetHighestKnownSpell(base)` walks the whole chain, so any ID in the chain works.
- [ ] Rogue/Hunter: rely on the class offensive table (Eviscerate/Sinister Strike/Garrote; Auto Shot/Arcane Shot/Serpent Sting) — no low-level class CC to add; generic `Melee`/`Ranged` fallback. Defer dedicated trees until higher-level (poison/aspect) features matter.
- [x] **Warrior: 6 missing Fury/Tank triggers (2026-09-17)** — `bloodthirst ready`, `whirlwind ready`, `shield slam ready`, `revenge ready`, `taunt needed`, `demoralizing shout needed` now implemented in `WarriorTriggers.{h,cpp}` and registered in `TriggerContext.cpp`. Use `FindSpellIdByName` (returns 0 when the bot lacks the spell) so they are harmless dead branches until a bot actually learns the ability (Bloodthirst 23880 / Shield Slam 23922 are talent spells absent from `skill_line_ability`; Whirlwind/Revenge/Taunt/Demoralizing Shout are class spells). Taunt gates on `target->GetVictim() != bot` (aggro lost); Demoralizing Shout re-applies only when the target lacks the debuff (`TargetHasAuraFromChain(target, 1160)`).
- Per-class verification: rotation logs show ability usage per class at level-appropriate targets.
- **Sources:** AC `strategy/<class>/`; cross-check spell availability tables against `tw_world` via tortoise-data (R0 tool) and `twow-class-spells-reference.md`.

### R3 — Item & loot pipeline hardening
- **Loot-pipeline investigation (2026-09-17)**: The "open loot FAILED 98%" is largely the **benign re-entry guard** — `OpenLootAction` returns false while a loot window is already open (`GetLootGuid()` set) waiting for `StoreLootAction`; `StoreLootAction` correctly clears the loot guid + loot target on every exit path (processed/not-tapped/empty) and replicates AC's `lootslot_type` permission logic. The code is sound; the pipeline is **content-limited**, not buggy.
- **"No gear" root cause (→ R5) — partially addressed**: bots were level 10-12 in starting zones fighting **appropriate-level beasts** (Mudpaw, Thalassian Stag — drop meat/junk, no equipment) instead of **humanoids** (gnolls/trolls/forsaken — drop gear). `ServerFacade::SelectNearestSafeTarget` picked the **nearest** target by pure distance. **FIX (2026-09-17):** added a tunable **humanoid distance-weight** (`PlayerBot.HumanoidGrindDistanceWeight`, default 0.6) — humanoid targets get a distance discount in `SelectNearestSafeTarget` so gear-dropping humanoids beat nearer beasts (1.0 = pure nearest). Effect is limited by **mob contention** (100+ bots in the same zones tap/attack mobs before a given bot reaches them — a separate R5 scale/distribution item). `equip upgrades` still mostly fails where only beasts drop (see world-population change below, which spreads bots into higher-level humanoid-heavy zones).
- Full AC `ItemUsage` semantics on top of our scorer: need/greed/pass group decisions, sell gray, quest items, bag slots, consumables.
- Group loot roles (ML distribution, roll participation) — extend the `lootslot_type` work.
- Vendor/sell cycles + gold accumulation for bots.
- **Sources:** AC `strategy/values/ItemUsageValue.cpp`, `strategy/actions/UseItemAction.cpp`; Shyalya ahbot as reference for economy sanity.

### R4 — Shyalya feature lifts (each = AC-style rewrite)
- **R4a Travel/taxi:** flight-master convergence, destination distribution, route persistence. Port ike3 `TravelMgr`/`Destination` concepts onto our engine. Fixes bots stranded in low-mob zones (the Desolace finding).
- **R4b BG tactics:** Arathi Basin node-capture state machine (real banner entries, cast throttle on click), then WSG/AV. `modules/mod-playerbots/src/playerbot/strategy/actions/BattleGroundTactics*.cpp` is the reference.
- **R4c AHBot:** auction posting bot with market policy + profession item supply (CMaNGOS-style, commands included). Requires AuctionHouseHandler core hooks — coordinate with R1 base sync.
- **R4d Scale & scheduling:** ManTech scheduler patterns, continent-spread bot scheduling, bounded maintenance. Target: 1,000 bots on current hardware as first milestone (6k aspirational).

### R5 — Depth behaviors
- **World population: level 1-60, all classes, spread across the world (2026-09-17, DONE):** `PlayerbotFactory` now creates a diverse population instead of 100 level-10 warriors-at-heart. Changes:
  - `PickRandomLevel()` returns a **weighted 1-60** level (exponent 1.6 low-level bias; `ceil` keeps 60 as a real top bucket) — was hardcoded 10.
  - New `PickSpawnPosition(level, race)` places each bot in a **level/faction-appropriate zone**: Alliance→Eastern Kingdoms (map 0), Horde→Kalimdor (map 1), high levels (50+)→shared contested Silithus. Curated open-field coordinates per level band (Elwynn/Durotar → Redridge/Tirisfal → WPL/Ashenvale → Arathi/Tanaris → Winterspring/Azshara → Silithus). Set via the `SetLocationMapId`/`Relocate`/`SetMap` pattern (mirrors `Player::Create`); server corrects slightly-off points to ground.
  - **Scale 100 → 300** bots (`FactoryBotCount`/`MinBots`/`MaxBots`). Headroom: 24 cores (100 bots ≈ 1.8 cores), 17GB RAM free. Regenerated via `DeleteAllBots=1` (wipe+shutdown) then `DeleteAllBots=0` + `FactoryBotCount=300` (recreate).
  - Spells for high-level bots are handled by the existing `AutoLearnSpellsForLevel()` (called at `OnPlayerLogin`) which learns all class spells ≤ bot level from `skill_line_ability`.
  - **Verified:** 300/300 online, 0 crashes, all alive, actively grinding. Distribution: 1-10=98, 11-20=44, 21-30=49, 31-40=36, 41-50=38, 51-60=35. All 9 classes present (Warrior 57, Mage 60, Hunter 45, Rogue 46, Priest 28, Warlock 27, Druid 14, Paladin 12, Shaman 11). Both continents (EK 173 / KM 127).
- Group play: party formation, role composition, assist/follow semantics (extend existing DpsAssist work).
- Questing: accept/complete cycles (AC quest strategies + bot-buddy command set as reference).
- Dungeons: pathing via mmaps, boss tactics (AC raid scripts as reference).
- Death handling hardening: corpse pathing edge cases, GY resurrection economy.

### R6 — Agent-driven operations (see `agent-bot-interface-plan.md`)
- In-process command API on `PlayerBotAI` (bot-buddy command set).
- Console + HTTP surfaces so pi agent sessions can drive/observe bots.
- Eval harness: scripted scenarios (login→grind→loot→equip→death→revive) asserted via state snapshots.
- This is the force multiplier: agents (GLM/pi sessions) test autonomously while humans review diffs.

## 6. Risks & mitigations

| Risk | Mitigation |
|---|---|
| Shyalya repo archived / history lost | **Mitigated 2026-06-27: full history fetched as remote `shyalya` into `/root/tortoise-wow` (903 commits)** + mirror at `/root/shyalya-tortoise-wow` |
| 251-commit rebase conflicts in core touchpoints | Rebase early (R1), small commits, soak gate before any new features |
| ike3 logic subtly wrong for vanilla 1.18.1 | Cross-check lifts against AC reference + tortoise-data DBC queries; soak tests per lift |
| Scope explosion now that limits are off | Phased exits above; each phase lands on green soak; master plan is the single source of truth |
| LLM/agent-driven testing destabilizes server | R6 interface is read-mostly by default; write commands gated behind config + GM rank |

## 7. Doc map

| Doc | Role |
|---|---|
| `bot-master-plan.md` | This file — strategy, decisions, roadmap |
| `playerbot-port-plan.md` | Historical: engine port detail, phase 0-3 internals (scope-limited) |
| `playerbot-review.md` | Local-AI critical review (2026-06-26) + reconciliation table above |
| `r1-base-sync-log.md` | R1 issues & solutions (merge conflicts, WorldSession plumbing, updater keys, migration reconciler) |
| `tortoise-data-plan.md` | tortoise-native data tool (`tw` CLI) — new build, not an acore-data fork; **live as global pi extension** (`~/.pi/agent/extensions/tortoise-data.ts`, tool `tw`, works from any directory) |
| `agent-bot-interface-plan.md` | bot-buddy logic port — agent↔bot control surface |
| `pi-websearch-extension.md` | **RESOLVED** — web search/fetch via `pi-web-access` package (Exa zero-config); history retained |
| `AGENTS.md` | Hub: server ops, working notes, doc pointers |
