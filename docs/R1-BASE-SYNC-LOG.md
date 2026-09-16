<!-- Canonical copy: /root/r1-base-sync-log.md — edit there, then re-copy -->

# R1 Base Sync — Issues & Solutions Log

**Status:** IN PROGRESS — core boots with 100 bots queued; one uncaught-exception crash under investigation
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

## 5. Verification status

- [x] Merge compiles (only 1 build fix needed, §2)
- [x] mangosd boots fully on the new base; DB checks pass
- [x] `PlayerBotMgr` queues all 100 bots for async login at boot
- [ ] Bot login soak — **BLOCKED**: uncaught `std::runtime_error("false")` → SIGABRT shortly after boot (stdout: `terminate called after throwing an instance of 'std::runtime_error' what(): false`). Backtrace capture pending (gdb.txt shows only thread exits). Suspects: our PlayerBotAI/PlayerBotMgr vs new core session/transport changes, or upstream subsystem hitting an unhandled case.
- [ ] Review P2 fixes (P1-3 spec wiring, P2-3 loot mutation, P2-6 FFA, P2-5 %s audit) — still queued after soak.

## 6. Key file states

| File | Change |
|---|---|
| `src/game/WorldSession.h` | bot plumbing restored + friend decl |
| `server/etc/mangosd.conf` | AutoUpdate keys fixed, duplicates removed (uncommitted runtime conf) |
| `tw_world` DB | 146 migrations reconciled; `migrations` table now tracks new hashes |
| branch | merge commit `a473d4f5` (uncommitted WorldSession.h fix pending) |
