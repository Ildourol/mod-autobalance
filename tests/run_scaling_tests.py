#!/usr/bin/env python3
"""
AutoBalance Mathematical Model & Scaling Regression Test Runner
Verifies:
1. 5-player dungeon baseline preservation (5 players = 1.0x multiplier)
2. Solo scaling down below baseline (1 player on 5-man baseline)
3. Scaling above baseline (10 players in 5-man dungeon = 2.0x HP, 1.57x Damage)
4. Scaling above baseline (25 players in 5-man dungeon = 5.0x HP, 2.85x Damage)
5. Raid baseline preservation (40 players on 40-man baseline = 1.0x)
6. In-combat health percentage preservation
7. Bot weighting calculations and exclusion toggles
8. Minimum & maximum effective player clamping
"""

import math
import sys

def calculate_power_scaling(effective_players, baseline_players, h_exp=1.0, d_exp=0.65, a_exp=0.5, m_exp=1.0):
    ratio = (effective_players / float(baseline_players)) if baseline_players > 0 else 1.0
    if ratio <= 0.0:
        ratio = 0.05
    return {
        "ratio": ratio,
        "health_scale": math.pow(ratio, h_exp),
        "damage_scale": math.pow(ratio, d_exp),
        "armor_scale": math.pow(ratio, a_exp),
        "mana_scale": math.pow(ratio, m_exp)
    }

def calculate_combat_rescaled_health(new_final_health, current_health_pct):
    return max(1, round(new_final_health * (current_health_pct / 100.0)))

def calculate_effective_players(human_count, bot_count, bot_weight=1.0, count_bots=True, min_clamp=1, max_clamp=40):
    eff = float(human_count)
    if count_bots:
        eff += float(bot_count) * bot_weight
    if eff < min_clamp:
        eff = float(min_clamp)
    if eff > max_clamp:
        eff = float(max_clamp)
    return eff

