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

class GuildHelper : public GuildScript{

public:

    GuildHelper() : GuildScript("GuildHelper") { }

    void OnCreate(Guild*, Player* leader, const std::string&)
    {
        ChatHandler(leader->GetSession()).PSendSysMessage("You now own a guild. You can purchase a guild house!");
    }

    uint32 GetGuildPhase(Guild* guild) {
        return guild->GetId() + 10;
    }

    void OnDisband(Guild* guild)
    {
        QueryResult result;
        
        result = WorldDatabase.PQuery("SELECT `guid` FROM `creature` WHERE `phaseMask` = %u and `map` = 1", GetGuildPhase(guild));

        if (result) {
            Map* creatureMap = sMapMgr->FindMap(1, 0);
            Creature* creature = nullptr;
            do
            {

                Field* fields = result->Fetch();
                uint32 lowguid = fields[0].GetInt32();

                if (CreatureData const* cr_data = sObjectMgr->GetCreatureData(lowguid)) {
                    creature = creatureMap->GetCreature(MAKE_NEW_GUID(lowguid, cr_data->id, HIGHGUID_UNIT));
                    creature->CombatStop();
                    creature->DeleteFromDB();
                    creature->AddObjectToRemoveList();
                    //delete creature;
                }


            } while (result->NextRow());
        }

        result = WorldDatabase.PQuery("SELECT `guid` FROM `gameobject` WHERE `phaseMask` = %u and `map` = 1", GetGuildPhase(guild));

        if (result) {
            Map* gobMap = sMapMgr->FindMap(1, 0);
            GameObject* object = nullptr;
            do
            {
                Field* fields = result->Fetch();
                uint32 lowguid = fields[0].GetInt32();

                if (GameObjectData const* gameObjectData = sObjectMgr->GetGOData(lowguid)) {
                    object = gobMap->GetGameObject(gameObjectData->id);
                    object->SetRespawnTime(0);
                    object->DeleteFromDB();
                    object->CleanupsBeforeDelete();
                    delete object;
                }

            } while (result->NextRow());
        }

        WorldDatabase.PQuery("DELETE FROM `creature` WHERE map = 1 AND phaseMask = %u", GetGuildPhase(guild));
        WorldDatabase.PQuery("DELETE FROM `gameobject` WHERE map = 1 AND phaseMask = %u", GetGuildPhase(guild));

        CharacterDatabase.PQuery("DELETE FROM `guild_house` WHERE guild = %u", guild->GetId());
        
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
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild currently doesn't own a Guild House!");
                CloseGossipMenuFor(player);
                return false;
            }

            ChatHandler(player->GetSession()).PSendSysMessage("Calling RemoveGuildHouse(player)");

