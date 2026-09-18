<!-- Canonical copy: /root/AGENTS.md — edits there, then re-sync. Synced 2026-09-18 (headless far-teleport world-drain CRITICAL FIX + R6.1 agent interface). -->

# Tortoise WoW Server - AGENTS.md

## Overview
Tortoise WoW (Vanilla 1.18.1, build 7272) private server on Debian 13.

**READ FIRST — Bot Master Plan (2026-06-27):** `/root/bot-master-plan.md` is the single source of truth for the bot effort. The original scope limits ("skip travel, dungeons, raids, questing, BGs, guilds") are **REMOVED** — the goal is best-of-both bots using any source (our AC-port chassis + Shyalya/ike3 feature mining + AC class strategies). Supporting docs:

| Doc | Role |
|-----|------|
| `bot-master-plan.md` | Master strategy, architecture decisions, roadmap R0-R6, review reconciliation |
| `tortoise-data-plan.md` | `tortoise-data` — new tortoise-native `tw` CLI (NOT an acore-data fork; mangos DBCfmt-driven) |
| `agent-bot-interface-plan.md` | Port of mod-ollama-bot-buddy logic — agent↔bot command/control surface + eval harness |
| `pi-websearch-extension.md` | **RESOLVED**: web search/fetch via `pi-web-access` pi package (zero-config Exa MCP fallback chain; installed 2026-09-16). Local SearXNG (:8890) left for opencode/kindly — this IP is engine-soft-banned |
| `playerbot-port-plan.md` | Historical: engine port detail, phases 0-3 (scope-limited era) |
| `playerbot-review.md` | Local-AI critical review 2026-06-26; fixed/open items tracked in master plan §4 |
| `r1-base-sync-log.md` | R1 issues & solutions (merge conflicts, WorldSession plumbing, updater keys, migration reconciler) |

**Repo state (2026-09-18):** `playerbot-engine-port` merged with Penqle `origin/main` (merge `a473d4f5`, backup branch `playerbot-engine-port-pre-r1`) + R1 fixes (`2136011f`…`dff5ce37`) + review fixes (`ceeb728c` P1-3/P3-4, `97ab55cc` P2-3/P2-6) + **critical AI-wiring fix `24512ec1`** (merge dropped `sPlayerBotMgr.OnPlayerInWorld` from `Player::AddToWorld` → bots were online but AI never ran; pre-fix soaks invalid) + pet-taming guard `f273bb10` + **R2 class parity: `c276ca66`** (factory class diversity) + **`3e5932cf`** (spell auto-learn fix — non-warrior bots had 0 class spells; + Mage v1) + **5 more class strategies** (Warlock Fear CC, Priest/Shaman/Druid self-heal, Paladin HoJ CC + self-heal; shared `SelfHealAction`) + **6 warrior Fury/Tank triggers** (bloodthirst/whirlwind/shield slam/revenge/taunt/demoralizing shout — dead branches at low level via `FindSpellIdByName`). **100/100 bots online, all 9 classes learning + casting class spells**, `fear`/`heal self` actions verified firing, 0 real crashes. **Learnable-spell lesson**: Hex (11641) absent from `skill_line_ability` → warlock CC uses Fear (5782); self-heal IDs = `character_spell` ground truth (2053/332/639/5186). **R2 class parity essentially complete** (7/9 classes have dedicated strategies; rogue/hunter on generic fallback — deferred). **R5 world population (`13dc93ad`): 300 bots, level 1-60 (weighted, low-level bias), all 9 classes, spread across both continents** — factory now assigns weighted 1-60 level + level/faction-appropriate open-field spawn (`PickSpawnPosition`: Alliance→EK map 0, Horde→KM map 1, 50+→shared Silithus) via `SetLocationMapId`/`Relocate`/`SetMap`; grind `SelectNearestSafeTarget` has a humanoid distance-weight (`PlayerBot.HumanoidGrindDistanceWeight`, default 0.6) so gear-dropping humanoids beat nearer beasts. Regenerated via `DeleteAllBots=1` wipe+shutdown then `DeleteAllBots=0`+`FactoryBotCount=300` recreate. Verified 300/300 online, 0 crashes, all alive, grinding. **R5 gear+skills (`2ee06bc5`):** `ApplyGear` rewritten as runtime gear-gen from `item_template` (top-16 ranked candidates/slot, best-first via engine `CanEquipItem(...,swap=true)`, `ItemLevel<=level+10` bound, starter junk destroyed in-slot) — verified every bot has level-tracked weapon+armor (0 empty mainhands, avg 14.2 items, ilvl tracks level per band); `TrainClassSkills` now grants `SKILL_DEFENSE` for all classes + cap = `min(level*5,300)` matching core `GetSkillMaxForLevel()`; **`AlwaysMaxSkillForLevel=1`** in mangosd.conf → core re-maxes weapon/defense skills on login AND every level-up (verified: skills exactly 5×level at 5/20/55); two-tier humanoid preference (`PlayerBot.HumanoidPreferRange`=30yd default, `SightDistance` 150yd, ±400u signed-cast spawn jitter + `MapManager::IsValidMapCoord` guard fixing the unsigned-underflow crash). Next: R4 group/dungeon work + quest pipeline (after groups). **R5c/d XP-targeting + self-heal (`7f995e99`):** user course-correct — target selection is XP-primary (not gold-aware): XP gate + z-diff + LOS filters, tap-claim contention model (tap = the only claim; groupmate pulls fair game — dungeon/raid assist ready), `effDist` scoring with `XPLevelPenalty`; diagnosed ~200/300 bots marooned idle at 6 graveyards (gray surroundings) → **idle-relocation sweep** (`PlayerBot.RelocateIdleSeconds`=180, teleport to band spawn) + `MaxTargetLevelDiff` 7→4 (AC parity) + stuck-COMBAT normalization in UpdateAI. Lessons in AGENTS.md (Engine::Reset bricks engines; BOT_STATE_COMBAT=0; CombatStrategy has no retargeting trigger; scan needs LOS; outInfo→info.log). Remaining: band-edge level gaps, per-cell saturation, long corpse-runs → R4d. **R5e real travel (`f7c46bb3`, deployed 2026-09-18):** per-bot random 1-5h travel to real towns — inn/flight/bank/city hubs (npc_flags&392) + quest-giver POIs, 25% city + homebind refresh, startup-cached destination pools (213 hubs / 2187 quest NPCs); NPC/quest level = zone-level proxy (zone DBC names are WotLK-era garbage); faction via `faction_template.hostile_mask` (bit 4=horde-hostile, 2=alliance-hostile). Night-verify (fast 10-30min window) exposed the level-band holes → tightened brackets (hub [-5,+3], POI ±2, uncapped POI = minLevel+8 window, span>15 excluded, hubs ≤lvl5 excluded) + **stale-combat breaker** (`StaleCombatSeconds=600`, breaks bots stuck >10min on unkillable city guards — they blocked both sweep and travel) + **mob-anchored relocation RETIRED** (user direction: sweeps now use the same town pool as travel; band anchor+GetHeight only as last resort). BUG: uninit struct fields in POI dedupe (first row = stack garbage "uncapped") — assign every field before push_back. Verified: 300/300 online, 559 travels overnight, level-matched landings, 0 crashes. See `bot-master-plan.md` R5e sections. **R3a item-scoring assessment (2026-09-18):** the "stats collector / weight calculator / item usage" port candidates **already exist in-tree** (`b8ab94e7`/`e7603c03`/`ceeb728c`) as a vanilla-adapted v1 — `StatsCollector` + `StatsWeightCalculator` (`PlayerBots/Mgr/Item/`, spec tables for all 9 classes × vanilla talent tabs) + `ItemUsageValue` (registered, `QueryItemUsageForEquip` done, threshold 1.1) — but the layer is **dead code**: `Calculate()` always returns NONE, no action consumes it, `EquipUpgradesAction` still scores `ilvl×quality`. Vanilla data-model lessons: **only 7 base stat types in the DBC array** (hit/crit/haste/defense/SP etc. come from **item spells** — the collector's spell path is primary); flat % aura semantics (existing `×0.02` hit/crit factors = WotLK leftover bug); `SPELL_AURA_MOD_STAT` stat index in `EffectMiscValue` (0=STRA…4=STAM, <0=all); **green suffixes roll the SQL `item_enchantment_template` table** (`entry/ench/chance`, 27,687 rows — chance-weighted AVERAGE is the honest score, not AC's "best suffix"); no sockets in data; no DK/expertise/resilience. Plan: P1 collector fixes, P2 calculator completion (overflow via `GetTotalAuraModifier`, set bonus via `m_ItemSetEff`/`sItemSetStore`, weapon-type penalties, slot param), P3 wiring (shared helper, `EquipUpgradesAction` on the calculator, potions/bandages, junk policy). Details in `bot-master-plan.md` R3a. **R3a P1 DONE + verified live (2026-09-18):** collector rewritten — flat-% hit/crit (dropped WotLK `×0.02`), collector-type routing (melee/ranged/caster; generic 52/54 auras credit both melee+ranged per engine), `MOD_STAT` via `EffectMiscValue`, haste (138/140/141), school-crit (71), percent-only dodge/parry/block (47/49/51 — 46/48/50 are `HandleUnused` in this engine), exact effect value (`base+die` per `CalculateSimpleValue`), 0-credit unknowns, `RandomSuffixCache` (startup SQL cache, 781 classes/27,687 rows; chance-weighted class avg + instance-accurate via `sItemRandomPropertiesStore`); calculator: `CalculateItem(Item*)` instance overload + **score floor `1+q×0.5+ilvl×0.05`** (replaced the `ilvl×10` fallback that drowned stat scores at high ilvl). New data-model lessons: **spells load from SQL `tw_world.spell_template` (150 cols, positional `SELECT *`), NOT DBC** (the `spell` table is an empty shell); **`ItemSpelltriggerType` is WotLK-style ON_USE=0/ON_EQUIP=1/ON_HIT=2** (data matches; engine passives = trigger 1 only); **`ITEM_CLASS_ARMOR=4`** (custom enum, GEM=3); **armor `SubClass` = armor type 1-4, not slot** (slot = `InventoryType`); `Name1` is `std::string` (7 `%s` UB sites fixed); 6 items (incl. 10030 hat) encode MOD_STAT via dead effect 35 → floor-only scores correct. `tw` gap: DBC catalog missing `Spell` store (parser covers ~54 stores) — SQL `spell_template` is the canonical spell-query path; tw parser fix = backlog. Verify hook: `PlayerBot.DebugScoreDump=1` (first 5 bots dump bag+ref scores at login; OFF in prod). Next: R3a P2 (overflow/set/weapon/slot) → P3 (wire `EquipUpgradesAction` to the calculator).

