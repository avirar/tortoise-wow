<!-- Canonical copy: /root/playerbot-review.md — edit there, then re-copy -->

# Playerbot Engine Port — Critical Code Review

**Date:** 2026-06-26
**Scope:** `/root/tortoise-wow/src/game/PlayerBots/` (71 .cpp / 80 .h files) vs AC reference `/root/azerothcore-wotlk/modules/mod-playerbots/`
**Reviewer stance:** extremely critical. Verified against actual source, not against the plan's claims.

## Resolution status (updated 2026-09-16, post R1 base-sync)

| Item | Status |
|---|---|
| P0-1 (8/9 classes never cast) | **FIXED in code** (verified 2026-09-16) — `cast spell` registered `ActionContext.cpp:31`, default action `CombatStrategy.cpp:15` (commit `435d129`) |
| P0-2 (ranged melee-forced) | **ALREADY FIXED in code** (2026-09-16 verified) — `shouldMelee = CanReachWithMeleeAutoAttack \|\| melee-class check` → `Attack(target, shouldMelee)`; ranged classes auto-attack ranged |
| P0-3 (stats engine dead) | **FIXED in code** (verified 2026-09-16) — `ItemUsageValue.cpp:71` uses `StatsWeightCalculator` (commit `e7603c0`) |
| P1-1 (warrior `GetSpecDefaultActions` dead) | **FIXED in code** — consumed by `GenericWarriorStrategy::getDefaultActions` (`435d129`) |
| P1-2 (death system gaps) | **FIXED in code** (`435d129`) — free-repair removed; `dead → spirit healer` wired in DeadStrategy |
| P1-3 (warrior spec hardcoded Arms) | **FIXED 2026-09-16** — `Util/SpecDetect.h` talent-tab detection; Arms/Fury/Tank selected by dominant tree at login (StatsWeightCalculator now delegates to the shared impl). Note: 6 Fury/Tank trigger names have no implementation (`bloodthirst ready`, `demoralizing shout needed`, `revenge ready`, `shield slam ready`, `taunt needed`, `whirlwind ready`) — harmless dead branches (Engine null-skips unknown triggers, `Engine.cpp:232`); implement in R2 |
| P2-1 (DoNextAction cap) | **FIXED in code** (`435d129`) — AC refresh-per-iteration pattern |
| P2-2 (double SetTargetGuid) | **ALREADY FIXED in code** (only `SetSelectionGuid` remains; comment notes redundancy) |
| P2-3 (is_looted mutation) | **FIXED 2026-09-16** — removed; opcode handler marks looted server-side (LootHandler.cpp:225-249) |
| P2-4 (one-shot dedup) | accepted tech debt until packet hook exists |
| P2-5 (GetName %s) | **NON-ISSUE** — tortoise `Player::GetName()` returns `char const*` (Player.h:2758), no UB |
| P2-6 (FFA skip) | **FIXED 2026-09-16** — review's "inverted" read was wrong: AC also skips FFA by default, gated by `freeMethodLoot` config; ported AC semantics (`PlayerBot.FreeMethodLoot=1` to opt in; IsSelfBot branch N/A — factory bots only) |
| P3-4 (static map leak) | **FIXED 2026-09-16** — prune entries older than 10 min when map > 200 |
| P3-5 (stale-target race) | **VERIFIED SAFE** — cleanup runs before engine actions in the same tick; dangling pointers already filtered by CurrentTargetValue |
| P3 ("release" action unwired) | open — registered but unused ("auto release" + "spirit healer" are the wired ones); low risk, revisit in R2 |

## TL;DR

The **decision-engine plumbing** (Engine, Queue, ActionBasket, 3-engine shared context, NamedObjectContext factories, trigger/action lazy instantiation) is a **faithful and correct** port of AC and genuinely works. The **login / persistence / factory / scale** layer is solid (100 bots stable).

However, the **actual gameplay behavior** layered on top has **two critical gaps** that make the plan's "phases complete" markings misleading:

1. **Combat abilities are dead for 8 of 9 classes.** The `CastSpellAction` ↔ `SelectOffensiveSpell` "bridge" that the plan's central design decision rests on is **never registered and never triggered** — it is unreachable code. Only Warriors cast abilities (via a separate, half-built class-strategy system). Mages/Priests/Warlocks/Hunters/Shaman/Rogues/Paladins/Druids **only melee-auto-attack**, and a `|| true` bug forces even ranged classes into melee auto-attack.
2. **The Phase 3B "AC-style stat-scoring engine" is 100% dead code.** `StatsWeightCalculator` and `StatsCollector` are referenced only by themselves and `CMakeLists.txt`. `ItemUsageValue::Calculate()` is a stub returning `ITEM_USAGE_NONE`. Equip decisions fall back to a crude `ItemLevel * (Quality+1)` formula. The phase is marked "Complete" but its deliverable does not run.

