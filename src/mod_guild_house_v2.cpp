#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Creature.h"
#include "Guild.h"
#include "SpellAuraEffects.h"
#include "Chat.h"
#include "ScriptedGossip.h"
#include "GuildMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "DataMap.h"
#include "GameObject.h"
#include "Transport.h"
#include "Maps/MapManager.h"

class GuildData : public DataMap::Base
{
public:
    GuildData() {}
    GuildData(uint32 phase, float posX, float posY, float posZ) : phase(phase), posX(posX), posY(posY), posZ(posZ) {}
    uint32 phase;
    float posX;
    float posY;
    float posZ;
};

class GuildHelper : public GuildScript {

public:

    GuildHelper() : GuildScript("GuildHelper") {}

    void OnCreate(Guild*, Player* leader, const std::string&)
    {
        ChatHandler(leader->GetSession()).PSendSysMessage("You now own a guild. You can purchase a guild house!");
    }

    uint32 GetGuildPhase(Guild* guild) {
        return guild->GetId() + 10;
    }

    void OnDisband(Guild* guild)
    {

        if (RemoveGuildHouse(guild))
        {       
            sLog->outBasic("GUILDHOUSE: Deleting guild house data due to disbanding of guild...");
        } else { sLog->outBasic("GUILDHOUSE: Error deleting guild house data during disbanding of guild!!"); }

    }

    bool RemoveGuildHouse(Guild* guild)
    {

        uint32 guildPhase = GetGuildPhase(guild);
        QueryResult CreatureResult;
        QueryResult GameobjResult;

        // Lets find all of the gameobjects to be removed
        GameobjResult = WorldDatabase.PQuery("SELECT `guid` FROM `gameobject` WHERE `map` = 1 AND `phaseMask` = '%u'", guildPhase);
        // Lets find all of the creatures to be removed
        CreatureResult = WorldDatabase.PQuery("SELECT `guid` FROM `creature` WHERE `map` = 1 AND `phaseMask` = '%u'", guildPhase);


        // remove creatures from the deleted guild house map
        if (CreatureResult) {
            do
            {
                Field* fields = CreatureResult->Fetch();
                uint32 lowguid = fields[0].GetInt32();
                if (CreatureData const* cr_data = sObjectMgr->GetCreatureData(lowguid)) {
                    if (Creature* creature = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(lowguid, cr_data->id, HIGHGUID_UNIT), (Creature*)NULL))
                    {
                        creature->CombatStop();
                        creature->DeleteFromDB();
                        creature->AddObjectToRemoveList();
                    }
                }
            } while (CreatureResult->NextRow());
        }


        // remove gameobjects from the deleted guild house map
        if (GameobjResult) {
            do
            {
                Field* fields = GameobjResult->Fetch();
                uint32 lowguid = fields[0].GetInt32();
                if (GameObjectData const* go_data = sObjectMgr->GetGOData(lowguid)) {
                    //if (GameObject* gobject = ObjectAccessor::GetObjectInWorld(lowguid, (GameObject*)NULL))
                    if (GameObject* gobject = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(lowguid, go_data->id, HIGHGUID_GAMEOBJECT), (GameObject*)NULL))
                    {
                        gobject->SetRespawnTime(0);
                        gobject->Delete();
                        gobject->DeleteFromDB();
                        gobject->CleanupsBeforeDelete();
                        //delete gobject;
                    }
                }

            } while (GameobjResult->NextRow());
        }

        // Delete actual guild_house data from characters database
        CharacterDatabase.PQuery("DELETE FROM `guild_house` WHERE `guild` = '%u'", guild->GetId());

        return true;

    }

};

class GuildHouseSeller : public CreatureScript {

public:
    GuildHouseSeller() : CreatureScript("GuildHouseSeller") {}

    bool OnGossipHello(Player *player, Creature * creature)
    {
        if (!player->GetGuild())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You are not a member of a guild.");
            CloseGossipMenuFor(player);
            return false;
        }
        
