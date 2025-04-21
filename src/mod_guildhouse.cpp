#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Creature.h"
#include "Guild.h"
#include "SpellAuraEffects.h"
#include "Chat.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GuildMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "DataMap.h"
#include "GameObject.h"
#include "Transport.h"
#include "Maps/MapMgr.h"

// Add global variable for GuildHousePhaseBase
int GuildHousePhaseBase = 10000; // default, will be overwritten by config

class GuildData : public DataMap::Base
{
public:
    GuildData() {}
    GuildData(uint32 phase, float posX, float posY, float posZ, float ori) : phase(phase), posX(posX), posY(posY), posZ(posZ), ori(ori) {}
    uint32 phase;
    float posX;
    float posY;
    float posZ;
    float ori;
};

struct LastPosition : public DataMap::Base
{
    uint32 mapId = 0;
    float x = 0, y = 0, z = 0, ori = 0;
};

class GuildHelper : public GuildScript
{

public:
    GuildHelper() : GuildScript("GuildHelper") {}

    void OnCreate(Guild * /*guild*/, Player *leader, const std::string & /*name*/)
    {
        ChatHandler(leader->GetSession()).PSendSysMessage("You now own a guild. You can purchase a Guild House!");
    }

    uint32 GetGuildPhase(Guild *guild)
    {
        return GuildHousePhaseBase + guild->GetId();
    }

    void OnDisband(Guild *guild)
    {

        if (RemoveGuildHouse(guild))
        {
            LOG_INFO("modules", "GUILDHOUSE: Deleting Guild House data due to disbanding of guild...");
        }
        else
        {
            LOG_INFO("modules", "GUILDHOUSE: Error deleting Guild House data during disbanding of guild!!");
        }
    }

    bool RemoveGuildHouse(Guild *guild)
    {
        uint32 guildPhase = GetGuildPhase(guild);
        QueryResult CreatureResult;
        QueryResult GameobjResult;

        // Lets find all of the gameobjects to be removed
        GameobjResult = WorldDatabase.Query("SELECT `guid` FROM `gameobject` WHERE `map`=1 AND `phaseMask`={}", guildPhase);
        // Lets find all of the creatures to be removed
        CreatureResult = WorldDatabase.Query("SELECT `guid` FROM `creature` WHERE `map`=1 AND `phaseMask`={}", guildPhase);

        Map *map = sMapMgr->FindMap(1, 0);
        if (!map)
            return false;

        // Remove creatures from the deleted guild house map
        if (CreatureResult)
        {
            do
            {
                Field *fields = CreatureResult->Fetch();
                uint32 lowguid = fields[0].Get<int32>();
                if (CreatureData const *cr_data = sObjectMgr->GetCreatureData(lowguid))
                {
                    Creature *creature = map->GetCreature(ObjectGuid::Create<HighGuid::Unit>(cr_data->id1, lowguid));
                    if (creature)
                    {
                        creature->CombatStop();
                        creature->DeleteFromDB();
                        creature->AddObjectToRemoveList();
                        // Do not use 'creature' after this point!
                        creature = nullptr;
                    }
                }
            } while (CreatureResult->NextRow());
        }

        // Remove gameobjects from the deleted guild house map
        if (GameobjResult)
        {
            do
            {
                Field *fields = GameobjResult->Fetch();
                uint32 lowguid = fields[0].Get<int32>();
                if (GameObjectData const *go_data = sObjectMgr->GetGameObjectData(lowguid))
                {
                    if (GameObject *gobject = map->GetGameObject(ObjectGuid::Create<HighGuid::GameObject>(go_data->id, lowguid)))
                    {
                        gobject->SetRespawnTime(0);
                        gobject->Delete();
                        gobject->DeleteFromDB();
                        gobject->CleanupsBeforeDelete();
                        // delete gobject;
                    }
                }

            } while (GameobjResult->NextRow());
        }

        // Delete actual guild_house data from characters database
        CharacterDatabase.Query("DELETE FROM `guild_house` WHERE `guild`={}", guild->GetId());

        // Ensure all purchases are cleared for this guild
        WorldDatabase.Execute("DELETE FROM guild_house_purchases WHERE guild={}", guild->GetId());

        return true;
    }
};

