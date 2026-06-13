# PlayerBot Engine Port: mod-playerbots → tortoise-wow

## Goal

Port the mod-playerbots strategy/action/trigger decision engine to tortoise-wow, then port a subset of useful behaviors (combat rotations, healing, buffing, basic movement). Skip travel, dungeons, raids, questing, BGs, guild.

## Source

- **mod-playerbots**: `~/azerothcore-wotlk/modules/mod-playerbots/` (~228K lines, 1,312 files)
- **tortoise-wow**: `~/tortoise-wow/` — target engine

## Estimated Effort

| Phase | Lines to port/rewrite | Status |
|-------|----------------------|--------|
| Phase 0 — Infrastructure | ~500 | [x] Done (7ec62ea) |
| Phase 1 — Engine Framework | ~2,400 | [~] In progress |
| Phase 2 — Combat (1 class: Mage) | ~2,000 | [ ] Not started |
| Phase 3 — Combat (all classes) | ~8,000 | [ ] Not started |
| Phase 4 — Healing/Buffing/Group | ~3,000 | [ ] Not started |
| Phase 5 — Movement/Death/Non-Combat | ~2,000 | [ ] Not started |
| **Total** | **~18,000** | |

---

## Phase 0: Infrastructure — Tortoise-WoW Adapter Layer

**Purpose:** Create the thin abstraction layer that lets the engine framework compile against tortoise-wow instead of AzerothCore.

**New files to create:**

```
src/game/PlayerBots/Engine/
├── CommonTypes.h          # Type aliases: Player*, Unit*, ObjectGuid, WorldPacket, etc.
├── ServerFacade.h/cpp     # Distance, facing, chase, packet sending
├── PlayerbotAIConfig.h    # Config struct + sPlayerbotAIConfig singleton
├── Helpers.h              # split(), trim(), strsti() — copy verbatim
├── PerfMonitor.h          # Copy verbatim (pure C++)
├── Timer.h                # getMSTime() wrapper around tortoise-wow timer
├── Logging.h              # LOG_ERROR/LOG_DEBUG macros → sLog.outError/sLog.outDebug
└── ChatHelper.h/cpp       # Minimal chat routing (whisper → bot command)
```

### AC API → tortoise-wow Mapping

| AC API | tortoise-wow equivalent | Notes |
|--------|------------------------|-------|
| `Player*` | `Player*` | Same class, different methods |
| `Unit*` | `Unit*` | Same |
| `ObjectGuid` | `ObjectGuid` | Same, `.GetCounter()` → `GetGUIDLow()` |
| `WorldPacket` | `WorldPacket` | Same, `.GetOpcode()` exists |
| `SpellInfo*` | `SpellEntry const*` | Different name, similar data |
| `ItemTemplate*` | `ItemPrototype const*` | Different name |
| `sConfigMgr->GetOption<T>()` | `sConfig.GetBoolDefault()` / `GetIntDefault()` | Different API |
| `sSpellMgr->GetSpellInfo()` | `sSpellMgr.GetSpellEntry()` | Different name |
| `player->CastSpell()` | `player->CastSpell()` (on Object) | Lives on Object, not Unit |
| `player->GetGroup()` | `player->GetGroup()` | Same |
| `player->GetName()` | `player->GetName()` | Same |
| `player->getClass()` | `player->GetClass()` | CamelCase difference |
| `player->GetGUID().GetCounter()` | `player->GetGUIDLow()` | Different API |
| `urand()`, `frand()` | `urand()`, `frand()` | Same |
| `LOG_ERROR("playerbots", ...)` | `sLog.outError("playerbots: ...")` | Different format |
| `getMSTime()` | `getMSTime()` or `time(NULL)*1000` | Check if exists |
| `PreparedStatement` / `PlayerbotsDatabase` | `CharacterDatabase.PQuery()` | Completely different |
| `WorldScript::OnPlayerAfterUpdate` | `PlayerBotMgr::Update()` → `botAI->UpdateAI()` | Already exists! |

### Action Items

- [x] Create `src/game/PlayerBots/Engine/` directory
- [x] Write `CommonTypes.h` with forward declarations and type aliases
- [x] Write `ServerFacade.h/cpp` — ~4 methods (distance, facing, chase, sendPacket)
- [x] Write `PlayerbotAIConfig.h` — struct with ~20 config fields, singleton
- [x] Copy `Helpers.h` verbatim from mod-playerbots
- [x] Copy `PerfMonitor.h` verbatim (or stub out)
- [x] Write `Logging.h` — map `LOG_ERROR`/`LOG_DEBUG` to `sLog`
- [x] Write `Timer.h` — `getMSTime()` wrapper
- [x] Write minimal `ChatHelper.h/cpp` — route whispers to bot command handler