        QueryResult has_gh = CharacterDatabase.PQuery("SELECT id, `guild` FROM `guild_house` WHERE guild = %u", player->GetGuildId());

        // Only show Teleport option if guild owns a guildhouse
        if (has_gh)
        {
            AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Teleport to Guild House", GOSSIP_SENDER_MAIN, 1);
        }

        if (player->GetGuild()->GetLeaderGUID() == player->GetGUID())
        {
            // Only show "Sell" option if they have a guild house & are guild leader
            if (has_gh)
            {
                AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Sell Guild House!", GOSSIP_SENDER_MAIN, 3, "Are you sure you want to sell your Guild house?", 0, false);
            }
            else {
            // Only leader of the guild can buy guild house & only if they don't already have a guild house
            AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Buy Guild House!", GOSSIP_SENDER_MAIN, 2);
            }
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Close", GOSSIP_SENDER_MAIN, 5);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature * m_creature, uint32, uint32 action)
    {
        uint32 map;
        float posX;
        float posY;
        float posZ;

        switch (action)
        {
        case 100: // gmsiland
            map = 1;
            posX = 16226.117f;
            posY = 16258.046f;
            posZ = 13.257628f;
            break;
        case 5: // close
            CloseGossipMenuFor(player);
            break;
        case 4: // --- MORE TO COME ---
            BuyGuildHouse(player->GetGuild(), player, m_creature);
            break;
        case 3: // Sell back guild house
        {
            QueryResult has_gh = CharacterDatabase.PQuery("SELECT id, `guild` FROM `guild_house` WHERE guild = %u", player->GetGuildId());
            if (!has_gh)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not own a Guild House!");
                CloseGossipMenuFor(player);
                return false;
            }

            // Calculate total gold returned: 1) cost of guildhouse and cost of each purchase made.
            if (RemoveGuildHouse(player))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You have successfully sold your guild house.");
                player->GetGuild()->BroadcastToGuild(player->GetSession(), false, "We just sold our guild house.", LANG_UNIVERSAL);
                player->ModifyMoney(+(sConfigMgr->GetIntDefault("CostGuildHouse", 10000000) / 2));
                sLog->outBasic("GUILDHOUSE: Successfully returned money and sold guildhouse");
                CloseGossipMenuFor(player);
            } else {
                ChatHandler(player->GetSession()).PSendSysMessage("There was an error selling your guild house.");
                CloseGossipMenuFor(player);
                }
            break;
        }
        case 2: // buy guild house
            BuyGuildHouse(player->GetGuild(), player, m_creature);
            break;
        case 1: // teleport to guild house
            TeleportGuildHouse(player->GetGuild(), player, m_creature);
            break;
        }

        if (action >= 100)
        {
            CharacterDatabase.PQuery("INSERT INTO `guild_house` (guild, phase, map, positionX, positionY, positionZ) VALUES (%u, %u, %u, %f, %f, %f)", 
                player->GetGuildId(), GetGuildPhase(player), map, posX, posY, posZ);
            player->ModifyMoney(-(sConfigMgr->GetIntDefault("CostGuildHouse", 10000000)));
            // Msg to purchaser and Msg Guild as purchaser 
            ChatHandler(player->GetSession()).PSendSysMessage("You have successfully purchased a guild house");
            player->GetGuild()->BroadcastToGuild(player->GetSession(), false, "We now have a Guild House!", LANG_UNIVERSAL);
            player->GetGuild()->BroadcastToGuild(player->GetSession(), false, "In chat, type `.guildhouse teleport` to meet me there!", LANG_UNIVERSAL);
            sLog->outBasic("GUILDHOUSE: GuildId: '%u' has purchased a guildhouse", player->GetGuildId());

            // Spawn a portal and the guild assistant automatically as part of purchase.
            SpawnStarterPortal(player);
            SpawnAssistantNPC(player);
            CloseGossipMenuFor(player);
        }

