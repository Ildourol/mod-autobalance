#include "ABCommandScript.h"

#include "ABConfig.h"
#include "ABCreatureInfo.h"
#include "ABMapInfo.h"
#include "ABUtils.h"
#include "ABWorldScript.h"
#include "Message.h"

bool AutoBalance_CommandScript::HandleABInfoCommand(ChatHandler* handler, const char* /*args*/)
{
    Player* player = handler->GetPlayer();
    if (!player)
        return false;

    Map* map = player->GetMap();
    if (!map || !map->IsDungeon())
    {
        handler->PSendSysMessage("|cff4CFF00[AutoBalance]|r This command can only be used inside a dungeon or raid instance.");
        return true;
    }

    AutoBalanceMapInfo* mapABInfo = GetMapInfo(map);
    InstanceMap* instanceMap = map->ToInstanceMap();

    handler->PSendSysMessage("|cff4CFF00[AutoBalance Instance Info]|r");
    handler->PSendSysMessage("Map: %s (ID: %u, Instance: %u), Difficulty: %s",
        map->GetMapName(),
        map->GetId(),
        map->GetInstanceId(),
        instanceMap && instanceMap->IsHeroic() ? "Heroic" : "Normal");
    handler->PSendSysMessage("Players: Active: %u, Effective: %.2f, Baseline: %u, Adjusted Count: %u",
        mapABInfo->actualPlayersInside,
        mapABInfo->effectiveScalingPlayers,
        mapABInfo->baselinePlayers,
        mapABInfo->adjustedPlayerCount);
    handler->PSendSysMessage("Capacity: Native: %u, Configured Max: %u",
        mapABInfo->originalMapCapacity,
        mapABInfo->configuredMaxCapacity);
    handler->PSendSysMessage("Scaling Multipliers: Health: %.1f%%, Damage: %.1f%%, Armor: %.1f%%, Mana: %.1f%%",
        mapABInfo->healthScale * 100.0f,
        mapABInfo->damageScale * 100.0f,
        mapABInfo->armorScale * 100.0f,
        mapABInfo->manaScale * 100.0f);
    handler->PSendSysMessage("Status: %s, Combat Locked: %s",
        mapABInfo->enabled ? "|cff00ff00Enabled|r" : "|cffff0000Disabled|r",
        mapABInfo->combatLocked ? "|cffff8000Yes|r" : "No");

    return true;
}

bool AutoBalance_CommandScript::HandleABMapCommand(ChatHandler* handler, const char* args)
{
    return HandleABMapStatsCommand(handler, args);
}

bool AutoBalance_CommandScript::HandleABCreatureCommand(ChatHandler* handler, const char* args)
{
    return HandleABCreatureStatsCommand(handler, args);
}

bool AutoBalance_CommandScript::HandleABRecalcCommand(ChatHandler* handler, const char* /*args*/)
{
    Player* player = handler->GetPlayer();
    if (!player)
        return false;

    Map* map = player->GetMap();
    if (!map || !map->IsDungeon())
    {
        handler->PSendSysMessage("|cff4CFF00[AutoBalance]|r This command can only be used inside an instance.");
        return true;
    }

    UpdateMapPlayerStats(map);
    UpdateMapDataIfNeeded(map, true);

    AutoBalanceMapInfo* mapABInfo = GetMapInfo(map);
    handler->PSendSysMessage("|cff4CFF00[AutoBalance]|r Instance recalculation forced.");
    handler->PSendSysMessage("Active Players: %u, Baseline: %u, Adjusted Count: %u, Health Scale: %.1f%%, Damage Scale: %.1f%%",
        mapABInfo->actualPlayersInside,
        mapABInfo->baselinePlayers,
        mapABInfo->adjustedPlayerCount,
        mapABInfo->healthScale * 100.0f,
        mapABInfo->damageScale * 100.0f);

    return true;
}

bool AutoBalance_CommandScript::HandleABReloadCommand(ChatHandler* handler, const char* /*args*/)
{
    handler->PSendSysMessage("|cff4CFF00[AutoBalance]|r Reloading module configuration...");
    AutoBalance_WorldScript worldScript;
    worldScript.OnBeforeConfigLoad(true);
    handler->PSendSysMessage("|cff4CFF00[AutoBalance]|r Module configuration successfully reloaded.");
    return true;
}

