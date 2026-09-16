<!-- Canonical copy: /root/r1-base-sync-log.md — edit there, then re-copy -->

# R1 Base Sync — Issues & Solutions Log

**Status:** IN PROGRESS — R1 core fixes pushed (`2136011f`…`dff5ce37`), review fixes pushed (`ceeb728c`, `97ab55cc`), 100/100 bots online. **Critical fix in flight:** merge had also dropped the bot-AI wiring call site (`Player::AddToWorld` → `OnPlayerInWorld`) so bots were online but idle; re-added, building. Remaining: verify AI active + real soak. See §6b.
**Date:** 2026-09-16
**Parent:** `bot-master-plan.md` §5 R1 (merge 251 upstream commits onto `playerbot-engine-port`)

---

## 1. Merge conflicts (resolved — keep in-core chassis)

Upstream Penqle deleted their own old in-core bot system and added the **module system** (`modules/` infra, `cmake/ConfigureModules.cmake`) during our absence.

| File | Conflict | Resolution |
|---|---|---|
| `src/game/PlayerBots/PlayerBotAI.{h,cpp}`, `PlayerBotMgr.{h,cpp}` | modify/delete (upstream deleted) | **Keep ours** (master plan A1: in-core chassis). Upstream has NO playerbots now; Shyalya's module is the only upstream-visible bot system. |
| `src/game/World.cpp` | include block | Keep our `PlayerBotMgr.h` + `PlayerbotAIConfig.h` includes |
| `src/game/WorldSession.cpp` | packet hook vs new `SERVERHOOK_CAN_PACKET_SEND` script hook | **Keep both**: our `PlayerBotAI::OnPacketReceived` notification first, then upstream's script hook + early-return |
| `src/game/Chat/Chat.cpp` | includes | Keep ours + upstream's `ScriptObjects.h` |
| `src/game/Objects/Player.cpp` | bot-save guard | Keep ours |
| `src/game/CMakeLists.txt` | PlayerBots source lists + include dirs | Keep ours |

Backup branch: `playerbot-engine-port-pre-r1` (pushed to avirar).

## 2. Build fix — upstream stripped WorldSession bot plumbing

Upstream's bot-system removal also deleted `WorldSession::GetBot()/SetBot()/m_bot` (the old tortoise-native `PlayerBotEntry` plumbing our port relies on).

**Fix** (`src/game/WorldSession.h`):
- re-added `PlayerBotEntry* GetBot()/SetBot(PlayerBotEntry*)` + `PlayerBotEntry* m_bot = nullptr;`
- added `struct PlayerBotEntry;` forward declaration
- `WorldSession::LoginPlayer` became **private** in the new code → added `friend class PlayerBotAI;` (alongside existing `CharacterHandler`, `HeadlessSessionMgr` friends)

## 3. DB auto-updater: config key rename + duplicate keys (crash: "database structure is not up to date")

New core reads different keys than our pre-merge conf. Also **mangos config: last duplicate occurrence wins** — a stale pair of keys at the bottom of `mangosd.conf` silently overrode the correct ones.

**Correct keys** (match `src/mangosd/mangosd.conf.dist.in`):
```
Database.AutoUpdate.Enabled = 1
Database.AutoUpdate.Path = "../../sql/database_updates/"
Database.AutoUpdate.AuthUpdateName = "unused"      # no auth updates in repo
Database.AutoUpdate.CharUpdateName = "character"
Database.AutoUpdate.WorldUpdateName = "world"
Database.AutoUpdate.AllowedModules = "all"
Database.AutoUpdate.SortByName = 1
```
**Fix:** replaced the old 3-line block AND deleted the leftover `CharUpdateName="unused"` / `WorldUpdateName="database_updates"` duplicates (lines 54-55). The duplicate keys made the updater scan `sql/database_updates/` (flat, 0 files) instead of `sql/database_updates/world/` (146 files).

**Diagnostic tip:** the old `Log::outInfo` writes to **stdout only** (not server.log). Updater messages like `[DB Auto-Updater] Found N possible migrations` appear in the tmux pane / direct stdout, not in `server/logs/server.log`. Run mangosd directly (not under GDB) with stdout capture to see them.

