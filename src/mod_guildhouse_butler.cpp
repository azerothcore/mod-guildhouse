#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Configuration/Config.h"
#include "Creature.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "DataMap.h"
#include "GameObject.h"
#include "Transport.h"
#include "CreatureAI.h"

int cost, GuildHouseInnKeeper, GuildHouseBank, GuildHouseMailBox, GuildHouseAuctioneer, GuildHouseTrainer, GuildHouseVendor, GuildHouseObject, GuildHousePortal, GuildHouseSpirit, GuildHouseProf, GuildHouseBuyRank;

class GuildHouseSpawner : public CreatureScript
{

public:
    GuildHouseSpawner() : CreatureScript("GuildHouseSpawner") {}

    struct GuildHouseSpawnerAI : public ScriptedAI
    {
        GuildHouseSpawnerAI(Creature* creature) : ScriptedAI(creature) {}

        void UpdateAI(uint32 /*diff*/) override
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }
    };

    CreatureAI* GetAI(Creature *creature) const override
    {
        return new GuildHouseSpawnerAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetGuild())
        {
            Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId());
            Guild::Member const* memberMe = guild->GetMember(player->GetGUID());

            if (!memberMe->IsRankNotLower(GuildHouseBuyRank))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You are not authorized to make Guild House purchases.");
                return false;
            }
        }
        else
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You are not in a guild!");
            return false;
        }

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Innkeeper", GOSSIP_SENDER_MAIN, 500032, "Add an Innkeeper?", GuildHouseInnKeeper, false);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Mailbox", GOSSIP_SENDER_MAIN, 184137, "Spawn a Mailbox?", GuildHouseMailBox, false);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Stable Master", GOSSIP_SENDER_MAIN, 28690, "Spawn a Stable Master?", GuildHouseVendor, false);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Class Trainer", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Vendor", GOSSIP_SENDER_MAIN, 3);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn City Portals / Objects", GOSSIP_SENDER_MAIN, 4);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Bank", GOSSIP_SENDER_MAIN, 30605, "Spawn a Banker?", GuildHouseBank, false);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Auctioneer", GOSSIP_SENDER_MAIN, 6, "Spawn an Auctioneer?", GuildHouseAuctioneer, false);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Neutral Auctioneer", GOSSIP_SENDER_MAIN, 9858, "Spawn a Neutral Auctioneer?", GuildHouseAuctioneer, false);
        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Spawn Primary Profession Trainers", GOSSIP_SENDER_MAIN, 7);
        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Spawn Secondary Profession Trainers", GOSSIP_SENDER_MAIN, 8);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Sprirt Healer", GOSSIP_SENDER_MAIN, 6491, "Spawn a Spirit Healer?", GuildHouseSpirit, false);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {

        switch (action)
        {
        case 2: // Spawn Class Trainer
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Death Knight", GOSSIP_SENDER_MAIN, 29195, "Spawn Death Knight Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Druid", GOSSIP_SENDER_MAIN, 26324, "Spawn Druid Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Hunter", GOSSIP_SENDER_MAIN, 26325, "Spawn Hunter Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Mage", GOSSIP_SENDER_MAIN, 26326, "Spawn Mage Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Paladin", GOSSIP_SENDER_MAIN, 26327, "Spawn Paladin Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Priest", GOSSIP_SENDER_MAIN, 26328, "Spawn Priest Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Rogue", GOSSIP_SENDER_MAIN, 26329, "Spawn Rogue Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Shaman", GOSSIP_SENDER_MAIN, 26330, "Spawn Shaman Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Warlock", GOSSIP_SENDER_MAIN, 26331, "Spawn Warlock Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Warrior", GOSSIP_SENDER_MAIN, 26332, "Spawn Warrior Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Go Back!", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 3: // Vendors
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Trade Supplies", GOSSIP_SENDER_MAIN, 28692, "Spawn Trade Supplies?", GuildHouseVendor, false);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Tabard Vendor", GOSSIP_SENDER_MAIN, 28776, "Spawn Tabard Vendor?", GuildHouseVendor, false);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Food & Drink Vendor", GOSSIP_SENDER_MAIN, 4255, "Spawn Food & Drink Vendor?", GuildHouseVendor, false);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Reagent Vendor", GOSSIP_SENDER_MAIN, 29636, "Spawn Reagent Vendor?", GuildHouseVendor, false);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Ammo & Repair Vendor", GOSSIP_SENDER_MAIN, 29493, "Spawn Ammo & Repair Vendor?", GuildHouseVendor, false);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Poisons Vendor", GOSSIP_SENDER_MAIN, 2622, "Spawn Poisons Vendor?", GuildHouseVendor, false);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Go Back!", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 4: // Objects & Portals
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Forge", GOSSIP_SENDER_MAIN, 1685, "Add a forge?", GuildHouseObject, false);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Anvil", GOSSIP_SENDER_MAIN, 4087, "Add an Anvil?", GuildHouseObject, false);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Guild Vault", GOSSIP_SENDER_MAIN, 187293, "Add Guild Vault?", GuildHouseObject, false);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Barber Chair", GOSSIP_SENDER_MAIN, 191028, "Add a Barber Chair?", GuildHouseObject, false);

            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                // ALLIANCE players get these options
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Ironforge", GOSSIP_SENDER_MAIN, 500003, "Add Ironforge Portal?", GuildHousePortal, false);
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Darnassus", GOSSIP_SENDER_MAIN, 500001, "Add Darnassus Portal?", GuildHousePortal, false);
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Exodar", GOSSIP_SENDER_MAIN, 500002, "Add Exodar Portal?", GuildHousePortal, false);
            }
            else
            {
                // HORDE players get these options
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Undercity", GOSSIP_SENDER_MAIN, 500007, "Add Undercity Portal?", GuildHousePortal, false);
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Thunderbluff", GOSSIP_SENDER_MAIN, 500006, "Add Thunderbuff Portal?", GuildHousePortal, false);
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Silvermoon", GOSSIP_SENDER_MAIN, 500005, "Add Silvermoon Portal?", GuildHousePortal, false);
            }

            // These two portals work for either Team
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Shattrath", GOSSIP_SENDER_MAIN, 500008, "Add Shattrath Portal?", GuildHousePortal, false);
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Portal: Dalaran", GOSSIP_SENDER_MAIN, 500009, "Add Dalaran Portal?", GuildHousePortal, false);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Go Back!", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 6: // Auctioneer
        {
            uint32 auctioneer = 0;
            auctioneer = player->GetTeamId() == TEAM_ALLIANCE ? 8719 : 9856;
            SpawnNPC(auctioneer, player);
            break;
        }
        case 9858: // Neutral Auctioneer
            cost = GuildHouseAuctioneer;
            SpawnNPC(action, player);
            break;
        case 7: // Spawn Profession Trainers
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Alchemy Trainer", GOSSIP_SENDER_MAIN, 19052, "Spawn Alchemy Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Blacksmithing Trainer", GOSSIP_SENDER_MAIN, 2836, "Spawn Blacksmithing Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Engineering Trainer", GOSSIP_SENDER_MAIN, 8736, "Spawn Engineering Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Tailoring Trainer", GOSSIP_SENDER_MAIN, 2627, "Spawn Tailoring Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Leatherworking Trainer", GOSSIP_SENDER_MAIN, 19187, "Spawn Leatherworking Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Skinning Trainer", GOSSIP_SENDER_MAIN, 19180, "Spawn Skinning Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Mining Trainer", GOSSIP_SENDER_MAIN, 8128, "Spawn Mining Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Herbalism Trainer", GOSSIP_SENDER_MAIN, 908, "Spawn Herbalism Trainer?", GuildHouseProf, false);

            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                // ALLIANCE players get these options
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Enchanting Trainer", GOSSIP_SENDER_MAIN, 18773, "Spawn Enchanting Trainer?", GuildHouseProf, false);
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Jewelcrafing Trainer", GOSSIP_SENDER_MAIN, 18774, "Spawn Jewelcrafting Trainer?", GuildHouseProf, false);
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Inscription Trainer", GOSSIP_SENDER_MAIN, 30721, "Spawn Inscription Trainer?", GuildHouseProf, false);
            }
            else
            {
                // HORDE players get these options
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Enchanting Trainer", GOSSIP_SENDER_MAIN, 18753, "Spawn Enchanting Trainer?", GuildHouseProf, false);
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Jewelcrafing Trainer", GOSSIP_SENDER_MAIN, 18751, "Spawn Jewelcrafting Trainer?", GuildHouseProf, false);
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Inscription Trainer", GOSSIP_SENDER_MAIN, 30722, "Spawn Inscription Trainer?", GuildHouseProf, false);
            }

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Go Back!", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 8: // Secondary Profession Trainers
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "First Aid Trainer", GOSSIP_SENDER_MAIN, 19184, "Spawn First Aid Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Fishing Trainer", GOSSIP_SENDER_MAIN, 2834, "Spawn Fishing Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Cooking Trainer", GOSSIP_SENDER_MAIN, 19185, "Spawn Cooking Trainer?", GuildHouseProf, false);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Go Back!", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 9: // Go back!
            OnGossipHello(player, creature);
            break;
        case 10: // PVP toggle
            break;
        case 30605: // Banker
            cost = GuildHouseBank;
            SpawnNPC(action, player);
            break;
        case 500032: // Innkeeper
            cost = GuildHouseInnKeeper;
            SpawnNPC(action, player);
            break;
        case 26327: // Paladin
        case 26324: // Druid
        case 26325: // Hunter
        case 26326: // Mage
        case 26328: // Priest
        case 26329: // Rogue
        case 26330: // Shaman
        case 26331: // Warlock
        case 26332: // Warrior
        case 29195: // Death Knight
            cost = GuildHouseTrainer;
            SpawnNPC(action, player);
            break;
        case 2836:  // Blacksmithing
        case 8128:  // Mining
        case 8736:  // Engineering
        case 18774: // Jewelcrafting (Alliance)
        case 18751: // Jewelcrafting (Horde)
        case 18773: // Enchanting (Alliance)
        case 18753: // Enchanting (Horde)
        case 30721: // Inscription (Alliance)
        case 30722: // Inscription (Horde)
        case 19187: // Leatherworking
        case 19180: // Skinning
        case 19052: // Alchemy
        case 908:   // Herbalism
        case 2627:  // Tailoring
        case 19185: // Cooking
        case 2834:  // Fishing
        case 19184: // First Aid
            cost = GuildHouseProf;
            SpawnNPC(action, player);
            break;
        case 28692: // Trade Supplies
        case 28776: // Tabard Vendor
        case 4255:  // Food & Drink Vendor
        case 29636: // Reagent Vendor
        case 29493: // Ammo & Repair Vendor
        case 28690: // Stable Master
        case 2622:  // Poisons Vendor
            cost = GuildHouseVendor;
            SpawnNPC(action, player);
            break;
        //
        // Objects
        //
        case 184137: // Mailbox
            cost = GuildHouseMailBox;
            SpawnObject(action, player);
            break;
        case 6491: // Spirit Healer
            cost = GuildHouseSpirit;
            SpawnNPC(action, player);
            break;
        case 1685:   // Forge
        case 4087:   // Anvil
        case 187293: // Guild Vault
        case 191028: // Barber Chair
            cost = GuildHouseObject;
            SpawnObject(action, player);
            break;
        case 500001: // Darnassus Portal
        case 500002: // Exodar Portal
        case 500003: // Ironforge Portal
        case 500005: // Silvermoon Portal
        case 500006: // Thunder Bluff Portal
        case 500007: // Undercity Portal
        case 500008: // Shattrath Portal
        case 500009: // Dalaran Portal
            cost = GuildHousePortal;
            SpawnObject(action, player);
            break;
        }
        return true;
    }

    uint32 GetGuildPhase(Player* player)
    {
        return player->GetGuildId() + 10;
    }

    void SpawnNPC(uint32 entry, Player* player)
    {
        if (player->FindNearestCreature(entry, VISIBILITY_RANGE, true))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You already have this creature!");
            CloseGossipMenuFor(player);
            return;
        }

        float posX;
        float posY;
        float posZ;
        float ori;

        QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);

        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            posX = fields[0].Get<float>();
            posY = fields[1].Get<float>();
            posZ = fields[2].Get<float>();
            ori = fields[3].Get<float>();

        } while (result->NextRow());

        Creature* creature = new Creature();

        if (!creature->Create(player->GetMap()->GenerateLowGuid<HighGuid::Unit>(), player->GetMap(), GetGuildPhase(player), entry, 0, posX, posY, posZ, ori))
        {
            delete creature;
            return;
        }
        creature->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), GetGuildPhase(player));
        uint32 db_guid = creature->GetSpawnId();

        creature->CleanupsBeforeDelete();
        delete creature;
        creature = new Creature();
        if (!creature->LoadCreatureFromDB(db_guid, player->GetMap()))
        {
            delete creature;
            return;
        }

        sObjectMgr->AddCreatureToGrid(db_guid, sObjectMgr->GetCreatureData(db_guid));
        player->ModifyMoney(-cost);
        CloseGossipMenuFor(player);
    }

    void SpawnObject(uint32 entry, Player* player)
    {
        if (player->FindNearestGameObject(entry, VISIBLE_RANGE))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You already have this object!");
            CloseGossipMenuFor(player);
            return;
        }

        float posX;
        float posY;
        float posZ;
        float ori;

        QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);

        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            posX = fields[0].Get<float>();
            posY = fields[1].Get<float>();
            posZ = fields[2].Get<float>();
            ori = fields[3].Get<float>();

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
        ObjectGuid::LowType guidLow = player->GetMap()->GenerateLowGuid<HighGuid::GameObject>();

        if (!object->Create(guidLow, objectInfo->entry, player->GetMap(), GetGuildPhase(player), posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete object;
            return;
        }

        // fill the gameobject data and save to the db
        object->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), GetGuildPhase(player));
        guidLow = object->GetSpawnId();
        // delete the old object and do a clean load from DB with a fresh new GameObject instance.
        // this is required to avoid weird behavior and memory leaks
        delete object;

        object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
        // this will generate a new guid if the object is in an instance
        if (!object->LoadGameObjectFromDB(guidLow, player->GetMap(), true))
        {
            delete object;
            return;
        }

        // TODO: is it really necessary to add both the real and DB table guid here ?
        sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGameObjectData(guidLow));
        player->ModifyMoney(-cost);
        CloseGossipMenuFor(player);
    }
};