bool AutoBalance_CommandScript::HandleABSetOffsetCommand(ChatHandler* handler, const char* args)
{
    if (!*args)
    {
        handler->PSendSysMessage(".autobalance setoffset #");
        handler->PSendSysMessage(ABGetLocaleText(handler->GetSession()->GetSessionDbLocaleIndex(), "set_offset_command_description").c_str());
        return false;
    }
    char* offset = strtok((char*)args, " ");
    int32 offseti = -1;

    if (offset)
    {
        offseti = (uint32)atoi(offset);
        std::vector<std::string> args = { std::to_string(offseti) };
        handler->PSendSysMessage(ABGetLocaleText(handler->GetSession()->GetSessionDbLocaleIndex(), "set_offset_command_success").c_str(), offseti);
        PlayerCountDifficultyOffset = offseti;
        globalConfigTime = GetCurrentConfigTime();
        return true;
    }
    else
        handler->PSendSysMessage(ABGetLocaleText(handler->GetSession()->GetSessionDbLocaleIndex(), "set_offset_command_error").c_str());
    return false;
}

bool AutoBalance_CommandScript::HandleABGetOffsetCommand(ChatHandler* handler, const char* /*args*/)
{
    handler->PSendSysMessage(ABGetLocaleText(handler->GetSession()->GetSessionDbLocaleIndex(), "get_offset_command_success").c_str(), PlayerCountDifficultyOffset);
    return true;
}

bool AutoBalance_CommandScript::HandleABMapStatsCommand(ChatHandler* handler, const char* /*args*/)
{
    Player* player = handler->GetPlayer();
    auto locale = handler->GetSession()->GetSessionDbLocaleIndex();

    AutoBalanceMapInfo* mapABInfo = GetMapInfo(player->GetMap());

    if (player->GetMap()->IsDungeon())
    {
        handler->PSendSysMessage("---");
        // Map basics
        handler->PSendSysMessage("{} ({}-player {}) | ID {}-{}{}",
            player->GetMap()->GetMapName(),
            player->GetMap()->ToInstanceMap()->GetMaxPlayers(),
            player->GetMap()->ToInstanceMap()->IsHeroic() ? "Heroic" : "Normal",
            player->GetMapId(),
            player->GetInstanceId(),
            mapABInfo->enabled ? "" : " | AutoBalance DISABLED");

        // if the map isn't enabled, don't display anything else
        // if (!mapABInfo->enabled) { return true; }

        // Player stats
        handler->PSendSysMessage("Players on map: {} (Lvl {} - {})",
            mapABInfo->playerCount,
            mapABInfo->lowestPlayerLevel,
            mapABInfo->highestPlayerLevel
        );

        // Adjusted player count (multiple scenarios)
        if (mapABInfo->combatLockTripped)
            handler->PSendSysMessage(ABGetLocaleText(locale, "adjusted_player_count_combat_locked").c_str(), mapABInfo->adjustedPlayerCount);
        else if (mapABInfo->playerCount < mapABInfo->minPlayers && !PlayerCountDifficultyOffset)
            handler->PSendSysMessage(ABGetLocaleText(locale, "adjusted_player_count_map_minimum").c_str(), mapABInfo->adjustedPlayerCount);
        else if (mapABInfo->playerCount < mapABInfo->minPlayers && PlayerCountDifficultyOffset)
            handler->PSendSysMessage(ABGetLocaleText(locale, "adjusted_player_count_map_minimum_difficulty_offset").c_str(), mapABInfo->adjustedPlayerCount, PlayerCountDifficultyOffset);
        else if (PlayerCountDifficultyOffset)
            handler->PSendSysMessage(ABGetLocaleText(locale, "adjusted_player_count_difficulty_offset").c_str(), mapABInfo->adjustedPlayerCount, PlayerCountDifficultyOffset);
        else
            handler->PSendSysMessage(ABGetLocaleText(locale, "adjusted_player_count").c_str(), mapABInfo->adjustedPlayerCount);

        // LFG levels
        handler->PSendSysMessage(ABGetLocaleText(locale, "lfg_range").c_str(), mapABInfo->lfgMinLevel, mapABInfo->lfgMaxLevel, mapABInfo->lfgTargetLevel);

        // Calculated map level (creature average)
        handler->PSendSysMessage(ABGetLocaleText(locale, "map_level").c_str(),
            (uint8)(mapABInfo->avgCreatureLevel + 0.5f),
            mapABInfo->isLevelScalingEnabled && mapABInfo->enabled ? "->" + std::to_string(mapABInfo->highestPlayerLevel) + ABGetLocaleText(locale, "level_scaling_enabled").c_str() : ABGetLocaleText(locale, "level_scaling_disabled").c_str()
        );

        // World Health Multiplier
        handler->PSendSysMessage(ABGetLocaleText(locale, "world_health_multiplier").c_str(), mapABInfo->worldHealthMultiplier);

        // World Damage and Healing Multiplier
        if (mapABInfo->worldDamageHealingMultiplier != mapABInfo->scaledWorldDamageHealingMultiplier)
        {
            handler->PSendSysMessage(ABGetLocaleText(locale, "world_hostile_damage_healing_multiplier_to").c_str(),
                mapABInfo->worldDamageHealingMultiplier,
                mapABInfo->scaledWorldDamageHealingMultiplier
            );
        }
        else
        {
            handler->PSendSysMessage(ABGetLocaleText(locale, "world_hostile_damage_healing_multiplier").c_str(),
                mapABInfo->worldDamageHealingMultiplier
            );
        }

        // Creature Stats
        handler->PSendSysMessage(ABGetLocaleText(locale, "original_creature_level_range").c_str(),
            mapABInfo->lowestCreatureLevel,
            mapABInfo->highestCreatureLevel,
            mapABInfo->avgCreatureLevel
        );
        handler->PSendSysMessage(ABGetLocaleText(locale, "active_total_creatures_in_map").c_str(),
            mapABInfo->activeCreatureCount,
            mapABInfo->allMapCreatures.size()
        );

        return true;
    }
    else
    {
        handler->PSendSysMessage(ABGetLocaleText(locale, "ab_command_only_in_instance").c_str());
        return false;
    }
}