class GuildHouseSeller : public CreatureScript
{

public:
    GuildHouseSeller() : CreatureScript("GuildHouseSeller") {}

    struct GuildHouseSellerAI : public ScriptedAI
    {
        GuildHouseSellerAI(Creature *creature) : ScriptedAI(creature) {}

        void UpdateAI(uint32 /*diff*/) override
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }
    };

    CreatureAI *GetAI(Creature *creature) const override
    {
        return new GuildHouseSellerAI(creature);
    }

    bool OnGossipHello(Player *player, Creature *creature) override
    {
        if (!player || !creature)
            return false;

        if (!player->GetGuild())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You are not a member of a guild.");
            CloseGossipMenuFor(player);
            return false;
        }

        QueryResult has_gh = CharacterDatabase.Query("SELECT id, `guild` FROM `guild_house` WHERE guild = {}", player->GetGuildId());

        // Only show Teleport option if guild owns a guild house
        if (has_gh)
        {
            AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Teleport to Guild House", GOSSIP_SENDER_MAIN, 1);

            Guild *guild = sGuildMgr->GetGuildById(player->GetGuildId());
            if (!guild)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Error: Guild not found.");
                CloseGossipMenuFor(player);
                return false;
            }
            Guild::Member const *memberMe = guild->GetMember(player->GetGUID());
            if (memberMe && memberMe->IsRankNotLower(sConfigMgr->GetOption<int32>("GuildHouseSellRank", 0)))
            {
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell Guild House!", GOSSIP_SENDER_MAIN, 3, "Are you sure you want to sell your Guild House?", 0, false);
            }

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Essential NPCs", GOSSIP_SENDER_MAIN, 10021, "Are you sure you want to buy all essential NPCs?", 0, false);
        }
        else
        {
            Guild *guild = player->GetGuild();
            if (guild && guild->GetLeaderGUID() == player->GetGUID())
            {
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy Guild House!", GOSSIP_SENDER_MAIN, 2);
            }
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Close", GOSSIP_SENDER_MAIN, 5);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *creature, uint32 /*sender*/, uint32 action) override
    {
        if (!player || !creature)
            return false;

        uint32 map;
        float posX;
        float posY;
        float posZ;
        float ori;

        switch (action)
        {
        case 100: // GM Island
            map = 1;
            posX = 16222.972f;
            posY = 16267.802f;
            posZ = 13.136777f;
            ori = 1.461173f;
            break;
        case 5: // close
            CloseGossipMenuFor(player);
            break;
        case 4: // --- MORE TO COME ---
            BuyGuildHouse(player->GetGuild(), player, creature);
            break;
        case 3: // sell back guild house
        {
            QueryResult has_gh = CharacterDatabase.Query("SELECT id, `guild` FROM `guild_house` WHERE guild={}", player->GetGuildId());
            if (!has_gh)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not own a Guild House!");
                CloseGossipMenuFor(player);
                return false;
            }

            if (RemoveGuildHouse(player))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You have successfully sold your Guild House.");
                Guild *guild = player->GetGuild();
                if (guild)
                    guild->BroadcastToGuild(player->GetSession(), false, "We just sold our Guild House.", LANG_UNIVERSAL);

                int houseCost = sConfigMgr->GetOption<int32>("CostGuildHouse", 10000000);
                player->ModifyMoney(houseCost / 2);

                int totalItemRefund = 0;
                QueryResult purchases = WorldDatabase.Query(
                    "SELECT entry, type FROM guild_house_purchases WHERE guild={}", player->GetGuildId());
                if (purchases)
                {
                    do
                    {
                        Field *f = purchases->Fetch();
                        uint32 entry = f[0].Get<uint32>();
                        std::string type = f[1].Get<std::string>();
                        int costValue = 0;

                        if (type == "creature")
                        {
                            switch (entry)
                            {
                            case 6930:
                                costValue = sConfigMgr->GetOption<int32>("GuildHouseInnKeeper", 1000000);
                                break;
                            case 28690:
                                costValue = sConfigMgr->GetOption<int32>("GuildHouseVendor", 500000);
                                break;
                            case 9858:
                            case 8719:
                            case 9856:
                                costValue = sConfigMgr->GetOption<int32>("GuildHouseAuctioneer", 500000);
                                break;
                            case 6491:
                                costValue = sConfigMgr->GetOption<int32>("GuildHouseSpirit", 100000);
                                break;
                            default:
                                costValue = sConfigMgr->GetOption<int32>("GuildHouseVendor", 500000);
                                break;
                            }
                        }
                        else if (type == "gameobject")
                        {
                            costValue = (entry == 184137)
                                            ? sConfigMgr->GetOption<int32>("GuildHouseMailbox", 500000)
                                            : sConfigMgr->GetOption<int32>("GuildHouseObject", 500000);
                        }
                        totalItemRefund += costValue * sConfigMgr->GetOption<int32>("GuildHouseRefundPercent", 50) / 100;

                        WorldDatabase.Execute(
                            "DELETE FROM guild_house_purchases WHERE guild={} AND entry={}",
                            player->GetGuildId(), entry);
                    } while (purchases->NextRow());
                }

                if (totalItemRefund > 0)
                {
                    player->ModifyMoney(totalItemRefund);
                    ChatHandler(player->GetSession()).PSendSysMessage("Refunded %u g for purchased items.", totalItemRefund / 10000);
                }

                WorldDatabase.Execute("DELETE FROM guild_house_purchases WHERE guild={}", player->GetGuildId());

                LOG_INFO("modules", "GUILDHOUSE: Successfully returned money and sold Guild House");
                CloseGossipMenuFor(player);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("There was an error selling your Guild House.");
                CloseGossipMenuFor(player);
            }
            break;
        }
        case 2: // buy guild house
            BuyGuildHouse(player->GetGuild(), player, creature);
            break;
        case 1: // teleport to guild house
            TeleportGuildHouse(player->GetGuild(), player, creature);
            break;
        case 10021: // Buy All Essential NPCs
        {
            std::vector<uint32> essentialNpcEntries = {6930, 28690, 9858, 8719, 9856, 6491};
            for (uint32 entry : essentialNpcEntries)
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), entry);
            }
            break;
        }
        default:
            CloseGossipMenuFor(player);
            break;
        }

        if (action >= 100)
        {
            if (!player->GetGuild())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You are not in a guild.");
                CloseGossipMenuFor(player);
                return false;
            }
            CharacterDatabase.Query("INSERT INTO `guild_house` (guild, phase, map, positionX, positionY, positionZ, orientation) VALUES ({}, {}, {}, {}, {}, {}, {})", player->GetGuildId(), GetGuildPhase(player), map, posX, posY, posZ, ori);
            player->ModifyMoney(-(sConfigMgr->GetOption<int32>("CostGuildHouse", 10000000)));
            ChatHandler(player->GetSession()).PSendSysMessage("You have successfully purchased a Guild House");
            Guild *guild = player->GetGuild();
            if (guild)
            {
                guild->BroadcastToGuild(player->GetSession(), false, "We now have a Guild House!", LANG_UNIVERSAL);
                guild->BroadcastToGuild(player->GetSession(), false, "In chat, type `.guildhouse teleport` or `.gh tele` to meet me there!", LANG_UNIVERSAL);
            }
            LOG_INFO("modules", "GUILDHOUSE: GuildId: '{}' has purchased a guildhouse", player->GetGuildId());

            SpawnStarterPortal(player);
            SpawnButlerNPC(player);
            CloseGossipMenuFor(player);
        }

        return true;
    }

    uint32 GetGuildPhase(Player *player)
    {
        return GuildHousePhaseBase + player->GetGuildId();
    }

    bool RemoveGuildHouse(Player *player)
    {
        if (!player)
            return false;

        uint32 guildPhase = GetGuildPhase(player);
        QueryResult CreatureResult;
        QueryResult GameobjResult;
        Map *map = sMapMgr->FindMap(1, 0);
        if (!map)
            return false;

        GameobjResult = WorldDatabase.Query("SELECT `guid` FROM `gameobject` WHERE `map` = 1 AND `phaseMask` = '{}'", guildPhase);
        CreatureResult = WorldDatabase.Query("SELECT `guid` FROM `creature` WHERE `map` = 1 AND `phaseMask` = '{}'", guildPhase);

        if (CreatureResult)
        {
            do
            {
                Field *fields = CreatureResult->Fetch();
                uint32 lowguid = fields[0].Get<uint32>();
                if (CreatureData const *cr_data = sObjectMgr->GetCreatureData(lowguid))
                {
                    Creature *creature = map->GetCreature(ObjectGuid::Create<HighGuid::Unit>(cr_data->id1, lowguid));
                    if (creature)
                    {
                        creature->CombatStop();
                        creature->DeleteFromDB();
                        creature->AddObjectToRemoveList();
                        creature = nullptr;
                    }
                }
            } while (CreatureResult->NextRow());
        }

        if (GameobjResult)
        {
            do
            {
                Field *fields = GameobjResult->Fetch();
                uint32 lowguid = fields[0].Get<uint32>();
                if (GameObjectData const *go_data = sObjectMgr->GetGameObjectData(lowguid))
                {
                    GameObject *gobject = map->GetGameObject(ObjectGuid::Create<HighGuid::GameObject>(go_data->id, lowguid));
                    if (gobject)
                    {
                        gobject->SetRespawnTime(0);
                        gobject->Delete();
                        gobject->DeleteFromDB();
                        gobject->CleanupsBeforeDelete();
                    }
                }
            } while (GameobjResult->NextRow());
        }

        CharacterDatabase.Query("DELETE FROM `guild_house` WHERE `guild`={}", player->GetGuildId());
        WorldDatabase.Execute("DELETE FROM guild_house_purchases WHERE guild={}", player->GetGuildId());

        return true;
    }

    void SpawnStarterPortal(Player *player)
    {
        if (!player)
            return;
        std::vector<uint32> portalEntries = {500000, 500004};
        float posX, posY, posZ, ori;
        Map *map = sMapMgr->FindMap(1, 0);
        if (!map)
            return;

        for (uint32 entry : portalEntries)
        {
            QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);
            if (!result)
            {
                LOG_INFO("modules", "GUILDHOUSE: Unable to find data on portal for entry: {}", entry);
                continue;
            }
            Field *fields = result->Fetch();
            posX = fields[0].Get<float>();
            posY = fields[1].Get<float>();
            posZ = fields[2].Get<float>();
            ori = fields[3].Get<float>();

            uint32 objectId = entry;
            if (!objectId)
            {
                LOG_INFO("modules", "GUILDHOUSE: objectId IS NULL, should be '{}'", entry);
                continue;
            }

            const GameObjectTemplate *objectInfo = sObjectMgr->GetGameObjectTemplate(objectId);
            if (!objectInfo)
            {
                LOG_INFO("modules", "GUILDHOUSE: objectInfo is NULL!");
                continue;
            }
            if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
            {
                LOG_INFO("modules", "GUILDHOUSE: Unable to find displayId??");
                continue;
            }

            GameObject *object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
            ObjectGuid::LowType guidLow = player->GetMap()->GenerateLowGuid<HighGuid::GameObject>();

            if (!object->Create(guidLow, objectInfo->entry, map, GetGuildPhase(player), posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
            {
                delete object;
                LOG_INFO("modules", "GUILDHOUSE: Unable to create object!!");
                continue;
            }

            object->SaveToDB(sMapMgr->FindMap(1, 0)->GetId(), (1 << sMapMgr->FindMap(1, 0)->GetSpawnMode()), GetGuildPhase(player));
            guidLow = object->GetSpawnId();
            delete object;

            object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
            if (!object->LoadGameObjectFromDB(guidLow, sMapMgr->FindMap(1, 0), true))
            {
                delete object;
                continue;
            }
            sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGameObjectData(guidLow));
        }
        CloseGossipMenuFor(player);
    }

    void SpawnButlerNPC(Player *player)
    {
        if (!player)
            return;
        uint32 entry = 500031;
        float posX = 16202.185547f;
        float posY = 16255.916992f;
        float posZ = 21.160221f;
        float ori = 6.195375f;

        Map *map = sMapMgr->FindMap(1, 0);
        if (!map)
            return;
        Creature *creature = new Creature();

        if (!creature->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, player->GetPhaseMaskForSpawn(), entry, 0, posX, posY, posZ, ori))
        {
            delete creature;
            return;
        }
        creature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), GetGuildPhase(player));
        uint32 lowguid = creature->GetSpawnId();

        creature->CleanupsBeforeDelete();
        delete creature;
        creature = new Creature();
        if (!creature->LoadCreatureFromDB(lowguid, map))
        {
            delete creature;
            return;
        }

        sObjectMgr->AddCreatureToGrid(lowguid, sObjectMgr->GetCreatureData(lowguid));
        return;
    }

    bool BuyGuildHouse(Guild *guild, Player *player, Creature *creature)
    {
        if (!guild || !player || !creature)
            return false;

        QueryResult result = CharacterDatabase.Query("SELECT `id`, `guild` FROM `guild_house` WHERE `guild`={}", guild->GetId());

        if (result)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Your guild already has a Guild House.");
            CloseGossipMenuFor(player);
            return false;
        }

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "GM Island", GOSSIP_SENDER_MAIN, 100, "Buy Guild House on GM Island?", sConfigMgr->GetOption<int32>("CostGuildHouse", 10000000), false);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    void TeleportGuildHouse(Guild *guild, Player *player, Creature *creature)
    {
        if (!guild || !player)
            return;

        GuildData *guildData = player->CustomData.GetDefault<GuildData>("phase");
        QueryResult result = CharacterDatabase.Query("SELECT `phase`, `map`,`positionX`, `positionY`, `positionZ`, `orientation` FROM `guild_house` WHERE `guild`={}", guild->GetId());

        if (!result)
        {
            ClearGossipMenuFor(player);
            if (player->GetGuild() && player->GetGuild()->GetLeaderGUID() == player->GetGUID())
            {
                AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Buy Guild House!", GOSSIP_SENDER_MAIN, 2);
                AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Sell Guild House!", GOSSIP_SENDER_MAIN, 3, "Are you sure you want to sell your Guild House?", 0, false);
            }

            AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Teleport to Guild House", GOSSIP_SENDER_MAIN, 1);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Close", GOSSIP_SENDER_MAIN, 5);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            ChatHandler(player->GetSession()).PSendSysMessage("Your Guild does not own a Guild House");
            return;
        }

        do
        {
            Field *fields = result->Fetch();
            guildData->phase = fields[0].Get<uint32>();
            uint32 map = fields[1].Get<uint32>();
            guildData->posX = fields[2].Get<float>();
            guildData->posY = fields[3].Get<float>();
            guildData->posZ = fields[4].Get<float>();
            guildData->ori = fields[5].Get<float>();

            player->TeleportTo(map, guildData->posX, guildData->posY, guildData->posZ, guildData->ori);

        } while (result->NextRow());
    }
};

