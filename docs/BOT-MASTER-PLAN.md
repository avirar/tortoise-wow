<!-- Canonical copy: /root/bot-master-plan.md — edit there, then re-copy -->

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
| P1-3 Warriors hardcoded to Arms; no spec detection | P1 | **OPEN (partial)** — spec detection EXISTS in `StatsWeightCalculator` (`GetPlayerSpecTab`, DBC talent parsing) but `PlayerbotAIBase::Initialize` still adds `ArmsWarriorStrategy` unconditionally. Wiring is ~10 lines. | — |
| P2-3 `StoreLootAction` mutates `is_looted` directly | P2 | **OPEN** — `LootAction.cpp:305` still writes server state | — |
| P2-5 `GetName()` (`std::string`) passed to `%s` | P2 | **OPEN** — audit needed tree-wide | — |
| P2-6 FFA loot inverted | P2 | **OPEN** — `LootAction.cpp:39-41` returns false for FREE_FOR_ALL (should allow) | — |
| P3 batch (static map leak, dead `lastRelevance`, RepopAction tautology) | P3 | **OPEN** — cleanup pass | — |

## 5. Roadmap (scope limits removed)

Phases are ordered by dependency, not priority — R4/R6 can interleave once R1 lands.

### R0 — Tooling & docs (this phase, docs only)
- [x] `bot-master-plan.md` (this file)
- [x] `tortoise-data` — **IMPLEMENTED** (v0.1: `tw` CLI at /root/tortoise-data, pushed to github.com/avirar/tortoise-data; DBCfmt-driven DBC reader + read-only SQL, tortoise spell_table/WorldSafeLocs specifics handled)
- [x] `agent-bot-interface-plan.md` — bot-buddy logic port
- [x] Websearch/fetch — RESOLVED via `pi-web-access` package (Exa zero-config fallback chain), custom SearXNG extension spec not needed
- [x] AC client data installed (wowgaming v20.0 → `env/dist/bin`) for acore-data reference runs
- [ ] Update `AGENTS.md` hub, supersede port-plan scope, commit docs

### R1 — Base sync & stabilization
- Rebase `playerbot-engine-port` onto current Penqle `main` (`d886113+`). Resolve conflicts in `World.cpp`, `WorldSession.cpp`, `CharacterHandler.cpp`, `CMakeLists.txt`, `GridNotifiers.h`, `Player.cpp`, `Unit.cpp/h`, `Chat.cpp/h` (our 58 commits' core touchpoints).
- Re-verify build + 100-bot soak (login, grind, no crashes, 30 min).
- Fix open review items: P1-3 spec wiring, P2-3, P2-6, P2-5 audit, P3 cleanup.
- **Exit criteria:** soak passes on new base; review table all-green.

### R2 — Class parity (AC strategies, 8 classes)
- Port AC per-class strategy trees following the established Warrior pattern (`Ai/Class/<Class>/`): rogue, priest, mage, warlock, hunter, shaman, paladin, druid.
- Spec detection: wire `GetPlayerSpecTab` into combat-engine strategy selection (P1-3).
- Per-class verification: rotation logs show ability usage per class at level-appropriate targets.
- **Sources:** AC `strategy/<class>/`; cross-check spell availability tables against `tw_world` via tortoise-data (R0 tool) and `twow-class-spells-reference.md`.

### R3 — Item & loot pipeline hardening
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
| `tortoise-data-plan.md` | tortoise-native data tool (`tw` CLI) — new build, not an acore-data fork |
| `agent-bot-interface-plan.md` | bot-buddy logic port — agent↔bot control surface |
| `pi-websearch-extension.md` | **RESOLVED** — web search/fetch via `pi-web-access` package (Exa zero-config); history retained |
| `AGENTS.md` | Hub: server ops, working notes, doc pointers |
