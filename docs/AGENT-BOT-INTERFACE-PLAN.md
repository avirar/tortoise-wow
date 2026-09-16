<!-- Canonical copy: /root/agent-bot-interface-plan.md — edit there, then re-copy -->

# Agent↔Bot Interface — Port of mod-ollama-bot-buddy Logic to Tortoise

**Status:** DRAFT — documentation phase
**Parent:** `bot-master-plan.md` (R6)
**Source:** `/root/azerothcore-wotlk/modules/mod-ollama-bot-buddy/` (user's AC module; LLM-driven bot control)

---

## 1. Goal

Let AI agent sessions (GLM in pi, local Qwen, future tooling) **observe and drive individual bots on the live tortoise server** so agents can autonomously test and debug the bot system as it's built: repro a bug, command a bot through the repro path, snapshot resulting state, iterate. Humans review agent reports and diffs.

## 2. What bot-buddy does (source audit)

| Piece | File(s) | Port? |
|---|---|---|
| **Command API** — `BotControlCommandType` enum (`MoveTo, Attack, Interact, CastSpell, Loot, Follow, Say, AcceptQuest, TurnInQuest, Stop`) + executor that drives PlayerbotAI | `_api.h/.cpp` | **PORT** (core of the design) |
| **Direct command entry** — `BuddyDirect::ExecuteBuddyCommand(bot, sender, text)`, verb parsing, forwarding to native playerbot chat pipeline | `_direct.h/.cpp` | **PORT** |
| **Chat mention override** — chat messages naming a bot dispatch commands with top priority over all bot logic | `_direct.cpp`, `_handler.cpp` | **PORT** (great for in-game human debugging too) |
| **GM/console command** — `.buddy` commandscript | `_gm.h/.cpp` | **PORT** — becomes our primary agent transport (see §4) |
| **Game-state snapshots** — group status, nearby players/creatures/objects, combat state, spells, waypoints, last commands | `_api.cpp` | **PORT** as JSON |
| **Ollama polling loop** — module polls an Ollama HTTP endpoint for instructions per bot | `_loop.h/.cpp`, `_config.cpp` | **REPLACE** — agents live outside the process; no in-server LLM loop wanted |
| **Thread-safe message/command history** | `_handler.cpp` | PORT (trivial deque+mutex pattern) |

## 3. Tortoise-side design

### 3.1 In-process command layer (new: `src/game/PlayerBots/Agent/`)

```
Agent/
├── BotCommandAPI.h/.cpp     # enum + ParseBotCommand(text) + Execute(bot, cmd) → result string
├── BotStateSnapshot.h/.cpp  # BuildBotStateJson(bot) → JSON string (single source of truth)
└── BotChatMention.h/.cpp    # chat-mention dispatch (hooks into ChatHandler path)
```

- `Execute` reuses existing engine primitives wherever possible: `MoveTo` → `MovePoint` pathfinding path already used by wander/grind; `Attack`/`CastSpell` → `AttackAction`/spell cast plumbing; `Loot` → loot stack; `Follow` → follow-movement; quests → quest handler opcodes like the death/loot actions do.
- Commands **override the strategy engine for that tick** (bot-buddy semantics): set a short-lived "external control" flag on `PlayerBotAI` that suppresses engine `UpdateAI` dispatch while an external command sequence is active (with a watchdog timeout, default ~10 s, so a crashed agent can't brick a bot).
- Result strings + `BuildBotStateJson` are the observability contract for agents.

### 3.2 State snapshot (JSON)

One call: `BuildBotStateJson(bot)` covering identity, position/map/zone, hp/mana/level/class/race/spec, combat state, current target (guid/entry/hp), threat attackers, group members, nearby creatures+gameobjects (guid, entry, name, pos, hostility flags — the bot-buddy "never invents GUIDs" rule: the snapshot is the only source of valid GUIDs an agent may reference), inventory summary (equipped + free bags), active quests, last N executed commands with results, engine state (COMBAT/NON_COMBAT/DEAD) and current strategy set.

### 3.3 Command reference (agent-facing)

```
bot <name> move to <x> <y> <z> [map]     bot <name> attack <guid>
bot <name> spell <spellid> [guid]        bot <name> interact <guid>
bot <name> loot                           bot <name> follow [player]
bot <name> stop                           bot <name> say <text>
bot <name> acceptquest <id>              bot <name> turninquest <id>
bot <name> state                          → full JSON snapshot
bot <name> engine [combat|noncombat|dead] → inspect/force engine state (debug)
bots list                                 → online bots + one-line status
```

## 4. Transport (how agents reach the server)

Phased, all read-mostly until proven:

1. **Console (R6.1):** commands via the `mangosd` CLI in tmux (`tmux send-keys` from agent bash) — same operational surface as `wow-server.sh`. Zero new server code beyond the command itself. Output read back from `tmux capture-pane`.
2. **State file (R6.1):** `bot <name> state` also dumps JSON to `server/logs/botstate/<name>.json` for cheap agent polling without screen-scraping.
3. **HTTP listener (R6.2, optional):** tiny in-process HTTP listener (mangosd already links event-driven network code) exposing POST `/bot/<name>/command` + GET `/bot/<name>/state`. Bind 127.0.0.1 only. This is the clean long-term agent surface and the eventual eval-harness driver.
4. **Chat mention (R6.1):** in-game `<BotName> come here` for humans supervising agent tests live.

Security: commands require GM rank on console path by default; HTTP path token-gated; `PlayerBot.AgentControl=0` config master-switch (server safe by default, mirroring the bot-buddy caution: *not for general use* until hardened).

## 5. Eval harness (the payoff)

Scripted scenarios agents can run and assert, via the state snapshot:

```
login soak      → bots list, assert N online stable across T
grind cycle     → bot X state over time: target acquired → in combat → target dead → loot event → equip event
death loop      → kill bot (GM command), assert DEAD engine → release → corpse path → revive
travel          → move to far coord, assert arrival + no strand
class rotation  → per class: assert ability usage in command history over a fight
```

Lives in `tools/boteval/` (agent-run via transport 1/2; CI-able later). Each R-phase exit criterion in the master plan maps to one eval.

## 6. Sequencing

After R1 (base sync) — the command layer touches `PlayerBotAI` update paths, pointless to build then rebase. R6.1 (console + state file + snapshot JSON) is the minimum viable agent loop; R6.2 HTTP + eval harness follow once R2 classes exist to test.