**MISSION RESTATED (2026-09-19): 100% port of azerothcore `mod-playerbots`** — no invented behavior; where AC does X, we port X (adapting core APIs, not bot behavior). The teleport-to-POI quest sweep design was REJECTED; AC's bots **WALK** to givers/POIs/towns via engine pathfinding, teleport = stuck-recovery only. R7 = 5-layer faithful port (L1 Movement → L2 RPG state → L3 Quest data → L4 Quest actions → L5 Wiring); details + all API mappings in `bot-master-plan.md` R7. **R7 L1 DONE + deployed (2026-09-19):** `MovementActions` rewritten with AC's full MovementAction API (MoveTo/MoveNear/MoveDelay/WaitForReach/IsWaitingForLastMove/IsDuplicateMove/IsMovingAllowed/SearchForBestPath/DoMovePoint) + `MoveFarTo` (AC verbatim: in-flight guard, committed-movement early-out, 5yd stuck rule → teleport, pathFinderDis straight-walk, PathFinder-to-true-dest w/ progress guard, 2-sample cone fallback) + `MoveWorldObjectTo` (±25% approach jitter) + `MoveRandomNear` (8-attempt path/water-validated wander — now the wander action); `LastMovementValue` = AC superset (lastdelayTime/lastMoveShort); `PlayerRpgInfo` (vanilla status subset + DoQuest + MOVE_FAR stuck fields); `PlayerBotAI::CanMove` full AC port. Config: `PlayerBot.PathFinderDis=70`, `MoveStuckTime=90000`, `MaxMovementSearchTime=3`. **API lessons (this core):** `PATHFIND_FARFROMPOLY` absent → SHORTCUT stands in; `rand_norm()` = `rand_norm_f()` (Util.h); `Unit::IsMoving` (capital I); `Unit::isFrozen` DEAD declaration → `IsFrozen()`; `Player::FindQuestSlot`/`GetQuestSlotQuestId` made PUBLIC (AC API parity, pure access change — AC module calls them 7×). Rejected sweep DORMANT (`PlayerBot.QuestEnabled=0`; cache kept for L3). Verified live: 300/300 online, 299 alive, 0 crashes 15min, equips/sweeps/travel still firing. **R6 agent interface (bot-buddy port) STARTED 2026-09-19**: `.playerbots` console hook exists (`PlayerBotMgr::HandleConsoleCommand`); **BLOCKER: mangosd under GDB eats console input** (tmux send-keys hits the GDB prompt) → restart WITHOUT GDB for agent sessions (output still flows to the pane); state-file dump + command API = R6.1 build. **R4/R7 remaining:** L2-L5 quest port (walking), R4 groups/dungeons, L1b combat movement actions (deferred — combat uses `MoveChase`).


**Reference repos on this machine:**
- `/root/azerothcore-wotlk/modules/mod-playerbots/` — AC reference (class strategies etc.)
- `/root/azerothcore-wotlk/modules/mod-ollama-bot-buddy/` — agent bot-control logic source
- `/root/shyalya-tortoise-wow` — Shyalya fork, **also fetched as git remote `shyalya` in /root/tortoise-wow (FULL 903-commit history, +52MB)** — use the remote for `git log -S`/`blame`/`show`/cherry-pick; the /root mirror is a browsable worktree (shallow). Upstream archived 2026-09-30. Mining discipline: master plan A5 — lift logic as rewrites, never merge the module wholesale
- `/root/tortoise-data` — **built**: `tw` CLI (DBCfmt-driven DBC + read-only SQL), pushed to github.com/avirar/tortoise-data. Also **global pi extension** `~/.pi/agent/extensions/tortoise-data.ts` → `tw` tool in every pi session (works from any directory)
- `/root/acore-data` — AC data tool (concepts only; tortoise-data is a fresh build)
- `gh` CLI authed as avirar (PAT from user; git credential helper active)
- AC client data (wowgaming v20.0): `/root/azerothcore-wotlk/env/dist/bin/{dbc,maps,vmaps,mmaps}`
- pi web access: `pi install npm:pi-web-access` (tools `web_search`, `fetch_content`, etc. — Exa zero-config; keys optional in `~/.pi/agent/web-search.json`)

## Server Details
- **Install path**: `/root/tortoise-wow`
- **Static IP**: 192.168.1.234
- **Realm**: AviTort (aviroth.ddns.net:8090)
- **Login port**: 3725 (realmd)
- **Game port**: 8090 (mangosd)
- **Admin account**: admin/admin (GM rank 3)

## Quick Commands
```bash
# Server management
./wow-server.sh start|stop|restart|status|logs

# Build & clean
./wow-server.sh build           # Incremental build (auto-cleans orphaned linker temps)
./wow-server.sh build all       # Full clean + configure + build (like AC's comp_all)
./wow-server.sh clean           # Remove orphaned linker temp files (st* from crashed builds)
./wow-server.sh clean full      # Wipe entire build dir (like AC's comp_clean)
./wow-server.sh clean git       # Run git gc --prune=now --aggressive
./wow-server.sh configure       # Run cmake with ccache launcher flags

# Debug
./wow-server.sh startgdb        # Start mangosd under GDB (crash backtrace capture)

# tmux sessions
tmux attach -t realmd    # Login server
tmux attach -t mangosd   # World server

# Database
mysql -u mangos -pmangos tw_world   # World DB
mysql -u mangos -pmangos tw_char    # Character DB
mysql -u mangos -pmangos tw_logon   # Auth DB
```

## Critical Config Files
| File | Purpose |
|------|---------|
| `/root/tortoise-wow/server/etc/realmd.conf` | Login server (port 3725) |
| `/root/tortoise-wow/server/etc/mangosd.conf` | World server (port 8090) |
| `/root/wow-server.sh` | Server management script |

## Data Directories
```
/root/tortoise-wow/server/data/
├── base.mpq          # Client MPQs
├── dbc*.mpq
├── patch*.mpq
├── dbc/              # 159 extracted DBC files
├── maps/             # 2805 extracted map files (154 MB)
├── vmaps/            # Assembled vmap files
└── mmaps/            # 2,133 movement map tiles (2.3 GB)
```

## Known Issues & Fixes

### Directory Case Sensitivity (FIXED)
- **Problem**: Extractors hardcoded uppercase `Data` but Linux FS is case-sensitive
- **Fix**: Changed `Data` → `data` in `System.cpp:1019` and `vmapexport.cpp:340`
- **Impact**: Without this fix, extractors find zero MPQ archives and fail

### Database Names
- Auth: `tw_logon` (not `tw_logon_db`)
- World: `tw_world`
- Characters: `tw_char` (not `tw_chars`)
- Logs: `tw_logs`

### Realm Configuration
- realmflags = 2 (not 1) to pass `(realmflags & 1) = 0` filter
- realmbuilds = 7272 (Chinese client twmoa_1181_cn)
- BindIP = 0.0.0.0 (LAN access)