Everything below is verified with `file:line` references and the AC counterpart.

---

## Severity Legend
- **P0 CRITICAL** — core advertised feature is non-functional at runtime
- **P1 HIGH** — correctness/integrity bug or large dead/incomplete subsystem
- **P2 MEDIUM** — real bug, low blast radius or easy fix
- **P3 LOW** — style/redundancy/cleanup

---

## P0 — Critical (advertised features that do not actually run)

### P0-1. `CastSpellAction` / `SelectOffensiveSpell` is dead code → 8/9 classes never cast

**Claim (plan "Key Decision #1 — Hybrid Combat"):** *"Spell selection uses tortoise's `SelectOffensiveSpell()` — no need to port 100 AC class strategy files … `CastSpellAction` is the bridge."*

**Reality:**
- `CastSpellAction` is defined in `Ai/Base/Actions/CastSpellAction.cpp:12` and calls `botAI->SelectOffensiveSpell(target)` (line 25). `SelectOffensiveSpell` (`PlayerBotAI.cpp:195`) is a genuinely sophisticated per-class spell-chain resolver (all 9 classes, `GetHighestKnownSpell`, aura-refresh checks).
- **It is never invoked.** Grep for `"cast spell"` across `Ai/` returns only the file itself. `ActionContext.cpp` does **not** register `"cast spell"`. **No strategy** pushes `"cast spell"` as a default action or trigger handler.
- `CombatStrategy::getDefaultActions()` returns `{}` (`CombatStrategy.cpp:14`). The only combat action that ever fires is `"dps assist"` (`DpsAssistStrategy.cpp:20`), which calls `AttackAction::DoAttack` → `bot->Attack(target, true)`.

**Consequence:** Mages never Frostbolt/Fireball, Priests never Smite/SWP, Warlocks never Shadow Bolt, Hunters never Auto Shot/Arcane Shot, etc. **All non-Warrior classes are pure melee auto-attack bots.** The plan/AGENTS.md claims of "200K dps assist OK ticks" and "grinding actively" are real, but they measure *auto-attack only* — there is zero ability usage for 8 classes.

**Fix options:**
- (A) Wire the bridge: register `"cast spell"` in `ActionContext.cpp`, add `MeleeCombatStrategy`/`RangedCombatStrategy` default action `"cast spell"` (low priority, so it fires after positioning), and have `CastSpellAction` fall back to melee attack (it already does, line 38). Cheapest path to "all classes cast".
- (B) Drop `CastSpellAction`/`SelectOffensiveSpell` entirely and finish porting AC per-class strategies (the Warrior pattern already exists — replicate for the other 8). More work, better behavior.

Either way, **the plan must stop claiming SelectOffensiveSpell "covers all 9 classes"** — it is unreachable.

### P0-2. AttackAction forces melee auto-attack for ranged classes (`|| true`)

`Ai/Base/Actions/AttackAction.cpp:87`:
```cpp
if (!WaitForAttackStrategy::ShouldWait(botAI))
    bot->Attack(target, bot->CanReachWithMeleeAutoAttack(target) || true);
```
The `|| true` makes the `shouldMelee` argument **always true**. AC's equivalent (`Ai/Base/Actions/AttackAction.cpp:142,196`):
```cpp
bool shouldMelee = bot->IsWithinMeleeRange(target) || botAI->IsMelee(bot);
...
bot->Attack(target, shouldMelee);
```
For a Mage at 25y, AC passes `shouldMelee=false` (no melee auto-attack; relies on spells). Tortoise passes `true`, so the Mage starts swinging its staff from range and, because of P0-1, never casts. Combined with P0-1, ranged classes are doubly broken.

**Fix:** `bot->Attack(target, bot->CanReachWithMeleeAutoAttack(target) || botAI->IsMelee());` (add `IsMelee()` helper or a class check; or just `bot->CanReachWithMeleeAutoAttack(target)` if P0-1 is fixed so spells drive ranged DPS).

### P0-3. StatsWeightCalculator + StatsCollector are 100% dead code (Phase 3B deliverable does not run)