## 4. Migrations collide with old-era DB state (the big one)

The pre-merge server's updater had already applied many content updates **under old file names** (its `migrations` table in tw_world held ~60 old hashes). The new upstream tree has **regenerated** migration files (new names, merged content) — so the new updater re-attempts content whose data is already (partially) present → duplicate-key failures → `DB AutoUpdater FAILED, cancelling server.`

First collision: `20260526123433_world.sql` (Dragonmaw Retreat spawns) — `Duplicate entry '2816001'` on gameobject INSERT; the old DB already had 2 of the 224 gameobjects.

**Solution — idempotent reconciler** (`/tmp/apply_migrations.py`):
- For each pending file (hash not in `tw_world.migrations`):
  - **pure-DML files** (only INSERT/UPDATE/DELETE/REPLACE): apply with `INSERT INTO` → `INSERT IGNORE INTO` and **statement-leading** `UPDATE` → `UPDATE IGNORE` (skips duplicate-key rows; leaves old rows in place)
  - **DDL files**: apply verbatim, abort on error for manual review
  - wrap in `START TRANSACTION; … COMMIT;`
- Record each applied file's SHA-1 hash into `migrations` (Name, Hash, Module='', AppliedAt) — matching `AutoUpdater::GetMigrationKey("", hash)`.

**Gotcha:** the UPDATE regex must be statement-anchored (`^\s*UPDATE`) — a naive `\bUPDATE\b` rewrite corrupts `ON DUPLICATE KEY UPDATE` → `UPDATE IGNORE` syntax error.

**Result:** all 146 world migrations applied (98 pending → 0), 2 DDL files passed verbatim. Character DB's single migration (`20260817151028_character`) had already been applied by the updater run.

## 5. The bot-login saga — upstream session architecture changes (RESOLVED via HeadlessSessionMgr)

Upstream rebuilt session handling: socketless sessions can no longer live in `World::m_sessions`.

1. **`WorldSession::Update()` returns false for non-connected sessions** → `World::UpdateSessions` deletes them within a tick. Our old flow (create session + `AddSession` + `LoginPlayer`) was silently destroyed before the async login callback could run → 0 bots online, `loading=100` forever.
2. **`WorldSession::HandlePlayerLogin` now gates on `IsLoginRequest(guid, transport, token)`** — the old direct `HandlePlayerLogin` call from our bot callback could never pass.
3. **`World::AddSession` is a queue** (`addSessQueue` drained in `UpdateSessions`) — racing the SQL callback either way.

**Solution — use upstream's `HeadlessSessionMgr`** (built exactly for socketless logins):

| Change | File | Detail |
|---|---|---|
| `ScheduleBotLogin` rewritten | `CharacterHandler.cpp` | `sWorld.StartHeadlessSession(accountId, guid, LOCALE_enUS, botName)` — creates `SessionTransport::Headless` session, `InitHeadlessSession()` sets `m_connected=true` (survives the socketless purge), `LoginPlayer` queues the holder. The callback routes via `HandleHeadlessLoginCallback` → manager resolves by (guid, accountId, token) — **no FindSession race** |
| `HeadlessSessionMgr::GetSession(guid)` | `HeadlessSessionMgr.h` (new inline accessor) | lets us attach the `PlayerBotEntry` right after `Start` (searches `m_pendingSessions` then `m_sessions`) |
| `World::GetHeadlessSessionMgr()` | `World.h` (new accessor) | exposes the manager |
| Bot completion block | `CharacterHandler.cpp`, end of `HandlePlayerLogin` | `if (PlayerBotEntry* e = GetBot())` → decrement loadingCount, `OnBotLogin`, then `TeleportTo` race start town (per-race switch, same coords as the old callback) |
| `LoginPlayer` made public | `WorldSession.h` | was private in new core (not strictly needed now, kept for cleanliness) |

**Result:** `[BOT_LOGIN]` ×300 lines, `characters.online = 100` via `tw sql`.

## 6. Verification status

