#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Configuration/Config.h"
#include "Creature.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Define.h" // This header defines uint32 and other types
#include "GossipDef.h"
#include "DataMap.h"
#include "GameObject.h"
#include "Transport.h"
#include "CreatureAI.h"
#include "MapMgr.h"

inline uint32 GetGuildPhase(Player *player)
{
    extern uint32 GuildHousePhaseBase;
    return GuildHousePhaseBase + player->GetGuildId();
}

void ClearGuildHousePhase(uint32 guildId, uint32 guildPhase, Map *map);
void SpawnNPC(uint32 entry, Player *player, bool force = false);
// Global configuration variables for guild house features
int cost, GuildHouseInnKeeper, GuildHouseBank, GuildHouseMailBox, GuildHouseAuctioneer, GuildHouseTrainer, GuildHouseVendor, GuildHouseObject, GuildHousePortal, GuildHouseSpirit, GuildHouseProf, GuildHouseBuyRank;
int GuildHouseRefundPercent = 50;  // default
uint32 GuildHouseMarkerEntry;      // read from config
extern uint32 GuildHousePhaseBase; // declared here, defined in mod_guildhouse.cpp

// Essential NPC entries for guild house management
std::set<uint32> essentialNpcEntries = {
    6930,  // Innkeeper
    28690, // Stable Master
    9858,  // Auctioneer Kresky
    8719,  // Auctioneer Fitch
    9856,  // Auctioneer Grimful
    6491   // Spirit Healer
};

// Stores a player's last position for teleportation
struct LastPosition : public DataMap::Base
{
    uint32 mapId = 0;
    float x = 0, y = 0, z = 0, ori = 0;
};

