#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <string>
#include <iomanip>

struct ScalingTestResult
{
    float ratio;
    float healthScale;
    float damageScale;
    float armorScale;
    float manaScale;
};

ScalingTestResult CalculatePowerScaling(float effectivePlayers, float baselinePlayers, float hExp, float dExp, float aExp, float mExp)
{
    float ratio = baselinePlayers > 0.0f ? (effectivePlayers / baselinePlayers) : 1.0f;
    if (ratio <= 0.0f)
        ratio = 0.05f;

    ScalingTestResult r;
    r.ratio = ratio;
    r.healthScale = std::pow(ratio, hExp);
    r.damageScale = std::pow(ratio, dExp);
    r.armorScale  = std::pow(ratio, aExp);
    r.manaScale   = std::pow(ratio, mExp);
    return r;
}

uint32_t CalculateCombatRescaledHealth(uint32_t newFinalHealth, float currentHealthPct)
{
    return static_cast<uint32_t>(std::max(1.0f, std::round(newFinalHealth * (currentHealthPct / 100.0f))));
}

float CalculateEffectivePlayers(uint32_t humanCount, uint32_t botCount, float botWeight, bool countBots)
{
    float eff = static_cast<float>(humanCount);
    if (countBots)
        eff += static_cast<float>(botCount) * botWeight;
    return eff;
}

int main()
{
    std::cout << "========================================================\n";
    std::cout << "  AutoBalance Scaling Regression & Verification Tests   \n";
    std::cout << "========================================================\n\n";

    int passed = 0;
    int total = 0;

    // Test 1: 5-Player Dungeon Baseline Preservation (5 players = 1.0x)
    {
        total++;
        auto res = CalculatePowerScaling(5.0f, 5.0f, 1.0f, 0.65f, 0.5f, 1.0f);
        std::cout << "[TEST 1] Baseline Preservation (5 players on 5-man baseline):\n";
        std::cout << "         Ratio: " << res.ratio << ", Health: " << res.healthScale 
                  << ", Damage: " << res.damageScale << "\n";
        assert(std::abs(res.ratio - 1.0f) < 0.001f);
        assert(std::abs(res.healthScale - 1.0f) < 0.001f);
        assert(std::abs(res.damageScale - 1.0f) < 0.001f);
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    // Test 2: Solo Scaling Down Below Baseline (1 player on 5-man baseline)
    {
        total++;
        auto res = CalculatePowerScaling(1.0f, 5.0f, 1.0f, 0.65f, 0.5f, 1.0f);
        std::cout << "[TEST 2] Solo Downscaling (1 player on 5-man baseline):\n";
        std::cout << "         Ratio: " << res.ratio << ", Health: " << res.healthScale 
                  << ", Damage: " << res.damageScale << "\n";
        assert(std::abs(res.ratio - 0.20f) < 0.001f);
        assert(std::abs(res.healthScale - 0.20f) < 0.001f);
        assert(std::abs(res.damageScale - 0.3511f) < 0.01f);
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    // Test 3: Scaling Above Baseline (10 players in 5-man dungeon)
    {
        total++;
        auto res = CalculatePowerScaling(10.0f, 5.0f, 1.0f, 0.65f, 0.5f, 1.0f);
        std::cout << "[TEST 3] Scaling Above Baseline (10 players on 5-man baseline):\n";
        std::cout << "         Ratio: " << res.ratio << ", Health: " << res.healthScale 
                  << ", Damage: " << res.damageScale << "\n";
        assert(std::abs(res.ratio - 2.0f) < 0.001f);
        assert(std::abs(res.healthScale - 2.0f) < 0.001f);
        assert(std::abs(res.damageScale - 1.569f) < 0.01f);
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    // Test 4: Scaling Above Baseline (25 players in 5-man dungeon)
    {
        total++;
        auto res = CalculatePowerScaling(25.0f, 5.0f, 1.0f, 0.65f, 0.5f, 1.0f);
        std::cout << "[TEST 4] Scaling Above Baseline (25 players on 5-man baseline):\n";
        std::cout << "         Ratio: " << res.ratio << ", Health: " << res.healthScale 
                  << ", Damage: " << res.damageScale << "\n";
        assert(std::abs(res.ratio - 5.0f) < 0.001f);
        assert(std::abs(res.healthScale - 5.0f) < 0.001f);
        assert(std::abs(res.damageScale - 2.848f) < 0.01f);
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    // Test 5: Raid Baseline Preservation (40 players on 40-man baseline)
    {
        total++;
        auto res = CalculatePowerScaling(40.0f, 40.0f, 1.0f, 0.65f, 0.5f, 1.0f);
        std::cout << "[TEST 5] Raid Baseline Preservation (40 players on 40-man baseline):\n";
        std::cout << "         Ratio: " << res.ratio << ", Health: " << res.healthScale 
                  << ", Damage: " << res.damageScale << "\n";
        assert(std::abs(res.ratio - 1.0f) < 0.001f);
        assert(std::abs(res.healthScale - 1.0f) < 0.001f);
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    // Test 6: In-Combat Health Percentage Preservation
    {
        total++;
        uint32_t origMaxHealth = 1000;
        uint32_t origCurHealth = 400; // exactly 40%
        float healthPct = (static_cast<float>(origCurHealth) / origMaxHealth) * 100.0f;

        uint32_t newMaxHealth = 2500;
        uint32_t rescaledCurHealth = CalculateCombatRescaledHealth(newMaxHealth, healthPct);
        std::cout << "[TEST 6] Combat Health Preservation:\n";
        std::cout << "         Orig: " << origCurHealth << "/" << origMaxHealth << " (" << healthPct << "%)\n";
        std::cout << "         New:  " << rescaledCurHealth << "/" << newMaxHealth << " (" 
                  << (static_cast<float>(rescaledCurHealth) / newMaxHealth * 100.0f) << "%)\n";
        assert(rescaledCurHealth == 1000); // 40% of 2500 is 1000
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    // Test 7: Bot Weighting Calculation
    {
        total++;
        float eff = CalculateEffectivePlayers(1, 4, 0.75f, true);
        std::cout << "[TEST 7] Bot Weighting (1 human + 4 bots @ 0.75 weight):\n";
        std::cout << "         Effective: " << eff << " (expected 4.0)\n";
        assert(std::abs(eff - 4.0f) < 0.001f);
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    // Test 8: Bot Weighting Exclusion
    {
        total++;
        float eff = CalculateEffectivePlayers(1, 4, 0.75f, false);
        std::cout << "[TEST 8] Bot Exclusion (1 human + 4 bots with bots disabled):\n";
        std::cout << "         Effective: " << eff << " (expected 1.0)\n";
        assert(std::abs(eff - 1.0f) < 0.001f);
        std::cout << "         -> PASS\n\n";
        passed++;
    }

    std::cout << "========================================================\n";
    std::cout << "  Summary: " << passed << "/" << total << " Tests Passed Successfully\n";
    std::cout << "========================================================\n";
    return 0;
}