            if (RemoveGuildHouse(player))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You have successfully sold your guild house.");
                player->GetGuild()->BroadcastToGuild(player->GetSession(), false, "We just sold our guild house.", LANG_UNIVERSAL);
                player->ModifyMoney(+(sConfigMgr->GetIntDefault("CostGuildHouse", 10000000) / 2));
                CloseGossipMenuFor(player);
            } else {
                ChatHandler(player->GetSession()).PSendSysMessage("There was an error selling your guild house.");
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
            // Spawn a portal and the guild assistant automatically as part of purchase.
            SpawnDalaranPortal(player);
            SpawnAssistantNPC(player);
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
        ChatHandler(player->GetSession()).PSendSysMessage("Selecting from gameobject where phaseMask = %u ", guildPhase);
        GameobjResult = WorldDatabase.PQuery("SELECT `guid` FROM `gameobject` WHERE `phaseMask` = '%u' AND `map` = 1", guildPhase);
        // Lets find all of the creatures to be removed
        ChatHandler(player->GetSession()).PSendSysMessage("Selecting from creature where phaseMask = %u ", guildPhase);   
        CreatureResult = WorldDatabase.PQuery("SELECT `guid` FROM `creature` WHERE `phaseMask` = '%u' and `map` = 1", guildPhase);


        sLog->outBasic("Removing gameobjects");
        // remove gameobjects from the deleted guild house map
        if (GameobjResult) {
            Map* gobMap = sMapMgr->FindMap(1, 0);
            GameObject* gobject = nullptr;
            do
            {
                Field* fields = GameobjResult->Fetch();
                uint32 lowguid = fields[0].GetInt32();
                if (GameObjectData const* gameObjectData = sObjectMgr->GetGOData(lowguid)) {
                    if ((gobject = gobMap->GetGameObject(gameObjectData->id))) {
                        if (!gobject){ sLog->outBasic("NULL gobject"); return false; }
                        sLog->outBasic("Deleting gameobject: %u", lowguid);
                        gobject->SetRespawnTime(0);
                        gobject->Delete();
                        gobject->DeleteFromDB();
                        gobject->CleanupsBeforeDelete();
                        delete gobject;
                        sLog->outBasic("Completed deletion of gameobject: %u", lowguid);
                    }
                }

            } while (GameobjResult->NextRow());
        }
        

        sLog->outBasic("Removing creatures");
        // remove creatures from the deleted guild house map
        if (CreatureResult) {
            Map* creatureMap = sMapMgr->FindMap(1, 0);
            if (!creatureMap) { sLog->outBasic("NULL creatureMap"); return false; }
            Creature* creature = nullptr;
            do
            {
                Field* fields = CreatureResult->Fetch();
                uint32 lowguid = fields[0].GetInt32();
                sLog->outBasic("Deleting creatures: %u", lowguid);
                if (CreatureData const* cr_data = sObjectMgr->GetCreatureData(lowguid)) {
                    if (!cr_data) { sLog->outBasic("Data for %u not found in `creature` table.", lowguid); return false; }
                    sLog->outBasic("cr_data->id: %u, lowguid: %u, HIGHGUID_UNIT: %u", cr_data->id, lowguid, HIGHGUID_UNIT);
                    creature = creatureMap->GetCreature(MAKE_NEW_GUID(lowguid, cr_data->id, HIGHGUID_UNIT));
                    if (!creature) { sLog->outBasic("NULL Creature!"); return false; }
                    creature->CombatStop();
                    creature->DeleteFromDB();
                    creature->AddObjectToRemoveList();
                    sLog->outBasic("Completed deletion of creature: %u", lowguid);
                }
            } while (CreatureResult->NextRow());
        } else {ChatHandler(player->GetSession()).PSendSysMessage("No creatures to delete?"); } 


        // Perform database deletes to clean up, although this may happen above in object->DeleteFromDB() & creature->DeleteFromDB()
        ChatHandler(player->GetSession()).PSendSysMessage("Deleting from guild_house where guild = %u ", player->GetGuildId());
        CharacterDatabase.PQuery("DELETE FROM `guild_house` WHERE guild = '%u'", player->GetGuildId());
        ChatHandler(player->GetSession()).PSendSysMessage("Deleting from creature where phaseMask = '%u' ", guildPhase); 
        WorldDatabase.PQuery("DELETE FROM `creature` WHERE `map` = 1 AND phaseMask = '%u'", guildPhase);
        ChatHandler(player->GetSession()).PSendSysMessage("Deleting from gameobject where phaseMask = '%u' ", guildPhase);
        WorldDatabase.PQuery("DELETE FROM `gameobject` WHERE `map` = 1 and phaseMask = '%u'", guildPhase);

        return true;

    }

    void SpawnDalaranPortal(Player* player)
    {
        // 195682 - Dalaran portal enterable by alliance
        uint32 entry = 195682;
        float posX;
        float posY;
        float posZ;
        float ori;

        QueryResult result = WorldDatabase.PQuery("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry` = %u", entry);

        if (!result)
            return;

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
            return;

        const GameObjectTemplate* objectInfo = sObjectMgr->GetGameObjectTemplate(objectId);

        if (!objectInfo)
            return;

        if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
            return;

        GameObject* object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
        uint32 guidLow = sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT);

        //if (!object->Create(guidLow, objectInfo->entry, sMapMgr->FindMap(1, 0), GetGuildPhase(player->GetGuild()), posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        if (!object->Create(guidLow, objectInfo->entry, sMapMgr->FindMap(1, 0), GetGuildPhase(player), posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete object;
            return;
        }

        // fill the gameobject data and save to the db
        //object->SaveToDB(sMapMgr->FindMap(1, 0)->GetId(), (1 << sMapMgr->FindMap(1, 0)->GetSpawnMode()), GetGuildPhase(player->GetGuild()));
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

        //if (!creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_UNIT), sMapMgr->FindMap(1, 0), GetGuildPhase(player->GetGuild()), 70102, 0, posX, posY, posZ, ori))
        if (!creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_UNIT), sMapMgr->FindMap(1, 0), GetGuildPhase(player), 70102, 0, posX, posY, posZ, ori))
        {
            delete creature;
            return;
        }
        //creature->SaveToDB(sMapMgr->FindMap(1, 0)->GetId(), (1 << sMapMgr->FindMap(1, 0)->GetSpawnMode()), GetGuildPhase(player->GetGuild()));
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
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, " ----- More to Come ----", GOSSIP_SENDER_MAIN, 4);
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
            player->SetPhaseMask(guildData->phase, true);

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

    void OnUpdateZone(Player* player, uint32 newZone, uint32 newArea)
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

                Field* fields = result->Fetch();
                uint32 id = fields[0].GetUInt32();
                uint32 guild = fields[1].GetUInt32();
                guildData->phase = fields[2].GetUInt32();
                uint32 map = fields[3].GetUInt32();
                guildData->posX = fields[4].GetFloat();
                guildData->posY = fields[5].GetFloat();
                guildData->posZ = fields[6].GetFloat();

            } while (result->NextRow());
        }

        if (player->GetZoneId() == 876 && player->GetAreaId() == 876) // GM Island
        {
            // If player is not in a guild he doesnt have a guild house teleport away

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

        if (player->GetGuild()->GetLeaderGUID() != player->GetGUID()) {
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
            uint32 id = fields[0].GetUInt32();
            uint32 guild = fields[1].GetUInt32();
            guildData->phase = fields[2].GetUInt32();
            uint32 map = fields[3].GetUInt32();
            guildData->posX = fields[4].GetFloat();
            guildData->posY = fields[5].GetFloat();
            guildData->posZ = fields[6].GetFloat();

            player->TeleportTo(map, guildData->posX, guildData->posY, guildData->posZ, player->GetOrientation());
            player->SetPhaseMask(guildData->phase, true);

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

void AddGuildHouseV2Scripts() {
    new GuildHelper();
    new GuildHouseSeller();
    new GuildHouseV2PlayerScript();
    new GuildHouseCommand();
}

