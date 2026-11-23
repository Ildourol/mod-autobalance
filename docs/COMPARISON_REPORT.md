# AutoBalance Comparison Report: Turtle WoW (vMaNGOS) vs AzerothCore

**Target Module:** `mod-autobalance` (AzerothCore 3.3.5a)  
**Reference Module:** `mod-twow-autobalance` (Turtle WoW / vMaNGOS 1.12.1)  
**Date:** November 23, 2026  
**Auditor:** Antigravity CLI 2 (Autonomous Subsystem)

---

## 1. Executive Summary

This comparison report analyzes `mod-twow-autobalance` (Turtle WoW / vMaNGOS) and `mod-autobalance` (AzerothCore 3.3.5a) prior to code modifications. Both modules share historical ancestry from the TrinityCore/CVMagic AutoBalance project, but each has evolved along distinct architectural paths dictated by their respective server emulators:

- **AzerothCore `mod-autobalance`** is a deep WotLK module utilizing `ScriptMgr` hooks (`AllMapScript`, `AllCreatureScript`, `UnitScript`, `PlayerScript`), `CustomData` containers on Entities, and WotLK 3.3.5a multi-tier difficulty modes (Normal, Heroic, 10M, 25M). It implements runtime entry/exit/spawn scaling using a hyperbolic tangent (`tanh`) curve.
- **Turtle WoW `mod-twow-autobalance`** is a vMaNGOS module designed for Vanilla (1.12.1/1.18) mechanics. Its standout innovations are **dynamic in-memory map capacity adjustments** (allowing 1–60 players in instances without SQL/DBC modifications), **Raid-in-Dungeon scaling** (scaling 5-man dungeons up to 10M, 20M, 40M, or 60M when entered by raid groups), and **bot weighting**.

---

## 2. Feature Comparison Matrix

| Feature Domain | AzerothCore (`mod-autobalance`) | Turtle WoW (`mod-twow-autobalance`) | Gap / Parity Analysis |
|---|---|---|---|
| **Runtime Entry Recalculation** | Implemented via `OnPlayerEnterAll` | Implemented via `OnPlayerEnter` | Present in both; AC handles multi-instance maps natively. |
| **Runtime Exit Recalculation** | Implemented via `OnPlayerLeaveAll` | Implemented via `OnPlayerLeave` | Present in both. |
| **Creature Spawn Scaling** | Implemented via `OnAllCreatureUpdate` | Implemented via `OnCreatureAdd` | Present in both. |
| **Combat Locking** | Implemented (`combatLocked`, `combatLockMinPlayers`) | Implemented (`combatLocked`, `combatLockMinPlayers`) | Present in both. Prevents downscaling mid-encounter. |
| **Stat Storage (Anti-Stacking)** | Uses `origCreatureBaseStats` from `sObjectMgr` | Stores `CreatureBaseData` per GUID | Present in both; AC re-derives from base stats, tWoW caches original struct. |
| **Scaling Mathematical Model** | `tanh` sigmoid saturation curve | `tanh` sigmoid saturation curve | Both saturate at capacity; neither has native power-law scaling above baseline. |
| **Scaling Above Baseline (>5 Players)** | **Missing / Saturated** (caps at 5-man baseline) | **Implemented** (`AutoBalance.RaidScalingInDungeons`) | **Gap in AC**: AC clamps to 5-man curve; tWoW dynamically steps into 10M..60M profiles. |
| **Dynamic Admission Capacity** | **Missing** (strictly DBC-bound to 5 or 40) | **Implemented** (`ApplyMemoryMapLimits` at startup) | **Gap in AC**: AC rejects player #6 at portal with `TRANSFER_ABORT_MAX_PLAYERS`. |
| **Bot Weighting & Configuration** | Bots counted symmetrically as players | Configurable `CountBots` and `BotWeight` (0.0–1.0) | **Gap in AC**: AC lacks bot weight configuration. |
| **Creature Rank Filtering** | General boss/trash distinction | Explicit filters for Normal, Elite, Rare, Critter, Trigger | **Gap in AC**: AC lacks granular toggle per creature rank/type. |
| **Preserve Health Percentage** | Partially handled during combat | Recalculates HP while preserving current percentage | **Gap in AC**: Needs formalization so mid-combat rescaling doesn't heal/kill mobs. |
| **Recalculation Throttle** | Immediate / Per-event | Map maintenance ticker | **Gap in AC**: Rapid joins/leaves can cause burst recalculations. |
| **Per-Instance State Caching** | Stored in `Map->CustomData` | Stored in `AutoBalanceManager` map table | Both functional; AC uses core `DataMap`. |

---

## 3. Detailed Architectural Dissection

### 3.1 Features Present in `mod-twow-autobalance` Missing from AzerothCore
1. **Dynamic Dungeon & Raid Scaling Above Baseline:**  
   When more than 5 players enter a dungeon (e.g., a 10-man raid in Deadmines), `mod-twow-autobalance` detects `activePlayers > baseCapacity` and activates raid scaling profiles (`AutoBalance.InflectionPointRaid10M`, etc.), scaling creature health and damage to meet the larger raid group.
2. **Bot Weighting (`AutoBalance.CountBots`, `AutoBalance.BotWeight`):**  
   Allows adjusting the effective player contribution of bots (e.g., 0.75 player weight instead of 1.0).