bool AutoBalance_CommandScript::HandleABCreatureStatsCommand(ChatHandler* handler, const char* /*args*/)
{
    Creature* target = handler->getSelectedCreature();

    auto locale = handler->GetSession()->GetSessionDbLocaleIndex();

    if (!target)
    {
        handler->SendSysMessage(LANG_SELECT_CREATURE);
        handler->SetSentErrorMessage(true);
        return false;
    }
    else if (!target->GetMap()->IsDungeon())
    {
        handler->PSendSysMessage(ABGetLocaleText(locale, "target_no_in_instance").c_str());
        handler->SetSentErrorMessage(true);
        return false;
    }

    AutoBalanceCreatureInfo* targetABInfo = target->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");

    handler->PSendSysMessage("---");
    handler->PSendSysMessage("{} ({}{}{}), {}",
        target->GetName(),
        targetABInfo->UnmodifiedLevel,
        isCreatureRelevant(target) && targetABInfo->UnmodifiedLevel != target->GetLevel() ? "->" + std::to_string(targetABInfo->selectedLevel) : "",
        isBossOrBossSummon(target) ? " | Boss" : "",
        targetABInfo->isActive ? ABGetLocaleText(locale, "active_for_map_stats").c_str() : ABGetLocaleText(locale, "ignored_for_map_stats").c_str());
    handler->PSendSysMessage(ABGetLocaleText(locale, "creature_difficulty_level").c_str(), targetABInfo->instancePlayerCount);

    // summon
    if (target->IsSummon() && targetABInfo->summoner && targetABInfo->isCloneOfSummoner)
        handler->PSendSysMessage(ABGetLocaleText(locale, "clone_of_summon").c_str(), targetABInfo->summonerName, targetABInfo->summonerLevel);
    else if (target->IsSummon() && targetABInfo->summoner)
        handler->PSendSysMessage(ABGetLocaleText(locale, "summon_of_summon").c_str(), targetABInfo->summonerName, targetABInfo->summonerLevel);
    else if (target->IsSummon())
        handler->PSendSysMessage(ABGetLocaleText(locale, "summon_without_summoner").c_str());

    // level scaled
    if (targetABInfo->UnmodifiedLevel != target->GetLevel())
    {
        handler->PSendSysMessage(ABGetLocaleText(locale, "health_multiplier_to").c_str(), targetABInfo->HealthMultiplier, targetABInfo->ScaledHealthMultiplier);
        handler->PSendSysMessage(ABGetLocaleText(locale, "mana_multiplier_to").c_str(), targetABInfo->ManaMultiplier, targetABInfo->ScaledManaMultiplier);
        handler->PSendSysMessage(ABGetLocaleText(locale, "armor_multiplier_to").c_str(), targetABInfo->ArmorMultiplier, targetABInfo->ScaledArmorMultiplier);
        handler->PSendSysMessage(ABGetLocaleText(locale, "damage_multiplier_to").c_str(), targetABInfo->DamageMultiplier, targetABInfo->ScaledDamageMultiplier);
    }
    // not level scaled
    else
    {
        handler->PSendSysMessage(ABGetLocaleText(locale, "health_multiplier").c_str(), targetABInfo->HealthMultiplier);
        handler->PSendSysMessage(ABGetLocaleText(locale, "mana_multiplier").c_str(), targetABInfo->ManaMultiplier);
        handler->PSendSysMessage(ABGetLocaleText(locale, "armor_multiplier").c_str(), targetABInfo->ArmorMultiplier);
        handler->PSendSysMessage(ABGetLocaleText(locale, "damage_multiplier").c_str(), targetABInfo->DamageMultiplier);
    }
    handler->PSendSysMessage(ABGetLocaleText(locale, "cc_duration_multiplier").c_str(), targetABInfo->CCDurationMultiplier);
    handler->PSendSysMessage(ABGetLocaleText(locale, "xp_money_multiplier").c_str(), targetABInfo->XPModifier, targetABInfo->MoneyModifier);

    return true;
}