---

## Phase 1: Port the Engine Framework

**Purpose:** Port the pure-logic decision engine. Once this works, you can add strategies/actions/triggers and the engine will execute them.

### Tier 0 — Copy verbatim (zero AC dependencies)

- [ ] `Engine/NamedObjectContext.h` (310 lines)
- [ ] `Engine/NamedObjectContext.cpp` (52 lines) — remove Playerbots.h include
- [ ] `Engine/Strategy/Strategy.h` (76 lines)
- [ ] `Engine/Multiplier.h` (23 lines)
- [ ] `Engine/CustomStrategy.h` (31 lines)

### Tier 1 — Minimal changes

- [ ] `Engine/PlayerbotAIAware.h` (20 lines) — rename PlayerbotAI→BotAI, remove AC includes
- [ ] `Engine/AiObject.h` (444 lines) — replace Common.h→CommonTypes.h, update macros
- [ ] `Engine/AiObject.cpp` (15 lines) — update method calls to use new facade

### Tier 2 — Core types

- [ ] `Engine/WorldPacket/Event.h` (45 lines) — replace AC WorldPacket/ObjectGuid→CommonTypes
- [ ] `Engine/Action/Action.h` (165 lines) — update includes, replace getMSTime()
- [ ] `Engine/Action/Action.cpp` (20 lines) — update timer call
- [ ] `Engine/Trigger/Trigger.h` (83 lines) — update includes
- [ ] `Engine/Trigger/Trigger.cpp` (46 lines) — update timer/type references
- [ ] `Engine/Value/Value.h` (419 lines) — port framework, stub concrete AC types

### Tier 3 — Engine core

- [ ] `Engine/AiObjectContext.h` (95 lines) — replace AC includes→CommonTypes
- [ ] `Engine/AiObjectContext.cpp` (148 lines) — update type references
- [ ] `Engine/Engine.h` (121 lines) — replace Queue→std::priority_queue or port Queue
- [ ] `Engine/Engine.cpp` (675 lines) — update config, logging, timer, botAI facade, Player types

### Tier 4 — Wiring (stub for now)

- [ ] `Engine/BuildSharedStrategyContexts.cpp` — stub, empty
- [ ] `Engine/BuildSharedActionContexts.cpp` — stub, empty
- [ ] `Engine/BuildSharedTriggerContexts.cpp` — stub, empty
- [ ] `Engine/BuildSharedValueContexts.cpp` — stub, empty
- [ ] `Engine/ExternalEventHelper.h` — stub packet/chat routing
- [ ] `Engine/ExternalEventHelper.cpp` — stub

### Also needed

- [ ] `Bot/Queue.h` + `Queue.cpp` (94 lines) — replace Common.h, config reference
- [ ] `Bot/PlayerbotAIBase.h` + `PlayerbotAIBase.cpp` — rewrite for tortoise-wow timing

### Integration with existing tortoise-wow

- Replace the existing `PlayerBotAI` class with a new one that inherits from `PlayerbotAIBase`
- The new `PlayerBotAI::UpdateAI()` delegates to `Engine::DoNextAction()`
- `PlayerBotMgr::Update()` already calls `botAI->UpdateAI(diff)` — this hook already exists!

---

## Phase 2: Combat for One Class (Proof of Concept)

**Purpose:** Prove the system works end-to-end with one class before doing all 9. Starting with **Mage** — ranged caster, straightforward rotation, no melee complexity.

### Files to port

```
Ai/Class/Mage/
├── GenericMageStrategy.h/cpp          — base combat strategy
├── FrostMageStrategy.h/cpp            — Frost spec rotation
├── FireMageStrategy.h/cpp             — Fire spec rotation
├── ArcaneMageStrategy.h/cpp           — Arcane spec rotation
├── MageAiObjectContext.h/cpp          — registers mage strategies/actions/triggers
└── MageActions.h/cpp                  — class-specific spell actions
```

### Base actions needed