class GuildHousePlayerScript : public PlayerScript
{
public:
    GuildHousePlayerScript() : PlayerScript("GuildHousePlayerScript") {}

    void OnPlayerLogin(Player *player)
    {
        CheckPlayer(player);
        if (sConfigMgr->GetOption<bool>("GuildHouseEnabled", true))
        {
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the Guild House module.");
            ChatHandler(player->GetSession()).SendSysMessage("Type `.guildhouse` or `.gh` to see the options.");
        }
    }

    void OnPlayerUpdateZone(Player *player, uint32 newZone, uint32 /*newArea*/)
    {
        if (newZone == 876)
            CheckPlayer(player);
        else
            player->SetPhaseMask(GetNormalPhase(player), true);
    }

    bool OnPlayerBeforeTeleport(Player *player, uint32 mapid, float x, float y, float z, float orientation, uint32 options, Unit *target)
    {
        (void)mapid;
        (void)x;
        (void)y;
        (void)z;
        (void)orientation;
        (void)options;
        (void)target;

        if (player->GetZoneId() == 876 && player->GetAreaId() == 876) // GM Island
        {
            // Remove the rested state when teleporting from the guild house
            player->RemoveRestState();
        }

        return true;
    }

    uint32 GetNormalPhase(Player *player) const
    {
        if (player->IsGameMaster())
            return PHASEMASK_ANYWHERE;

        uint32 phase = player->GetPhaseByAuras();
        if (!phase)
            return PHASEMASK_NORMAL;
        else
            return phase;
    }