### 3-Engine Context Bug (FIXED 2026-06-25)
- **Problem:** `BuildSharedBaseAiObjectContext(botAI)` looked up context via `botAI->GetEngine()` → `GetCurrentEngine()` → `engines[currentState]`
- During `PlayerbotAIBase::Initialize()`, `currentState` was always `BOT_STATE_NON_COMBAT`, so **all values went to NON_COMBAT engine only**
- COMBAT and DEAD engines had empty `values` maps → `GetValue<Unit*>("current target")` returned null → segfault in `DropTargetAction::Execute()`
- **Fix:** Changed signature to `BuildSharedBaseAiObjectContext(PlayerBotAI*, AiObjectContext*)` — each engine passes `this` to its own context
- **Defense-in-depth:** Added null check in `DropTargetAction::Execute()` for "current target" value

### 3-Engine Shared Context Bug (FIXED 2026-06-25, commit 44e3a59)
- **Problem:** `Engine::Reset()` called `context->Reset()` which wiped ALL values from the shared `AiObjectContext`. When engine switched (e.g. COMBAT→NON_COMBAT), Reset() cleared "current target" for ALL engines
- **AC pattern:** ONE `AiObjectContext` shared across all 3 engines (`AiFactory::createAiObjectContext` passed to `createCombatEngine`, `createNonCombatEngine`, `createDeadEngine`). `Engine::Reset()` does NOT touch context
- **Fix:** Removed `context->Reset()` from `Engine::Reset()`; `Engine::Init()` now calls `Reset()` first (AC pattern); `PlayerbotAIBase::Initialize()` cleaned up (strategies added before single Init per engine)
- **Verification:** All 3 engines confirmed sharing same context pointer in logs; engine switching works without context corruption

### MovementBroadcaster Null Crash (FIXED 2026-06-26, commit 41332ae)
- **Problem:** `PlayerBotMgr::Load()` called before `MovementBroadcaster` was initialized in `World::Initialize()`. Bots casting spells during login triggered `MovementBroadcaster::IsEnabled(this=0x0)` → SIGSEGV
- **Fix:** Moved `PlayerBotMgr::Load()` call in `World.cpp` to after `m_broadcaster` initialization (line 2316 → after line 2320)
- **Impact:** Without this fix, server crashes during bot creation with >10 bots

### Bot Name Generation Exhaustion (FIXED 2026-06-26, commit b975565)
- **Problem:** `GenerateName()` had only 26×20=520 syllable combinations with 50 attempts. After ~48 bots, algorithm exhausted unique names → factory failed to create remaining bots
- **Fix:** Expanded syllable pools to 76 start × 100 end = 7600 combinations, increased attempts to 500
- **Impact:** Without this fix, factory creates only 48/100 bots, rest fail with "Could not generate unique name"

### Bot Scale Testing (100 bots verified 2026-06-26)
- **Config:** `FactoryBotCount=100`, `MinBots=100`, `MaxBots=100`, `UpdateMs=100`
- **Performance:** 96/100 bots online, CPU ~183%, Memory ~1.6GB, stable
- **Class distribution:** Warrior=15, Paladin=7, Hunter=14, Rogue=6, Priest=16, Shaman=6, Mage=18, Warlock=15, Druid=3
- **Race distribution:** Human=15, Orc=3, Dwarf=10, NightElf=7, Undead=9, Tauren=10, Gnome=12, Troll=14, Goblin=11, HighElf=9

### World Population Scale (300 bots, level 1-60, 2026-09-17, commit 13dc93ad)
- **Config:** `FactoryBotCount=300`, `MinBots=300`, `MaxBots=300`. Hardware: 24 cores, 23GB RAM (100 bots ≈ 1.8 cores / 1.6GB, so 300 ≈ 5.4 cores / ~5GB — comfortable).
- **Level distribution (weighted 1-60, exponent 1.6 low-level bias):** 1-10=99, 11-20=55, 21-30=48, 31-40=26, 41-50=35, 51-60=37.
- **Spawn placement:** `PickSpawnPosition(level, race)` → level/faction-appropriate open-field zone (Alliance→EK map 0, Horde→KM map 1, 50+→shared Silithus). EK 173 / KM 127 bots.
- **Class:** all 9 present (Warrior 58, Mage 43, Hunter 43, Rogue 46, Priest 38, Warlock 27, Druid 17, Paladin 14, Shaman 14).
- **Regenerate recipe:** `PlayerBot.DeleteAllBots=1` + restart (wipe+shutdown) → set `DeleteAllBots=0` + `FactoryBotCount=300` + restart (recreate). High-level bot spells via `AutoLearnSpellsForLevel()` at login.
- **Verified:** 300/300 online, 0 crashes, all alive, actively grinding.

### City NPCs / Elites Are Not Grind Targets (FIXED 2026-09-17)
- **Problem:** ~30 bots (level 1-57) suicide-looped the elite-50 **Sewer Beast** (entry 3581, spawns INSIDE the Stormwind canals at map0 -8784,487) — every humanoid in a major city is FRIENDLY (filtered by the unfriendly check) so the elite croc was the only "attackable" creature near the gate
- **Fixes:** `PlayerBot.MaxTargetLevelDiff` (default 7) — never grind >+7-level targets; elite/rare/worldboss skip (`rank > CREATURE_ELITE_NORMAL`) in `SelectNearestSafeTarget`; spawn table v2 = hostility+level-matched clusters only (faction blacklist 12,29,55,68,80,104,35,371,1682 + name eyeball; `rank=0`; `level_min ∈ [band_low, band_high-4]`)
- **Lesson:** "humanoid ⇒ gold" is imperfect on tortoise-custom data — the Mudpaw family are type-7 humanoids with gold 0-0; verify `gold_min>0` for economy clusters

### R5d Engine-State Lessons (2026-09-17)
- **`Engine::Reset()` DELETES all strategies + triggers** (only `Engine::Init()` re-adds them). NEVER call it standalone (e.g. after teleporting a bot) — it bricks the engine (no triggers, no actions, bot does nothing). Shyalya's `GetBotAI(bot)->Reset(true)` does NOT map 1:1 to ours. Clearing a value via the shared context (`GetValue<Unit*>("current target")->Set(nullptr)`) is safe.
- **`BOT_STATE_COMBAT = 0` in OUR port** (reversed from AC where NON_COMBAT=0). `state=0` in tick logs = COMBAT engine.
- **`CombatStrategy::InitTriggers` deliberately has NO "no target" trigger** (retargeting happens on NON_COMBAT via GrindingStrategy; AC design). The only COMBAT→NON_COMBAT exit is `DropTargetAction` → bots whose fight ends without a kill (evade/leash) get STUCK in the COMBAT engine forever. Fix deployed: `PlayerbotAIBase::UpdateAI` normalizes `currentState == COMBAT && !bot->IsInCombat()` → NON_COMBAT **and clears the dangling "current target"** (else "no target" never fires on NON_COMBAT). Server combat flag is authoritative.
- **Scan must check LOS if the attack does** — `DoAttack` returns false on `!IsWithinLOS` → bot stuck in action-FAILED loop picking an invisible-behind-terrain target. `SelectNearestSafeTarget` now filters `!IsWithinLOS` (AC pattern) with `los=` reason counter.
- **Graveyard marooning:** ~200/300 bots stacked idle at 6 GYs (mixed levels, one GY serving 1-58) — everything nearby gray → XP filter rejects all → idle forever. Fixed via idle-relocation sweep (`PlayerBot.RelocateIdleSeconds`, default 180) → teleport to level-appropriate band spawn. Relocations log to info.log (`playerbots: relocated idle bot ...`).
- **Idle clock design:** `ServerFacade::MarkViableGrindTargetSeen` on scan success; sweep reads `SecondsWithoutViableGrindTarget` (first-seen registers fresh → grace window). Only alive/overworld/not-fighting bots relocate.