**Claim (plan Phase 3B):** *"Full looting and equipping pipeline with AC's stat-weight scoring engine … StatsWeightCalculator scores item vs current gear."* Marked `[x] Compiled`.

**Reality (verified):**
- `grep -rln StatsWeightCalculator src/` → only `StatsWeightCalculator.{h,cpp}` and `CMakeLists.txt`. **Zero instantiation sites.** Same for `StatsCollector`.
- `ItemUsageValue::Calculate()` (`Ai/Base/Value/ItemUsageValue.cpp:11`) is a stub: `return ITEM_USAGE_NONE;`
- `ItemUsageValue::QueryItemUsageForEquip()` (line 51, 65, 67) scores with `proto->ItemLevel * (proto->Quality + 1)` — **not** StatsWeightCalculator.
- `ItemUpgradeValue::Calculate()` (line 77) returns `ITEM_USAGE_NONE` (stub).
- `EquipUpgradesAction::Execute()` (`LootAction.cpp:335,376`) also uses the crude `ItemLevel*(Quality+1)` formula, never StatsWeightCalculator.

**Why loot still appears to "work":** `NormalLootStrategy::CanLoot` (`LootStrategyValue.h:22`) only rejects `ITEM_QUALITY_POOR`; `LootStrategyValue` defaults to `AllLootStrategy` (loots everything). Neither consults `ItemUsageValue`. So bots **grab everything** and **equip by ilvl×quality** — there is no AC-style stat-weight evaluation at all. The ~1,000-line scoring engine compiles but is inert.

**Fix:** Either wire `StatsWeightCalculator` into `ItemUsageValue::Calculate()` + `EquipUpgradesAction` (the intended design), or delete the dead files and stop describing Phase 3B as "AC-style stat scoring". Do not leave it both claimed-complete and non-functional.

---

## P1 — High

### P1-1. Warrior `GetSpecDefaultActions()` is dead code
`ArmsWarriorStrategy::GetSpecDefaultActions()` (`ArmsWarriorStrategy.cpp:51`), `FuryWarriorStrategy` and `TankWarriorStrategy` define priority lists (`{"execute",10.0f}, {"mortal strike",6.0f}, …`). **Grep shows zero call sites.** The `Engine` only ever calls `Strategy::getDefaultActions()` (`Engine.cpp:267`), which `GenericWarriorStrategy` inherits as the empty `CombatStrategy::getDefaultActions()`. So the entire spec-priority machinery is unused; warrior ability *timing* comes only from the `InitTriggers` registrations. Either wire `GetSpecDefaultActions()` into `getDefaultActions()` or delete it.

### P1-2. Death system: free repair exploit + incomplete resurrection loop

`Ai/Base/Actions/ReleaseSpiritActions.cpp:25` and `:38` — both `ReleaseSpiritAction` and `AutoReleaseSpiritAction` call:
```cpp
bot->DurabilityRepairAll(false, 1.0f);
```
**before** releasing. This **freely repairs all gear on death** with no cost — a clear exploit absent from AC's death actions. Dying is supposed to cost 10% durability; this reverses it. Remove these calls (or gate behind a debug config).

**Incomplete res paths:** `SpiritHealerAction` ("spirit healer") and `ReleaseSpiritAction` ("release") are registered in `ActionContext.cpp:47,41` but **never triggered by any strategy**. `DeadStrategy.cpp` only wires: `can self resurrect`, `often→auto release`, `dead→find corpse`, `corpse near→revive from corpse`, `resurrect request→accept`, `falling far→repop`. If a bot's corpse is unreachable (under terrain, deep in mobs) it auto-releases, walks toward corpse, never reaches it, and **there is no "use spirit healer" trigger to resurrect at the GY**. Bots can strand as ghosts indefinitely. The phase is honestly marked "awaiting test verification" in AGENTS.md — this review confirms it is not complete.

`RepopAction::isUseful()` (`ReleaseSpiritActions.cpp:88`) = `bot->IsDead() && !bot->IsAlive()` — tautological (`IsDead()` ≡ `!IsAlive()`). It does not actually express the intended "corpse is falling/stuck" condition.

### P1-3. `PlayerbotAIBase::Initialize` hardcodes Warriors to Arms spec
`Bot/PlayerbotAIBase.cpp:78-89`: warriors get `ArmsWarriorStrategy` unconditionally with a comment "use Arms as default. Spec detection via talents will be added." Fury/Tank strategies exist but are never selected. Non-warriors get no class strategy at all (see P0-1). This is a known gap but should be tracked, not silently shipped.