    void CheckPlayer(Player *player)
    {
        if (!player)
            return;

        GuildData *guildData = player->CustomData.GetDefault<GuildData>("phase");
        QueryResult result = CharacterDatabase.Query("SELECT `id`, `guild`, `phase`, `map`,`positionX`, `positionY`, `positionZ`, `orientation` FROM guild_house WHERE `guild` = {}", player->GetGuildId());

        if (result)
        {
            do
            {
                Field *fields = result->Fetch();
                guildData->phase = fields[2].Get<uint32>();
            } while (result->NextRow());
        }

        if (player->GetZoneId() == 876 && player->GetAreaId() == 876) // GM Island
        {
            player->SetRestState(0);

            if (!result || !player->GetGuild())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not own a Guild House.");
                teleportToDefault(player);
                return;
            }

            player->SetPhaseMask(guildData->phase, true);
        }
        else
            player->SetPhaseMask(GetNormalPhase(player), true);
    }

    void teleportToDefault(Player *player)
    {
        if (!player)
            return;

        if (player->GetTeamId() == TEAM_ALLIANCE)
            player->TeleportTo(0, -8833.379883f, 628.627991f, 94.006599f, 1.0f);
        else
            player->TeleportTo(1, 1486.048340f, -4415.140625f, 24.187496f, 0.13f);
    }
};