### R5e Travel System (2026-09-17/18)
- **Design (user direction):** all bot *travel* is teleport to TOWNS — 1-5h random travel per bot (`PlayerBot.TravelMin/MaxSeconds`, default 3600/18000) to real innkeeper/flight/bank/city hubs (npc_flags & 392) + quest-giver POIs; 25% of lvl-10+ travel goes to a city and refreshes homebind (`SetHomebindToLocation` + `sTerrainMgr.GetAreaId`). Anti-cam 150y (deferred 30s if a real player is near). `PlayerTravelMgr` caches hubs + quest POIs at startup (one-time PQuery, ~2.4k entries; NOT per-tick DB).
- **Level proxy = NPC/quest level, not zones:** AreaTable.dbc zone IDs are WotLK-era with empty/inconsistent English names → unreliable; hub `level_min` (10=Elwynn, 20=Southshore, 30=mid-town default, 55+=city default) and quest `MinLevel` are live, self-contained, level-matched. **Faction check:** `faction_template` (underscore! `factiontemplate` is empty) `hostile_mask` bit 2 = hostile-to-Alliance, bit 4 = hostile-to-Horde, runtime via `sObjectMgr.GetFactionTemplateEntry(factionId)->hostileMask` (needs `Database/DBCStructure.h` for the complete type).
- **R5e.2 — mob-anchored relocation RETIRED (user direction):** the emergency sweeps (idle-relocation + stale-combat breaker) used to land bots next to a hostile mob's spawn (z/LOS hack). They now share `PlayerTravelMgr::PickDestination` (town pool) with `DoTravel`; band anchor + `GetHeight` is only the last-resort fallback (no level-matched town). Lesson: emergency self-heal should use the same destination semantics as normal travel — otherwise the "safety net" becomes the source of bad landings (lvl-12 bot into Venture Co lvl-14-17 camps).
- **Stale-combat breaker:** `PlayerBot.StaleCombatSeconds` (default 600) — a combat streak >10min (unkillable city guards, elite camps) blocks BOTH the relocation sweep and travel (both guard on `IsInCombat`); the breaker clears the streak via town teleport + shared-context target clear. Bots stuck in a city are re-traveled with `allowCity=false`.
- **POI matching rules (hard-won):** (a) every struct field assigned before `push_back` — the dedupe-merge pattern left `minLevel`/`maxLevel` as stack garbage on the first row per (entry,map) (often 0 = "uncapped" → ANY level matched); (b) hub pool excludes level ≤5 (wilderness caravan/mount NPCs); (c) wide quest span (>15) excluded (mixed-level givers); (d) `MaxLevel=0` ("uncapped") treated as minLevel+8 window (else a MinLevel=1 starting-zone POI matched every level). Verified landings: lvl 10→POI min 10, lvl 37→POI min 38.
- **API lessons (this core):** `ObjectGuid::GetCounter()` is the player low-guid accessor (NOT `GetGUIDLow`); `Map` is `GridRefManager<NGridType>` (no `GetAreaId`) → `sTerrainMgr.GetAreaId(mapid,x,y,z)`; `INVALID_HEIGHT=-100000.0f` (GridMap.h) — guard `GetHeight` with `<= INVALID_HEIGHT` before use; `Player::GetName()` returns `char const*`.
- **Remaining known issue (pre-existing, not travel):** band-10 KM anchor (1035,-3088) serves lvl-1-9 bots but the dense cluster there is Venture Co. camps (lvl 14-17) — band-edge level gap (R4d). Walking inter-town travel (upstream `TravelSystem-pr` branch: TravelNode graph, prepath/spline) is the R6+ reference; quest pipelines (accept/track/turn-in, `killEntry` already cached but unused) follow R4 groups.

### Headless Far-Teleport World-Drain (CRITICAL FIX, 2026-09-18)
- **Symptom:** ~40% of bots (121-162/300) left the world ~5-10min after login and never returned; DB still showed 300 online. Reproduced WITHOUT GDB. Looked like a logout but ZERO "Logout Character" lines; sessions NOT destroyed (dropped bots `online=1`, only ~7 ever `online=0`); players NOT deleted (destructor diag fired 0 times).
- **Root cause:** bots run on socketless `HeadlessSession`s. A **FAR (cross-map) teleport** (`Player::TeleportTo` → `ExecuteTeleportFar`) does `oldmap->Remove(this,false)` (bot leaves the world) + `SetSemaphoreTeleportFar(true)` and then **waits for the client's world-port ACK** (`CMSG_MOVE_MAP_CHANGE_QUERY`/`MSG_MOVE_WORLDPORT_ACK` → `HandleMoveWorldportAckOpcode`) to create the destination map + `Add()` the bot back. A headless bot has **no client → the ack never arrives → the bot is removed from its old map and never re-added** — stuck "between maps" forever (`IsBeingTeleportedFar()=true`, `IsInWorld()=false`, still `online=1`). Confirmed by an OUT-OF-WORLD diagnostic: every lost bot showed `teleport=1 near=0 far=1`.
- **Trigger:** the **per-login race start-town teleport** (CharacterHandler.cpp:954) is a far teleport for any bot whose *saved* map differs from its race start-town map (scattered save positions → ~40%). Travel/relocation sweeps (R5e) also do far teleports. (Design note: existing chars are force-teleported to the start town on EVERY login, discarding their saved position — revisit later.)
- **Fix:** in `HeadlessSessionMgr::Update`, for each headless player with `IsBeingTeleportedFar()`, call `session->HandleMoveWorldportAckOpcode()` (the no-arg server-side overload, WorldSession.h:664). It creates the destination map, `Relocate()`s + `Add()`s the bot, and clears the flag. Runs on the next tick → the bot is out-of-world for at most one tick. No-op unless actually mid-far-teleport.
- **Verified:** 300/300 stable (was draining 300→240→138 within ~10min); far-tp completions log (throttled 30s) shows a login burst then steady travel/relocation rate; 0 crashes; bots functioning (combat/leveling/death-respawn all observed).
- **Diagnostics kept (low-noise, event-driven):** far-tp completion log; `HeadlessSessionMgr` session-DESTROYED log (+ `IsHeadlessLoginRequested()` getter on WorldSession); OUT-OF-WORLD regression detector in `PlayerBotMgr::Update` (60s gate, ≤25 lines); Player-destructor log for headless bots.
- **Lesson:** headless bots cannot complete any client-ack-gated operation. Far teleports are the main one; near (same-map) teleports self-complete via the `MOVEMENT_CHANGE_ACK_TIME*5` timeout path (Unit.cpp:7440) so they were unaffected. Any future client-ack-dependent flow must get a headless self-heal.