---

## P2 — Medium

### P2-1. `Engine::DoNextAction` iteration cap is computed once
`Engine/Engine.cpp:108-111`: `iterationsPerTick = queue.Size() * 2` is captured **before** the loop, but the loop pushes prerequisites (`PushAgain`, `MultiplyAndPush`) that grow the queue. The cap does not refresh, so a prerequisite chain can be truncated mid-resolution, and conversely the cap has no relationship to current queue depth. Compare to AC's loop bound semantics. Low impact today (few actions), but a latent correctness issue.

### P2-2. `AttackAction` calls both `SetTargetGuid` and `SetSelectionGuid`
`AttackAction.cpp:44` `bot->SetTargetGuid(target->GetGUID())` then `:82` `bot->SetSelectionGuid(target->GetGUID())`. In tortoise, `Player::SetSelectionGuid` (`Objects/Player.h:2487`) **already calls `SetTargetGuid`** internally. So line 44 is redundant and sets the low-level target field without setting `m_curSelectionGuid`, leaving state briefly inconsistent. AC calls `SetSelection` once. Drop line 44 (keep `:82`). Same redundancy in `DropTargetAction` is fine (it only clears selection).

### P2-3. `StoreLootAction` mutates server loot state directly
`LootAction.cpp:305` sets `iter->is_looted = true` after `HandleAutostoreLootItemOpcode`. The opcode handler already marks the item looted server-side, so this is redundant — but it also bypasses the "server is authoritative" pattern the rest of the file carefully follows (it went to lengths to replicate `LootView::operator<<` to respect server permissions, then turns around and writes server state directly). Remove the direct mutation; trust the opcode result.

### P2-4. `StoreLootAction` one-shot dedup is fragile
`LootAction.cpp:117-124` uses a per-instance `lastStoredGuid` to approximate AC's one-shot `SMSG_LOOT_RESPONSE` packet trigger. Works because the action is cached, but it is a hack: if `Execute` returns false on the same guid it clears `bot->SetLootGuid()` and abandons remaining items. Acceptable interim; the comment is honest about it. Track as tech-debt until a packet hook exists.

### P2-5. `LOG_DEBUG` passing `std::string` to `%s`
Throughout (e.g. `Engine.cpp:118`, `PlayerbotAIBase.cpp:165`): `LOG_DEBUG("playerbots", "%s ...", bot->GetName(), …)`. If `Player::GetName()` returns `std::string const&` (mangos convention), passing it to a printf-style `%s` is technically UB (works on glibc because of SSO/small-string layout, but not guaranteed). Verify `GetName()`'s return type; if `std::string`, use `.c_str()` everywhere for safety.

### P2-6. `LootAction::isUseful` skips FFA
`LootAction.cpp:39`: returns false when group loot method is `FREE_FOR_ALL`. FFA means *everyone* can loot independently — skipping it here is the opposite of intended. Verify against AC intent; likely inverted.

---

## P3 — Low / cleanup

- **Dead `lastRelevance`/`lastAction` machinery** in `Engine` is only used for verbose logging; fine but heavy for a hot path.
- **`ReleaseSpiritAction` ("release") and `SpiritHealerAction` ("spirit healer")** registered in `ActionContext` but never triggered (see P1-2). Either wire or remove to avoid implying capability that isn't there.
- **Plan inconsistency:** "Key Decision #1" says *skip class strategies, use SelectOffensiveSpell*, yet a full Warrior class-strategy tree (`Ai/Class/Warrior/`, 6 files) was built. Two parallel, inconsistent spell systems now coexist. Pick one.
- **`PlayerbotAIBase::UpdateAI` strategy-report `static std::map`** (`PlayerbotAIBase.cpp:200`) keyed by bot GUID grows unboundedly across bot logouts (never pruned). Minor leak.
- **`stale-target` clear** (`PlayerbotAIBase.cpp:187-196`) cites "PlayerbotAI.cpp:1514" but is a local heuristic; verify it does not race with `AttackAction` setting the target the same tick.

---

## What is genuinely good (so it isn't rewritten by accident)

