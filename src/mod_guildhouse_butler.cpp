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
int GuildHouseRefundPercent = 50;  // default
uint32 GuildHouseMarkerEntry;      // read from config
extern uint32 GuildHousePhaseBase; // declared here, defined in mod_guildhouse.cpp

// Essential NPCs (for submenu and "Buy All"/"Sell All" if needed)
std::set<uint32> essentialNpcEntries = {
    6930,  // Innkeeper
    28690, // Stable Master
    9858,  // Auctioneer Kresky
    8719,  // Auctioneer Fitch
    9856,  // Auctioneer Grimful
    6491   // Spirit Healer
};

// per-player last-location storage
struct LastPosition : public DataMap::Base
{
    uint32 mapId = 0;
    float x = 0, y = 0, z = 0, ori = 0;
};

class RemoveGlowAuraEvent : public BasicEvent
{
    Creature *creature_;
    uint32 spellId_;

public:
    RemoveGlowAuraEvent(Creature *creature, uint32 spellId) : creature_(creature), spellId_(spellId) {}
    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (creature_ && creature_->IsInWorld())
            creature_->RemoveAurasDueToSpell(spellId_);
        return true;
    }
};

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
        // return false to remove this event after execution
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
            uint32 guildPhase = GetGuildPhase(player);

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
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Spirit Healer (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 706491, "Remove the Spirit Healer and get a refund?", GuildHouseSpirit * GuildHouseRefundPercent / 100, false);

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
                                 GuildHouseInnKeeper + GuildHouseVendor + GuildHouseAuctioneer * 3 + GuildHouseSpirit, false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Essential NPCs", GOSSIP_SENDER_MAIN, 20021, "Remove all essential NPCs and get a refund?",
                                 (GuildHouseInnKeeper + GuildHouseVendor + GuildHouseAuctioneer * 3 + GuildHouseSpirit) * GuildHouseRefundPercent / 100, false);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Objects submenu
        case 10006:
        {
            ClearGossipMenuFor(player);
            std::set<uint32> objectEntries = {1685, 4087, 180715, 2728, 184137};
            uint32 guildPhase = GetGuildPhase(player);
            int notSpawned = 0;
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
                if (!objectSpawned)
                {
                    notSpawned++;
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Spawn " + label, GOSSIP_SENDER_MAIN,
                                     entry, "Spawn a " + label + "?", costValue, false);
                }
                else
                {
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove " + label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN,
                                     700000 + entry, "Remove the " + label + " and get a refund?",
                                     costValue * GuildHouseRefundPercent / 100, false);
                }
            }
            if (notSpawned > 0)
            {
                int totalCost = 0;
                for (auto entry : objectEntries)
                    totalCost += (entry == 184137 ? GuildHouseMailBox : GuildHouseObject);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Objects", GOSSIP_SENDER_MAIN,
                                 10015, "Buy and spawn all objects?", totalCost, false);
            }
            else
            {
                int totalRefund = 0;
                for (auto entry : objectEntries)
                    totalRefund += (entry == 184137 ? GuildHouseMailBox : GuildHouseObject) * GuildHouseRefundPercent / 100;
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Objects", GOSSIP_SENDER_MAIN,
                                 20015, "Remove all objects and get a refund?", totalRefund, false);
            }
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Reset Guild House
        case 900000:
        {
            uint32 guildPhase = GetGuildPhase(player);
            Map *map = player->GetMap();

            // wrap removal in a transaction
            auto trans = WorldDatabase.BeginTransaction();

            for (auto const &creaturePair : map->GetCreatureBySpawnIdStore())
            {
                Creature *c = creaturePair.second;
                if (c && c->GetPhaseMask() == guildPhase)
                {
                    if (c->GetEntry() != 500031)
                    {
                        if (c->IsInWorld())
                            c->DespawnOrUnsummon();
                        c->DeleteFromDB();
                    }
                }
            }
            for (auto const &goPair : map->GetGameObjectBySpawnIdStore())
            {
                GameObject *go = goPair.second;
                if (go && go->GetPhaseMask() == guildPhase)
                {
                    if (go->IsInWorld())
                        go->RemoveFromWorld();
                    if (sObjectMgr->GetGameObjectData(go->GetSpawnId()))
                        go->DeleteFromDB();
                }
            }
            WorldDatabase.CommitTransaction(trans);

            // respawn purchases
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
                uint32 guildPhase = GetGuildPhase(player);
                int notSpawned = 0;
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
                        notSpawned++;
                    if (!spawned)
                        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string("Spawn ") + t.label, GOSSIP_SENDER_MAIN, t.entry, std::string("Spawn a ") + t.label + "?", t.cost, false);
                    else
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, std::string("Remove ") + t.label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7000000 + t.entry, std::string("Remove the ") + t.label + " and get a refund?", t.cost * GuildHouseRefundPercent / 100, false);
                }
                if (notSpawned > 0)
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Class Trainers", GOSSIP_SENDER_MAIN, 10012, "Buy and spawn all class trainers?", GuildHouseTrainer * 10, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Class Trainers", GOSSIP_SENDER_MAIN, 20012, "Remove all class trainers and get a refund?", GuildHouseTrainer * 10 * GuildHouseRefundPercent / 100, false);
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
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
                uint32 guildPhase = GetGuildPhase(player);
                int notSpawned = 0;
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
                    if (!spawned)
                        notSpawned++;
                    if (!spawned)
                        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string("Spawn ") + t.label, GOSSIP_SENDER_MAIN, t.entry, std::string("Spawn a ") + t.label + "?", t.cost, false);
                    else
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, std::string("Remove ") + t.label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7000000 + t.entry, std::string("Remove the ") + t.label + " and get a refund?", t.cost * GuildHouseRefundPercent / 100, false);
                }
                if (notSpawned > 0)
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Profession Trainers", GOSSIP_SENDER_MAIN, 10013, "Buy and spawn all profession trainers?", GuildHouseProf * 11, false); // Adjust cost as needed
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Profession Trainers", GOSSIP_SENDER_MAIN, 20013, "Remove all profession trainers and get a refund?", GuildHouseProf * 11 * GuildHouseRefundPercent / 100, false);
            }
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Vendor submenu
        case 3:
        {
            ClearGossipMenuFor(player);
            uint32 guildPhase = GetGuildPhase(player);
            // Trade Supplies
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 28692 && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Trade Supplies", GOSSIP_SENDER_MAIN, 28692, "Spawn a Trade Supplies Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Trade Supplies (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7028692, "Remove the Trade Supplies Vendor and get a refund?", GuildHouseVendor * GuildHouseRefundPercent / 100, false);
            }
            // Tabard Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 28776 && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Tabard Vendor", GOSSIP_SENDER_MAIN, 28776, "Spawn a Tabard Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Tabard Vendor (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7028776, "Remove the Tabard Vendor and get a refund?", GuildHouseVendor * GuildHouseRefundPercent / 100, false);
            }
            // Food & Drink Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 4255 && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Food & Drink Vendor", GOSSIP_SENDER_MAIN, 4255, "Spawn a Food & Drink Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Food & Drink Vendor (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 704255, "Remove the Food & Drink Vendor and get a refund?", GuildHouseVendor * GuildHouseRefundPercent / 100, false);
            }
            // Reagent Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 29636 && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Reagent Vendor", GOSSIP_SENDER_MAIN, 29636, "Spawn a Reagent Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Reagent Vendor (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7029636, "Remove the Reagent Vendor and get a refund?", GuildHouseVendor * GuildHouseRefundPercent / 100, false);
            }
            // Ammo & Repair Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 29493 && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Ammo & Repair Vendor", GOSSIP_SENDER_MAIN, 29493, "Spawn an Ammo & Repair Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Ammo & Repair Vendor (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 7029493, "Remove the Ammo & Repair Vendor and get a refund?", GuildHouseVendor * GuildHouseRefundPercent / 100, false);
            }
            // Poisons Vendor
            {
                bool spawned = false;
                for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
                {
                    Creature *c = pair.second;
                    if (c && c->GetEntry() == 2622 && c->GetPhaseMask() == guildPhase)
                    {
                        spawned = true;
                        break;
                    }
                }
                if (!spawned)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Spawn Poisons Vendor", GOSSIP_SENDER_MAIN, 2622, "Spawn a Poisons Vendor?", GuildHouseVendor, false);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove Poisons Vendor (Refund " + std::to_string(GuildHouseRefundPercent) + "%)", GOSSIP_SENDER_MAIN, 702622, "Remove the Poisons Vendor and get a refund?", GuildHouseVendor * GuildHouseRefundPercent / 100, false);
            }
            int notSpawned = 0;
            for (auto entry : {28692, 28776, 4255, 29636, 29493, 2622})
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
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Vendors", GOSSIP_SENDER_MAIN, 10003, "Buy and spawn all vendors?", GuildHouseVendor * 6, false); // Adjust cost as needed
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Vendors", GOSSIP_SENDER_MAIN, 20003, "Remove all vendors and get a refund?", GuildHouseVendor * 6 * GuildHouseRefundPercent / 100, false);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // City Portals submenu
        case 10005:
        {
            ClearGossipMenuFor(player);
            std::set<uint32> portalEntries = {
                500000, // Stormwind Portal
                500001, // Darnassus Portal
                500003, // Ironforge Portal
                500004, // Orgrimmar Portal
                500005, // Silvermoon Portal
                500008, // Shattrath Portal
                500009  // Dalaran Portal
            };
            uint32 guildPhase = GetGuildPhase(player);
            int notSpawned = 0;
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
                case 500003:
                    label = "Ironforge Portal";
                    break;
                case 500004:
                    label = "Orgrimmar Portal";
                    break;
                case 500005:
                    label = "Silvermoon Portal";
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
                if (!spawned)
                {
                    notSpawned++;
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Spawn " + label, GOSSIP_SENDER_MAIN, entry, "Spawn a " + label + "?", GuildHousePortal, false);
                }
                else
                {
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Remove " + label + " (Refund " + std::to_string(GuildHouseRefundPercent) + "%)",
                                     GOSSIP_SENDER_MAIN, 700000 + entry, "Remove the " + label + " and get a refund?",
                                     GuildHousePortal * GuildHouseRefundPercent / 100, false);
                }
            }
            if (notSpawned > 0)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy All Portals", GOSSIP_SENDER_MAIN, 10014, "Buy and spawn all portals?", GuildHousePortal * portalEntries.size(), false);
            else
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell All Portals", GOSSIP_SENDER_MAIN, 20014, "Remove all portals and get a refund?", GuildHousePortal * portalEntries.size() * GuildHouseRefundPercent / 100, false);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }

        // Neutral Auctioneer
        case 709858:
        {
            uint32 guildPhase = GetGuildPhase(player);
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
                if (found->IsInWorld())
                    found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 9858);
                player->ModifyMoney(GuildHouseAuctioneer * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Neutral Auctioneer removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Neutral Auctioneer found to remove.");
            }
            OnGossipHello(player, creature);
            break;
        }

        // Alliance Auctioneer
        case 708719:
        {
            uint32 guildPhase = GetGuildPhase(player);
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
                if (found->IsInWorld())
                    found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 8719);
                player->ModifyMoney(GuildHouseAuctioneer * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Alliance Auctioneer removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Alliance Auctioneer found to remove.");
            }
            OnGossipHello(player, creature);
            break;
        }

        // Horde Auctioneer
        case 709856:
        {
            uint32 guildPhase = GetGuildPhase(player);
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
                if (found->IsInWorld())
                    found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 9856);
                player->ModifyMoney(GuildHouseAuctioneer * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Horde Auctioneer removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Horde Auctioneer found to remove.");
            }
            OnGossipHello(player, creature);
            break;
        }

        // Spirit Healer
        case 706491:
        {
            uint32 guildPhase = GetGuildPhase(player);
            Creature *found = nullptr;
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
            {
                Creature *c = pair.second;
                if (c && c->GetEntry() == 6491 && c->GetPhaseMask() == guildPhase)
                {
                    found = c;
                    break;
                }
            }
            if (found)
            {
                if (found->IsInWorld())
                    found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 6491);
                player->ModifyMoney(GuildHouseSpirit * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Spirit Healer removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Spirit Healer found to remove.");
            }
            OnGossipHello(player, creature);
            break;
        }

        // Tabard Vendor
        case 7028776:
        {
            uint32 guildPhase = GetGuildPhase(player);
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
                if (found->IsInWorld())
                    found->DespawnOrUnsummon();
                found->DeleteFromDB();
                WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={} AND `type`='creature'", player->GetGuildId(), 28776);
                player->ModifyMoney(GuildHouseVendor * GuildHouseRefundPercent / 100);
                ChatHandler(player->GetSession()).PSendSysMessage("Tabard Vendor removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("No Tabard Vendor found to remove.");
            }
            OnGossipHello(player, creature);
            break;
        }

        // Sell All Objects
        case 20015:
        {
            std::set<uint32> objectEntries = {1685, 4087, 180715, 2728, 184137};
            uint32 guildPhase = GetGuildPhase(player);
            int refund = 0;
            for (auto entry : objectEntries)
            {
                GameObject *found = nullptr;
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *object = pair.second;
                    if (object && object->GetEntry() == entry && object->GetPhaseMask() == guildPhase)
                    {
                        found = object;
                        break;
                    }
                }
                if (found)
                {
                    if (found->IsInWorld())
                        found->RemoveFromWorld();
                    if (sObjectMgr->GetGameObjectData(found->GetSpawnId()))
                        found->DeleteFromDB();
                    WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={}",
                                          player->GetGuildId(), entry);
                    refund += (entry == 184137 ? GuildHouseMailBox : GuildHouseObject) * GuildHouseRefundPercent / 100;
                }
            }
            player->ModifyMoney(refund);
            ChatHandler(player->GetSession()).PSendSysMessage("All objects removed and %u g refunded.", refund / 10000);
            OnGossipHello(player, creature);
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
            std::set<uint32> portalEntries = {
                500000, 500001, 500003, 500004, 500005, 500008, 500009};
            std::set<uint32> objectEntries = {
                1685, 4087, 180715, 2728, 184137 // Mailbox included
            };
            int cost = 0;

            if (essentialNpcSingleEntries.count(action))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), action);
                SpawnNPC(action, player);
                int npcCost = 0;
                if (action == 6930)
                    npcCost = GuildHouseInnKeeper;
                else if (action == 28690)
                    npcCost = GuildHouseVendor;
                else if (action == 6491)
                    npcCost = GuildHouseSpirit;
                else // Auctioneers
                    npcCost = GuildHouseAuctioneer;
                player->ModifyMoney(-npcCost);
                ChatHandler(player->GetSession()).PSendSysMessage("Essential NPC purchased and spawned.");
            }
            else if (trainerEntries.count(action))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), action);
                SpawnNPC(action, player);
                cost = (professionTrainerEntries.count(action)) ? GuildHouseProf : GuildHouseTrainer;
                player->ModifyMoney(-cost);
                ChatHandler(player->GetSession()).PSendSysMessage("Trainer purchased and spawned.");
            }
            else if (vendorEntries.count(action))
            {
                WorldDatabase.Execute("REPLACE INTO `guild_house_purchases` (`guild`, `entry`, `type`) VALUES ({}, {}, 'creature')", player->GetGuildId(), action);
                SpawnNPC(action, player);
                player->ModifyMoney(-GuildHouseVendor);
                ChatHandler(player->GetSession()).PSendSysMessage("Vendor purchased and spawned.");
            }
            else if (portalEntries.count(action))
            {
                // Remove portal with phase logic
                uint32 guildPhase = GetGuildPhase(player);
                GameObject *found = nullptr;
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *object = pair.second;
                    if (object && object->GetEntry() == action && object->GetPhaseMask() == guildPhase)
                    {
                        found = object;
                        break;
                    }
                }
                if (!found)
                {
                    SpawnObject(action, player, creature);
                    player->ModifyMoney(-GuildHousePortal);
                    ChatHandler(player->GetSession()).PSendSysMessage("Portal purchased and spawned.");
                }
                else
                {
                    if (found->IsInWorld())
                        found->RemoveFromWorld();
                    if (sObjectMgr->GetGameObjectData(found->GetSpawnId()))
                        found->DeleteFromDB();
                    WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={}", player->GetGuildId(), action);
                    player->ModifyMoney(GuildHousePortal * GuildHouseRefundPercent / 100);
                    ChatHandler(player->GetSession()).PSendSysMessage("Portal removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
                }
            }
            else if (objectEntries.count(action))
            {
                // Remove object with phase logic
                uint32 guildPhase = GetGuildPhase(player);
                GameObject *found = nullptr;
                for (auto const &pair : player->GetMap()->GetGameObjectBySpawnIdStore())
                {
                    GameObject *object = pair.second;
                    if (object && object->GetEntry() == action && object->GetPhaseMask() == guildPhase)
                    {
                        found = object;
                        break;
                    }
                }
                if (!found)
                {
                    SpawnObject(action, player, creature);
                    player->ModifyMoney(-GuildHouseObject);
                    ChatHandler(player->GetSession()).PSendSysMessage("Object purchased and spawned.");
                }
                else
                {
                    if (found->IsInWorld())
                        found->RemoveFromWorld();
                    if (sObjectMgr->GetGameObjectData(found->GetSpawnId()))
                        found->DeleteFromDB();
                    WorldDatabase.Execute("DELETE FROM `guild_house_purchases` WHERE `guild`={} AND `entry`={}", player->GetGuildId(), action);
                    player->ModifyMoney(GuildHouseObject * GuildHouseRefundPercent / 100);
                    ChatHandler(player->GetSession()).PSendSysMessage("Object removed and %u%% of the cost refunded.", GuildHouseRefundPercent);
                }
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("Unknown purchase action.");
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

    void SpawnNPC(uint32 entry, Player *player, bool force = false)
    {
        if (!player || !player->GetMap())
        {
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Error: Invalid player or map context.");
            return;
        }

        uint32 guildPhase = GetGuildPhase(player);

        if (!force)
        {
            for (auto const &pair : player->GetMap()->GetCreatureBySpawnIdStore())
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

        // Create and save to DB
        Creature *creature = new Creature();
        if (!creature->Create(player->GetMap()->GenerateLowGuid<HighGuid::Unit>(), player->GetMap(), guildPhase, entry, 0, posX, posY, posZ, ori))
        {
            delete creature;
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Failed to create NPC.");
            return;
        }
        creature->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), guildPhase);
        uint32 db_guid = creature->GetSpawnId();
        delete creature;

        // Load from DB and add to world
        Creature *loaded = new Creature();
        if (!loaded->LoadCreatureFromDB(db_guid, player->GetMap()))
        {
            delete loaded;
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Failed to load NPC from DB.");
            return;
        }
        loaded->SetPhaseMask(guildPhase, true);
        loaded->AddToWorld();

        // Add this block to make the NPC glow for 5 seconds
        loaded->CastSpell(loaded, 23108, true); // Blue Glow visual
        loaded->m_Events.AddEvent(
            new RemoveGlowAuraEvent(loaded, 23108),
            loaded->m_Events.CalculateTime(5000));

        // Refresh the gossip menu
        OnGossipHello(player, loaded);
        return;
    }

    void SpawnObject(uint32 entry, Player *player, Creature *creature, bool force = false)
    {
        if (!player || !player->GetMap())
        {
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Error: Invalid player or map context.");
            return;
        }

        uint32 guildPhase = GetGuildPhase(player);

        if (!force)
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
            if (objectSpawned)
            {
                if (player && player->GetSession())
                    ChatHandler(player->GetSession()).PSendSysMessage("This object already exists!");
                return;
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
        if (!newObject->Create(player->GetMap()->GenerateLowGuid<HighGuid::GameObject>(), entry, player->GetMap(), guildPhase, posX, posY, posZ, ori, G3D::Quat(), 0, GO_STATE_READY))
        {
            delete newObject;
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Failed to create object.");
            return;
        }
        newObject->SaveToDB(player->GetMapId(), (1 << player->GetMap()->GetSpawnMode()), guildPhase);
        uint32 db_guid = newObject->GetSpawnId();
        delete newObject;

        newObject = new GameObject();
        if (!newObject->LoadGameObjectFromDB(db_guid, player->GetMap(), true))
        {
            delete newObject;
            if (player && player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage("Failed to load object from DB.");
            return;
        }

        sObjectMgr->AddGameobjectToGrid(db_guid, sObjectMgr->GetGameObjectData(db_guid));

        // Spawn glow marker
        SpawnGlowMarker(posX, posY, posZ, ori, player->GetMap(), guildPhase);

        // Refresh the gossip menu
        OnGossipHello(player, creature);
        return;
    }

    void SpawnGlowMarker(float x, float y, float z, float o, Map *map, uint32 phase)
    {
        uint32 markerEntry = GuildHouseMarkerEntry; // use config value
        Creature *marker = new Creature();
        if (!marker->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, phase, markerEntry, 0, x, y, z, o))
        {
            delete marker;
            return;
        }
        marker->SetPhaseMask(phase, true);
        marker->SetVisible(false); // Make the marker invisible
        marker->AddToWorld();
        marker->CastSpell(marker, 23108, true); // Blue Glow
        marker->m_Events.AddEvent(
            new DespawnMarkerEvent(marker),
            marker->m_Events.CalculateTime(5000));
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
        GuildHouseRefundPercent = sConfigMgr->GetOption<int32>("GuildHouseRefundPercent", 50);

        // New options
        GuildHouseMarkerEntry = sConfigMgr->GetOption<uint32>("GuildHouseMarkerEntry", 999999);
        GuildHousePhaseBase = sConfigMgr->GetOption<uint32>("GuildHousePhaseBase", 10000);

        WorldDatabase.Execute("UPDATE guild_house_spawns SET posX=16236.0, posY=16264.1 WHERE entry=500004;");
    }
};

void AddGuildHouseButlerScripts()
{
    new GuildHouseSpawner();
    new GuildHouseButlerConf();
}