        return true;
    }

    uint32 GetGuildPhase(Player* player) {
        return player->GetGuildId() + 10;
    }


    bool RemoveGuildHouse(Player* player)
    {

        uint32 guildPhase = GetGuildPhase(player);
        QueryResult CreatureResult;
        QueryResult GameobjResult; 

        // Lets find all of the gameobjects to be removed       
        GameobjResult = WorldDatabase.PQuery("SELECT `guid` FROM `gameobject` WHERE `map` = 1 AND `phaseMask` = '%u'", guildPhase);
        // Lets find all of the creatures to be removed
        CreatureResult = WorldDatabase.PQuery("SELECT `guid` FROM `creature` WHERE `map` = 1 AND `phaseMask` = '%u'", guildPhase);


        // remove creatures from the deleted guild house map
        if (CreatureResult) {
            do
            {
                Field* fields = CreatureResult->Fetch();
                uint32 lowguid = fields[0].GetInt32();
                if (CreatureData const* cr_data = sObjectMgr->GetCreatureData(lowguid)) {
                    if (Creature* creature = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(lowguid, cr_data->id, HIGHGUID_UNIT), (Creature*)NULL))
                    {
                        creature->CombatStop();
                        creature->DeleteFromDB();
                        creature->AddObjectToRemoveList();
                    } 
                }
            } while (CreatureResult->NextRow());
        }


        // remove gameobjects from the deleted guild house map
        if (GameobjResult) {
            do
            {
                Field* fields = GameobjResult->Fetch();
                uint32 lowguid = fields[0].GetInt32();
                if (GameObjectData const* go_data = sObjectMgr->GetGOData(lowguid)) {
                    //if (GameObject* gobject = ObjectAccessor::GetObjectInWorld(lowguid, (GameObject*)NULL))
                    if (GameObject* gobject = ObjectAccessor::GetObjectInWorld(MAKE_NEW_GUID(lowguid, go_data->id, HIGHGUID_GAMEOBJECT), (GameObject*)NULL))
                    {
                        gobject->SetRespawnTime(0);
                        gobject->Delete();
                        gobject->DeleteFromDB();
                        gobject->CleanupsBeforeDelete();
                        //delete gobject;
                    } 
                } 

            } while (GameobjResult->NextRow());
        }
        
        // Delete actual guild_house data from characters database
        CharacterDatabase.PQuery("DELETE FROM `guild_house` WHERE `guild` = '%u'", player->GetGuildId());

        return true;

    }

    void SpawnStarterPortal(Player* player)
    {
        
        uint32 entry = 0;
        float posX;
        float posY;
        float posZ;
        float ori;

        Map* map = sMapMgr->FindMap(1,0);

        if (player->GetTeamId() == TEAM_ALLIANCE) 
        {
            // Portal to Stormwind
            entry = 183325;
        } else {
            // Portal to Orgrimmar
            entry = 183323;
        }
            

        if (entry == 0) { sLog->outBasic("Error with SpawnStarterPortal in GuildHouse Module!"); return; }

        QueryResult result = WorldDatabase.PQuery("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry` = %u", entry);

        if (!result)
        {
            sLog->outBasic("GUILDHOUSE: Unable to find data on portal for entry: '%u'", entry);
            return;
        }

        do
        {
            Field* fields = result->Fetch();
            posX = fields[0].GetFloat();
            posY = fields[1].GetFloat();
            posZ = fields[2].GetFloat();
            ori = fields[3].GetFloat();

        } while (result->NextRow());


        uint32 objectId = entry;
        if (!objectId)
        {
            sLog->outBasic("GUILDHOUSE: objectId IS NULL, should be '%u'", entry);
            return;
        }

        const GameObjectTemplate* objectInfo = sObjectMgr->GetGameObjectTemplate(objectId);

        if (!objectInfo)
        {
            sLog->outBasic("GUILDHOUSE: objectInfo is NULL!");
            return;
        }

        if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
        {
            sLog->outBasic("GUILDHOUSE: Unable to find displayId??");
            return;
        }

        GameObject* object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
        uint32 guidLow = sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT);


        if (!object->Create(guidLow, objectInfo->entry, map, GetGuildPhase(player), posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete object;
            sLog->outBasic("GUILDHOUSE: Unable to create object!!");
            return;
        }
        

        // fill the gameobject data and save to the db
        object->SaveToDB(sMapMgr->FindMap(1, 0)->GetId(), (1 << sMapMgr->FindMap(1, 0)->GetSpawnMode()), GetGuildPhase(player));
        // delete the old object and do a clean load from DB with a fresh new GameObject instance.
        // this is required to avoid weird behavior and memory leaks
        delete object;

        object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
        // this will generate a new guid if the object is in an instance
        if (!object->LoadGameObjectFromDB(guidLow, sMapMgr->FindMap(1, 0)))
        {
            delete object;
            return;
        }

        // TODO: is it really necessary to add both the real and DB table guid here ?
        sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGOData(guidLow));
        CloseGossipMenuFor(player);
    }

    void SpawnAssistantNPC(Player* player)
    {
        uint32 entry = 70102;
        float posX = 16202.185547f;
        float posY = 16255.916992f;
        float posZ = 21.160221f;
        float ori = 6.195375f;

        Creature* creature = new Creature();

        if (!creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_UNIT), sMapMgr->FindMap(1, 0), GetGuildPhase(player), entry, 0, posX, posY, posZ, ori))
        {
            delete creature;
            return;
        }
        creature->SaveToDB(sMapMgr->FindMap(1, 0)->GetId(), (1 << sMapMgr->FindMap(1, 0)->GetSpawnMode()), GetGuildPhase(player));
        uint32 db_guid = creature->GetDBTableGUIDLow();

        creature->CleanupsBeforeDelete();
        delete creature;
        creature = new Creature();
        if (!creature->LoadCreatureFromDB(db_guid, sMapMgr->FindMap(1, 0)))
        {
            delete creature;
            return;
        }

        sObjectMgr->AddCreatureToGrid(db_guid, sObjectMgr->GetCreatureData(db_guid));
        return;
    }

    bool BuyGuildHouse(Guild* guild, Player* player, Creature* creature)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT `id`, `guild` FROM guild_house WHERE `guild` = %u", guild->GetId());

        if (result)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Your guild already has a Guild House.");
            CloseGossipMenuFor(player);
            return false;
        }

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "GM Island", GOSSIP_SENDER_MAIN, 100, "Buy Guild House on GM Island?", sConfigMgr->GetIntDefault("CostGuildHouse", 10000000), false);
        // Removing this tease for now, as right now the phasing code is specific go GM Island, so its not a simple thing to add new areas yet.
        //AddGossipItemFor(player, GOSSIP_ICON_CHAT, " ----- More to Come ----", GOSSIP_SENDER_MAIN, 4);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;

    }

    void TeleportGuildHouse(Guild* guild, Player* player, Creature* creature)
    {
        GuildData* guildData = player->CustomData.GetDefault<GuildData>("phase");
        QueryResult result = CharacterDatabase.PQuery("SELECT `phase`, `map`,`positionX`, `positionY`, `positionZ` FROM guild_house WHERE `guild` = %u", guild->GetId());

        if (!result)
        {
            ClearGossipMenuFor(player);
            if (player->GetGuild()->GetLeaderGUID() == player->GetGUID())
            {
                // Only leader of the guild can buy / sell guild house
                AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Buy Guild House!", GOSSIP_SENDER_MAIN, 2);
                AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Sell Guild House!", GOSSIP_SENDER_MAIN, 3, "Are you sure you want to sell your Guild house?", 0, false);
            }

            AddGossipItemFor(player, GOSSIP_ICON_TABARD, "Teleport to Guild House", GOSSIP_SENDER_MAIN, 1);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Close", GOSSIP_SENDER_MAIN, 5);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            ChatHandler(player->GetSession()).PSendSysMessage("Your Guild does not own a guild house");
            return;
        }

        do {

            Field* fields = result->Fetch();
            guildData->phase = fields[0].GetUInt32();
            uint32 map = fields[1].GetUInt32();
            guildData->posX = fields[2].GetFloat();
            guildData->posY = fields[3].GetFloat();
            guildData->posZ = fields[4].GetFloat();

            player->TeleportTo(map, guildData->posX, guildData->posY, guildData->posZ, player->GetOrientation());

        } while (result->NextRow());
    }

};

