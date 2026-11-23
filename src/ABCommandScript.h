/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE
 */

#ifndef __AB_COMMAND_SCRIPT_H
#define __AB_COMMAND_SCRIPT_H

#include "Chat.h"
#include "Config.h"
#include "ScriptMgr.h"

using namespace Acore::ChatCommands;

class AutoBalance_CommandScript : public CommandScript
{
public:
    AutoBalance_CommandScript() : CommandScript("AutoBalance_CommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable ABCommandTable =
        {
            { "info",          HandleABInfoCommand,           SEC_PLAYER,      Console::Yes },
            { "map",           HandleABMapCommand,            SEC_PLAYER,      Console::Yes },
            { "creature",      HandleABCreatureCommand,       SEC_PLAYER,      Console::Yes },
            { "recalc",        HandleABRecalcCommand,         SEC_GAMEMASTER,  Console::Yes },
            { "reload",        HandleABReloadCommand,         SEC_ADMINISTRATOR, Console::Yes },
            { "setoffset",     HandleABSetOffsetCommand,      SEC_GAMEMASTER,  Console::Yes },
            { "getoffset",     HandleABGetOffsetCommand,      SEC_PLAYER,      Console::Yes },
            { "mapstat",       HandleABMapStatsCommand,       SEC_PLAYER,      Console::Yes },
            { "creaturestat",  HandleABCreatureStatsCommand,  SEC_PLAYER,      Console::Yes }
        };

        static ChatCommandTable commandTable =
        {
            { "autobalance",  ABCommandTable },
            { "ab",           ABCommandTable },
        };

        return commandTable;
    };

    static bool HandleABInfoCommand(ChatHandler* handler, const char* args);
    static bool HandleABMapCommand(ChatHandler* handler, const char* args);
    static bool HandleABCreatureCommand(ChatHandler* handler, const char* args);
    static bool HandleABRecalcCommand(ChatHandler* handler, const char* args);
    static bool HandleABReloadCommand(ChatHandler* handler, const char* args);
    static bool HandleABSetOffsetCommand(ChatHandler* handler, const char* args);
    static bool HandleABGetOffsetCommand(ChatHandler* handler, const char* args);
    static bool HandleABMapStatsCommand(ChatHandler* handler, const char* args);
    static bool HandleABCreatureStatsCommand(ChatHandler* handler, const char* args);
};

#endif /* __AB_COMMAND_SCRIPT_H */