// Event: Despawns a marker creature after a delay
class DespawnMarkerEvent : public BasicEvent
{
    Creature *marker_;

public:
    DespawnMarkerEvent(Creature *marker) : marker_(marker) {}

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (marker_)
        {
            if (marker_->IsInWorld())
                marker_->DespawnOrUnsummon();
            delete marker_;
            marker_ = nullptr;
        }
        // Remove this event after execution
        return false;
    }
};

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

        QueryResult result = CharacterDatabase.Query("SELECT 1 FROM `guild_house` WHERE `guild`={}", player->GetGuildId());
        if (!result)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not own a Guild House yet.");
            CloseGossipMenuFor(player);
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

        // Essential NPCs submenu
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Manage Essential NPCs", GOSSIP_SENDER_MAIN, 10020);

        // Vendors, Trainers, Portals, Objects
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Class Trainers", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Profession Trainers", GOSSIP_SENDER_MAIN, 5);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Vendors", GOSSIP_SENDER_MAIN, 3);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn City Portals", GOSSIP_SENDER_MAIN, 10005);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Objects", GOSSIP_SENDER_MAIN, 10006);

        // Reset
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Reset Guild House (Remove & Re-place All)", GOSSIP_SENDER_MAIN, 900000, "Are you sure you want to reset your Guild House? This will remove and re-place all purchased objects and NPCs.", 0, false);

        // return option if we have a stored position
        auto last = player->CustomData.Get<LastPosition>("lastPos");
        if (last && last->mapId)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Return to Previous Location", GOSSIP_SENDER_MAIN, 10030);

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

        // Essential NPCs submenu
        case 10020:
        {
            ClearGossipMenuFor(player);
            uint32 guildPhase = this->GetGuildPhase(player);

            // Innkeeper
            bool innkeeperSpawned = false;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 6930 && c->GetPhaseMask() == guildPhase)
                {
                    innkeeperSpawned = true;
                    break;
                }
            }
            if (!innkeeperSpawned)
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "Spawn Innkeeper", GOSSIP_SENDER_MAIN, 6930, "Add an Innkeeper?", GuildHouseInnKeeper, false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Innkeeper (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 700032, "Remove the Innkeeper and get a refund?", GuildHouseInnKeeper * GuildHouseRefundPercent / 100, false);

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
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Stable Master (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7028690, "Remove the Stable Master and get a refund?", GuildHouseVendor * GuildHouseRefundPercent / 100, false);

            // Neutral Auctioneer
            bool neutralAuctioneerSpawned = false;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 9858 && c->GetPhaseMask() == guildPhase)
                {
                    neutralAuctioneerSpawned = true;
                    break;
                }
            }
            if (!neutralAuctioneerSpawned)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Neutral Auctioneer", GOSSIP_SENDER_MAIN, 9858, "Spawn a Neutral Auctioneer?", GuildHouseAuctioneer, false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Neutral Auctioneer (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 709858, "Remove the Neutral Auctioneer and get a refund?", GuildHouseAuctioneer * GuildHouseRefundPercent / 100, false);

            // Alliance Auctioneer
            bool allianceAuctioneerSpawned = false;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 8719 && c->GetPhaseMask() == guildPhase)
                {
                    allianceAuctioneerSpawned = true;
                    break;
                }
            }
            if (!allianceAuctioneerSpawned)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Alliance Auctioneer", GOSSIP_SENDER_MAIN, 8719, "Spawn Alliance Auctioneer?", GuildHouseAuctioneer, false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Alliance Auctioneer (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 708719, "Remove the Alliance Auctioneer and get a refund?", GuildHouseAuctioneer * GuildHouseRefundPercent / 100, false);

            // Horde Auctioneer
            bool hordeAuctioneerSpawned = false;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 9856 && c->GetPhaseMask() == guildPhase)
                {
                    hordeAuctioneerSpawned = true;
                    break;
                }
            }
            if (!hordeAuctioneerSpawned)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Spawn Horde Auctioneer", GOSSIP_SENDER_MAIN, 9856, "Spawn Horde Auctioneer?", GuildHouseAuctioneer, false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Horde Auctioneer (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 709856, "Remove the Horde Auctioneer and get a refund?", GuildHouseAuctioneer * GuildHouseRefundPercent / 100, false);

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

            // Buy All/Sell All Essential NPCs
            int notSpawned = 0;
            for (auto entry : essentialNpcEntries)
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
                    notSpawned++;
            }
            if (notSpawned > 0)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Essential NPCs", GOSSIP_SENDER_MAIN, 10021, "Buy and spawn all essential NPCs?",
                                 GuildHouseInnKeeper + GuildHouseVendor + GuildHouseAuctioneer * 3, false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Essential NPCs", GOSSIP_SENDER_MAIN, 20021, "Remove all essential NPCs and get a refund?",
                                 (GuildHouseInnKeeper + GuildHouseVendor + GuildHouseAuctioneer * 3) * GuildHouseRefundPercent / 100, false);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Buy All Essential NPCs
        case 10021:
        {
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalCost = 0;
            std::vector<uint32> toSpawn;

            for (auto entry : essentialNpcEntries)
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
                    toSpawn.push_back(entry);
                    int npcCost = 0;
                    if (entry == 6930)
                        npcCost = GuildHouseInnKeeper;
                    else if (entry == 28690)
                        npcCost = GuildHouseVendor;
                    else if (entry == 6491)
                        npcCost = GuildHouseSpirit;
                    else
                        npcCost = GuildHouseAuctioneer;
                    totalCost += npcCost;
                }
            }

            if (totalCost == 0)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All essential NPCs are already spawned.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
                return false;
            }

            if (!player->HasEnoughMoney(totalCost))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not have enough gold in the bank.");
                CloseGossipMenuFor(player);
                return false;
            }

            player->ModifyMoney(-totalCost);

            for (uint32 entry : toSpawn)
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), entry);
                SpawnNPC(entry, player, true);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All missing essential NPCs purchased and spawned for %u g.", totalCost / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
            break;
        }

        // Sell All Essential NPCs
        case 20021:
        {
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalRefund = 0;
            std::vector<Creature *> toRemove;

            for (auto entry : essentialNpcEntries)
            {
                if (entry == 6491)
                    continue; // Never remove Spirit Healer
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
                    {
                        toRemove.push_back(c);
                        int npcCost = 0;
                        if (entry == 6930)
                            npcCost = GuildHouseInnKeeper;
                        else if (entry == 28690)
                            npcCost = GuildHouseVendor;
                        else if (entry == 6491)
                            npcCost = GuildHouseSpirit;
                        else
                            npcCost = GuildHouseAuctioneer;
                        totalRefund += npcCost * GuildHouseRefundPercent / 100;
                    }
                }
            }

            if (toRemove.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No essential NPCs found to remove.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
                return false;
            }

            player->ModifyMoney(totalRefund);

            for (Creature *c : toRemove)
            {
                if (!c)
                    continue;
                c->RemoveAllAuras();
                if (c->IsInWorld())
                    c->RemoveFromWorld();
                c->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), c->GetEntry());
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All essential NPCs removed and %u g refunded.", totalRefund / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
            return true;
        }

        // Objects submenu
        case 10006:
        {
            ClearGossipMenuFor(player);
            std::set<uint32> objectEntries = {1685, 4087, 180715, 2728, 184137};
            uint32 guildPhase = this->GetGuildPhase(player);
            int notSpawned = 0;
            int totalCostAll = 0;
            int totalRefundAll = 0;

            for (auto entry : objectEntries)
            {
                std::string label;
                switch (entry)
                {
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
                case 184137:
                    label = "Mailbox";
                    break;
                default:
                    label = "Object";
                    break;
                }
                bool objectSpawned = false;
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *go = pair.second;
                    if (go && go->GetEntry() == entry && go->GetPhaseMask() == guildPhase)
                    {
                        objectSpawned = true;
                        break;
                    }
                }
                int costValue = (entry == 184137 ? GuildHouseMailBox : GuildHouseObject);
                totalCostAll += costValue;
                if (!objectSpawned)
                {
                    notSpawned++;
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Spawn " + label, GOSSIP_SENDER_MAIN,
                                     entry, "Spawn a " + label + "?", costValue, false);
                }
                else
                {
                    int refundValue = costValue * GuildHouseRefundPercent / 100;
                    totalRefundAll += refundValue;
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove " + label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN,
                                     700000 + entry, "Remove the " + label + " and get a refund?",
                                     refundValue, false);
                }
            }
            if (notSpawned > 0)
            {
                int buyAllCost = 0;
                for (auto entry : objectEntries)
                {
                    bool spawned = false;
                    for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                    {
                        if (pair.second && pair.second->GetEntry() == entry && pair.second->GetPhaseMask() == guildPhase)
                        {
                            spawned = true;
                            break;
                        }
                    }
                    if (!spawned)
                    {
                        buyAllCost += (entry == 184137 ? GuildHouseMailBox : GuildHouseObject);
                    }
                }
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Objects", GOSSIP_SENDER_MAIN,
                                 10015, "Buy and spawn all missing objects?", buyAllCost, false);
            }
            else
            {
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Objects", GOSSIP_SENDER_MAIN,
                                 20015, "Remove all objects and get a refund?", totalRefundAll, false);
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Buy All Objects
        case 10015:
        {
            std::set<uint32> objectEntries = {1685, 4087, 180715, 2728, 184137};
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalCost = 0;
            std::vector<uint32> toSpawn;

            for (auto entry : objectEntries)
            {
                bool objectSpawned = false;
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *go = pair.second;
                    if (go && go->GetEntry() == entry && go->GetPhaseMask() == guildPhase)
                    {
                        objectSpawned = true;
                        break;
                    }
                }
                if (!objectSpawned)
                {
                    toSpawn.push_back(entry);
                    totalCost += (entry == 184137 ? GuildHouseMailBox : GuildHouseObject);
                }
            }

            if (totalCost == 0)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All objects are already spawned.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10006);
                return false;
            }

            if (!player->HasEnoughMoney(totalCost))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not have enough gold in the bank.");
                CloseGossipMenuFor(player);
                return false;
            }

            player->ModifyMoney(-totalCost);

            for (uint32 entry : toSpawn)
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), entry);
                SpawnObject(entry, player, creature, true);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All missing objects purchased and spawned for %u g.", totalCost / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10006);
            break;
        }

        // Sell All Objects
        case 20015:
        {
            std::set<uint32> objectEntries = {1685, 4087, 180715, 2728, 184137};
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalRefund = 0;
            std::vector<GameObject *> toRemove;

            for (auto entry : objectEntries)
            {
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *go = pair.second;
                    if (go && go->GetEntry() == entry && go->GetPhaseMask() == guildPhase)
                    {
                        toRemove.push_back(go);
                        totalRefund += (entry == 184137 ? GuildHouseMailBox : GuildHouseObject) * GuildHouseRefundPercent / 100;
                        break;
                    }
                }
            }

            if (toRemove.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No objects found to remove.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10006);
                return false;
            }

            player->ModifyMoney(totalRefund);

            for (GameObject *go : toRemove)
            {
                uint32 entry = go->GetEntry();
                uint32 spawnId = go->GetSpawnId();
                if (go->IsInWorld())
                    go->RemoveFromWorld();
                if (sObjectMgr->GetGameObjectData(spawnId))
                    go->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'",
                                      player->GetGuildId(), entry);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All objects removed and %u g refunded.", totalRefund / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10006);
            return true;
        }

        // Reset Guild House
        case 900000:
        {
            uint32 guildPhase = this->GetGuildPhase(player);
            Map *map = player->GetMap();

            ClearGuildHousePhase(player->GetGuildId(), guildPhase, map);

            QueryResult result = WorldDatabase.Query("SELECT `entry`, `type` FROM `guild_house_purchases` WHERE `guild`={}", player->GetGuildId());
            if (result)
            {
                do
                {
                    Field *fields = result->Fetch();
                    uint32 entry = fields[0].Get<uint32>();
                    std::string type = fields[1].Get<std::string>();

                    if (type == "creature")
                        SpawnNPC(entry, player, true);
                    else if (type == "gameobject")
                        SpawnObject(entry, player, creature, true);
                } while (result->NextRow());
            }

            CharacterDatabase.Execute("DELETE FROM `guild_house` WHERE `guild`={}", player->GetGuildId());

            ChatHandler(player->GetSession()).PSendSysMessage("Guild House has been reset and all purchased items restored!");
            OnGossipHello(player, creature);
            break;
        }

        // Class Trainer submenu
        case 2:
        {
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
                    {29195, "Death Knight Trainer", GuildHouseTrainer}};
                uint32 guildPhase = this->GetGuildPhase(player);
                int notSpawned = 0;
                int totalCostAll = 0;
                int totalRefundAll = 0;

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
                    totalCostAll += t.cost;
                    if (!spawned)
                        notSpawned++;
                    if (!spawned)
                        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string("Spawn ") + t.label, GOSSIP_SENDER_MAIN, t.entry, std::string("Spawn a ") + t.label + "?", t.cost, false);
                    else
                    {
                        int refundValue = t.cost * GuildHouseRefundPercent / 100;
                        totalRefundAll += refundValue;
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, std::string("Remove ") + t.label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7000000 + t.entry, std::string("Remove the ") + t.label + " and get a refund?", refundValue, false);
                    }
                }
                if (notSpawned > 0)
                {
                    int buyAllCost = 0;
                    for (auto const &t : trainers)
                    {
                        bool spawned = false;
                        for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                        {
                            if (pair.second && pair.second->GetEntry() == t.entry && pair.second->GetPhaseMask() == guildPhase)
                            {
                                spawned = true;
                                break;
                            }
                        }
                        if (!spawned)
                        {
                            buyAllCost += t.cost;
                        }
                    }
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Class Trainers", GOSSIP_SENDER_MAIN, 10012, "Buy and spawn all missing class trainers?", buyAllCost, false);
                }
                else
                {
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Class Trainers", GOSSIP_SENDER_MAIN, 20012, "Remove all class trainers and get a refund?", totalRefundAll, false);
                }
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Buy All Class Trainers
        case 10012:
        {
            std::set<uint32> classTrainerEntries = {
                26327, // Paladin Trainer
                26324, // Druid Trainer
                26325, // Hunter Trainer
                26326, // Mage Trainer
                26328, // Priest Trainer
                26329, // Rogue Trainer
                26330, // Shaman Trainer
                26331, // Warlock Trainer
                26332, // Warrior Trainer
                29195  // Death Knight Trainer
            };

            uint32 guildPhase = this->GetGuildPhase(player);
            int totalCost = 0;
            std::vector<uint32> toSpawn;

            for (uint32 entry : classTrainerEntries)
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
                    toSpawn.push_back(entry);
                    totalCost += GuildHouseTrainer;
                }
            }

            if (totalCost == 0)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All class trainers are already spawned.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 2);
                return false;
            }

            if (!player->HasEnoughMoney(totalCost))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not have enough gold in the bank.");
                CloseGossipMenuFor(player);
                return false;
            }

            player->ModifyMoney(-totalCost);

            for (uint32 entry : toSpawn)
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), entry);
                SpawnNPC(entry, player, true);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All missing class trainers have been purchased and spawned for %u g!", totalCost / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 2);
            break;
        }

        // Sell All Class Trainers
        case 20012:
        {
            std::set<uint32> classTrainerEntries = {
                26327, // Paladin Trainer
                26324, // Druid Trainer
                26325, // Hunter Trainer
                26326, // Mage Trainer
                26328, // Priest Trainer
                26329, // Rogue Trainer
                26330, // Shaman Trainer
                26331, // Warlock Trainer
                26332, // Warrior Trainer
                29195  // Death Knight Trainer
            };
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalRefund = 0;
            std::vector<Creature *> toRemove;

            for (auto entry : classTrainerEntries)
            {
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
                    {
                        toRemove.push_back(c);
                        totalRefund += GuildHouseTrainer * GuildHouseRefundPercent / 100;
                        break;
                    }
                }
            }

            if (toRemove.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No class trainers found to remove.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 2);
                return false;
            }

            player->ModifyMoney(totalRefund);

            for (Creature *c : toRemove)
            {
                if (!c)
                    continue;
                c->RemoveAllAuras();
                if (c->IsInWorld())
                    c->RemoveFromWorld();
                c->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), c->GetEntry());
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All class trainers removed and %u g refunded.", totalRefund / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 2);
            return true;
        }

        // Profession Trainers submenu
        case 5:
        {
            ClearGossipMenuFor(player);
            {
                struct TrainerEntry
                {
                    uint32 entry;
                    const char *label;
                    int cost;
                };
                TrainerEntry profs[] = {
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
                uint32 guildPhase = this->GetGuildPhase(player);
                int notSpawned = 0;
                int totalCostAll = 0;
                int totalRefundAll = 0;

                for (auto const &t : profs)
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
                    totalCostAll += t.cost;
                    if (!spawned)
                        notSpawned++;
                    if (!spawned)
                        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string("Spawn ") + t.label, GOSSIP_SENDER_MAIN, t.entry, std::string("Spawn a ") + t.label + "?", t.cost, false);
                    else
                    {
                        int refundValue = t.cost * GuildHouseRefundPercent / 100;
                        totalRefundAll += refundValue;
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, std::string("Remove ") + t.label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7000000 + t.entry, std::string("Remove the ") + t.label + " and get a refund?", refundValue, false);
                    }
                }
                if (notSpawned > 0)
                {
                    int buyAllCost = 0;
                    for (auto const &t : profs)
                    {
                        bool spawned = false;
                        for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                        {
                            if (pair.second && pair.second->GetEntry() == t.entry && pair.second->GetPhaseMask() == guildPhase)
                            {
                                spawned = true;
                                break;
                            }
                        }
                        if (!spawned)
                        {
                            buyAllCost += t.cost;
                        }
                    }
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Profession Trainers", GOSSIP_SENDER_MAIN, 10013, "Buy and spawn all missing profession trainers?", buyAllCost, false);
                }
                else
                {
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Profession Trainers", GOSSIP_SENDER_MAIN, 20013, "Remove all profession trainers and get a refund?", totalRefundAll, false);
                }
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Buy All Profession Trainers
        case 10013:
        {
            std::set<uint32> profTrainerEntries = {
                2836,  // Blacksmithing Trainer
                8128,  // Mining Trainer
                8736,  // Engineering Trainer
                19187, // Leatherworking Trainer
                19180, // Skinning Trainer
                19052, // Alchemy Trainer
                908,   // Herbalism Trainer
                2627,  // Tailoring Trainer
                19184, // First Aid Trainer
                2834,  // Fishing Trainer
                19185  // Cooking Trainer
            };
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalCost = 0;
            std::vector<uint32> toSpawn;

            for (auto entry : profTrainerEntries)
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
                    toSpawn.push_back(entry);
                    totalCost += GuildHouseProf;
                }
            }

            if (totalCost == 0)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All profession trainers are already spawned.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 5);
                return false;
            }

            if (!player->HasEnoughMoney(totalCost))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not have enough gold in the bank.");
                CloseGossipMenuFor(player);
                return false;
            }

            player->ModifyMoney(-totalCost);

            for (uint32 entry : toSpawn)
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), entry);
                SpawnNPC(entry, player, true);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All missing profession trainers purchased and spawned for %u g.", totalCost / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 5);
            break;
        }

        // Sell All Profession Trainers
        case 20013:
        {
            std::set<uint32> profTrainerEntries = {
                2836,  // Blacksmithing Trainer
                8128,  // Mining Trainer
                8736,  // Engineering Trainer
                19187, // Leatherworking Trainer
                19180, // Skinning Trainer
                19052, // Alchemy Trainer
                908,   // Herbalism Trainer
                2627,  // Tailoring Trainer
                19184, // First Aid Trainer
                2834,  // Fishing Trainer
                19185  // Cooking Trainer
            };
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalRefund = 0;
            std::vector<Creature *> toRemove;

            for (auto entry : profTrainerEntries)
            {
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
                    {
                        toRemove.push_back(c);
                        totalRefund += GuildHouseProf * GuildHouseRefundPercent / 100;
                        break;
                    }
                }
            }

            if (toRemove.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No profession trainers found to remove.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 5);
                return false;
            }

            player->ModifyMoney(totalRefund);

            for (Creature *c : toRemove)
            {
                if (!c)
                    continue;
                c->RemoveAllAuras();
                if (c->IsInWorld())
                    c->RemoveFromWorld();
                c->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), c->GetEntry());
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All profession trainers removed and %u g refunded.", totalRefund / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 5);
            return true;
        }

        // Vendor submenu
        case 3:
        {
            ClearGossipMenuFor(player);
            uint32 guildPhase = this->GetGuildPhase(player);
            std::set<uint32> vendorEntries = {28692, 28776, 4255, 29636, 29493, 2622};
            int notSpawned = 0;
            int totalCostAll = 0;
            int totalRefundAll = 0;

            std::map<uint32, std::string> vendorLabels = {
                {28692, "Trade Supplies"}, {28776, "Tabard Vendor"}, {4255, "Food & Drink Vendor"}, {29636, "Reagent Vendor"}, {29493, "Ammo & Repair Vendor"}, {2622, "Poisons Vendor"}};

            for (auto entry : vendorEntries)
            {
                std::string label = vendorLabels.count(entry) ? vendorLabels[entry] : "Vendor";
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
                totalCostAll += GuildHouseVendor;
                if (!spawned)
                {
                    notSpawned++;
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn " + label, GOSSIP_SENDER_MAIN, entry, "Spawn a " + label + "?", GuildHouseVendor, false);
                }
                else
                {
                    int refundValue = GuildHouseVendor * GuildHouseRefundPercent / 100;
                    totalRefundAll += refundValue;
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove " + label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7000000 + entry, "Remove the " + label + " and get a refund?", refundValue, false);
                }
            }

            if (notSpawned > 0)
            {
                int buyAllCost = 0;
                for (auto entry : vendorEntries)
                {
                    bool spawned = false;
                    for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                    {
                        if (pair.second && pair.second->GetEntry() == entry && pair.second->GetPhaseMask() == guildPhase)
                        {
                            spawned = true;
                            break;
                        }
                    }
                    if (!spawned)
                    {
                        buyAllCost += GuildHouseVendor;
                    }
                }
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Vendors", GOSSIP_SENDER_MAIN, 10003, "Buy and spawn all missing vendors?", buyAllCost, false);
            }
            else
            {
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Vendors", GOSSIP_SENDER_MAIN, 20003, "Remove all vendors and get a refund?", totalRefundAll, false);
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Buy All Vendors
        case 10003:
        {
            std::set<uint32> vendorEntries = {28692, 28776, 4255, 29636, 29493, 2622};
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalCost = 0;
            std::vector<uint32> toSpawn;

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
                    toSpawn.push_back(entry);
                    totalCost += GuildHouseVendor;
                }
            }

            if (totalCost == 0)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All vendors are already spawned.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 3);
                return false;
            }

            if (!player->HasEnoughMoney(totalCost))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not have enough gold in the bank.");
                CloseGossipMenuFor(player);
                return false;
            }

            player->ModifyMoney(-totalCost);

            for (uint32 entry : toSpawn)
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), entry);
                SpawnNPC(entry, player, true);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All missing vendors purchased and spawned for %u g.", totalCost / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 3);
            break;
        }

        // Sell All Vendors
        case 20003:
        {
            std::set<uint32> vendorEntries = {28692, 28776, 4255, 29636, 29493, 2622};
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalRefund = 0;
            std::vector<Creature *> toRemove;

            for (auto entry : vendorEntries)
            {
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
                    {
                        toRemove.push_back(c);
                        totalRefund += GuildHouseVendor * GuildHouseRefundPercent / 100;
                        break;
                    }
                }
            }

            if (toRemove.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No vendors found to remove.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 3);
                return false;
            }

            player->ModifyMoney(totalRefund);

            for (Creature *c : toRemove)
            {
                if (!c)
                    continue;
                c->RemoveAllAuras();
                if (c->IsInWorld())
                    c->RemoveFromWorld();
                c->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), c->GetEntry());
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All vendors removed and %u g refunded.", totalRefund / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 3);
            return true;
        }

        // City Portals submenu
        case 10005:
        {
            ClearGossipMenuFor(player);
            std::vector<uint32> portalEntries = {
                500000, // Stormwind
                500001, // Darnassus
                500002, // Exodar
                500003, // Ironforge
                500004, // Orgrimmar
                500005, // Silvermoon
                500006, // Thunder Bluff
                500007, // Undercity
                500008, // Shattrath
                500009  // Dalaran
            };
            std::set<uint32> starterPortals = {
                500000, // Stormwind
                500004  // Orgrimmar
            };
            uint32 guildPhase = this->GetGuildPhase(player);
            int notSpawned = 0;
            int totalCostAll = 0;
            int totalRefundAll = 0;

            for (auto entry : portalEntries)
            {
                std::string label;
                switch (entry)
                {
                case 500000:
                    label = "Stormwind Portal";
                    break;
                case 500001:
                    label = "Darnassus Portal";
                    break;
                case 500002:
                    label = "Exodar Portal";
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
                case 500006:
                    label = "Thunder Bluff Portal";
                    break;
                case 500007:
                    label = "Undercity Portal";
                    break;
                case 500008:
                    label = "Shattrath Portal";
                    break;
                case 500009:
                    label = "Dalaran Portal";
                    break;
                default:
                    label = "Portal";
                    break;
                }
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *go = pair.second;
                    if (go && go->GetEntry() == entry && go->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                totalCostAll += GuildHousePortal;
                if (!spawned)
                {
                    notSpawned++;
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Spawn " + label, GOSSIP_SENDER_MAIN, entry, "Spawn a " + label + "?", GuildHousePortal, false);
                }
                else
                {
                    if (starterPortals.find(entry) == starterPortals.end())
                    {
                        int refundValue = GuildHousePortal * GuildHouseRefundPercent / 100;
                        totalRefundAll += refundValue;
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove " + label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)",
                                         GOSSIP_SENDER_MAIN, 700000 + entry, "Remove the " + label + " and get a refund?",
                                         refundValue, false);
                    }
                }
            }
            if (notSpawned > 0)
            {
                int buyAllCost = 0;
                for (auto entry : portalEntries)
                {
                    bool spawned = false;
                    for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                    {
                        if (pair.second && pair.second->GetEntry() == entry && pair.second->GetPhaseMask() == guildPhase)
                        {
                            spawned = true;
                            break;
                        }
                    }
                    if (!spawned)
                    {
                        buyAllCost += GuildHousePortal;
                    }
                }
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Portals", GOSSIP_SENDER_MAIN, 10014, "Buy and spawn all missing portals?", buyAllCost, false);
            }
            else
            {
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Portals", GOSSIP_SENDER_MAIN, 20014, "Remove all non-starter portals and get a refund?", totalRefundAll, false);
            }

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Buy All Portals
        case 10014:
        {
            std::vector<uint32> portalEntries = {
                500000, // Stormwind
                500001, // Darnassus
                500002, // Exodar
                500003, // Ironforge
                500004, // Orgrimmar
                500005, // Silvermoon
                500006, // Thunder Bluff
                500007, // Undercity
                500008, // Shattrath
                500009  // Dalaran
            };
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalCost = 0;
            std::vector<uint32> toSpawn;

            for (auto entry : portalEntries)
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *go = pair.second;
                    if (go && go->GetEntry() == entry && go->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                {
                    toSpawn.push_back(entry);
                    totalCost += GuildHousePortal;
                }
            }

            if (totalCost == 0)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("All portals are already spawned.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10005);
                return false;
            }

            if (!player->HasEnoughMoney(totalCost))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not have enough gold in the bank.");
                CloseGossipMenuFor(player);
                return false;
            }

            player->ModifyMoney(-totalCost);

            for (uint32 entry : toSpawn)
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'gameobject')", player->GetGuildId(), entry);
                SpawnObject(entry, player, creature, true);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All missing portals purchased and spawned for %u g.", totalCost / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10005);
            break;
        }

        // Sell All Portals
        case 20014:
        {
            std::vector<uint32> portalEntries = {
                500000, // Stormwind
                500001, // Darnassus
                500002, // Exodar
                500003, // Ironforge
                500004, // Orgrimmar
                500005, // Silvermoon
                500006, // Thunder Bluff
                500007, // Undercity
                500008, // Shattrath
                500009  // Dalaran
            };
            std::set<uint32> starterPortals = {500000, 500004};
            uint32 guildPhase = this->GetGuildPhase(player);
            int totalRefund = 0;
            std::vector<GameObject *> toRemove;

            for (auto entry : portalEntries)
            {
                if (starterPortals.find(entry) != starterPortals.end())
                    continue;

                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *go = pair.second;
                    if (go && go->GetEntry() == entry && go->GetPhaseMask() == guildPhase)
                    {
                        toRemove.push_back(go);
                        totalRefund += GuildHousePortal * GuildHouseRefundPercent / 100;
                        break;
                    }
                }
            }

            if (toRemove.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No non-starter portals found to remove.");
                OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10005);
                return false;
            }

            player->ModifyMoney(totalRefund);

            for (GameObject *go : toRemove)
            {
                uint32 entry = go->GetEntry();
                uint32 spawnId = go->GetSpawnId();
                if (go->IsInWorld())
                    go->RemoveFromWorld();
                if (sObjectMgr->GetGameObjectData(spawnId))
                    go->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), entry);
            }

            ChatHandler(player->GetSession()).PSendSysMessage("All non-starter portals removed and %u g refunded.", totalRefund / 10000);
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10005);
            return true;
        }

        // Neutral Auctioneer
        case 709858:
        {
            uint32 guildPhase = this->GetGuildPhase(player);
            Creature *found = nullptr;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 9858 && c->GetPhaseMask() == guildPhase)
                {
                    found = c;
                    break;
                }
            }
            if (found)
            {
                found->RemoveAllAuras();
                if (found->IsInWorld())
                    found->RemoveFromWorld();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 9858);
                player->ModifyMoney(GuildHouseAuctioneer * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Neutral Auctioneer removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Neutral Auctioneer found to remove.");
            }
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
            break;
        }

        // Alliance Auctioneer
        case 708719:
        {
            uint32 guildPhase = this->GetGuildPhase(player);
            Creature *found = nullptr;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 8719 && c->GetPhaseMask() == guildPhase)
                {
                    found = c;
                    break;
                }
            }
            if (found)
            {
                found->RemoveAllAuras();
                if (found->IsInWorld())
                    found->RemoveFromWorld();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 8719);
                player->ModifyMoney(GuildHouseAuctioneer * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Alliance Auctioneer removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Alliance Auctioneer found to remove.");
            }
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
            break;
        }

        // Horde Auctioneer
        case 709856:
        {
            uint32 guildPhase = this->GetGuildPhase(player);
            Creature *found = nullptr;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 9856 && c->GetPhaseMask() == guildPhase)
                {
                    found = c;
                    break;
                }
            }
            if (found)
            {
                found->RemoveAllAuras();
                if (found->IsInWorld())
                    found->RemoveFromWorld();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 9856);
                player->ModifyMoney(GuildHouseAuctioneer * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Horde Auctioneer removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Horde Auctioneer found to remove.");
            }
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
            break;
        }

        // Tabard Vendor
        case 7028776:
        {
            uint32 guildPhase = this->GetGuildPhase(player);
            Creature *found = nullptr;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 28776 && c->GetPhaseMask() == guildPhase)
                {
                    found = c;
                    break;
                }
            }
            if (found)
            {
                found->RemoveAllAuras();
                if (found->IsInWorld())
                    found->RemoveFromWorld();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 28776);
                player->ModifyMoney(GuildHouseVendor * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Tabard Vendor removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Tabard Vendor found to remove.");
            }
            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 3);
            break;
        }

        // Return to Previous Location
        case 10030:
        {
            auto last = player->CustomData.Get<LastPosition>("lastPos");
            if (last && last->mapId)
            {
                player->TeleportTo(last->mapId, last->x, last->y, last->z, last->ori);
                player->CustomData.Erase("lastPos");
            }
            CloseGossipMenuFor(player);
            break;
        }

        default:
        {
            std::set<uint32> essentialNpcSingleEntries = {
                6930,  // Innkeeper
                28690, // Stable Master
                9858,  // Neutral Auctioneer
                8719,  // Alliance Auctioneer
                9856,  // Horde Auctioneer
                6491   // Spirit Healer
            };
            std::set<uint32> trainerEntries = {
                26327, 26324, 26325, 26326, 26328, 26329, 26330, 26331, 26332, 29195,
                2836, 8128, 8736, 19187, 19180, 19052, 908, 2627, 19184, 2834, 19185};
            std::set<uint32> vendorEntries = {
                28692, 28776, 4255, 29636, 29493, 2622};
            std::set<uint32> professionTrainerEntries = {
                2836, 8128, 8736, 19187, 19180, 19052, 908, 2627, 19184, 2834, 19185};
            std::vector<uint32> portalEntries = {
                500000, 500001, 500002, 500003, 500004, 500005, 500006, 500007, 500008, 500009};
            std::set<uint32> starterPortals = {500000, 500004};
            std::set<uint32> objectEntries = {
                1685, 4087, 180715, 2728, 184137 // Mailbox included
            };
            int cost = 0;
            uint32 guildPhase = this->GetGuildPhase(player);

            if (action >= 7000000) // Removal action
            {
                uint32 entryToRemove = action - 7000000;
                int refund = 0;
                bool found = false;

                if (entryToRemove == 6491)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("You cannot remove the Spirit Healer.");
                    return true;
                }

                std::vector<Creature *> creaturesToRemove;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == entryToRemove && c->GetPhaseMask() == guildPhase)
                    {
                        creaturesToRemove.push_back(c);
                        found = true;
                        if (essentialNpcEntries.count(entryToRemove))
                        {
                            if (entryToRemove == 6930)
                                refund = GuildHouseInnKeeper * GuildHouseRefundPercent / 100;
                            else if (entryToRemove == 28690)
                                refund = GuildHouseVendor * GuildHouseRefundPercent / 100;
                            else if (entryToRemove == 6491)
                                refund = GuildHouseSpirit * GuildHouseRefundPercent / 100;
                            else
                                refund = GuildHouseAuctioneer * GuildHouseRefundPercent / 100;
                        }
                        else if (trainerEntries.count(entryToRemove))
                        {
                            refund = (professionTrainerEntries.count(entryToRemove) ? GuildHouseProf : GuildHouseTrainer) * GuildHouseRefundPercent / 100;
                        }
                        else if (vendorEntries.count(entryToRemove))
                        {
                            refund = GuildHouseVendor * GuildHouseRefundPercent / 100;
                        }
                        break;
                    }
                }

                std::vector<GameObject *> objectsToRemove;
                if (!found)
                {
                    for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                    {
                        GameObject *go = pair.second;
                        if (go && go->GetEntry() == entryToRemove && go->GetPhaseMask() == guildPhase)
                        {
                            objectsToRemove.push_back(go);
                            found = true;
                            if (std::find(portalEntries.begin(), portalEntries.end(), entryToRemove) != portalEntries.end())
                            {
                                refund = GuildHousePortal * GuildHouseRefundPercent / 100;
                            }
                            else if (objectEntries.count(entryToRemove))
                            {
                                refund = (entryToRemove == 184137 ? GuildHouseMailBox : GuildHouseObject) * GuildHouseRefundPercent / 100;
                            }
                            break;
                        }
                    }
                }

                if (found)
                {
                    player->ModifyMoney(refund);

                    for (Creature *c : creaturesToRemove)
                    {
                        if (!c)
                            continue;
                        c->RemoveAllAuras();
                        if (c->IsInWorld())
                            c->RemoveFromWorld();
                        c->DeleteFromDB();
                        WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), entryToRemove);
                    }
                    for (GameObject *go : objectsToRemove)
                    {
                        uint32 spawnId = go->GetSpawnId();
                        if (go->IsInWorld())
                            go->RemoveFromWorld();
                        if (sObjectMgr->GetGameObjectData(spawnId))
                            go->DeleteFromDB();
                        WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='gameobject'", player->GetGuildId(), entryToRemove);
                    }
                    ChatHandler(player->GetSession()).PSendSysMessage("Item removed and %u g refunded.", refund / 10000);
                }
                else
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("Item not found or already removed.");
                }

                // Refresh appropriate menu based on removed item type
                if (essentialNpcEntries.count(entryToRemove))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
                else if (trainerEntries.count(entryToRemove))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, professionTrainerEntries.count(entryToRemove) ? 5 : 2);
                else if (vendorEntries.count(entryToRemove))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 3);
                if (std::find(portalEntries.begin(), portalEntries.end(), entryToRemove) != portalEntries.end())
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10005);
                else if (objectEntries.count(entryToRemove))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10006);
                else
                    OnGossipHello(player, creature); // Fallback refresh

                return true; // Ensure the menu is refreshed and no further code runs
            }
            else // Spawn action
            {
                bool alreadySpawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    if (pair.second && pair.second->GetEntry() == action && pair.second->GetPhaseMask() == guildPhase)
                    {
                        alreadySpawned = true;
                        break;
                    }
                }
                if (!alreadySpawned)
                {
                    for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                    {
                        if (pair.second && pair.second->GetEntry() == action && pair.second->GetPhaseMask() == guildPhase)
                        {
                            alreadySpawned = true;
                            break;
                        }
                    }
                }

                if (alreadySpawned)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("This item is already spawned.");
                    OnGossipHello(player, creature);
                    return false;
                }

                std::string type = "";
                if (essentialNpcEntries.count(action))
                {
                    type = "creature";
                    if (action == 6930)
                        cost = GuildHouseInnKeeper;
                    else if (action == 28690)
                        cost = GuildHouseVendor;
                    else if (action == 6491)
                        cost = GuildHouseSpirit;
                    else
                        cost = GuildHouseAuctioneer;
                }
                else if (trainerEntries.count(action))
                {
                    type = "creature";
                    cost = (professionTrainerEntries.count(action)) ? GuildHouseProf : GuildHouseTrainer;
                }
                else if (vendorEntries.count(action))
                {
                    type = "creature";
                    cost = GuildHouseVendor;
                }
                else if (std::find(portalEntries.begin(), portalEntries.end(), action) != portalEntries.end())
                {
                    type = "gameobject";
                    cost = GuildHousePortal;
                }
                else if (objectEntries.count(action))
                {
                    type = "gameobject";
                    cost = (action == 184137 ? GuildHouseMailBox : GuildHouseObject);
                }
                else
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("Unknown purchase action.");
                    CloseGossipMenuFor(player);
                    return false;
                }

                if (!player->HasEnoughMoney(cost))
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("Your guild does not have enough gold in the bank.");
                    CloseGossipMenuFor(player);
                    return false;
                }

                player->ModifyMoney(-cost);

                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, '{}')", player->GetGuildId(), action, type);

                if (type == "creature")
                {
                    SpawnNPC(action, player, true);
                    ChatHandler(player->GetSession()).PSendSysMessage("NPC purchased and spawned for %u g.", cost / 10000);
                }
                else if (type == "gameobject")
                {
                    SpawnObject(action, player, creature, true);
                    ChatHandler(player->GetSession()).PSendSysMessage("Object purchased and spawned for %u g.", cost / 10000);
                }
                // Refresh appropriate menu based on spawned item type
                if (essentialNpcEntries.count(action))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10020);
                else if (trainerEntries.count(action))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, professionTrainerEntries.count(action) ? 5 : 2);
                else if (vendorEntries.count(action))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 3);
                else if (std::find(portalEntries.begin(), portalEntries.end(), action) != portalEntries.end())
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10005);
                else if (objectEntries.count(action))
                    OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 10006);
                else
                    OnGossipHello(player, creature); // Fallback refresh
            }
            break;
        }
        }
        return true;
    }

    uint32 GetGuildPhase(Player *player)
    {
        return GuildHousePhaseBase + player->GetGuildId();
    };

    void SpawnObject(uint32 entry, Player *player, Creature *creature, bool force = false)
    {
        Map *map = player ? player->GetMap() : nullptr;
        if (!map)
        {
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Error: Invalid player or map context.");
            return;
        }

        uint32 guildPhase = this->GetGuildPhase(player);

        if (!force)
        {
            for (auto const &pair : map->GetGameObjectBySpawnIdStore())
            {
                GameObject *go = pair.second;
                if (go && go->GetEntry() == entry && go->GetPhaseMask() == guildPhase)
                {
                    if (player && player->GetSession())
                        ChatHandler(player->GetSession()).PSendSysMessage("This object already exists!");
                    return;
                }
            }
        }

        float posX, posY, posZ, ori;
        QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);
        if (!result)
        {
            LOG_ERROR("mod_guildhouse", "SpawnObject: failed to query spawn location for entry %u", entry);
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("No spawn location found for object entry %u.", entry);
            return;
        }
        Field *fields = result->Fetch();
        posX = fields[0].Get<float>();
        posY = fields[1].Get<float>();
        posZ = fields[2].Get<float>();
        ori = fields[3].Get<float>();

        GameObject *newObject = new GameObject();
        uint32 lowguid = map->GenerateLowGuid<HighGuid::GameObject>();

        if (!newObject->Create(lowguid, entry, map, guildPhase, posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete newObject;
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Failed to create object.");
            return;
        }

        newObject->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), guildPhase);
        uint32 db_guid = newObject->GetSpawnId(); // Use GetSpawnId()
        delete newObject;

        newObject = new GameObject();
        if (!newObject->LoadGameObjectFromDB(db_guid, map, true))
        {
            delete newObject;
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Failed to load object from DB.");
            return;
        }

        return;
    }
};