- [ ] `GenericSpellActions.h/cpp` — "cast spell", "cast spell on target", etc.
- [ ] `GenericActions.h/cpp` — "generic", "generic target"
- [ ] `ChooseTargetActions.h/cpp` — "choose target", "choose closest"
- [ ] `ReachTargetActions.h/cpp` — "reach target"
- [ ] `AttackAction.h/cpp` — "attack"
- [ ] `CombatActions.h/cpp` — "enter combat", "leave combat"

### What to skip for Phase 2

- Dungeon boss AI for mage
- Raid boss AI for mage
- Non-combat strategies (movement, looting, questing)
- Chat commands beyond basic "change strategy"

---

## Phase 3: Combat for Remaining Classes

**Purpose:** Add combat strategies for all 9 classes.

### Priority order

1. **Priest** — healer, high value for groups
2. **Paladin** — tank/healer hybrid
3. **Warrior** — tank/melee DPS
4. **Druid** — tank/healer/DPS, most versatile
5. **Shaman** — healer/ranged DPS
6. **Hunter** — ranged DPS, pet complexity
7. **Warlock** — ranged DPS, pet complexity
8. **Rogue** — melee DPS
9. **Death Knight** — depends on expansion support

### Per-class checklist (repeat for each)

- [ ] Generic strategy (base)
- [ ] 2-4 spec strategies (tank/dps/heal)
- [ ] Class-specific actions and triggers
- [ ] Class AiObjectContext (registration)

---

## Phase 4: Healing, Buffing, Group Support

**Purpose:** Make bots useful in groups, not just solo DPS.

### Files to port

- [ ] `Ai/Base/Strategy/HealStrategies.h/cpp` — heal strategies
- [ ] `Ai/Base/Actions/HealActions.h/cpp` — "heal", "heal party", "heal raid"
- [ ] `Ai/Base/Actions/BuffAction.h/cpp` — "buff", "buff party"
- [ ] `Ai/Base/Actions/WorldBuffAction.h/cpp` — "world buff"
- [ ] `Ai/Base/Actions/InterruptActions.h/cpp` — "interrupt"
- [ ] `Ai/Base/Actions/CCActions.h/cpp` — crowd control
- [ ] `Ai/Base/Actions/DispelActions.h/cpp` — "dispel"
- [ ] `Ai/Base/Actions/RemoveAuraAction.h/cpp` — "remove aura"
- [ ] `Ai/Base/Strategy/GroupStrategy.h/cpp` — group awareness
- [ ] `Ai/Base/Actions/InviteToGroupAction.h/cpp` — "invite to group"
- [ ] `Ai/Base/Actions/AcceptInvitationAction.h/cpp` — "accept invitation"
- [ ] `Ai/Base/Actions/FollowActions.h/cpp` — "follow", "follow master"
- [ ] `Ai/Base/Actions/StayActions.h/cpp` — "stay"
- [ ] `Ai/Base/Actions/CombatFormationMoveAction.h` — formation positioning
- [ ] `Ai/Base/Actions/TankFaceAction.h/cpp` — tank face positioning

---

## Phase 5: Movement, Death, Basic Non-Combat

**Purpose:** Handle death, basic movement, and survival.

### Files to port

- [ ] `Ai/Base/Strategy/DeadStrategy.h/cpp` — death handling
- [ ] `Ai/Base/Actions/ReleaseSpiritAction.h/cpp` — "release spirit"
- [ ] `Ai/Base/Actions/AcceptResurrectAction.h/cpp` — "accept resurrect"
- [ ] `Ai/Base/Actions/ReviveFromCorpseAction.h/cpp` — "revive from corpse"
- [ ] `Ai/Base/Actions/MovementActions.h/cpp` — "move to", "move to random"
- [ ] `Ai/Base/Actions/RangeAction.h/cpp` — "range" (distance check)
- [ ] `Ai/Base/Strategy/LootNonCombatStrategy.h` — loot awareness (optional)
- [ ] `Ai/Base/Actions/LootAction.h/cpp` — "loot" (optional)
- [ ] `Ai/Base/Actions/RepairAllAction.h/cpp` — "repair all"
- [ ] `Ai/Base/Actions/BuyAction.h/cpp` — "buy" (food/drink)
- [ ] `Ai/Base/Actions/SellAction.h/cpp` — "sell"

---

## What to SKIP (not in scope)