def run_all_tests():
    print("========================================================")
    print("  AutoBalance Standalone Scaling Regression Test Suite  ")
    print("========================================================\n")
    passed = 0
    total = 0

    # Test 1: Baseline Preservation
    total += 1
    res1 = calculate_power_scaling(5.0, 5.0)
    print(f"[TEST 1] Baseline Preservation (5 players on 5-man baseline):")
    print(f"         Ratio: {res1['ratio']:.3f}, HealthScale: {res1['health_scale']:.3f}, DamageScale: {res1['damage_scale']:.3f}")
    assert abs(res1["ratio"] - 1.0) < 1e-4, "Ratio must be 1.0"
    assert abs(res1["health_scale"] - 1.0) < 1e-4, "Health scale must be 1.0"
    assert abs(res1["damage_scale"] - 1.0) < 1e-4, "Damage scale must be 1.0"
    print("         -> PASS\n")
    passed += 1

    # Test 2: Solo Downscaling Below Baseline
    total += 1
    res2 = calculate_power_scaling(1.0, 5.0)
    print(f"[TEST 2] Solo Downscaling (1 player on 5-man baseline):")
    print(f"         Ratio: {res2['ratio']:.3f}, HealthScale: {res2['health_scale']:.3f}, DamageScale: {res2['damage_scale']:.3f}")
    assert abs(res2["ratio"] - 0.20) < 1e-4, "Ratio must be 0.20"
    assert abs(res2["health_scale"] - 0.20) < 1e-4, "Health scale must be 0.20"
    assert abs(res2["damage_scale"] - 0.3511) < 1e-2, "Damage scale must be ~0.35"
    print("         -> PASS\n")
    passed += 1

    # Test 3: Scaling Above Baseline (10 players in 5-man dungeon)
    total += 1
    res3 = calculate_power_scaling(10.0, 5.0)
    print(f"[TEST 3] Scaling Above Baseline (10 players on 5-man baseline):")
    print(f"         Ratio: {res3['ratio']:.3f}, HealthScale: {res3['health_scale']:.3f}, DamageScale: {res3['damage_scale']:.3f}")
    assert abs(res3["ratio"] - 2.0) < 1e-4, "Ratio must be 2.0"
    assert abs(res3["health_scale"] - 2.0) < 1e-4, "Health scale must be 2.0"
    assert abs(res3["damage_scale"] - 1.569) < 1e-2, "Damage scale must be ~1.57"
    print("         -> PASS\n")
    passed += 1

    # Test 4: Scaling Above Baseline (25 players in 5-man dungeon)
    total += 1
    res4 = calculate_power_scaling(25.0, 5.0)
    print(f"[TEST 4] Scaling Above Baseline (25 players on 5-man baseline):")
    print(f"         Ratio: {res4['ratio']:.3f}, HealthScale: {res4['health_scale']:.3f}, DamageScale: {res4['damage_scale']:.3f}")
    assert abs(res4["ratio"] - 5.0) < 1e-4, "Ratio must be 5.0"
    assert abs(res4["health_scale"] - 5.0) < 1e-4, "Health scale must be 5.0"
    assert abs(res4["damage_scale"] - 2.848) < 1e-2, "Damage scale must be ~2.85"
    print("         -> PASS\n")
    passed += 1

    # Test 5: Raid Baseline Preservation (40 players on 40-man baseline)
    total += 1
    res5 = calculate_power_scaling(40.0, 40.0)
    print(f"[TEST 5] Raid Baseline Preservation (40 players on 40-man baseline):")
    print(f"         Ratio: {res5['ratio']:.3f}, HealthScale: {res5['health_scale']:.3f}, DamageScale: {res5['damage_scale']:.3f}")
    assert abs(res5["ratio"] - 1.0) < 1e-4, "Ratio must be 1.0"
    assert abs(res5["health_scale"] - 1.0) < 1e-4, "Health scale must be 1.0"
    print("         -> PASS\n")
    passed += 1

    # Test 6: In-Combat Health Percentage Preservation
    total += 1
    orig_max = 1000
    orig_cur = 400  # 40%
    pct = (orig_cur / float(orig_max)) * 100.0
    new_max = 2500
    rescaled = calculate_combat_rescaled_health(new_max, pct)
    print(f"[TEST 6] Combat Health Preservation:")
    print(f"         Orig: {orig_cur}/{orig_max} ({pct:.1f}%), Rescaled: {rescaled}/{new_max} ({(rescaled/float(new_max))*100.0:.1f}%)")
    assert rescaled == 1000, f"Expected 1000, got {rescaled}"
    print("         -> PASS\n")
    passed += 1

    # Test 7: Bot Weighting (1 human + 4 bots @ 0.75 weight)
    total += 1
    eff1 = calculate_effective_players(1, 4, bot_weight=0.75, count_bots=True)
    print(f"[TEST 7] Bot Weighting (1 human + 4 bots @ 0.75 weight):")
    print(f"         Effective: {eff1:.2f} (expected 4.00)")
    assert abs(eff1 - 4.0) < 1e-4, "Effective must be 4.0"
    print("         -> PASS\n")
    passed += 1

    # Test 8: Bot Exclusion
    total += 1
    eff2 = calculate_effective_players(1, 4, bot_weight=0.75, count_bots=False)
    print(f"[TEST 8] Bot Exclusion (1 human + 4 bots with bots disabled):")
    print(f"         Effective: {eff2:.2f} (expected 1.00)")
    assert abs(eff2 - 1.0) < 1e-4, "Effective must be 1.0"
    print("         -> PASS\n")
    passed += 1

    # Test 9: Min Clamping (0 players clamped to Min 1)
    total += 1
    eff3 = calculate_effective_players(0, 0, min_clamp=1, max_clamp=40)
    print(f"[TEST 9] Min Clamping (0 players with MinPlayers=1):")
    print(f"         Effective: {eff3:.2f} (expected 1.00)")
    assert abs(eff3 - 1.0) < 1e-4, "Effective must clamp to 1.0"
    print("         -> PASS\n")
    passed += 1

    # Test 10: Max Clamping (50 players clamped to Max 25)
    total += 1
    eff4 = calculate_effective_players(50, 0, min_clamp=1, max_clamp=25)
    print(f"[TEST 10] Max Clamping (50 players with MaxPlayers=25):")
    print(f"          Effective: {eff4:.2f} (expected 25.00)")
    assert abs(eff4 - 25.0) < 1e-4, "Effective must clamp to 25.0"
    print("          -> PASS\n")
    passed += 1

    print("========================================================")
    print(f"  Summary: {passed}/{total} Tests Passed Successfully (100%)")
    print("========================================================")
    return passed == total

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