// Add this function if not present, or update your existing starter spawn logic:
void SpawnStarterNPCs(Player *player)
{
    uint32 guildPhase = GuildHousePhaseBase + player->GetGuildId();
    Map *map = player->GetMap();

    // Spawn Spirit Healer at its default location if not already present
    bool spiritHealerSpawned = false;
    for (auto const &pair : map->GetCreatureBySpawnIdStore())
    {
        Creature *c = pair.second;
        if (c && c->GetEntry() == 6491 && c->GetPhaseMask() == guildPhase)
        {
            spiritHealerSpawned = true;
            break;
        }
    }
    if (!spiritHealerSpawned)
    {
        WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), 6491);
        SpawnNPC(6491, player, true);
    }
}

void SpawnStarterPortal(Player *player)
{
    if (!player)
        return;
    std::vector<uint32> portalEntries = {500000, 500004}; // Stormwind, Orgrimmar
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
        float posX = fields[0].Get<float>();
        float posY = fields[1].Get<float>();
        float posZ = fields[2].Get<float>();
        float ori = fields[3].Get<float>();

        const GameObjectTemplate *objectInfo = sObjectMgr->GetGameObjectTemplate(entry);
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

        ObjectGuid::LowType guidLow = map->GenerateLowGuid<HighGuid::GameObject>();
        GameObject *object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();

        if (!object->Create(guidLow, objectInfo->entry, map, GetGuildPhase(player), posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete object;
            LOG_INFO("modules", "GUILDHOUSE: Unable to create object!!");
            continue;
        }

        object->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), GetGuildPhase(player));
        guidLow = object->GetSpawnId();
        delete object;

        object = sObjectMgr->IsGameObjectStaticTransport(objectInfo->entry) ? new StaticTransport() : new GameObject();
        if (!object->LoadGameObjectFromDB(guidLow, map, true))
        {
            delete object;
            continue;
        }
        sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGameObjectData(guidLow));
    }
    CloseGossipMenuFor(player);
}