- **Engine core** (`Engine.cpp` `DoNextAction`/`ProcessTriggers`/`PushDefaultActions`, `Queue.cpp` ActionBasket priority queue, lazy trigger instantiation `Engine.cpp:216-220`) is a correct, faithful port of AC `Engine.cpp:447-451`.
- **3-engine shared `AiObjectContext`** — the critical fix (one context, `Engine::Reset()` does **not** wipe it, `PlayerbotAIBase::Initialize` creates one and passes to all three engines) matches AC `AiFactory` exactly. `Engine.cpp:64-93` correctly avoids the context-reset bug.
- **ActionNode ownership** — `ActionNode::~ActionNode(){}` is empty and does not delete its cached `Action*` (owned by context). All `DoNextAction` paths `delete actionNode`. No double-free. ✓
- **`StoreLootAction` lootslot_type logic** (`LootAction.cpp:230-279`) is a careful, correct replication of `LootView::operator<<` (correct if/else priority, not the buggy `||` in `GetSlotTypeForSharedLoot`). This was done right.
- **Login/persistence/factory** — name table, async login queue, character reuse across restarts, session-collision fix (`HandleBotLoginCallback` creates session inside callback), cascade delete. 100 bots stable. Solid engineering.
- **Tap-based target filtering**, **thread yielding with per-bot stagger**, **grind target lifecycle** (`GrindTargetValue`/`AttackAnythingAction::GetTargetName()` override) — all match AC patterns.
- **`SelectOffensiveSpell` implementation itself** (spell chains, aura-refresh, range/power checks) is high quality — it is a tragedy that it is unreachable (P0-1).

---

## Crash Fixes (2026-06-27)

### DropTargetAction Dangling Pointer (FIXED)
- **Crash:** SIGSEGV in `DropTargetAction::Execute()` and `InvalidTargetTrigger::IsActive()` — `Unit*` in "current target" context became dangling when creature despawned. Accessing freed memory on `target->IsDead()` / `target->GetMapId()`.
- **Fix:** `CurrentTargetValue` now stores `ObjectGuid` alongside `Unit*`. `Get()` validates `IsInWorld()` → `IsDeleted()` → GUID match. Returns nullptr for stale pointers. Existing null checks in triggers/actions now sufficient.
- **Files:** `CurrentTargetValue.h`, `CurrentTargetValue.cpp` (playerbot only, no core changes)

### MovementBroadcaster Null During Shutdown (FIXED)
- **Crash:** SIGSEGV in `MovementBroadcaster::IsEnabled(this=0x0)` — `InternalShutdown()` resets broadcaster while map threads still running bot AI. Bot sending speed change packet → null dereference.
- **Fix:** `PlayerbotAIBase::UpdateAI()` checks `sWorld.IsStopped()` and skips all processing during shutdown.
- **Files:** `PlayerbotAIBase.cpp` (playerbot only, no core changes)

### EquipUpgrades Zero Equips (INVESTIGATED)
- **Finding:** EquipUpgradesAction 100% failing — bots in Desolace killing level 5-7 beasts that drop only meat/junk (`inventory_type=0`). Zero equippable gear available. Loot pipeline works (StoreLootAction stores items), but items are non-equipable. Requires bots in areas with gear-dropping creatures.

---

## Recommended priority of fixes

1. **P0-1 + P0-2:** make all 9 classes actually cast (wire `CastSpellAction`, fix `|| true`). This is the single biggest behavior gap and ~30 lines.
2. **P0-3:** decide stat-scoring fate — wire `StatsWeightCalculator` into `ItemUsageValue`+`EquipUpgradesAction`, or delete it and re-scope Phase 3B. Stop marking it complete.
3. **P1-2:** remove `DurabilityRepairAll` exploit; add a real spirit-healer resurrection trigger or acknowledge bots strand as ghosts.
4. **P1-1:** delete or wire `GetSpecDefaultActions()`.
5. **P2 batch:** iteration cap, redundant `SetTargetGuid`, direct loot mutation, FFA inversion.

## Action items for the plan doc (`playerbot-port-plan.md`)
- Phase 3B status is **inaccurate** ("[x] Compiled, wiring pending" reads as nearly done; the wiring is the entire feature). Re-mark as **incomplete / stat engine not wired**.
- "Key Decision #1 (Hybrid Combat)" is **not realized** — the SelectOffensiveSpell bridge is dead. Either implement it or strike the claim.
- Phase 3H Death system is marked "awaiting verification" in AGENTS.md — this review confirms it is **not complete** (no spirit-healer path, repair exploit). Re-scope.
- "Playerbot scale testing (100 bots verified)" is accurate for *login stability*, but **not** for combat depth — bots auto-attack only. Clarify.