class GuildHouseV2PlayerScript : public PlayerScript
{
public:
    GuildHouseV2PlayerScript() : PlayerScript("GuildHouseV2PlayerScript") { }

    void OnLogin(Player* player)
    {
        CheckPlayer(player);
    }

    void OnUpdateZone(Player* player, uint32 newZone, uint32 /*newArea*/)
    {
        if (newZone == 876)
            CheckPlayer(player);
        else
            player->SetPhaseMask(GetNormalPhase(player), true);
    }

    uint32 GetNormalPhase(Player* player) const
    {
        if (player->IsGameMaster())
            return PHASEMASK_ANYWHERE;

        uint32 phase = player->GetPhaseByAuras();
        if (!phase)
            return PHASEMASK_NORMAL;
        else
            return phase;
    }

    void CheckPlayer(Player* player)
    {
        GuildData* guildData = player->CustomData.GetDefault<GuildData>("phase");
        QueryResult result = CharacterDatabase.PQuery("SELECT `id`, `guild`, `phase`, `map`,`positionX`, `positionY`, `positionZ` FROM guild_house WHERE `guild` = %u", player->GetGuildId());

        if (result)
        {
            do {
                                                            // commented out due to travis, but keeping for future expansion into other areas
                Field* fields = result->Fetch();
                //uint32 id = fields[0].GetUInt32();        // fix for travis
                //uint32 guild = fields[1].GetUInt32();     // fix for travis
                guildData->phase = fields[2].GetUInt32();
                //uint32 map = fields[3].GetUInt32();       // fix for travis
                //guildData->posX = fields[4].GetFloat();   // fix for travis
                //guildData->posY = fields[5].GetFloat();   // fix for travis
                //guildData->posZ = fields[6].GetFloat();   // fix for travis

            } while (result->NextRow());
        }

        if (player->GetZoneId() == 876 && player->GetAreaId() == 876) // GM Island
        {
            // If player is not in a guild he doesnt have a guild house teleport away
            // TODO: What if they are in a guild, but somehow are in the wrong phaseMask and seeing someone else's area?

            if (!result || !player->GetGuild())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your Guild does not own a guild house.");
                teleport(player);
                return;
            }
            player->SetPhaseMask(guildData->phase, true);
        }
        else
            player->SetPhaseMask(GetNormalPhase(player), true);
    }

    void teleport(Player* player)
    {
        if (player->GetTeamId() == TEAM_ALLIANCE)
            player->TeleportTo(0, -8833.379883f, 628.627991f, 94.006599f, 1.0f);
        else
            player->TeleportTo(1, 1486.048340f, -4415.140625f, 24.187496f, 0.13f);
    }
};