void ClearGuildHousePhase(uint32 guildId, uint32 guildPhase, Map *map)
{
    // Remove all creatures in the guild's phase except Spirit Healer
    for (auto const &pair : map->GetCreatureBySpawnIdStore())
    {
        Creature *c = pair.second;
        if (c && c->GetPhaseMask() == guildPhase && c->GetEntry() != 6491)
        {
            c->RemoveAllAuras();
            if (c->IsInWorld())
                c->DespawnOrUnsummon();
            c->DeleteFromDB();
        }
    }

    // Remove all gameobjects in the guild's phase
    for (auto const &pair : map->GetGameObjectBySpawnIdStore())
    {
        GameObject *go = pair.second;
        if (go && go->GetPhaseMask() == guildPhase)
        {
            if (go->IsInWorld())
                go->RemoveFromWorld();
            if (sObjectMgr->GetGameObjectData(go->GetSpawnId()))
                go->DeleteFromDB();
        }
    }

    // Remove all purchase records for this guild
    WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={}", guildId);

    // Remove guild house record for this guild
    // CharacterDatabase.Execute("DELETE FROM `guild_house` WHERE `guild`={}", guildId);
}

void SpawnNPC(uint32 entry, Player *player, bool force)
{
    Map *map = player ? player->GetMap() : nullptr;
    if (!map)
    {
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).PSendSysMessage("Error: Invalid player or map context.");
        return;
    }

    uint32 guildPhase = GuildHousePhaseBase + player->GetGuildId();

    // If not force, skip if already exists
    if (!force)
    {
        for (auto const &pair : map->GetCreatureBySpawnIdStore())
        {
            Creature *c = pair.second;
            if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
            {
                if (player && player->GetSession())
                    ChatHandler(player->GetSession()).PSendSysMessage("This NPC already exists!");
                CloseGossipMenuFor(player);
                return;
            }
        }
    }
    else
    {
        // If force, remove any existing NPC of this entry in this phase
        std::vector<Creature *> toRemove;
        for (auto const &pair : map->GetCreatureBySpawnIdStore())
        {
            Creature *c = pair.second;
            if (c && c->GetEntry() == entry && c->GetPhaseMask() == guildPhase)
                toRemove.push_back(c);
        }
        for (Creature *c : toRemove)
        {
            c->RemoveAllAuras();
            if (c->IsInWorld())
                c->DespawnOrUnsummon();
            c->DeleteFromDB();
        }
    }

    float posX, posY, posZ, ori;
    QueryResult result = WorldDatabase.Query("SELECT `posX`, `posY`, `posZ`, `orientation` FROM `guild_house_spawns` WHERE `entry`={}", entry);
    if (!result)
    {
        LOG_ERROR("mod_guildhouse", "SpawnNPC: failed to query spawn location for entry %u", entry);
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).PSendSysMessage("No spawn location found for NPC entry %u.", entry);
        CloseGossipMenuFor(player);
        return;
    }
    Field *fields = result->Fetch();
    posX = fields[0].Get<float>();
    posY = fields[1].Get<float>();
    posZ = fields[2].Get<float>();
    ori = fields[3].Get<float>();

    Creature *newCreature = new Creature();
    uint32 lowguid = map->GenerateLowGuid<HighGuid::Unit>();

    if (!newCreature->Create(lowguid, map, guildPhase, entry, 0, posX, posY, posZ, ori))
    {
        delete newCreature;
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).PSendSysMessage("Failed to create creature object.");
        return;
    }

    newCreature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), guildPhase);
    uint32 db_guid = newCreature->GetSpawnId();
    delete newCreature;

    newCreature = new Creature();
    if (!newCreature->LoadFromDB(db_guid, map, true))
    {
        delete newCreature;
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).PSendSysMessage("Failed to load creature from DB.");
        return;
    }

    if (!map->AddToMap(newCreature))
    {
        delete newCreature;
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).PSendSysMessage("Failed to add creature to map.");
        return;
    }
}

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
        GuildHouseRefundPercent = sConfigMgr->GetOption<int32>("GuildHouseRefundPercent", 50);

        GuildHouseMarkerEntry = sConfigMgr->GetOption<uint32>("GuildHouseMarkerEntry", 999999);
        GuildHousePhaseBase = sConfigMgr->GetOption<uint32>("GuildHousePhaseBase", 10000);

        WorldDatabase.Execute("UPDATE guild_house_spawns SET posX=16236.0, posY=16264.1 WHERE entry=500004;");
    }

    void OnStartup() override
    {
        LOG_INFO("modules", "Guild House Module is enabled. Guild houses are available on this server!");
    }
};

void AddGuildHouseButlerScripts()
{
    new GuildHouseSpawner();
    new GuildHouseButlerConf();
}
