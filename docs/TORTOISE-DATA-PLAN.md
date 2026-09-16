<!-- Canonical copy: /root/tortoise-data-plan.md — edit there, then re-copy -->

# tortoise-data — A Tortoise-Native Data Tool (new build, not a fork)

**Status:** v0.1 IMPLEMENTED 2026-09-16 — live at `/root/tortoise-data`, pushed to https://github.com/avirar/tortoise-data (repo scoped with gh). All core commands work against the live server data (see README). Remaining from the original plan: SQL-layer polish, `--json` everywhere, mmap/vmap queries (future).
**Parent:** `bot-master-plan.md` (R0 tooling)
**Decision:** build new. acore-data took ages *because* of its AC-specific machinery — a 475-store registry generated from WotLK C++ headers, cross-ref graphs, SQL overlays, encounter rollups. A fork pays that cost again while fighting AC assumptions (WotLK DBCs, `acore_*` schemas, AC column layouts) in every file. Tortoise's data universe is smaller and — crucially — **self-describing**.

---

## 1. Why a new tool is cheaper here

Tortoise is mangos-lineage, and mangos declares DBC layouts as **format strings**:

```c
// src/game/Database/DBCfmt.h
const char ChrRacesEntryfmt[] = "niixiixxiixxixixissssssssxxxx";
const char AreaTableEntryfmt[] = "niiiixxxxxissssssssxixxxi";
```

`n`=uint32 id, `i`=int32, `f`=float, `s`=string, `x`=skip, localized-string blocks included. Paired with `src/game/Database/DBCStructure.h` (field names, in declaration order) and the loading table in `src/game/Database/DBCStores.cpp` (struct ↔ `.dbc` file ↔ store variable), this is a complete, authoritative machine-readable schema for all 159 DBCs — **no registry generation needed**. The generic WDBC binary format (magic `WDBC`, record count, field count, record size, string-block) is trivial to read from these specs.

acore-data's expensive core problem simply does not exist on tortoise. What's worth keeping from it is *product shape*: name-based entry from any angle, type-aware output, `--json`, smart routing.

## 2. Product: `tw` CLI

Python 3, **zero hard dependencies** (SQL via `mysql` CLI subprocess with `MYSQL_PWD`, like acore-data's fallback path; `pymysql` used opportunistically if present). CLI-first because agents live in bash; MCP/pi-extension wrapping is a thin later addition, not the foundation.

```
tw list [--category dbc|sql]              # known stores/tables
tw fmt <Store>                            # show format string + field names (introspection)

tw dbc <Store> [options]                  # any of the 159 DBCs by store or file name
    --id 118 | --where 'Name$like:Poly' | --fields SpellLevel,BaseLevel | --limit N | --json
tw sql "<query>" [--db world|char|logon]  # read-only guard: SELECT/SHOW/DESCRIBE only
tw table <name> [--db world]              # describe + 5 sample rows

# Convenience (thin wrappers over dbc/sql):
tw spell <id|name>      tw item <id|name>      tw quest <id|name>
tw spawns <entry>       # creature spawns for template (creature/gameobject joins)
tw graveyard <map> <x> <y>   # nearest GY (game_graveyard or WorldSafeLocs DBC)
```

- Store lookup by DBC file (`Spell`), struct (`SpellEntry`), or store var (`sSpellStore`) — all resolve via the DBCStores loading table.
- Output: aligned table for humans, `--json` for agents. `tw` also caches parsed DBC files (mtime-keyed) under `~/.cache/tortoise-data/`.
- Config: DB creds from `TWDATA_*` env vars, else auto-parsed from `/root/tortoise-wow/server/etc/mangosd.conf` (`WorldDatabaseInfo` = `host;port;user;pass;db` mangos format), else `mangos`/`mangos` defaults on localhost.

## 3. Implementation plan (small)

| Piece | What | Est. |
|---|---|---|
| `dbc_schema.py` | Parse `DBCfmt.h` (format strings) + `DBCStructure.h` (structs → ordered field names; type→name heuristics per mangos idioms) + `DBCStores.cpp` (struct↔file↔store map) | ~2 h |
| `dbc_reader.py` | Generic WDBC parser driven by a format string; string-block offsets → text; localized blocks → locale-aware collapse | ~1 h |
| `sql.py` | mysql CLI wrapper, read-only guard, per-db routing, table introspection | ~1 h |
| `cli.py` | argparse surface above, table + JSON renderers, DBC cache | ~1 h |
| Convenience wrappers + graveyard/spawns | thin joins | ~1–2 h |
| Soak | spot-verify ~20 knowns: Spell 118 Polymorph; SkillRaceClassInfo class bits vs factory table (AGENTS.md); `playerbots_names` ≈97K; ChrRaces races 9/10 exist (Goblin/HighElf) | ~1 h |

Total ≈ one focused day. Compare: acore-data fork = registry regeneration + tool-surface rewrite + schema reconciliation across 475 stores (multi-day, per the user's experience).

Later (optional, post-R1): MCP stdio server mode and a pi `registerTool` wrapper over the same core; `terrain` queries against mmaps if R4a travel work needs navmesh introspection.

## 4. Sources of truth (all local)

| Source | Use |
|---|---|
| `/root/tortoise-wow/src/game/Database/DBCfmt.h` | field types/format per store |
| `/root/tortoise-wow/src/game/Database/DBCStructure.h` | field names per struct |
| `/root/tortoise-wow/src/game/Database/DBCStores.cpp` | struct ↔ DBC file ↔ store wiring |
| `/root/tortoise-wow/server/data/dbc/` (159 files) | the data |
| MariaDB `tw_world` / `tw_char` / `tw_logon` | SQL data (creds in `mangosd.conf`) |
| `/root/acore-data` | product-shape inspiration only; no code reuse planned (different schema universe; its LICENSE stays with it) |

## 5. Consumers

- **pi agent sessions** (primary): `tw` via bash for debugging bot behavior against real data.
- **Bot evals (R6)**: assertions query `tw --json` instead of hardcoding expectations.
- **Humans**: quick lookups during manual playtesting.

## 6. Sequencing

Nothing blocks it; ideal filler during R1 soak windows. Order: `dbc_schema` → `dbc_reader` → `cli` → `sql` → convenience → soak.