class GuildHouseCommand : public CommandScript
{
public:
    GuildHouseCommand() : CommandScript("GuildHouseCommand") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> GuildHouseCommandTable =
        {
            // View Command
            { "Teleport", SEC_PLAYER, false, &HandleGuildHouseTeleCommand, "" },
            // Set Command
            { "SpawnNpc", SEC_PLAYER, false, &HandleSpawnNPCCommand, "" },
        };

        static std::vector<ChatCommand> GuildHouseCommandBaseTable =
        {
            { "Guildhouse", SEC_PLAYER, false, nullptr, "", GuildHouseCommandTable }
        };

        return GuildHouseCommandBaseTable;
    }

    static uint32 GetGuildPhase(Player* player) {
        return player->GetGuildId() + 10;
    }

    static bool HandleSpawnNPCCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (!player->GetGuild() || (player->GetGuild()->GetLeaderGUID() != player->GetGUID())) {
            handler->SendSysMessage("You must be the Guild Master of a guild to use this command!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (player->GetAreaId() != 876) {
            handler->SendSysMessage("You must be in your Guild House to use this command!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (player->FindNearestCreature(70102, VISIBLE_RANGE, true)) {
            handler->SendSysMessage("You already have the Guild House Assistant!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        float posX = 16202.185547f;
        float posY = 16255.916992f;
        float posZ = 21.160221f;
        float ori = 6.195375f;

        Creature* creature = new Creature();

        if (!creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_UNIT), player->GetMap(), GetGuildPhase(player), 70102, 0, posX, posY, posZ, ori))
        {
            handler->SendSysMessage("You already have the Guild House Assistant!");
            handler->SetSentErrorMessage(true);
            delete creature;
            return false;
        }
        creature->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), GetGuildPhase(player));
        uint32 db_guid = creature->GetDBTableGUIDLow();

        creature->CleanupsBeforeDelete();
        delete creature;
        creature = new Creature();
        if (!creature->LoadCreatureFromDB(db_guid, player->GetMap()))
        {
            handler->SendSysMessage("Something went wrong when adding the NPC.");
            handler->SetSentErrorMessage(true);
            delete creature;
            return false;
        }

        sObjectMgr->AddCreatureToGrid(db_guid, sObjectMgr->GetCreatureData(db_guid));
        return true;
    }

    static bool HandleGuildHouseTeleCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (!player)
            return false;

        if (player->IsInCombat()) {
            handler->SendSysMessage("You can't use this command while in combat!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        GuildData* guildData = player->CustomData.GetDefault<GuildData>("phase");
        QueryResult result = CharacterDatabase.PQuery("SELECT `id`, `guild`, `phase`, `map`,`positionX`, `positionY`, `positionZ` FROM guild_house WHERE `guild` = %u", player->GetGuildId());

        if (!result)
        {
            handler->SendSysMessage("Your Guild does not own a guild house!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        do {

            Field* fields = result->Fetch();
            //uint32 id = fields[0].GetUInt32();        // fix for travis
            //uint32 guild = fields[1].GetUInt32();     // fix for travis
            guildData->phase = fields[2].GetUInt32();
            uint32 map = fields[3].GetUInt32();
            guildData->posX = fields[4].GetFloat();
            guildData->posY = fields[5].GetFloat();
            guildData->posZ = fields[6].GetFloat();

            player->TeleportTo(map, guildData->posX, guildData->posY, guildData->posZ, player->GetOrientation());

        } while (result->NextRow());

        return true;
    }
};

class GuildHouseConf : public WorldScript
{
public:
    GuildHouseConf() : WorldScript("GuildHouseConf") {}

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) {
            std::string conf_path = _CONF_DIR;
            std::string cfg_file = conf_path + "/mod_guild_house_v2.conf";

#ifdef WIN32
            cfg_file = "mod_guild_house_v2.conf";
#endif

            std::string cfg_def_file = cfg_file + ".dist";
            sConfigMgr->LoadMore(cfg_def_file.c_str());
            sConfigMgr->LoadMore(cfg_file.c_str());
        }
    }
};

class GuildHouseGlobal : public GlobalScript
{
public:
    GuildHouseGlobal() : GlobalScript("GuildHouseGlobal") {}

    void OnBeforeWorldObjectSetPhaseMask(WorldObject const* worldObject, uint32& /*oldPhaseMask*/, uint32& /*newPhaseMask*/, bool& useCombinedPhases, bool& /*update*/) override
    {
        if (worldObject->GetZoneId() == 876)
            useCombinedPhases = false;
        else
            useCombinedPhases = true;
    }
};

void AddGuildHouseV2Scripts() {
    new GuildHelper();
    new GuildHouseSeller();
    new GuildHouseV2PlayerScript();
    new GuildHouseCommand();
    new GuildHouseGlobal();
}