3. **Selective Creature Rank/Type Filtering:**  
   Ability to disable scaling for summons, totems, critters, or specific creature ranks via explicit config toggles.
4. **Enhanced GM Commands:**  
   Clean `.ab info`, `.ab recalc`, `.ab creature`, `.ab reload` command suite with concise output.

### 3.2 Features Already Present in AzerothCore `mod-autobalance`
1. **Runtime Event Scaling:**  
   `OnPlayerEnterAll`, `OnPlayerLeaveAll`, `OnPlayerLevelChanged`, and creature update hooks already execute dynamically at runtime. Worldserver startup does **not** globally pre-scale all dungeon instances in advance.
2. **WotLK Multi-Difficulty Support:**  
   Detailed configuration for 10M, 10M Heroic, 25M, 25M Heroic, 15M, 20M, and 40M instances.
3. **Dynamic Level Scaling:**  
   Scales creature levels dynamically based on highest player level in the map, with expansion base stats lookup (`getBaseExpansionValueForLevel`).
4. **Reward Scaling:**  
   XP and money modifications hooked directly into `PlayerScript::OnPlayerGiveXP` and `OnPlayerBeforeLootMoney`.
5. **Multi-Language Announcements:**  
   Fully translated localization tables in `Message.cpp` supporting 9 client languages.

### 3.3 Features That Cannot Be Ported Directly (API Incompatibilities)
1. **`sMapStorage.begin<MapEntry>()`:**  
   vMaNGOS stores map definitions in `sMapStorage`. AzerothCore stores them in `sMapStore` (DBC) and evaluates instance player limits using `MapDifficulty.dbc` (`InstanceMap::GetMaxPlayers()`). Mutating `MapEntry::maxPlayers` alone in AC does not affect 5-man dungeons because `mapDiff->maxPlayers` takes precedence.
2. **`player->GetSession()->IsHeadless()`:**  
   vMaNGOS TortoiseBots uses headless sessions. AzerothCore Playerbots are full player entities identified through `player->GetPlayerbotAI()` or the Playerbots manager.
3. **vmangos `QueryResult*` with manual delete:**  
   AzerothCore uses `std::shared_ptr<ResultSet>` (`QueryResult`) with RAII memory management.

### 3.4 Features Requiring a Core Hook vs Pure Module Implementation

#### A. Pure Module Implementation (Safe & Fully Supported)
- **Power-law ratio scaling** ($Health \propto (N / N_{base})^{exp}$) scaling creatures smoothly above and below baseline.
- **Baseline preservation** (dungeon baseline = 5, raid baselines = 10/25/40).
- **Bot weighting & GM exclusion rules**.
- **Creature rank & type filter masks** (Normal, Elite, Boss, Summon, Totem, Critter).
- **Recalculation throttle & combat HP preservation**.
- **Per-map overrides, whitelists, and blacklists**.
- **GM command suite** (`.ab info`, `.ab recalc`, `.ab creature`, `.ab reload`).

#### B. Requiring a Minimal Core Hook (Admission Capacity Override)
- **The "Sixth Player Problem":**  
  In AzerothCore, [`Map.cpp:2024`](file:///C:/Users/Admin/AntigravityProfiles/Projects%20Azerothcore/Azerothcore%20server/azerothcore-wotlk/src/server/game/Maps/Map.cpp#L2024) checks `GetPlayersCountExceptGMs() >= maxPlayers` during map transfer. For a 5-man dungeon, `maxPlayers = 5`. If a 6th player attempts to enter, the core sends `TRANSFER_ABORT_MAX_PLAYERS` before the player ever enters the map.
- **Hook Proposal:**  
  A minimal core hook `OnBeforeCheckInstanceMaxPlayers(Player* player, MapEntry const* mapEntry, uint32 mapId, Difficulty difficulty, uint32& maxPlayers)` allows modules to safely raise `maxPlayers` dynamically before rejection.
- **Pending Status:**  
  In compliance with the Live Server & Core Immutability directives, this hook is delivered as an optional patch artifact (`patches/azerothcore_dynamic_instance_capacity.patch`) without modifying the live server core.

---

## 4. Implementation Strategy

1. **Implement Module-Level Scaler & Manager:**  
   Introduce a clean, robust scaling engine supporting:
   - Configurable baseline (default 5 for dungeons, 10/25/40 for raids).
   - Bi-directional scaling: scales down below baseline (solo/duo) and scales up above baseline (>5 players).
   - Configurable exponents (`AutoBalance.Scaling.HealthExponent`, `DamageExponent`, etc.).
   - Granular creature type filters (Summon, Pet, Totem, Critter, Trigger).
   - Bot count & weight support.
   - Recalculation throttling (1000ms debounce).
   - Combat HP percentage preservation.
2. **Preserve Existing Configuration & Backwards Compatibility:**  
   Retain all existing `AutoBalance.*` configuration options, providing seamless fallback and alias resolution.
3. **Provide Standalone Regression Tests:**  
   Pure C++ regression test harness verifying formula arithmetic, baseline preservation, ratio calculations, and boundary clamping.
4. **Deliver Core Patch Artifact:**  
   Optional core patch for `Map.cpp` and `ScriptMgr` documenting the dynamic capacity hook.
