# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore - AutoBalance

Dynamic runtime creature, health, mana, and damage scaling module for AzerothCore (WotLK 3.3.5a), featuring flexible group scaling below and above standard instance capacities.

---

## 1. Primary Architecture & Runtime Lifecycle

AutoBalance dynamically adapts dungeon and raid encounters to active player counts in real time:

1. **Player Entry:**  
   When a player enters an instance (`AllMapScript::OnPlayerEnterAll`), the module verifies map eligibility, tallies active humans and bots (applying configured bot weights and GM exclusions), computes the baseline and scaling ratios, and schedules creatures for rescaling.
2. **Player Exit / Disconnect:**  
   When a player leaves or logs out (`AllMapScript::OnPlayerLeaveAll`), the module recalculates the remaining group. If the group is engaged in combat, combat locking maintains the difficulty floor until combat ends.
3. **Creature Spawn:**  
   When a creature spawns (`AllCreatureScript::OnAllCreatureUpdate`), it is automatically balanced according to the current instance player count and difficulty mode before engaging players.
4. **Zero SQL & Zero DBC Mutability:**  
   All stat calculations occur in memory on live game objects. No permanent database updates (`UPDATE creature_template`) or DBC file edits are required.

---

## 2. Important: Dynamic Capacity vs. Creature Scaling

> [!IMPORTANT]
> **Understanding the Boundary Between Scaling and Admission**
>
> - **Creature Scaling (Module-Safe):** AutoBalance creature stat scaling is 100% module-level code. It requires no core modifications and operates safely out of the box.
> - **Dynamic Admission (>5 Players in Dungeons):** Standard AzerothCore hard-rejects players at the portal if an instance has reached its DBC player limit (`MapDifficulty.dbc` maxPlayers = 5). AutoBalance cannot bypass this check from a post-entry hook because the player is rejected **before** entering the map.
> - **Optional Core Hook:** To admit groups larger than 5 players into standard dungeons without editing DBC files, apply the optional patch provided at [`patches/azerothcore_dynamic_instance_capacity.patch`](patches/azerothcore_dynamic_instance_capacity.patch).

---

## 3. Scaling Modes & Mathematical Curves

AutoBalance supports three scaling algorithms configured via `AutoBalance.Scaling.Mode`:

- **`hybrid` (Default):** Uses the calibrated hyperbolic tangent (`tanh`) sigmoid curve for counts below or equal to the instance baseline (1–5 players in dungeons), and smoothly transitions to power-law scaling above baseline.
- **`power`:** Pure power-law scaling across all counts:
  $$\text{Scale} = \left(\frac{\text{Effective Players}}{\text{Baseline Players}}\right)^{\text{Exponent}}$$
  - **Health Exponent:** Default `1.0` (linear scaling)
  - **Damage Exponent:** Default `0.65` (dampened to prevent sudden burst one-shots)
  - **Armor Exponent:** Default `0.5`
- **`tanh`:** Classic AutoBalance sigmoid saturation curve.

### Baseline Preservations
- **5-Player Dungeons:** Baseline is fixed at 5 players ($1.0\times$). Solo players scale down to $\approx 20\%$ HP; 10-player raid groups scale up to $200\%$ HP and $157\%$ damage.
- **Raids:** Baselines preserve native tier sizes (10M = 10, 25M = 25, 40M = 40).

---

## 4. In-Game Commands

| Command | Permission | Description |
|---|---|---|
| `.ab info` | Player | Displays concise current instance scaling summary, baseline, active players, and multipliers. |
| `.ab map` / `.ab mapstat` | Player | Displays detailed map statistics, difficulty offset, and active world modifiers. |
| `.ab creature` / `.ab creaturestat` | Player | Displays selected creature's base and scaled stats, level, and boss classifications. |
| `.ab recalc` | Game Master | Forces an immediate recalculation and rescale of all creatures in the current instance. |
| `.ab reload` | Administrator | Reloads `AutoBalance.conf` configuration in memory without restarting the worldserver. |
| `.ab setoffset <#>` | Game Master | Sets a temporary difficulty offset (+1, -1 player difficulty). |
| `.ab getoffset` | Player | Displays current difficulty offset. |

---

## 5. Verification & Test Checklist

Use this checklist for regression testing:

- [x] **Solo Ragefire Chasm Entry:** Verify creature HP scales down to $\approx 20\%$ and damage to $\approx 35\%$.
- [x] **Two-Player Entry:** Verify stats scale up proportionally toward 5-player baseline.
- [x] **Five-Player Baseline:** Verify creature stats match authentic unscaled baseline ($1.0\times$).
- [x] **Six or More Players Entry:** Verify dynamic scaling scales creature HP above $100\%$ with power exponent.
- [x] **Player Leave During Trash:** Verify remaining creatures rescale to smaller party size.
- [x] **Player Join During Boss Combat:** Verify combat floor raises immediately to new count.
- [x] **Player Leave During Boss Combat:** Verify combat lock preserves difficulty floor until combat ends.
- [x] **In-Combat Rescaling:** Verify creature health percentage is strictly preserved without healing to full.
- [x] **Creature Respawn:** Verify newly respawned creatures inherit current instance scale.
- [x] **GM Bypass:** Verify GMs are not counted when `AutoBalance.PlayerCount.CountGMs = 0`.
- [x] **Battleground / Arena Exclusion:** Verify PvP maps are excluded by default.
- [x] **Map Blacklist:** Verify blacklisted maps bypass all scaling logic.
- [x] **Configuration Reload:** Verify `.ab reload` applies changes cleanly.
- [x] **Server Boot Zero-SQL:** Confirm server boots with zero database modifications.

---

## 6. Standalone Regression Tests

Run the included standalone regression test suite:

```bash
python tests/run_scaling_tests.py
```
