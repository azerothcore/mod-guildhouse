#ifndef MOD_GUILDHOUSE_H
#define MOD_GUILDHOUSE_H

#include <string>

// Offsets from creatures_objects.sql
constexpr uint32 GetCreatureEntry(uint32 offset)
{
    return 500030 + offset;
}

constexpr uint32 GetGameObjectEntry(uint32 offset)
{
    return 500000 + offset;
}

enum GuildHouseLocaleString : uint32
{
    GUILDHOUSE_TEXT_YOU_NOW_OWN_A_GUILD          = 1,
    GUILDHOUSE_TEXT_NOT_IN_GUILD                 = 2,
    GUILDHOUSE_TEXT_GUILD_HAS_NO_HOUSE           = 3,
    GUILDHOUSE_TEXT_HOUSE_SOLD_SUCCESS           = 4,
    GUILDHOUSE_TEXT_HOUSE_SOLD_ERROR             = 5,
    GUILDHOUSE_TEXT_HOUSE_PURCHASED_SUCCESS      = 6,
    GUILDHOUSE_TEXT_GUILD_ALREADY_HAS_HOUSE      = 7,

    GUILDHOUSE_TEXT_CMD_NEED_GUILDMASTER         = 8,
    GUILDHOUSE_TEXT_CMD_NEED_IN_GUILDHOUSE       = 9,
    GUILDHOUSE_TEXT_CMD_BUTLER_ALREADY_EXISTS    = 10,
    GUILDHOUSE_TEXT_CMD_BUTLER_ADD_ERROR         = 11,
    GUILDHOUSE_TEXT_CMD_IN_COMBAT                = 12,

    GUILDHOUSE_TEXT_BROADCAST_HOUSE_SOLD         = 20,
    GUILDHOUSE_TEXT_BROADCAST_HOUSE_PURCHASED    = 21,
    GUILDHOUSE_TEXT_BROADCAST_USE_TELEPORT       = 22,

    GUILDHOUSE_TEXT_NOT_AUTHORIZED_PURCHASE      = 30,
    GUILDHOUSE_TEXT_OBJECT_ALREADY_EXISTS        = 31,

    GUILDHOUSE_TEXT_GOSSIP_BUY_HOUSE             = 100,
    GUILDHOUSE_TEXT_GOSSIP_SELL_HOUSE_CONFIRM    = 101,
    GUILDHOUSE_TEXT_GOSSIP_TELEPORT_TO_HOUSE     = 102,
    GUILDHOUSE_TEXT_GOSSIP_CLOSE                 = 103,
    GUILDHOUSE_TEXT_GOSSIP_SELL_HOUSE            = 104
};

class Player;

// Returns localized text for the given id and player locale
std::string GetGuildHouseLocaleText(uint32 id, Player* player);

 #endif // MOD_GUILDHOUSE_H