- [x] Merge compiles (only 1 build fix needed, §2)
- [x] DB auto-updater fixed (config key rename + duplicate-key removal; 146 world + 1 char migration reconciled idempotently)
- [x] mangosd boots on new base
- [x] **100/100 bots online** (headless-session flow, §5)
- [~] **Soak in progress**: 100 bots online, world stable at time of writing (14:30+). One earlier crash (pre-gdb-upgrade, uncaught exception `what(): false` + one SIGSEGV) is NOT yet root-caused. GDB capture upgraded (§6a) — if it crashes again, `server/logs/gdb.txt` will contain the backtrace.
- [ ] Fix open review items: P1-3 spec wiring, P2-3 loot mutation, P2-6 FFA, P2-5 %s audit, P3 cleanup
- **Exit criteria:** soak passes on new base; review table all-green.

### 6a. GDB crash capture upgraded (`/root/wow-server.sh`)

`wow-server.sh` regenerates `server/.gdb_cmds` on each start; the template now catches **SIGSEGV + SIGABRT + C++ `throw`** with `bt full` into `server/logs/gdb.txt`. Note: `catch throw` fires on *every* C++ exception (even caught ones) — the game throws many, so expect noise; the real crash backtrace is the last one before `Program terminated`.

### 6b. Bots online but AI dead — merge dropped the `OnPlayerInWorld` call site (RESOLVED 2026-09-16)

**Symptom:** 100/100 bots online, but `PlayerBotMgr::PrintStats` shows `Engine: non-combat=0, combat=0, dead=0` and no `AI Tick` logs. Bots logged in and idled; none moved, targeted, or cast.

**Root cause:** The pre-merge port wired bot AI via `Player::AddToWorld()` ending in `sPlayerBotMgr.OnPlayerInWorld(this);`. That method does the actual AI wiring:

```cpp
void PlayerBotMgr::OnPlayerInWorld(Player* player) {
    if (PlayerBotEntry* e = player->GetSession()->GetBot()) {
        player->setAI(e->ai);      // store PlayerBotAI in Player::i_AI
        e->ai->SetPlayer(player);
        e->ai->OnPlayerLogin();    // → Initialize() builds the 3 engines
    }
}
```

The merge dropped that line from `Player::AddToWorld()` (`src/game/Objects/Player.cpp`), so `Player::i_AI` stayed `nullptr` for every bot. `Player::Update()` only calls `i_AI->UpdateAI()` when `i_AI` is non-null — with it null, no bot engine ever ran. The packet-interception path (`WorldSession::SendPacket` → `dynamic_cast<PlayerBotAI*>`) also keys off `player->AI()`, so it was dead too.

**Fix:** re-added `sPlayerBotMgr.OnPlayerInWorld(this);` at the end of `Player::AddToWorld()` plus `#include "PlayerBots/PlayerBotMgr.h"`. Verified `PlayerBotAI::Remove()` only clears the pointer (no delete), so the entry-owned AI is safe.

**Verification after fix:** `PrintStats` should show non-zero engine counts and `AI Tick` debug lines should appear.

## 7. Handover checklist (next session)

1. [x] ~~Strip debug traces~~ (done — `[BOT_DBG]` removed before commits; `[BOT_LOGIN]` kept)
2. [x] ~~Commit R1 fixes + push~~ (commits `2136011f`…`dff5ce37` pushed to `avirar/playerbot-engine-port`)
3. [x] Rebuild + install + restart with committed source (tmux session `r1build` chains build→install→restart; check `/tmp/build-r1.log` for `RESTART_DONE`)
4. [ ] **Finish soak**: ≥30 min no crash with bots grinding. If crash: read `server/logs/gdb.txt` (now catches SIGSEGV + SIGABRT + C++ throws with `bt full`), root-cause.
5. [ ] **P2 review fixes** (master plan §4 open items): P1-3 spec wiring (Arms hardcode; `GetPlayerSpecTab` exists in StatsWeightCalculator), P2-3 StoreLootAction direct `is_looted` mutation, P2-6 FFA inversion, P2-5 `GetName()` `%s` audit, P3 cleanup.
6. Continue master-plan roadmap: R2 class parity → R3 item pipeline → R4 Shyalya lifts.