### R3a P1 Item-Scoring Lessons (2026-09-18, DONE + verified live)
- **Spells are SQL, not DBC:** `SpellMgr::LoadSpells()` reads `tw_world.spell_template` (150 cols, positional `SELECT *` — column order is the contract). The `spell` table is an EMPTY shell (0 rows). Query spells via `spell_template` (lowercase cols: `entry`, `effect1`, `effectApplyAuraName1`, `effectBasePoints1`, `effectMiscValue1`, `effectDieSides1`). `tw dbc Spell` is broken (catalog gap — parser covers ~54 stores).
- **Effect value = `basePoints + dieSides`** (`SpellEntry::CalculateSimpleValue`, SpellEntry.h:656). Item auras universally use die ∈ {0,1} (99% die=1), so `base+die` == the exact engine value.
- **`ItemSpelltriggerType` (this core, WotLK-style): ON_USE=0, ON_EQUIP=1, CHANCE_ON_HIT=2, SOULSTONE=4, ON_NO_DELAY_USE=5.** Data matches: trigger 0 = consumables (5078), trigger 1 = passive gear (6155: 5290 armor + 828 weapons), trigger 2 = on-hit procs (316, all weapons). Engine applies passive item spells ONLY at trigger==1 (`Player::ApplyItemEquipSpell`, Player.cpp:8760). Scoring multipliers must follow this: equip 1.0 / on-use 0.15-0.25 / on-hit PPM.
- **`ITEM_CLASS` is custom: ARMOR=4** (not 5; GEM=3, WEAPON=2, CONSUMABLE=0, JUNK=15).
- **Armor `SubClass` = ARMOR TYPE (1=cloth, 2=leather, 3=mail, 4=plate), NOT slot** — slot is `InventoryType` (e.g. item 3341 "Gauntlets of Ogre Strength" = subclass 3 = mail shoulders despite the name). The calculator's armor-type ×0.0/×0.7/×0.5 penalties use this.
- **Dead auras in this engine (HandleUnused):** 46 MOD_PARRY_SKILL, 48 MOD_DODGE_SKILL, 50 MOD_BLOCK_SKILL — only 47/49/51 (percent) are live. Generic 52 (crit) / 54 (hit) apply to BOTH melee and ranged (`HandleModHitChance` adds both; `HandleAuraModCritPercent` → both crits unless weapon-class aura). Haste = auras 138/140/141 (raw %, `HandleModMeleeSpeedPct` applies `m_amount` directly). Aura 71 = MOD_SPELL_CRIT_CHANCE_SCHOOL. 85 = MOD_POWER_REGEN. 30 = MOD_SKILL (miscValue=skillId; 305 items = SKILL_DEFENSE=95; engine requires `HasSkill`).
- **Effect 35 (APPLY_AREA_AURA_PARTY) item spells are dead in this world:** 6 rare items (incl. 10030 Admiral's Hat +9 INT) encode MOD_STAT via effect 35; the engine's target condition (Attributes==0x9050000/0x10000) never fires for item spells → no in-game effect → floor-only score is honest. Standard passive stat path = effect 6 (APPLY_AURA).
- **`ItemPrototype::Name1` is `std::string`** (loaded via `strdup` into it) — `%s` in printf-style logs is UB (garbage/crash). Use `.c_str()`. Fixed 7 playerbot sites (2026-09-18).
- **Score floor (calculator):** `max(weight_, 1 + q×0.5 + ilvl×0.05)` — the old `ilvl×10` REPLACE-fallback drowned real stat scores at high ilvl and degenerated scoring back to ilvl ranking. Any equippable item must score >0 (`ItemUsageValue` treats ≤0 as never-equip), but the floor must stay below stat-score magnitudes (10-100).
- **Verify hook:** `PlayerBot.DebugScoreDump=1` → first 5 bots dump bag items (instance suffix) + 5 reference items (727/10030/20130/3341/3841) at login (info.log `playerbots: scores: ...`). OFF in prod (config default 0).

### AC-Style Login System (2026-06-26, commit e0464d2)
- **Name table:** Imported `playerbots_names` from AC (~100K names, 18 gender categories). Loaded into memory cache (50K names). Conlang fallback, then syllable fallback.
- **Character persistence:** `PlayerbotFactory::GenerateBots()` checks for existing characters before creating. On restart, reuses existing characters (no re-creation).
- **Async login queue:** `PlayerBotMgr::AddBotAsync()` queues logins, `ProcessLoginQueue()` processes batch per tick (configurable `AsyncLoginBatchSize`). Non-blocking.
- **Config options:** `PlayerBot.AsyncLogin=1` (default), `PlayerBot.AsyncLoginBatchSize=10` (logins per tick)
- **DeleteAllBots:** AC cascade delete pattern (character_queststatus → character_spell → ... → characters → accounts)
- **Verified:** First startup creates 100 chars, second startup reuses all 100 (zero new creation). All 100 bots online.
- **SQL:** `sql/playerbots_names.sql` — InnoDB, ~97K names after cleanup (removes names > 12 chars)

### PerfMonitor & Stats Commands (2026-06-26, commits e698488, ef166bb, b57c1a5)
- **PerfMonitor integration:** `sPlayerbotPerfMonitor.start()`/`finish()` in Engine.cpp (triggers/actions), PlayerbotAIBase::UpdateAI() (FullTick)
- **AC FullTick pattern:** `totalPmo` member, finished at START of next UpdateAI call (measures inter-tick cycle time)
- **PerfMonitor::PrintStats():** Two modes — per-tick (normalized by FullTick count) and total (percentage of FullTick time)
- **`.playerbots` command (SEC_MODERATOR):**
  - `rndbot stats` — bot activity readout (online, combat, dead, moving, engine states, race/class/level distribution)
  - `pmon [tick|reset|toggle|stack]` — performance monitor (total stats, per-tick, reset, enable/disable, full stack)
  - `bot list` — online/offline counts
- **Periodic stats output:** `PrintStats()` (bot stats only) called every 30s in `PlayerBotMgr::Update()`
- **Config:** `PlayerBot.PerfMonEnabled=0` (disabled by default, zero overhead). Toggle via `pmon toggle`
- **Singleton rename:** `sPerfMonitor` → `sPlayerbotPerfMonitor` to avoid conflict with core `PerformanceMonitor`

### SaveToDB False Return Bug (FIXED 2026-06-26, commit c248341)
- **Problem:** `Player::SaveToDB()` returned `false` from `CommitTransaction()` even when data was saved to DB. Factory treated this as failure → `CreateBotCharacter()` returned 0 → `RegisterInPlayerbotTable()` never called → 0 bots queued for login
- **Root cause:** Tortoise's transaction layer can return false even on successful commit (async transaction quirk)
- **Fix:** Ignore `SaveToDB()` return value in `PlayerbotFactory::CreateBotCharacter()` (matches AC pattern which also doesn't check return)
- **Impact:** Without this fix, factory creates characters in DB but thinks they failed → 0 bots online

### DeleteAllBots Cleanup (2026-06-26, commit c248341)
- **Config:** `PlayerBot.DeleteAllBots=1` in mangosd.conf triggers cascade cleanup on startup
- **Cascade:** playerbot → characters → corpse → character_inventory → item_instance → character_account_data → character_action → character_aura → character_homebind → character_queststatus → character_reputation → character_skills → character_social → character_spell → character_spell_cooldown → character_pet → pet_aura → pet_spell → pet_spell_cooldown → groups → group_member → group_instance → mail_items → mail → guild → guild_member → guild_rank
- **Tortoise-specific tables:** character_instance, character_battleground_data, character_deleted_items, character_destroyed_items, character_gifts, character_titles, character_transmogs, character_variables, petition, petition_sign, hardcore_deaths
- **Shutdown:** Uses `ShutdownServ(1, ...)` for graceful shutdown (not `StopNow()` which crashes socket manager during init)
- **Verified:** Deleted 240 bot accounts and all associated characters in ~5 seconds

### AI Config Initialization (FIXED 2026-06-26, commit c248341)
- **Problem:** `sPlayerbotAIConfig.Initialize()` was never called during server startup. All AI config values used constructor defaults.
- **Fix:** Added `sPlayerbotAIConfig.Initialize()` call in `World.cpp` before `PlayerBotMgr::Load()`
- **Impact:** Without this fix, `DeleteAllBots` flag was always false, and other AI configs (reactDelay, etc.) used wrong defaults

### World::StopNow() Segfault (FIXED 2026-06-26, commit c248341)
- **Problem:** Calling `World::StopNow()` during `SetInitialWorldSettings()` (before main update loop starts) crashes `MangosSocketMgr::StopNetwork()` with SIGSEGV
- **Root cause:** Network threads not fully initialized when forced shutdown occurs during init phase
- **Fix:** Use `sWorld.ShutdownServ(1, SHUTDOWN_MASK_RESTART, SHUTDOWN_EXIT_CODE)` for graceful shutdown instead
- **Impact:** Without this fix, server segfaults on cleanup shutdown

### Death/Resurrection System (2026-06-26, commit c248341)
- **Files:** DeadStrategy.h/.cpp, ReleaseSpiritActions.h/.cpp, ReviveFromCorpseActions.h/.cpp, AcceptResurrectAction.h/.cpp, DeathTriggers.h/.cpp
- **DeadStrategy triggers:** `"can self resurrect" → "self resurrect"`, `"often" → "auto release"`, `"dead" → "find corpse"`, `"corpse near" → "revive from corpse"`, `"resurrect request" → "accept resurrect"`, `"falling far" → "repop"`
- **Critical bug fix:** Removed `if (!me->IsAlive()) return;` early return in `PlayerBotAI::UpdateAI()` that prevented dead bots from switching to DEAD engine
- **Tortoise API differences:** `sObjectMgr.GetClosestGraveYard()` returns `WorldSafeLocsEntry const*` with fields `map_id`, `x`, `y`, `z`. `bot->GetTeam()` returns `Team` enum. No `SPELL_AURA_ADD_EXTRA_SCHOOL_IMMUNITY`.
- **Awaiting test verification** before marking complete

### StoreLootAction lootslot_type Enforcement (FIXED 2026-06-26, commit 13cf152)
- **Problem:** `StoreLootAction` iterated `creature->loot.items` directly and took anything not `is_looted`, bypassing server's per-item permission system. In GROUP_LOOT mode, bots grabbed items with active rolls (`is_blocked`) and under-threshold items meant for other group members.
- **AC pattern:** AC's `StoreLootAction` parses `SMSG_LOOT_RESPONSE` packet and checks `lootslot_type` per item — only takes `ALLOW_LOOT`/`OWNER`, skips `ROLL_ONGOING`/`MASTER`/`LOCKED`.
- **Fix:** Replicate `LootView::operator<<` logic in `StoreLootAction` to compute `lootslot_type` from `Loot` object directly (can't intercept packet — no `WorldPacketTrigger`). Computes `PermissionTypes` from group context, then per-item slot type:
  - GROUP_LOOT: `is_blocked` → `ROLL_ONGOING` (skip), `is_underthreshold` + round-robin check → `ALLOW_LOOT`, else → `ROLL_ONGOING` (skip)
  - MASTER_LOOT: `is_underthreshold` → `ALLOW_LOOT`, else → `MASTER` (skip)
  - ROUND_ROBIN: only `roundRobinPlayer` can take
  - FFA/OWNER/ALL: always `ALLOW_LOOT`
- **Key difference from buggy `GetSlotTypeForSharedLoot`:** uses correct if/else priority (`is_blocked` → `ROLL_ONGOING`) instead of buggy `is_blocked || is_underthreshold` → `ALLOW_LOOT`
- **Verified:** Bots no longer attempt items with active rolls, respect group loot distribution

### Async Login Session Collision (FIXED 2026-06-26, commit d873c2a)
- **Problem:** `World::m_sessions` is keyed by `accountId`. When `charsPerAccount=10`, 10 bots shared each account. Each new session REPLACED the previous one in the map. After all 10 logged in, only the last survived - other 9 were orphaned and cleaned up within seconds.
- **Symptom:** 100 bots logged in (in-memory counter reached 100), but only 10 showed `online=1` in DB. Bots appeared then disappeared within a minute.
- **Fix (AC pattern):** `HandleBotLoginCallback` creates session INSIDE the login callback (not before). `ScheduleBotLogin()` wrapper in `CharacterHandler.cpp`. `charsPerAccount=1` so each bot has unique account ID.
- **Verified:** 100/100 bots persistent online, `online=1` in DB, visible in `/who`.

### DropTargetAction Dangling Pointer Crash (FIXED 2026-06-27)
- **Problem:** `Unit*` stored in "current target" context became dangling when creature despawned. `InvalidTargetTrigger::IsActive()` called `target->GetMapId()` on freed memory → SIGSEGV. Also crashed in `DropTargetAction::Execute()` on `target->IsDead()`.
- **Root cause:** Context stores raw `Unit*` pointers with no lifecycle tracking. Creature despawn → object destroyed → pointer dangling.
- **Fix:** `CurrentTargetValue` now stores `ObjectGuid` alongside `Unit*`. `Get()` validates: `IsInWorld()` (cheapest), `IsDeleted()` (backup), GUID match (catches memory reuse). Returns nullptr for stale pointers, clears context.
- **Defense-in-depth:** Existing `if (!target) return false;` in `InvalidTargetTrigger`/`DropTargetAction` now catches dangling pointers safely.
- **Files modified:** `CurrentTargetValue.h`, `CurrentTargetValue.cpp` (playerbot files only, no core changes)

### MovementBroadcaster Null Crash (FIXED 2026-06-27)
- **Problem:** `sWorld.GetBroadcaster()` returned null during shutdown. `InternalShutdown()` resets `m_broadcaster` while map threads still running bot AI. Bot sending speed change packet → `SendMovementMessageToSet()` → null dereference → SIGSEGV.
- **Fix:** `PlayerbotAIBase::UpdateAI()` checks `sWorld.IsStopped()` and skips all processing. Prevents bot AI from running during shutdown.
- **Files modified:** `PlayerbotAIBase.cpp` (playerbot files only, no core changes)

### EquipUpgrades Zero Equips (INVESTIGATED 2026-06-27)
- **Problem:** EquipUpgradesAction 100% failing — zero items equipped across all 100 bots.
- **Root cause:** Bots killing level 5-7 beasts (Mudpaw, Thalassian Stag, Hawkstrider) in Desolace. These drop only meat/junk (`inventory_type=0`). Zero equippable gear available.
- **Loot pipeline stats:** "loot" OK 6433 times, "open loot" FAILED 1954 times (98%), "store loot" 120 times, "equip upgrades" 100% FAILED.
- **OpenLootAction failures:** `GetLootGuid()` re-entry guard blocks re-attempt on subsequent ticks.
- **Resolution:** Requires bots in areas with gear-dropping creatures (humanoid mobs, higher level zones).

## Network
- **ens18**: 192.168.1.234/24, gateway 192.168.1.1
- **DNS**: 8.8.8.8, 8.8.4.4
- **DHCP**: `nohook resolv.conf` prevents DNS overwrite
- **Firewall**: iptables default ACCEPT (no port blocking)

## Services
- **MariaDB**: Running, databases auto-updated on mangosd start
- **SearXNG**: Docker container on localhost:8890 (MCP search)
- **realmd**: tmux session `realmd`, port 3725
- **mangosd**: tmux session `mangosd`, port 8090

## Rebuilding Extractors
```bash
cd /root/tortoise-wow
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../server -DUSE_EXTRACTORS=ON
make -j$(nproc)
make install
```

## Playerbot Factory & Debug Notes

### Factory cache note — fixed 2026-06-25
- `AccountMgr::GetId()` uses an internal `m_accountNameToId` map that caches names on first query
- After `CleanupOldBots()` deletes accounts, the cache still has old entries and `GetId()` returns 0 for newly created accounts
- **Fix:** Call `sAccountMgr.LoadAccountNames()` after cleanup to refresh the cache
- All 10 bots create successfully (botacc0-9, rank=0 non-GM)

### Debug scanning architecture — added 2026-06-25
- `ServerFacade::DebugNearbyCreatures(Player* bot)` — cell-based scanner that logs all nearby creatures with rejection reasons (dist, friendly, dead, self) and PASS count
- Uses file-scope `NearbyCreatureDebugger` struct with `Visit()` container iteration methods (NOT operator())
- Called from `AttackAnythingAction::isUseful()` every 30s when no target found (throttled via per-bot `std::map`)
- `DpsAssistAction` path4b: also tries `SelectNearestHostileTarget` when path4 returns nil and logs result
- Requires `#include "Maps/CellImpl.h"` for inline `Cell::Cell()` constructor template

### Debug findings — overnight run 2026-06-24/25
- Bots wandered correctly (no spawn limit) but spent ~95% of ticks in creature-sparse areas
- Debug scan logs show many nearby units rejected as "dead" (corpses), "distance" (>50yds), or "friendly" (other bots)
- `SelectNearestUnfriendlyTarget()` DOES find neutral creatures (Rabbit entry 721, Timber Wolf entry 69, Young Wolf entry 299)
- Encounter rate is the problem: bots in towns/roads/fields with few mobs nearby
- Short test (2 min post-restart): bots found and attacked creatures immediately — system works when creatures are nearby

### Target lifecycle fix (2026-06-25, commit 1f41154)
- **Problem:** After first attack, "current target" held dead unit pointer. `NoTargetTrigger` (checks `!target`) never fired again because dead pointer is non-null. `InvalidTargetTrigger` only existed on COMBAT engine, but bots may not enter COMBAT state if creatures don't aggro back (GM issue)
- **Fix:** GrindingStrategy (NON_COMBAT) now has `"invalid target" → { NextAction("drop target", 99.0f) }` matching CombatStrategy
- **InvalidTargetTrigger** rewritten to check "current target" context value (AC `AI_VALUE2(bool, "invalid target", "current target")` pattern) instead of `bot->GetVictim()`/`GetSelectionGuid()`
- **Grind cycle verified:** attack → target dies → invalid target fires → drop target clears value → no target fires → attack anything finds new target → repeat

### Grind cycle infinite loop fix (2026-06-25, commit 299bc06)
- **Problem 1:** `InvalidTargetTrigger::IsActive()` returned `true` when target was null (after drop target). This pushed "drop target" (priority 99) every tick, always outranking "attack anything" (priority 4). Bots stuck in infinite "drop target" loop, never finding new targets
- **Fix 1:** `InvalidTargetTrigger::IsActive()` now returns `false` when target is null — only fires when there IS a stale/invalid target. Null target handled by "no target" trigger
- **Problem 2:** `AttackAnythingAction::isUseful()` returned `false` when `bot->IsInCombat()`. After dropping target, server combat flag persists for a tick, blocking re-target
- **Fix 2:** Removed `IsInCombat()` check from `AttackAnythingAction::isUseful()` — engine state controls whether grinding should be active
- **Problem 3:** CombatStrategy had no "no target" trigger, so COMBAT engine couldn't find new targets
- **Fix 3:** Added `"no target" → { NextAction("attack anything", ACTION_IDLE) }` to CombatStrategy::InitTriggers()
- **Problem 4:** Distance/positioning triggers used `bot->GetVictim()` instead of "current target" context value
- **Fix 4:** EnemyOutOfMeleeTrigger, EnemyOutOfSpellTrigger, EnemyTooCloseForSpellTrigger, NotFacingTargetTrigger, NotBehindTargetTrigger all now use `GetAiObjectContext()->GetValue<Unit*>("current target")`
- **Additional:** Added "old target" context value, LootObjectStack wiring in AttackAction/DropTargetAction, ChangeEngine() public API
- **Grind cycle fully verified:** all 10 bots actively grinding with correct COMBAT ↔ NON_COMBAT engine switching

### Thread yielding (added 2026-06-25)
- **AC pattern:** `PlayerbotAIBase::YieldThread()` sets `nextAICheckDelay` with per-bot random offset (0-200ms) to stagger AI ticks and prevent CPU spikes
- **Implementation:** Added `nextAICheckDelay`, `CanUpdateAI()`, `SetNextCheckDelay()`, `IncreaseNextCheckDelay()`, `YieldThread()`, `IsActive()` to `PlayerbotAIBase`
- `UpdateAI()` decrements delay each tick, skips processing if delay > 0, calls `YieldThread(reactDelay)` after processing
- Per-bot offset: `bot->GetGUIDLow() % 201` — deterministic but spreads bots across 201ms window
- Config: `reactDelay` (default 100ms), `maxWaitForMove` (default 5000ms), `globalCoolDown` (default 500ms)

### Tap-based target filtering (added 2026-06-25)
- **Problem:** Multiple bots simultaneously attack same creature. WoW tap mechanic: first player to deal damage gets loot rights via `SetLootRecipient()`. Subsequent damage doesn't change tap. Only tapper (or their group) gets XP/loot.
- **AC approach:** `GetTargetingPlayerCount()` counts group members already targeting a creature. Only works when bots are in a group.
- **Our approach (no groups):** Use server's authoritative tap tracking via `Creature::HasLootRecipient()` and `Creature::IsTappedBy(player)`
- **`SelectNearestSafeTarget()`:** Scans nearby unfriendly units, skips creatures where `HasLootRecipient() && !IsTappedBy(bot)` — i.e., tapped by outsider
- **`InvalidTargetTrigger`:** Also checks tap — if current target is tapped by someone else, fires "drop target" so bot finds a new target
- **`GetTargetingPlayerCount()`:** Ported from AC `GrindTargetValue` — counts group members with matching "current target". Returns 0 if no group (fallback to tap check).
- **Key insight:** Server handles tap on first damage. Bots just need to check `HasLootRecipient()`/`IsTappedBy()` and drop untapped targets.

### Loot System Investigation (2026-06-25)
- **AC approach:** `WorldPacketHandlerStrategy` with `SMSG_LOOT_RESPONSE` packet trigger. `StoreLootAction::Execute()` reads loot data from packet event parameter — fires ONCE per loot response
- **Our issue:** `LootOpenTrigger` checks `bot->GetLootGuid()` every tick, fires repeatedly. `StoreLootAction` runs every tick while GUID is set
- **AC dedup:** `Engine::ProcessTriggers()` uses `std::unordered_map<Trigger*, Event>` — deduplicates by pointer. Two `TriggerNode`s with same name get same `Trigger*`, only first action pushes
- **AC config:** `lootDelay` (default 500ms) — `SetNextCheckDelay()` after each loot op to stagger async processing
- **AC flow:** `LootAction` → `LootObjectStack::GetLoot(distance)` → `OpenLootAction` → `DoLoot()` → remove from stack → clear value → `StoreLootAction` (packet-triggered)

### Food/Drink System (Phase 3C, COMPLETE)
- **SelfTargetValue:** returns bot unit (AC SelfTargetValue pattern)
- **StatsValues:** HealthValue (%), ManaValue (%), HasManaValue (bool), IsDeadValue (bool)
- **EatAction:** finds food item (spellcategory_1 == 11), sits, CMSG_USE_ITEM, delay = max(10s, 27s * (100-hp)/100)
- **DrinkAction:** finds drink item (spellcategory_1 == 59), sits, CMSG_USE_ITEM, delay = max(10s, 27s * (100-mp)/100)
- **UseFoodStrategy:** `"low health" → "food" (3.0f)`, `"low mana" → "drink" (3.0f)`
- **GrindingStrategy defaults:** `"drink" (4.2f)`, `"food" (4.1f)` (AC pattern)
- **GrindingStrategy triggers:** `"low health" → "food"`, `"low mana" → "drink"`
- **HealthTriggers:** `LowHealthTrigger` (≤30%), `MediumHealthTrigger` (≤60%), `LowManaTrigger` (≤10%), `HighManaTrigger` (≥60%)
- **GiveFoodDrink:** 5x Westfall Stew (entry 733) + 5x Refreshing Spring Water (entry 159) on login
- **Vanilla note:** item_template has subclass=0 for all consumables, use spellcategory_1 instead
- **Verified:** Bots get food/drink on login, actions fire when health/mana drops below thresholds

### AttackAction Missing Features
- **Set facing:** `ServerFacade::SetFacingTo(bot, target)` if bot can move and not facing target
- **Clear movement:** Clears `LastMovement` and stops moving if combat priority is higher
- **WaitForAttack:** Checks `WaitForAttackStrategy::ShouldWait()` before attacking
- **ChangeEngine:** AC explicitly calls `botAI->ChangeEngine(BOT_STATE_COMBAT)` (we use auto-switch via UpdateAI)

### AC Grind Pattern Port (2026-06-25, commits 0c4f1b0, 907ee52)
- **AC pattern:** `AttackAnythingAction` reads `"grind target"` value (via `GetTargetName()` override), `DpsAssistAction` reads `"dps target"`
- **`GrindTargetValue`:** New calculated value, calls `SelectNearestSafeTarget()` to find nearest valid grind target
- **`AttackAction::GetTarget()` BUG:** Was hardcoded to read `"current target"` instead of using `GetTargetName()`. Fixed to use `GetTargetName()` so subclasses' overrides work
- **`GrindingStrategy` matches AC:** Only `"no target" → "attack anything"` (finds target), `"invalid target" → "drop target"` (clears stale). Server auto-attack handles repeated swings
- **`DpsAssistStrategy` on COMBAT engine:** `"not dps target active" → "dps assist"` (continuous attack via `"dps target"` value)
- **`DpsAssistAction` simplified:** Inherits `Execute()` from `AttackAction` (reads target via `GetTargetName()`), only overrides `isUseful()` (AC pattern)
- **Grind cycle verified:** NON_COMBAT `"no target"` → `"attack anything"` (reads `"grind target"`) → attacks → enters COMBAT → `"dps assist"` (reads `"dps target"`) → keeps attacking → target dies → `"invalid target"` → `"drop target"` → NON_COMBAT → repeat
- **Loot cycle verified:** attack → kill → `"can loot"` → `"open loot"` → `"store loot"` → release → repeat

## LLM Working Notes

### Build & Deploy Workflow
```bash
# Build (from /root/tortoise-wow/build)
make -j$(nproc)

# Deploy — binary MUST be installed after every rebuild
make install

# Restart
/root/wow-server.sh restart

# Quick build+deploy
./wow-server.sh build

# Full clean rebuild (like AC's comp_all)
./wow-server.sh build all

# Debug crash capture
./wow-server.sh startgdb
```
- Build outputs to `build/src/mangosd/mangosd`, server runs from `server/bin/mangosd`
- Use `make install` (not `cp`) to deploy — copies binary to `server/bin/mangosd`
- `wow-server.sh` is the only way to stop/start/restart the server (never use `pkill`/`killall`)
- Build type: `RelWithDebInfo` for debug symbols in crash backtraces
- GDB crash capture: `wow-server.sh startgdb` → logs to `/root/tortoise-wow/server/logs/gdb.txt`
- ccache is auto-detected and enabled (sets `CMAKE_C_COMPILER_LAUNCHER`/`CMAKE_CXX_COMPILER_LAUNCHER`)
- **Orphan prevention:** `build()` and `clean` auto-remove orphaned `st*` linker temp files (GNU `ar` crash artifacts matching `^st[A-Za-z0-9]{6}$`) before compiling

### Logging
- `sLog.outInfo("playerbots: msg")` — goes to **`server/logs/info.log`** (NOT server.log — with `LogFileLevel = 2` the detail level routes to info.log). Playerbots relocations/stats land there.
- `LOG_DEBUG("playerbots", ...)` — maps to `sLog.outDebug` → console (tmux pane, gated by `LogLevel=3`) AND `server/logs/server.log` (gated by `LogFileLevel`). **Current: `LogFileLevel = 2`** → debug lines do NOT go to the file (they filled 927MB in ~15 min at 300 bots); console keeps them WITH timestamps (`LogTime = 1`). For a deep-debug session: set `LogFileLevel = 3` + restart, then flip back.
- `LOG_DEBUG` uses `%s`/`%p` format (printf-style), NOT `{}` format specifiers
- **`server/logs/loot.log` is the authoritative loot ledger** — every item loot (`loots 1x<entry>`) and money credit (`gets 0g0s10c [loot from ...]`) via `sLog.out(LOG_LOOTS, ...)`. ⚠ date-filter carefully: `awk '$1=="2026-09-17" && $2>="12:50"'` — a bare `$2>="12:50"` matches old lines from ANY day.
- Check logs: `tmux capture-pane -t mangosd -p` (timestamps on) or `grep playerbots server/logs/info.log`

### Log rotation & NAS archival
- `/root/wow-log-rotate.sh` — hourly via root crontab (`17 * * * *`), status in `/var/log/wow-log-rotate.status`
- Copytruncate-style: any live log > **400MB** (real size via `du`, not apparent) → gzipped to `server/logs/archive/`, then truncated in place
- Daily: archives older than **1 day** are moved to the NAS at **`/root/nas/temp/tortoise-backups`** (CIFS //192.168.1.4/smb); if NAS unreachable, local >14d archives are pruned instead; 4-5GB local safety net
- ⚠ **Known quirk:** mangosd's `FILE*` keeps its offset across an external truncate → the live log becomes SPARSE (giant null hole, apparent size ≫ real). After any manual truncate, restart mangosd soon to reset the offset (fresh open at 0). Symptom: `head` shows nulls / `stat -c%s` huge while `du` is small.

### AC Reference Source
- AzerothCore mod-playerbots lives at `/root/azerothcore-wotlk/modules/mod-playerbots/`
- Always check AC source when debugging engine behavior — it's the authoritative reference
- AC `Engine` receives `AiObjectContext*` in constructor, stores as own member (NO circular delegation)

### 3-Engine Architecture (matching AC)
- AC uses 3 engines: `BOT_STATE_COMBAT`, `BOT_STATE_NON_COMBAT`, `BOT_STATE_DEAD`
- `PlayerbotAIBase::engines[BOT_STATE_MAX]` — array of 3 Engine pointers
- `PlayerbotAIBase::currentState` — which engine is active
- `ChangeEngine(state)` — disables old engine, switches to new, enables new
- Engine switching in `UpdateAI()`: bot died → DEAD, in combat → COMBAT, not in combat → NON_COMBAT
- **Non-combat engine**: NonCombatStrategy, WanderStrategy, GrindingStrategy, LootNonCombatStrategy
- **Combat engine**: MeleeCombatStrategy or RangedCombatStrategy (class-dependent)
- **Dead engine**: minimal for now (just initialized, no strategies yet)
- Each engine has its own `triggers` vector, `strategies` map, and `AiObjectContext`
- Only the current engine's `Update()` is called each tick
- **Implementation complete** — matches AC pattern exactly
- `PlayerbotAIBase::Initialize()` creates 3 engines: `CreateEngine(BOT_STATE_COMBAT)`, `CreateEngine(BOT_STATE_NON_COMBAT)`, `CreateEngine(BOT_STATE_DEAD)`
- `PlayerbotAIBase::UpdateAI()` calls `ChangeEngine()` based on `bot->IsInCombat()` and `bot->IsAlive()`
- `ChangeEngine()` calls `DisableAll()`, switches `currentState`, calls `EnableAll()` for new engine
- **CRITICAL:** `BuildSharedBaseAiObjectContext(botAI, context)` takes the context as a parameter — each engine's `Init()` passes `this` so values go to the correct engine's context
- **FIXED:** All 3 engines share ONE `AiObjectContext` (matches AC) — `AiFactory::createAiObjectContext` passed to all engine creators

### NamedObjectContextList Pattern (Phase 2)
- AC uses `NamedObjectContextList<T>` for actions, triggers, strategies, values
- `SharedNamedObjectContextList<T>` is static, built once at startup via `BuildAllSharedContexts()`
- `NamedObjectContextList<T>` is per-instance, references the shared list, caches created objects
- `ActionContext` / `TriggerContext` classes inherit `NamedObjectContext<T>`, register name→creator in constructor
- `GetAction(name)` → `actionContexts.GetContextObject(name, botAI)` — lazy instantiation + caching
- `GetTrigger(name)` → `triggerContexts.GetContextObject(name, botAI)` — same pattern
- Triggers lazily set on TriggerNode in `Engine::ProcessTriggers()` when null (AC `Engine.cpp:447-451`)
- The `NamedObjectContext.h` framework is **already fully ported** to tortoise

### Tortoise vs AC API Differences
| AC | Tortoise |
|----|----------|
| `Event const&` | `Event` (by value) |
| `FORCED_MOVEMENT_NONE` | `MOVE_NONE` |
| `UNIT_STATE_IN_FLIGHT` | `UNIT_STAT_TAXI_FLIGHT` |
| `IsSitState()` | `GetStandState()` |
| `IsNonMeleeSpellCast()` | `IsNonMeleeSpellCasted()` |
| `GetSource()` | `getSource()` |
| `sConfigMgr->GetOption<T>()` | `sConfig.GetBoolDefault()` / `GetIntDefault()` |
| `LOG_ERROR("cat", ...)` | `sLog.outError("cat: ...")` |
| `ObjectGuid::GetCounter()` | `ObjectGuid::GetGUIDLow()` |
| `player->getClass()` | `player->GetClass()` |
| `ItemTemplate const*` | `ItemPrototype const*` |
| `Item::GetItemTemplate()` | `Item::GetProto()` |
| `player->GetLootGuid()` | Same method |

### Pitfalls
- `Engine::GetContext()` MUST NOT delegate to `botAI->GetAiObjectContext()` (infinite recursion)
- `Engine` owns `AiObjectContext* context` as its own member
- Static initialization order: `RegisterBaseActions()` uses a static guard, called from `AiObjectContext::Init()`
- `Player::Update()` calls `i_AI->UpdateAI(p_time)` for bots — engine runs every ~300ms tick
- `Engine::ProcessTriggers()` skips triggers when `node->getTrigger()` is null — must lazily instantiate via `context->GetTrigger(name)` (AC Engine.cpp:447-451)
- Strategies MUST override `InitTriggers()` or no TriggerNodes are created (base class is empty no-op)
- CombatStrategy default actions should use "dps assist" not "attack" — "attack" reads null "current target" value
- `Unit::GetVictim()` = who is attacking THIS unit (incoming attacker); `Player::GetSelectionGuid()` = what this player is currently targeting
- `IsValidAttackTarget()` blocks neutral creatures unless faction is "at war"; `!IsFriendlyTo()` allows neutral + hostile
- `Player::SaveToDB()` returns false for bot sessions (`GetSession()->GetBot()` check) — factory must use non-bot WorldSession
- `ACTION_IDLE` must be > 0.0f or `relevance > 0` check in Engine fails → "IMPOSSIBLE" log
- `BuildSharedBaseAiObjectContext(botAI, context)` must take the context as a parameter — each engine passes `this` so values go to the correct engine's context (see 3-Engine Context Bug)
- **AC shares ONE `AiObjectContext` across all 3 engines** — `AiFactory::createAiObjectContext` passed to `createCombatEngine`, `createNonCombatEngine`, `createDeadEngine` (see 3-Engine Shared Context Bug)
- **`InvalidTargetTrigger` must return `false` when target is null** — null target is handled by "no target" trigger. Returning true causes infinite "drop target" spam loop
- **`AttackAnythingAction::isUseful()` must NOT check `IsInCombat()`** — server combat flag persists after dropping target, blocking re-target. Engine state controls grinding mode
- **CombatStrategy needs "no target" → "attack anything"** — without it, COMBAT engine has no way to find new targets after dropping
- **All triggers must use "current target" context value** — never `bot->GetVictim()` (that's who's attacking the bot, not the bot's target)
- **`AttackAction::GetTarget()` must use `GetTargetName()`** — hardcoding `"current target"` breaks subclasses like `AttackAnythingAction` (reads `"grind target"`) and `DpsAssistAction` (reads `"dps target"`)
- **`GrindTargetValue` registered in `BuildSharedBaseAiObjectContext`** — must be added to context before engines are created
- **Build cache corruption:** Interrupted builds can leave 0-byte `.o` files. Fix: delete corrupted `.o` files and rebuild. Don't modify core ScriptLoader.cpp as workaround.
- **`StoreLootAction` must respect `lootslot_type` per item** (AC packet pattern) — iterating `creature->loot.items` directly bypasses server's per-item permissions. Must compute `PermissionTypes` from group context, then `LootSlotType` matching `LootView::operator<<` logic. Only take `ALLOW_LOOT`/`OWNER`, skip `ROLL_ONGOING`/`MASTER`/`LOCKED`. `GetSlotTypeForSharedLoot` has buggy `is_blocked || is_underthreshold` → use correct if/else priority instead.
- **`LootView::operator<<` is authoritative for `lootslot_type`** — `GetSlotTypeForSharedLoot` has `||` bug (`is_blocked` items get `ALLOW_LOOT` instead of `ROLL_ONGOING`). `LootView::operator<<` uses correct if/else chain.
- **R3a item-scoring pitfalls (this core):** spells come from SQL `tw_world.spell_template` (NOT DBC; the `spell` table is an empty shell) — positional `SELECT *` contract, lowercase cols; effect value = `basePoints + dieSides` (`CalculateSimpleValue`); `ItemSpelltriggerType` = ON_USE 0 / ON_EQUIP 1 / ON_HIT 2 (WotLK-style; engine passive = trigger 1 only); `ITEM_CLASS_ARMOR=4` (custom enum); armor `SubClass` = armor type 1-4 (not slot); auras 46/48/50 are `HandleUnused` (only 47/49/51 percent variants live); generic auras 52/54 hit BOTH melee and ranged; `Name1` is `std::string` → `.c_str()` in printf logs; effect-35 item spells are dead (6 items incl. 10030 hat). See "R3a P1 Item-Scoring Lessons" above.
- **`tw` CLI: DBC store catalog is incomplete** — `tw dbc Spell` fails ("unknown store"); parser covers ~54 stores (`DBCfmt.h` = `const char Xxxfmt[]` arrays). For spells use SQL `spell_template` until the tw parser is fixed.

## Useful Database Queries
```sql
-- List accounts
SELECT id, username, gmlevel FROM tw_logon.account;

-- Online players
SELECT COUNT(*) FROM tw_char.characters WHERE online > 0;

-- Realm info
SELECT * FROM tw_logon.realmlist;

-- Create admin account
CREATE ACCOUNT 'username' 'password' 3;
```
