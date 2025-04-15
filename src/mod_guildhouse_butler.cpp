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
        if (!player->GetGuild())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You are not in a guild!");
            return false;
        }

        Guild *guild = sGuildMgr->GetGuildById(player->GetGuildId());
        Guild::Member const *memberMe = guild->GetMember(player->GetGUID());
        if (!memberMe->IsRankNotLower(GuildHouseBuyRank))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You are not authorized to make Guild House purchases.");
            return false;
        }

        ClearGossipMenuFor(player);

        // Innkeeper
        bool innkeeperSpawned = false;
        uint32 guildPhase = GetGuildPhase(player);
        for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
        {
            Creature *c = pair.second;
            if (c && c->GetEntry() == 500032 && c->GetPhaseMask() == guildPhase)
            {
                innkeeperSpawned = true;
                break;
            }
        }
        if (!innkeeperSpawned)
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Innkeeper", GOSSIP_SENDER_MAIN, 500032, "Add an Innkeeper?", GuildHouseInnKeeper, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Innkeeper (Refund 50%)", GOSSIP_SENDER_MAIN, 700032, "Remove the Innkeeper and get a refund?", GuildHouseInnKeeper / 2, false);

        // Mailbox
        if (!player->FindNearestGameObject(184137, VISIBLE_RANGE))
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Mailbox", GOSSIP_SENDER_MAIN, 184137, "Spawn a Mailbox?", GuildHouseMailBox, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Mailbox (Refund 50%)", GOSSIP_SENDER_MAIN, 700137, "Remove the Mailbox and get a refund?", GuildHouseMailBox / 2, false);

        // Stable Master
        bool stableMasterSpawned = false;
        for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
        {
            Creature *c = pair.second;
            if (c && c->GetEntry() == 28690 && c->GetPhaseMask() == guildPhase)
            {
                stableMasterSpawned = true;
                break;
            }
        }
        if (!stableMasterSpawned)
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Stable Master", GOSSIP_SENDER_MAIN, 28690, "Spawn a Stable Master?", GuildHouseVendor, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Stable Master (Refund 50%)", GOSSIP_SENDER_MAIN, 7028690, "Remove the Stable Master and get a refund?", GuildHouseVendor / 2, false);

        // Bank
        if (!player->FindNearestCreature(30605, VISIBILITY_RANGE, true))
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Bank", GOSSIP_SENDER_MAIN, 30605, "Spawn a Banker?", GuildHouseBank, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Banker (Refund 50%)", GOSSIP_SENDER_MAIN, 700605, "Remove the Banker and get a refund?", GuildHouseBank / 2, false);

        // Auctioneers
        if (!player->FindNearestCreature(9858, VISIBILITY_RANGE, true))
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Neutral Auctioneer", GOSSIP_SENDER_MAIN, 9858, "Spawn a Neutral Auctioneer?", GuildHouseAuctioneer, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Neutral Auctioneer (Refund 50%)", GOSSIP_SENDER_MAIN, 709858, "Remove the Neutral Auctioneer and get a refund?", GuildHouseAuctioneer / 2, false);

        if (!player->FindNearestCreature(8719, VISIBILITY_RANGE, true))
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Alliance Auctioneer", GOSSIP_SENDER_MAIN, 8719, "Spawn Alliance Auctioneer?", GuildHouseAuctioneer, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Alliance Auctioneer (Refund 50%)", GOSSIP_SENDER_MAIN, 708719, "Remove the Alliance Auctioneer and get a refund?", GuildHouseAuctioneer / 2, false);

        if (!player->FindNearestCreature(9856, VISIBILITY_RANGE, true))
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Horde Auctioneer", GOSSIP_SENDER_MAIN, 9856, "Spawn Horde Auctioneer?", GuildHouseAuctioneer, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Horde Auctioneer (Refund 50%)", GOSSIP_SENDER_MAIN, 709856, "Remove the Horde Auctioneer and get a refund?", GuildHouseAuctioneer / 2, false);

        // Spirit Healer
        bool spiritHealerSpawned = false;
        for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
        {
            Creature *c = pair.second;
            if (c && c->GetEntry() == 6491 && c->GetPhaseMask() == guildPhase)
            {
                spiritHealerSpawned = true;
                break;
            }
        }
        if (!spiritHealerSpawned)
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Spirit Healer", GOSSIP_SENDER_MAIN, 6491, "Spawn a Spirit Healer?", GuildHouseSpirit, false);
        else
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Spirit Healer (Refund 50%)", GOSSIP_SENDER_MAIN, 706491, "Remove the Spirit Healer and get a refund?", GuildHouseSpirit / 2, false);

        // Submenus
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Class Trainer", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Vendor", GOSSIP_SENDER_MAIN, 3);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn City Portals / Objects", GOSSIP_SENDER_MAIN, 4);

        // Reset
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Reset Guild House (Remove & Re-place All)", GOSSIP_SENDER_MAIN, 900000, "Are you sure you want to reset your Guild House? This will remove and re-place all purchased objects and NPCs.", 0, false);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *creature, uint32 /*sender*/, uint32 action) override
    {
        switch (action)
        {
        case 9:  // Go Back!
        case 10: // PVP toggle
            OnGossipHello(player, creature);
            break;

        // Innkeeper
        case 500032:
            if (!player->FindNearestCreature(500032, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 500032);
                SpawnNPC(500032, player);
                player->ModifyMoney(-GuildHouseInnKeeper);
                ChatHandler(player->GetSession()).PSendSysMessage("Innkeeper purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Innkeeper already exists.");
            OnGossipHello(player, creature);
            break;
        case 700032:
            if (Creature *found = player->FindNearestCreature(500032, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 500032);
                player->ModifyMoney(GuildHouseInnKeeper / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Innkeeper removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Mailbox
        case 184137:
            if (!player->FindNearestGameObject(184137, VISIBLE_RANGE))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), 184137);
                SpawnObject(184137, player, creature);
                player->ModifyMoney(-GuildHouseMailBox);
                ChatHandler(player->GetSession()).PSendSysMessage("Mailbox purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Mailbox already exists.");
            OnGossipHello(player, creature);
            break;
        case 700137:
            if (GameObject *object = player->FindNearestGameObject(184137, VISIBLE_RANGE))
            {
                object->DeleteFromDB();
                object->RemoveFromWorld();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), 184137);
                player->ModifyMoney(GuildHouseMailBox / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Mailbox removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Stable Master
        case 28690:
            if (!player->FindNearestCreature(28690, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 28690);
                SpawnNPC(28690, player);
                player->ModifyMoney(-GuildHouseVendor);
                ChatHandler(player->GetSession()).PSendSysMessage("Stable Master purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Stable Master already exists.");
            OnGossipHello(player, creature);
            break;
        case 7028690:
            if (Creature *found = player->FindNearestCreature(28690, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 28690);
                player->ModifyMoney(GuildHouseVendor / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Stable Master removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Bank
        case 30605:
            if (!player->FindNearestCreature(30605, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 30605);
                SpawnNPC(30605, player);
                player->ModifyMoney(-GuildHouseBank);
                ChatHandler(player->GetSession()).PSendSysMessage("Banker purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Banker already exists.");
            OnGossipHello(player, creature);
            break;
        case 700605:
            if (Creature *found = player->FindNearestCreature(30605, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 30605);
                player->ModifyMoney(GuildHouseBank / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Banker removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Neutral Auctioneer
        case 9858:
            if (!player->FindNearestCreature(9858, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 9858);
                SpawnNPC(9858, player);
                player->ModifyMoney(-GuildHouseAuctioneer);
                ChatHandler(player->GetSession()).PSendSysMessage("Neutral Auctioneer purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Neutral Auctioneer already exists.");
            OnGossipHello(player, creature);
            break;
        case 709858:
            if (Creature *found = player->FindNearestCreature(9858, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 9858);
                player->ModifyMoney(GuildHouseAuctioneer / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Neutral Auctioneer removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Alliance Auctioneer
        case 8719:
            if (!player->FindNearestCreature(8719, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 8719);
                SpawnNPC(8719, player);
                player->ModifyMoney(-GuildHouseAuctioneer);
                ChatHandler(player->GetSession()).PSendSysMessage("Alliance Auctioneer purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Alliance Auctioneer already exists.");
            OnGossipHello(player, creature);
            break;
        case 708719:
            if (Creature *found = player->FindNearestCreature(8719, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 8719);
                player->ModifyMoney(GuildHouseAuctioneer / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Alliance Auctioneer removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Horde Auctioneer
        case 9856:
            if (!player->FindNearestCreature(9856, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 9856);
                SpawnNPC(9856, player);
                player->ModifyMoney(-GuildHouseAuctioneer);
                ChatHandler(player->GetSession()).PSendSysMessage("Horde Auctioneer purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Horde Auctioneer already exists.");
            OnGossipHello(player, creature);
            break;
        case 709856:
            if (Creature *found = player->FindNearestCreature(9856, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 9856);
                player->ModifyMoney(GuildHouseAuctioneer / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Horde Auctioneer removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Spirit Healer
        case 6491:
            if (!player->FindNearestCreature(6491, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 6491);
                SpawnNPC(6491, player);
                player->ModifyMoney(-GuildHouseSpirit);
                ChatHandler(player->GetSession()).PSendSysMessage("Spirit Healer purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Spirit Healer already exists.");
            OnGossipHello(player, creature);
            break;
        case 706491:
            if (Creature *found = player->FindNearestCreature(6491, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 6491);
                player->ModifyMoney(GuildHouseSpirit / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Spirit Healer removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Stormwind Portal
        case 500000:
            if (!player->FindNearestGameObject(500000, VISIBLE_RANGE))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), 500000);
                SpawnObject(500000, player, creature);
                player->ModifyMoney(-GuildHousePortal);
                ChatHandler(player->GetSession()).PSendSysMessage("Stormwind Portal purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Stormwind Portal already exists.");
            OnGossipHello(player, creature);
            break;

        case 700000:
            if (GameObject *object = player->FindNearestGameObject(500000, VISIBLE_RANGE))
            {
                object->DeleteFromDB();
                object->RemoveFromWorld();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), 500000);
                player->ModifyMoney(GuildHousePortal / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Stormwind Portal removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Darnassus Portal
        case 500001:
            if (!player->FindNearestGameObject(500001, VISIBLE_RANGE))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), 500001);
                SpawnObject(500001, player, creature);
                player->ModifyMoney(-GuildHousePortal);
                ChatHandler(player->GetSession()).PSendSysMessage("Darnassus Portal purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Darnassus Portal already exists.");
            OnGossipHello(player, creature);
            break;

        case 700001:
            if (GameObject *object = player->FindNearestGameObject(500001, VISIBLE_RANGE))
            {
                object->DeleteFromDB();
                object->RemoveFromWorld();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), 500001);
                player->ModifyMoney(GuildHousePortal / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Darnassus Portal removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Ironforge Portal
        case 500003:
            if (!player->FindNearestGameObject(500003, VISIBLE_RANGE))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), 500003);
                SpawnObject(500003, player, creature);
                player->ModifyMoney(-GuildHousePortal);
                ChatHandler(player->GetSession()).PSendSysMessage("Ironforge Portal purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Ironforge Portal already exists.");
            OnGossipHello(player, creature);
            break;
        case 700003:
            if (GameObject *object = player->FindNearestGameObject(500003, VISIBLE_RANGE))
            {
                object->DeleteFromDB();
                object->RemoveFromWorld();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), 500003);
                player->ModifyMoney(GuildHousePortal / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Ironforge Portal removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Orgrimmar Portal
        case 500004:
            if (!player->FindNearestGameObject(500004, VISIBLE_RANGE))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), 500004);
                SpawnObject(500004, player, creature);
                player->ModifyMoney(-GuildHousePortal);
                ChatHandler(player->GetSession()).PSendSysMessage("Orgrimmar Portal purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Orgrimmar Portal already exists.");
            OnGossipHello(player, creature);
            break;
        case 700004:
            if (GameObject *object = player->FindNearestGameObject(500004, VISIBLE_RANGE))
            {
                object->DeleteFromDB();
                object->RemoveFromWorld();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), 500004);
                player->ModifyMoney(GuildHousePortal / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Orgrimmar Portal removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Silvermoon Portal
        case 500005:
            if (!player->FindNearestGameObject(500005, VISIBLE_RANGE))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), 500005);
                SpawnObject(500005, player, creature);
                player->ModifyMoney(-GuildHousePortal);
                ChatHandler(player->GetSession()).PSendSysMessage("Silvermoon Portal purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Silvermoon Portal already exists.");
            OnGossipHello(player, creature);
            break;
        case 700005:
            if (GameObject *object = player->FindNearestGameObject(500005, VISIBLE_RANGE))
            {
                object->DeleteFromDB();
                object->RemoveFromWorld();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), 500005);
                player->ModifyMoney(GuildHousePortal / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Silvermoon Portal removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Reset Guild House
        case 900000:
        {
            uint32 guildPhase = GetGuildPhase(player);
            Map *map = player->GetMap();

            for (auto const &creaturePair : map->GetCreatureBySpawnIdStore())
            {
                Creature *c = creaturePair.second;
                if (c && c->GetPhaseMask() == guildPhase)
                {
                    // Do not remove the butler
                    if (c->GetEntry() == 500031)
                        continue;

                    c->DespawnOrUnsummon();
                    c->DeleteFromDB();
                }
            }
            for (auto const &goPair : map->GetGameObjectBySpawnIdStore())
            {
                GameObject *go = goPair.second;
                if (go && go->GetPhaseMask() == guildPhase)
                {
                    go->DeleteFromDB();
                    go->RemoveFromWorld();
                }
            }
            QueryResult result = WorldDatabase.Query("SELECT `entry`, `type` FROM `guild_house_purchases` WHERE `guild`={}", player->GetGuildId());
            if (result)
            {
                do
                {
                    Field *fields = result->Fetch();
                    uint32 entry = fields[0].Get<uint32>();
                    std::string type = fields[1].Get<std::string>();

                    if (type == "creature")
                        SpawnNPC(entry, player);
                    else if (type == "gameobject")
                        SpawnObject(entry, player, creature);
                } while (result->NextRow());
            }
            ChatHandler(player->GetSession()).PSendSysMessage("Guild House has been reset and all purchased items restored!");
            OnGossipHello(player, creature);
            break;
        }

        // Class Trainer submenu
        case 2:
            ClearGossipMenuFor(player);
            {
                struct TrainerEntry
                {
                    uint32 entry;
                    const char *label;
                    int cost;
                };
                TrainerEntry trainers[] = {
                    {26327, "Paladin Trainer", GuildHouseTrainer},
                    {26324, "Druid Trainer", GuildHouseTrainer},
                    {26325, "Hunter Trainer", GuildHouseTrainer},
                    {26326, "Mage Trainer", GuildHouseTrainer},
                    {26328, "Priest Trainer", GuildHouseTrainer},
                    {26329, "Rogue Trainer", GuildHouseTrainer},
                    {26330, "Shaman Trainer", GuildHouseTrainer},
                    {26331, "Warlock Trainer", GuildHouseTrainer},
                    {26332, "Warrior Trainer", GuildHouseTrainer},
                    {29195, "Death Knight Trainer", GuildHouseTrainer},
                    {2836, "Blacksmithing Trainer", GuildHouseProf},
                    {8128, "Mining Trainer", GuildHouseProf},
                    {8736, "Engineering Trainer", GuildHouseProf},
                    {19187, "Leatherworking Trainer", GuildHouseProf},
                    {19180, "Skinning Trainer", GuildHouseProf},
                    {19052, "Alchemy Trainer", GuildHouseProf},
                    {908, "Herbalism Trainer", GuildHouseProf},
                    {2627, "Tailoring Trainer", GuildHouseProf},
                    {19184, "First Aid Trainer", GuildHouseProf},
                    {2834, "Fishing Trainer", GuildHouseProf},
                    {19185, "Cooking Trainer", GuildHouseProf}};
                uint32 guildPhase = GetGuildPhase(player);
                for (auto const &t : trainers)
                {
                    bool spawned = false;
                    for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                    {
                        Creature *c = pair.second;
                        if (c && c->GetEntry() == t.entry && c->GetPhaseMask() == guildPhase)
                        {
                            spawned = true;
                            break;
                        }
                    }
                    if (!spawned)
                        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string("Spawn ") + t.label, GOSSIP_SENDER_MAIN, t.entry, std::string("Spawn a ") + t.label + "?", t.cost, false);
                    else
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, std::string("Remove ") + t.label + " (Refund 50%)", GOSSIP_SENDER_MAIN, 7000000 + t.entry, std::string("Remove the ") + t.label + " and get a refund?", t.cost / 2, false);
                }
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Trainers", GOSSIP_SENDER_MAIN, 10002, "Buy and spawn all trainers?", GuildHouseTrainer * 10, false); // Adjust cost as needed
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;

        // Vendor submenu
        case 3:
            ClearGossipMenuFor(player);
            // Trade Supplies
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 28692 && c->GetPhaseMask() == GetGuildPhase(player))
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Trade Supplies", GOSSIP_SENDER_MAIN, 28692, "Spawn a Trade Supplies Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Trade Supplies (Refund 50%)", GOSSIP_SENDER_MAIN, 7028692, "Remove the Trade Supplies Vendor and get a refund?", GuildHouseVendor / 2, false);
            }
            // Tabard Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 28776 && c->GetPhaseMask() == GetGuildPhase(player))
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Tabard Vendor", GOSSIP_SENDER_MAIN, 28776, "Spawn a Tabard Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Tabard Vendor (Refund 50%)", GOSSIP_SENDER_MAIN, 7028776, "Remove the Tabard Vendor and get a refund?", GuildHouseVendor / 2, false);
            }
            // Food & Drink Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 4255 && c->GetPhaseMask() == GetGuildPhase(player))
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Food & Drink Vendor", GOSSIP_SENDER_MAIN, 4255, "Spawn a Food & Drink Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Food & Drink Vendor (Refund 50%)", GOSSIP_SENDER_MAIN, 704255, "Remove the Food & Drink Vendor and get a refund?", GuildHouseVendor / 2, false);
            }
            // Reagent Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 29636 && c->GetPhaseMask() == GetGuildPhase(player))
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Reagent Vendor", GOSSIP_SENDER_MAIN, 29636, "Spawn a Reagent Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Reagent Vendor (Refund 50%)", GOSSIP_SENDER_MAIN, 7029636, "Remove the Reagent Vendor and get a refund?", GuildHouseVendor / 2, false);
            }
            // Ammo & Repair Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 29493 && c->GetPhaseMask() == GetGuildPhase(player))
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Ammo & Repair Vendor", GOSSIP_SENDER_MAIN, 29493, "Spawn an Ammo & Repair Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Ammo & Repair Vendor (Refund 50%)", GOSSIP_SENDER_MAIN, 7029493, "Remove the Ammo & Repair Vendor and get a refund?", GuildHouseVendor / 2, false);
            }
            // Poisons Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 2622 && c->GetPhaseMask() == GetGuildPhase(player))
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Poisons Vendor", GOSSIP_SENDER_MAIN, 2622, "Spawn a Poisons Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Poisons Vendor (Refund 50%)", GOSSIP_SENDER_MAIN, 702622, "Remove the Poisons Vendor and get a refund?", GuildHouseVendor / 2, false);
            }
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Vendors", GOSSIP_SENDER_MAIN, 10003, "Buy and spawn all vendors?", GuildHouseVendor * 6, false); // Adjust cost as needed
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;

        // City Portals / Objects submenu
        case 4:
            ClearGossipMenuFor(player);
            {
                std::set<uint32> portalEntries = {
                    500000, // Stormwind Portal
                    500001, // Darnassus Portal
                    500003, // Ironforge Portal
                    500004, // Orgrimmar Portal
                    500005, // Silvermoon Portal
                    1685,   // Forge
                    4087,   // Anvil
                    180715, // Alchemy Lab
                    2728    // Cooking Fire
                };
                for (auto entry : portalEntries)
                {
                    int cost = (entry >= 500000 && entry <= 500005) ? GuildHousePortal : GuildHouseObject;
                    std::string label;
                    switch (entry)
                    {
                    case 500000:
                        label = "Stormwind Portal";
                        break;
                    case 500001:
                        label = "Darnassus Portal";
                        break;
                    case 500003:
                        label = "Ironforge Portal";
                        break;
                    case 500004:
                        label = "Orgrimmar Portal";
                        break;
                    case 500005:
                        label = "Silvermoon Portal";
                        break;
                    case 1685:
                        label = "Forge";
                        break;
                    case 4087:
                        label = "Anvil";
                        break;
                    case 180715:
                        label = "Alchemy Lab";
                        break;
                    case 2728:
                        label = "Cooking Fire";
                        break;
                    default:
                        label = "Object";
                        break;
                    }
                    if (!player->FindNearestGameObject(entry, VISIBLE_RANGE))
                        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Spawn " + label, GOSSIP_SENDER_MAIN, entry, "Spawn a " + label + "?", cost, false);
                    else
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove " + label + " (Refund 50%)", GOSSIP_SENDER_MAIN, 700000 + entry, "Remove the " + label + " and get a refund?", cost / 2, false);
                }
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Portals", GOSSIP_SENDER_MAIN, 10004, "Buy and spawn all portals?", GuildHousePortal * 5, false); // Adjust cost as needed
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;

        // Trade Supplies Vendor
        case 28692:
            if (!player->FindNearestCreature(28692, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 28692);
                SpawnNPC(28692, player);
                player->ModifyMoney(-GuildHouseVendor);
                ChatHandler(player->GetSession()).PSendSysMessage("Trade Supplies Vendor purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Trade Supplies Vendor already exists.");
            OnGossipHello(player, creature);
            break;

        // Tabard Vendor
        case 28776:
            if (!player->FindNearestCreature(28776, VISIBILITY_RANGE, true))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 28776);
                SpawnNPC(28776, player);
                player->ModifyMoney(-GuildHouseVendor);
                ChatHandler(player->GetSession()).PSendSysMessage("Tabard Vendor purchased and spawned.");
            }
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Tabard Vendor already exists.");
            OnGossipHello(player, creature);
            break;

        case 7028776:
            if (Creature *found = player->FindNearestCreature(28776, VISIBILITY_RANGE, true))
            {
                found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 28776);
                player->ModifyMoney(GuildHouseVendor / 2);
                ChatHandler(player->GetSession()).PSendSysMessage("Tabard Vendor removed and 50% of the cost refunded.");
            }
            OnGossipHello(player, creature);
            break;

        // Buy All Trainers
        case 10002: // Buy All Trainers
        {
            std::set<uint32> trainerEntries = {
                26327, 26324, 26325, 26326, 26328, 26329, 26330, 26331, 26332, 29195,
                2836, 8128, 8736, 19187, 19180, 19052, 908, 2627, 19184, 2834, 19185};
            uint32 guildPhase = GetGuildPhase(player);
            int totalCost = 0;
            for (auto entry : trainerEntries)
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                {
                    WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), entry);
                    SpawnNPC(entry, player);
                    // Use correct cost for profession trainers
                    if (entry == 2836 || entry == 8128 || entry == 8736 || entry == 19187 || entry == 19180 ||
                        entry == 19052 || entry == 908 || entry == 2627 || entry == 19184 || entry == 2834 || entry == 19185)
                        totalCost += GuildHouseProf;
                    else
                        totalCost += GuildHouseTrainer;
                }
            }
            if (totalCost > 0)
            {
                player->ModifyMoney(-totalCost);
                ChatHandler(player->GetSession()).PSendSysMessage("All missing trainers purchased and spawned for %ug.", totalCost / 10000);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All trainers are already purchased.");
            }
            OnGossipHello(player, creature);
            break;
        }

        // Buy All Vendors
        case 10003: // Buy All Vendors
        {
            std::set<uint32> vendorEntries = {
                28692, // Trade Supplies
                28776, // Tabard Vendor
                4255,  // Food & Drink Vendor
                29636, // Reagent Vendor
                29493, // Ammo & Repair Vendor
                2622   // Poisons Vendor
            };
            uint32 guildPhase = GetGuildPhase(player);
            int totalCost = 0;
            for (auto entry : vendorEntries)
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                {
                    WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), entry);
                    SpawnNPC(entry, player);
                    totalCost += GuildHouseVendor;
                }
            }
            if (totalCost > 0)
            {
                player->ModifyMoney(-totalCost);
                ChatHandler(player->GetSession()).PSendSysMessage("All missing vendors purchased and spawned for %ug.", totalCost / 10000);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All vendors are already purchased.");
            }
            OnGossipHello(player, creature);
            break;
        }

        // Buy All Portals
        case 10004:
        {
            std::set<uint32> portalEntries = {
                500000, // Stormwind Portal
                500001, // Darnassus Portal
                500003, // Ironforge Portal
                500004, // Orgrimmar Portal
                500005, // Silvermoon Portal
                1685,   // Forge
                4087,   // Anvil
                180715, // Alchemy Lab
                2728    // Cooking Fire
            };
            uint32 guildPhase = GetGuildPhase(player);
            int totalCost = 0;
            for (auto entry : portalEntries)
            {
                if (!player->FindNearestGameObject(entry, VISIBLE_RANGE))
                {
                    WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), entry);
                    SpawnObject(entry, player, creature);
                    // Use correct cost for portals vs. objects
                    if (entry >= 500000 && entry <= 500005)
                        totalCost += GuildHousePortal;
                    else
                        totalCost += GuildHouseObject;
                }
            }
            if (totalCost > 0)
            {
                player->ModifyMoney(-totalCost);
                ChatHandler(player->GetSession()).PSendSysMessage("All missing portals/objects purchased and spawned for %ug.", totalCost / 10000);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All portals/objects are already purchased.");
            }
            OnGossipHello(player, creature);
            break;
        }

        default:
        {
            std::set<uint32> trainerEntries = {
                26327, 26324, 26325, 26326, 26328, 26329, 26330, 26331, 26332, 29195,
                2836, 8128, 8736, 19187, 19180, 19052, 908, 2627, 19184, 2834, 19185};
            std::set<uint32> vendorEntries = {
                28692, 28776, 4255, 29636, 29493, 2622};

            if (trainerEntries.count(action))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), action);
                SpawnNPC(action, player);
                player->ModifyMoney(-GuildHouseTrainer);
                ChatHandler(player->GetSession()).PSendSysMessage("Trainer purchased and spawned.");
            }
            else if (vendorEntries.count(action))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), action);
                SpawnNPC(action, player);
                player->ModifyMoney(-GuildHouseVendor);
                ChatHandler(player->GetSession()).PSendSysMessage("Vendor purchased and spawned.");
            }
            else
            {
                cost = GuildHouseTrainer; // fallback
                SpawnObject(action, player, creature);
                player->ModifyMoney(-cost);
            }
            break;
        }
        }
        return true;
    }

    uint32 GetGuildPhase(Player *player)
    {
        return player->GetGuildId() + 10;
    };

    void SpawnNPC(uint32 entry, Player *player)
    {
        if (!player || !player->GetMap())
        {
            ChatHandler(player ? player->GetSession() : nullptr).PSendSysMessage("Error: Invalid player or map context.");
            return;
        }

        uint32 guildPhase = GetGuildPhase(player);

        // Check for existing NPC in the same phase
        for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
        {
            Creature *c = pair.second;
            if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("This NPC already exists!");
                CloseGossipMenuFor(player);
                return;
            }
        }

        float posX, posY, posZ, ori;
        QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);
        if (!result)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("No spawn location found for this NPC.");
            CloseGossipMenuFor(player);
            return;
        }
        Field *fields = result->Fetch();
        posX = fields[0].Get<float>();
        posY = fields[1].Get<float>();
        posZ = fields[2].Get<float>();
        ori = fields[3].Get<float>();

        Creature *creature = new Creature();
        if (!creature->Create(player->GetMap()->GenerateLowGuid<HighGuid::Unit>(), player->GetMap(), guildPhase, entry, 0, posX, posY, posZ, ori))
        {
            delete creature;
            ChatHandler(player->GetSession()).PSendSysMessage("Failed to create NPC.");
            return;
        }
        creature->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), guildPhase);

        // Add the creature to the world immediately
        creature->AddToWorld();

        // Refresh the gossip menu
        OnGossipHello(player, creature);
    }

    void SpawnObject(uint32 entry, Player *player, Creature *creature)
    {
        if (player->FindNearestGameObject(entry, VISIBLE_RANGE))
        {
            return;
        }

        float posX, posY, posZ, ori;
        uint32 guildPhase = GetGuildPhase(player);

        // Fetch position from DB
        QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);
        if (!result)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("No spawn location found for this object.");
            return;
        }
        Field *fields = result->Fetch();
        posX = fields[0].Get<float>();
        posY = fields[1].Get<float>();
        posZ = fields[2].Get<float>();
        ori = fields[3].Get<float>();

        GameObject *newObject = new GameObject();
        if (!newObject->Create(player->GetMap()->GenerateLowGuid<HighGuid::GameObject>(), entry, player->GetMap(), guildPhase, posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete newObject;
            return;
        }
        newObject->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), guildPhase);
        uint32 db_guid = newObject->GetSpawnId();
        delete newObject;

        newObject = new GameObject();
        if (!newObject->LoadGameObjectFromDB(db_guid, player->GetMap(), true))
        {
            delete newObject;
            return;
        }

        sObjectMgr->AddGameobjectToGrid(db_guid, sObjectMgr->GetGameObjectData(db_guid));

        // Refresh the gossip menu
        OnGossipHello(player, creature);
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

        WorldDatabase.Execute("UPDATE guild_house_spawns SET posX=16236.0, posY=16264.1 WHERE entry=500004;"); // Move Orgrimmar portal
    }
};

void AddGuildHouseButlerScripts()
{
    new GuildHouseSpawner();
    new GuildHouseButlerConf();
}