| System | Why skip | Lines skipped |
|--------|----------|---------------|
| Travel/Navigation (TravelMgr, TravelNode) | Requires 20+ DB tables, waypoint graph, flight paths | ~8,000 |
| Questing | Deeply coupled to gossip, quest log, NPC interaction | ~5,000 |
| Dungeons (16 instances) | Boss-specific mechanics, instance scripts | ~30,000 |
| Raids (19 instances) | Same + more complex mechanics | ~40,000 |
| Battlegrounds | BG-specific positioning, flag mechanics | ~5,000 |
| Guild system | Petitions, guild bank, ranks | ~3,000 |
| RPG open-world | Grind routes, camp, explore | ~8,000 |
| Mail/LFG | Mail system, LFG queue | ~3,000 |
| Vehicle system | Enter/exit vehicles | ~2,000 |
| Fishing/Pets/Taming | Pet AI, fishing game | ~3,000 |

---

## Key Technical Decisions

### 1. Database layer
- mod-playerbots uses a separate `PlayerbotsDatabase`
- **Decision:** Start with `CharacterDatabase.PQuery()`, migrate later if needed

### 2. Config
- mod-playerbots has ~200 config options
- **Decision:** Start with ~10 essential options, add more as needed

### 3. Packet interception
- mod-playerbots intercepts packets via `PlayerbotScript::OnPlayerbotPacketSent()`
- tortoise-wow doesn't have this hook
- **Decision:** Start with `UpdateAI()` polling, add packet hooks later

### 4. Chat commands
- mod-playerbots intercepts whispers via `OnPlayerCanUseChat`
- tortoise-wow has `WorldSessionScript::OnWhispered`
- **Decision:** Implement `WorldSessionScript` subclass for whisper routing

---

## Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| API incompatibilities deeper than expected | High | Phase 2 (one class) reveals real cost early |
| Performance: engine adds overhead per bot tick | Medium | PerfMonitor.h already exists to measure |
| Thread safety: bot sessions vs world thread | Medium | mod-playerbots has `WorldThr/Queue` for this; may need similar |
| Spell ID differences between AC and tortoise-wow | Low | Spell IDs are game data, not engine data — should be identical |
| Scope creep: "just one more class" | Medium | Strict phase boundaries, skip list is firm |

---

## File Structure (Final Target)

```
src/game/PlayerBots/
├── PlayerBotMgr.h/cpp              # Existing — keep, minimal changes
├── PlayerBotAI.h/cpp               # Existing — REWRITE to use Engine
│
├── Engine/                          # NEW — decision engine framework
│   ├── NamedObjectContext.h/cpp
│   ├── AiObject.h/cpp
│   ├── PlayerbotAIAware.h
│   ├── Strategy/Strategy.h
│   ├── Multiplier.h
│   ├── CustomStrategy.h
│   ├── Action/Action.h/cpp
│   ├── Trigger/Trigger.h/cpp
│   ├── Value/Value.h
│   ├── WorldPacket/Event.h
│   ├── AiObjectContext.h/cpp
│   ├── Engine.h/cpp
│   ├── Queue.h/cpp
│   ├── BuildShared*Contexts.cpp
│   └── ExternalEventHelper.h/cpp
│
├── Bot/                             # NEW — AI base
│   └── PlayerbotAIBase.h/cpp
│
├── Util/                            # NEW — adapter layer
│   ├── CommonTypes.h
│   ├── ServerFacade.h/cpp
│   ├── PlayerbotAIConfig.h
│   ├── Helpers.h
│   ├── PerfMonitor.h
│   ├── Timer.h
│   ├── Logging.h
│   └── ChatHelper.h/cpp
│
├── Ai/                              # NEW — behaviors
│   ├── Base/
│   │   ├── Actions/                 # ~40 ported action files
│   │   ├── Strategy/                # ~15 ported strategy files
│   │   ├── Trigger/                 # trigger files
│   │   ├── Value/                   # value files
│   │   └── *Context.h/cpp           # registration
│   └── Class/
│       ├── Mage/                    # Phase 2
│       ├── Priest/                  # Phase 3
│       ├── Paladin/                 # Phase 3
│       ├── Warrior/                 # Phase 3
│       └── ...                      # remaining classes
│
└── Factory/                         # NEW — bot creation
    └── AiFactory.h/cpp              # Creates engines per class/spec
```

---

## Change Log

| Date | Change |
|------|--------|
| 2026-06-13 | Plan created |
| 2026-06-13 | Phase 0 complete — adapter layer committed (7ec62ea) |