using namespace Acore::ChatCommands;

class GuildHouseCommand : public CommandScript
{
public:
    GuildHouseCommand() : CommandScript("GuildHouseCommand") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable GuildHouseCommandTable =
            {
                {"teleport", HandleGuildHouseTeleCommand, SEC_PLAYER, Console::Yes},
                {"butler", HandleSpawnButlerCommand, SEC_PLAYER, Console::Yes},
            };

        static ChatCommandTable GuildHouseCommandBaseTable =
            {
                {"guildhouse", GuildHouseCommandTable},
                {"gh", GuildHouseCommandTable}};

        return GuildHouseCommandBaseTable;
    }

    static uint32 GetGuildPhase(Player *player)
    {
        return GuildHousePhaseBase + player->GetGuildId();
    }

    static bool HandleSpawnButlerCommand(ChatHandler *handler)
    {
        Player *player = handler->GetSession()->GetPlayer();
        Map *map = player->GetMap();

        if (!player || !player->GetGuild() || (player->GetGuild()->GetLeaderGUID() != player->GetGUID()))
        {
            handler->SendSysMessage("You must be the Guild Master of a guild to use this command!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (player->GetAreaId() != 876)
        {
            handler->SendSysMessage("You must be in your Guild House to use this command!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (player->FindNearestCreature(500031, VISIBLE_RANGE, true))
        {
            handler->SendSysMessage("You already have the Guild House Butler!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        float posX = 16202.185547f;
        float posY = 16255.916992f;
        float posZ = 21.160221f;
        float ori = 6.195375f;

        Creature *creature = new Creature();
        if (!creature->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, GetGuildPhase(player), 500031, 0, posX, posY, posZ, ori))
        {
            handler->SendSysMessage("You already have the Guild House Butler!");
            handler->SetSentErrorMessage(true);
            delete creature;
            return false;
        }
        creature->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), GetGuildPhase(player));
        uint32 lowguid = creature->GetSpawnId();

        creature->CleanupsBeforeDelete();
        delete creature;
        creature = new Creature();
        if (!creature->LoadCreatureFromDB(lowguid, player->GetMap()))
        {
            handler->SendSysMessage("Something went wrong when adding the Butler.");
            handler->SetSentErrorMessage(true);
            delete creature;
            return false;
        }

        sObjectMgr->AddCreatureToGrid(lowguid, sObjectMgr->GetCreatureData(lowguid));
        return true;
    }

    static bool HandleGuildHouseTeleCommand(ChatHandler *handler)
    {
        Player *player = handler->GetSession()->GetPlayer();

        if (!player)
            return false;

        if (player->IsInCombat())
        {
            handler->SendSysMessage("You can't use this command while in combat!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto last = player->CustomData.GetDefault<LastPosition>("lastPos");
        last->mapId = player->GetMapId();
        last->x = player->GetPositionX();
        last->y = player->GetPositionY();
        last->z = player->GetPositionZ();
        last->ori = player->GetOrientation();

        GuildData *guildData = player->CustomData.GetDefault<GuildData>("phase");
        QueryResult result = CharacterDatabase.Query("SELECT `id`, `guild`, `phase`, `map`,`positionX`, `positionY`, `positionZ`, `orientation` FROM `guild_house` WHERE `guild`={}", player->GetGuildId());

        if (!result)
        {
            handler->SendSysMessage("Your guild does not own a Guild House!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        do
        {
            Field *fields = result->Fetch();
            guildData->phase = fields[2].Get<uint32>();
            uint32 map = fields[3].Get<uint32>();
            guildData->posX = fields[4].Get<float>();
            guildData->posY = fields[5].Get<float>();
            guildData->posZ = fields[6].Get<float>();
            guildData->ori = fields[7].Get<float>();

            player->TeleportTo(map, guildData->posX, guildData->posY, guildData->posZ, guildData->ori);

        } while (result->NextRow());

        return true;
    }
};

class GuildHouseGlobal : public GlobalScript
{
public:
    GuildHouseGlobal() : GlobalScript("GuildHouseGlobal") {}

    void OnBeforeWorldObjectSetPhaseMask(WorldObject const *worldObject, uint32 & /*oldPhaseMask*/, uint32 & /*newPhaseMask*/, bool &useCombinedPhases, bool & /*update*/) override
    {
        if (worldObject->GetZoneId() == 876)
            useCombinedPhases = false;
        else
            useCombinedPhases = true;
    }
};

// Add config loader for GuildHousePhaseBase
class GuildHouseConf : public WorldScript
{
public:
    GuildHouseConf() : WorldScript("GuildHouseConf") {}

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        GuildHousePhaseBase = sConfigMgr->GetOption<int32>("GuildHousePhaseBase", 10000);
    }
};

void AddGuildHouseScripts()
{
    new GuildHelper();
    new GuildHouseSeller();
    new GuildHousePlayerScript();
    new GuildHouseCommand();
    new GuildHouseGlobal();
    new GuildHouseConf(); // Register config loader
}
