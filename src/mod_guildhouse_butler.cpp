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
        GuildHouseSpawnerAI(Creature *creature) : ScriptedAI(creature) {}

        void UpdateAI(uint32 /*diff*/) override
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }
    };

    CreatureAI *GetAI(Creature *creature) const override
    {
        return new GuildHouseSpawnerAI(creature);
    }

    bool OnGossipHello(Player *player, Creature *creature) override
    {
        if (player->GetGuild())
        {
            Guild *guild = sGuildMgr->GetGuildById(player->GetGuildId());
            Guild::Member const *memberMe = guild->GetMember(player->GetGUID());

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

        // Check if each NPC or object already exists and update the gossip menu text accordingly
        AddGossipItemFor(player, GOSSIP_ICON_TALK,
                         player->FindNearestCreature(500032, VISIBILITY_RANGE, true) ? "Spawn Innkeeper (Already Purchased)" : "Spawn Innkeeper",
                         GOSSIP_SENDER_MAIN, 500032, "Add an Innkeeper?", GuildHouseInnKeeper, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
                         player->FindNearestGameObject(184137, VISIBLE_RANGE) ? "Spawn Mailbox (Already Purchased)" : "Spawn Mailbox",
                         GOSSIP_SENDER_MAIN, 184137, "Spawn a Mailbox?", GuildHouseMailBox, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
                         player->FindNearestCreature(28690, VISIBILITY_RANGE, true) ? "Spawn Stable Master (Already Purchased)" : "Spawn Stable Master",
                         GOSSIP_SENDER_MAIN, 28690, "Spawn a Stable Master?", GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Class Trainer", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Vendor", GOSSIP_SENDER_MAIN, 3);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn City Portals / Objects", GOSSIP_SENDER_MAIN, 4);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                         player->FindNearestCreature(30605, VISIBILITY_RANGE, true) ? "Spawn Bank (Already Purchased)" : "Spawn Bank",
                         GOSSIP_SENDER_MAIN, 30605, "Spawn a Banker?", GuildHouseBank, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Auctioneer", GOSSIP_SENDER_MAIN, 6, "Spawn an Auctioneer?", GuildHouseAuctioneer, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                         player->FindNearestCreature(9858, VISIBILITY_RANGE, true) ? "Spawn Neutral Auctioneer (Already Purchased)" : "Spawn Neutral Auctioneer",
                         GOSSIP_SENDER_MAIN, 9858, "Spawn a Neutral Auctioneer?", GuildHouseAuctioneer, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                         player->FindNearestCreature(8719, VISIBILITY_RANGE, true) ? "Spawn Alliance Auctioneer (Already Purchased)" : "Spawn Alliance Auctioneer",
                         GOSSIP_SENDER_MAIN, 8719, "Spawn Alliance Auctioneer?", GuildHouseAuctioneer, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                         player->FindNearestCreature(9856, VISIBILITY_RANGE, true) ? "Spawn Horde Auctioneer (Already Purchased)" : "Spawn Horde Auctioneer",
                         GOSSIP_SENDER_MAIN, 9856, "Spawn Horde Auctioneer?", GuildHouseAuctioneer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Spawn Primary Profession Trainers", GOSSIP_SENDER_MAIN, 7);
        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Spawn Secondary Profession Trainers", GOSSIP_SENDER_MAIN, 8);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
                         player->FindNearestCreature(6491, VISIBILITY_RANGE, true) ? "Spawn Spirit Healer (Already Purchased)" : "Spawn Spirit Healer",
                         GOSSIP_SENDER_MAIN, 6491, "Spawn a Spirit Healer?", GuildHouseSpirit, false);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *creature, uint32 /*sender*/, uint32 action) override
    {

        switch (action)
        {
        case 2: // Spawn Class Trainer
            ClearGossipMenuFor(player);
            {
                bool npcExists = player->FindNearestCreature(29195, VISIBILITY_RANGE, true);
                std::string npcText = npcExists ? "Spawn Death Knight Trainer (Already Purchased)" : "Spawn Death Knight Trainer";
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, npcText, GOSSIP_SENDER_MAIN, 29195, "Spawn Death Knight Trainer?", GuildHouseTrainer, false);
            }
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

            // Add neutral objects
            AddGossipItemFor(player, GOSSIP_ICON_TALK,
                             player->FindNearestGameObject(1685, VISIBLE_RANGE) ? "Forge (Already Purchased)" : "Forge",
                             GOSSIP_SENDER_MAIN, 1685, "Add a forge?", GuildHouseObject, false);

            AddGossipItemFor(player, GOSSIP_ICON_TALK,
                             player->FindNearestGameObject(4087, VISIBLE_RANGE) ? "Anvil (Already Purchased)" : "Anvil",
                             GOSSIP_SENDER_MAIN, 4087, "Add an Anvil?", GuildHouseObject, false);

            // Add all portals (both Alliance and Horde)
            AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                             player->FindNearestGameObject(500000, VISIBLE_RANGE) ? "Portal: Stormwind (Already Purchased)" : "Portal: Stormwind",
                             GOSSIP_SENDER_MAIN, 500000, "Add Stormwind Portal?", GuildHousePortal, false);

            AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                             player->FindNearestGameObject(500004, VISIBLE_RANGE) ? "Portal: Orgrimmar (Already Purchased)" : "Portal: Orgrimmar",
                             GOSSIP_SENDER_MAIN, 500004, "Add Orgrimmar Portal?", GuildHousePortal, false);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Go Back!", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 6: // Auctioneer
        {
            uint32 auctioneer = 0;
            auctioneer = player->GetTeamId() == TEAM_ALLIANCE ? 8719 : 9856;
            SpawnNPC(auctioneer, player);
            player->ModifyMoney(-cost);
            break;
        }
        case 9858: // Neutral Auctioneer
            cost = GuildHouseAuctioneer;
            SpawnNPC(action, player);
            player->ModifyMoney(-cost);
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
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Neutral Jewelcrafting Trainer", GOSSIP_SENDER_MAIN, 500035, "Spawn Neutral Jewelcrafting Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Neutral Enchanting Trainer", GOSSIP_SENDER_MAIN, 500036, "Spawn Neutral Enchanting Trainer?", GuildHouseTrainer, false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Neutral Inscription Trainer", GOSSIP_SENDER_MAIN, 500037, "Spawn Neutral Inscription Trainer?", GuildHouseTrainer, false);
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
        case 9: // Go Back!
            OnGossipHello(player, creature);
            break;
        case 10: // PVP toggle
            break;
        case 30605: // Banker
            cost = GuildHouseBank;
            SpawnNPC(action, player);
            player->ModifyMoney(-cost);
            break;
        case 500032: // Innkeeper
            cost = GuildHouseInnKeeper;
            SpawnNPC(action, player);
            player->ModifyMoney(-cost);
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
            player->ModifyMoney(-cost);
            break;
        case 2836:   // Blacksmithing
        case 8128:   // Mining
        case 8736:   // Engineering
        case 500035: // Neutral Jewelcrafting Trainer
        case 500036: // Neutral Enchanting Trainer
        case 500037: // Neutral Inscription Trainer
        case 19187:  // Leatherworking
        case 19180:  // Skinning
        case 19052:  // Alchemy
        case 908:    // Herbalism
        case 2627:   // Tailoring
        case 19185:  // Cooking
        case 2834:   // Fishing
        case 19184:  // First Aid
            cost = GuildHouseProf;
            SpawnNPC(action, player);
            player->ModifyMoney(-cost);
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
            player->ModifyMoney(-cost);
            break;
        case 184137: // Mailbox
            cost = GuildHouseMailBox;
            SpawnObject(action, player);
            player->ModifyMoney(-cost);
            break;
        case 6491: // Spirit Healer
            cost = GuildHouseSpirit;
            SpawnNPC(action, player);
            player->ModifyMoney(-cost);
            break;
        case 1685:   // Forge
        case 4087:   // Anvil
        case 187293: // Guild Vault
        case 191028: // Barber Chair
            cost = GuildHouseObject;
            SpawnObject(action, player);
            player->ModifyMoney(-cost);
            break;
        case 500000: // Stormwind Portal
        case 500001: // Darnassus Portal
        case 500002: // Exodar Portal
        case 500003: // Ironforge Portal
        case 500004: // Orgrimmar Portal
        case 500005: // Silvermoon Portal
        case 500006: // Thunder Bluff Portal
        case 500007: // Undercity Portal
        case 500008: // Shattrath Portal
        case 500009: // Dalaran Portal
            cost = GuildHousePortal;
            SpawnObject(action, player);
            player->ModifyMoney(-cost);
            break;
        }
        return true;
    }

    uint32 GetGuildPhase(Player *player)
    {
        return player->GetGuildId() + 10;
    };

    void SpawnNPC(uint32 entry, Player *player)
    {
        if (player->FindNearestCreature(entry, VISIBILITY_RANGE, true))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("This trainer already exists!");
            CloseGossipMenuFor(player);
            return;
        }

        float posX;
        float posY;
        float posZ;
        float ori;

        float basePosX = 16232.9f; // Base X coordinate for the portals
        float basePosY = 16264.1f; // Base Y coordinate for the first portal
        float basePosZ = 13.5557f; // Z coordinate (same for all portals)
        float spacing = 3.0f;      // Spacing between each portal
        ori = 0.0f;                // Orientation (default)

        uint32 portalIndex = 0;
        switch (entry)
        {
        case 500000: // Stormwind
            portalIndex = 0;
            break;
        case 500004: // Orgrimmar
            portalIndex = 1;
            break;
        case 500008: // Shattrath
            portalIndex = 2;
            break;
        case 500009: // Dalaran
            portalIndex = 3;
            break;
        default:
            ChatHandler(player->GetSession()).PSendSysMessage("Invalid portal entry ID!");
            return;
        }

        posX = basePosX;
        posY = basePosY + (portalIndex * spacing);
        posZ = basePosZ;

        Creature *creature = new Creature();
        if (!creature->Create(player->GetMap()->GenerateLowGuid<HighGuid::Unit>(), player->GetMap(), GetGuildPhase(player), entry, 0, posX, posY, posZ, ori))
        {
            delete creature;
            return;
        }
        creature->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), GetGuildPhase(player));
        uint32 db_guid = creature->GetSpawnId();
        delete creature;

        creature = new Creature();
        if (!creature->LoadCreatureFromDB(db_guid, player->GetMap()))
        {
            delete creature;
            return;
        }

        sObjectMgr->AddCreatureToGrid(db_guid, sObjectMgr->GetCreatureData(db_guid));
    }

    void SpawnObject(uint32 entry, Player *player)
    {
        if (player->FindNearestGameObject(entry, VISIBLE_RANGE))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("This portal already exists!");
            CloseGossipMenuFor(player);
            return;
        }

        float posX;
        float posY;
        float posZ;
        float ori;

        QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);

        if (!result)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Failed to find spawn data for this object!");
            return;
        }

        do
        {
            Field *fields = result->Fetch();
            posX = fields[0].Get<float>();
            posY = fields[1].Get<float>();
            posZ = fields[2].Get<float>();
            ori = fields[3].Get<float>();
        } while (result->NextRow());

        uint32 objectId = entry;
        if (!objectId)
            return;

        const GameObjectTemplate *objectInfo = sObjectMgr->GetGameObjectTemplate(objectId);
        if (!objectInfo)
            return;

        if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
            return;

        GameObject *object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
        ObjectGuid::LowType guidLow = player->GetMap()->GenerateLowGuid<HighGuid::GameObject>();

        if (!object->Create(guidLow, objectInfo->entry, player->GetMap(), GetGuildPhase(player), posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete object;
            return;
        }

        object->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), GetGuildPhase(player));
        guidLow = object->GetSpawnId();
        delete object;

        object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
        if (!object->LoadGameObjectFromDB(guidLow, player->GetMap(), true))
        {
            delete object;
            return;
        }

        sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGameObjectData(guidLow));
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