class GuildHouseButlerConf : public WorldScript
{
public:
    GuildHouseButlerConf() : WorldScript("GuildHouseButlerConf") {}

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        GuildHouseInnKeeper = sConfigMgr->GetOption<int32>("GuildHouseInnKeeper", 1000000);
        GuildHouseBank = sConfigMgr->GetOption<int32>("GuildHouseBank", 1000000);
        GuildHouseMailBox = sConfigMgr->GetOption<int32>("GuildHouseMailbox", 500000);
        GuildHouseAuctioneer = sConfigMgr->GetOption<int32>("GuildHouseAuctioneer", 500000);
        GuildHouseTrainer = sConfigMgr->GetOption<int32>("GuildHouseTrainerCost", 1000000);
        GuildHouseVendor = sConfigMgr->GetOption<int32>("GuildHouseVendor", 500000);
        GuildHouseObject = sConfigMgr->GetOption<int32>("GuildHouseObject", 500000);
        GuildHousePortal = sConfigMgr->GetOption<int32>("GuildHousePortal", 500000);
        GuildHouseProf = sConfigMgr->GetOption<int32>("GuildHouseProf", 500000);
        GuildHouseSpirit = sConfigMgr->GetOption<int32>("GuildHouseSpirit", 100000);
        GuildHouseBuyRank = sConfigMgr->GetOption<int32>("GuildHouseBuyRank", 4);
    }
};

void AddGuildHouseButlerScripts()
{
    new GuildHouseSpawner();
    new GuildHouseButlerConf();
}
