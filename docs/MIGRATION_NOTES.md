# AutoBalance Migration & Implementation Notes

**Module:** `mod-autobalance` (AzerothCore 3.3.5a)  
**Reference Alignment:** `mod-twow-autobalance` (Turtle WoW / vMaNGOS)  
**Date:** November 23, 2026

---

## 1. Migration Overview

This release extends AzerothCore `mod-autobalance` with features inspired by `mod-twow-autobalance` while strictly preserving AzerothCore architectural invariants, module conventions, and 100% backwards compatibility with existing configuration files.

### Key Capabilities Added
1. **Dynamic Scaling Above Baseline:**  
   Normal 5-player dungeons and raids can now dynamically scale creature attributes for groups larger than the baseline (e.g. 6 to 25+ players in dungeons) using power-law scaling exponents.
2. **Preservation of Authentic Baselines:**  
   5-player dungeons maintain a 5-player baseline ($1.0\times$), scaling down for solo/small groups and scaling up for larger groups. Raids maintain their native 10M, 25M, or 40M baselines.
3. **Bot Weighting & Exclusion:**  
   Playerbots can now be weighted independently (e.g., `BotWeight = 0.75`) or excluded from scaling counts entirely.
4. **Creature Classification & Rank Filtering:**  
   Granular configuration toggles allow excluding Normal, Elite, Boss, Summon, Pet, Totem, Trigger, or Critter entities from scaling.
5. **Combat Safety & Health Preservation:**  
   Mid-combat recalculations preserve exact current health percentages (`PreserveHealthPercent`), preventing bosses from being healed to full when group size shifts.
6. **Recalculation Debounce / Throttle:**  
   `ThrottleMs = 1000` debounces rapid player entries/exits to protect server tick times.
7. **Expanded GM Command Suite:**  
   Added `.ab info`, `.ab map`, `.ab creature`, `.ab recalc`, and `.ab reload` alongside legacy `.autobalance` commands.

---

## 2. Configuration Aliases & Backward Compatibility

All existing configuration keys continue to function identically. The following table details the relationship between existing keys and extended features:

| Extended Config Key | Legacy Equivalent | Behavior |
|---|---|---|
| `AutoBalance.MinPlayers.Dungeon` | `AutoBalance.MinPlayers` | Falls back to `AutoBalance.MinPlayers` if unset |
| `AutoBalance.MinPlayers.Raid` | `AutoBalance.MinPlayers.Raid` | Unchanged |
| `AutoBalance.Announce.Enable` | `AutoBalanceAnnounce.enable` | Falls back to `AutoBalanceAnnounce.enable` |
| `AutoBalance.LevelScaling.Enable` | `AutoBalance.LevelScaling` | Falls back to `AutoBalance.LevelScaling` |
| `AutoBalance.Rewards.ScaleXP` | `AutoBalance.RewardScaling.XP` | Falls back to `AutoBalance.RewardScaling.XP` |
| `AutoBalance.Rewards.ScaleMoney` | `AutoBalance.RewardScaling.Money` | Falls back to `AutoBalance.RewardScaling.Money` |

---

## 3. Important: Dynamic Capacity vs. Creature Scaling

### What the Module Does (Module-Safe)
AutoBalance creature scaling is **100% module-safe and zero-SQL**:
- Modifies creature attributes (`Health`, `Mana`, `Armor`, `Damage`, `Level`) in memory at runtime.
- Never mutates database tables (`creature_template`, `instance_template`, `access_requirement`).
- Calculates scaling dynamically upon player entry, exit, or creature spawn.

### The "Sixth Player" Admission Barrier
In stock AzerothCore, admitting more than 5 players into a standard 5-man dungeon (or >40 into a raid) encounters a hard core check in `InstanceMap::CannotEnter`:
```cpp
uint32 maxPlayers = GetMaxPlayers();
if (GetPlayersCountExceptGMs() >= (loginCheck ? maxPlayers + 1 : maxPlayers))
    return CANNOT_ENTER_MAX_PLAYERS;
```
`GetMaxPlayers()` reads from `MapDifficulty.dbc` (where dungeons are hardcoded to 5). When a 6th player attempts to enter, the core rejects the player with `TRANSFER_ABORT_MAX_PLAYERS` **before** module map-entry hooks run.

### The Solution: Optional Core Hook Patch
To allow dynamic admission without modifying DBC files on disk, an optional, minimal core hook is provided in `patches/azerothcore_dynamic_instance_capacity.patch`:
```cpp
sScriptMgr->OnBeforeCheckInstanceMaxPlayers(player, GetEntry(), GetId(), Difficulty(GetSpawnMode()), maxPlayers);
```
Applying this patch allows `mod-autobalance` to dynamically raise `maxPlayers` in memory prior to the admission check.

---

## 4. Features Ported from Turtle WoW (`mod-twow-autobalance`)

| Feature | Port Status | Technical Notes |
|---|---|---|
| **Raid-in-Dungeon Scaling** | **Ported** | `AutoBalance.RaidScalingInDungeons` dynamically scales 5-man dungeons past 5 players. |
| **Power-Law Scaling Exponents** | **Ported** | Ratio power scaling ($ratio^{exp}$) with dampened damage exponent (0.65). |
| **Bot Weighting** | **Ported** | Configurable `CountBots` and `BotWeight` (default 1.0). |
| **Combat Health Preservation** | **Ported** | Calculates proportional health during mid-combat rescales. |
| **Recalculation Throttle** | **Ported** | 1000ms debounce prevents CPU load spikes on rapid zoning. |
| **DBC Map Mutation at Boot** | **Not Ported** | Replaced with optional non-intrusive core hook patch to adhere to DBC immutability. |
| **vmangos Headless Sessions** | **Not Ported** | Adapted to AzerothCore `PlayerbotAI` session detection. |
