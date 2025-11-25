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
#include "guildhouse.h"

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
                ChatHandler(player->GetSession()).PSendSysMessage("%s",
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_NOT_AUTHORIZED_PURCHASE, player).c_str());
                return false;
            }
        }
        else
        {
            ChatHandler(player->GetSession()).PSendSysMessage("%s",
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_NOT_IN_GUILD, player).c_str());
            return false;
        }

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_INNKEEPER, player),
            GOSSIP_SENDER_MAIN, GetCreatureEntry(2),
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_INNKEEPER, player),
            GuildHouseInnKeeper, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_MAILBOX, player),
            GOSSIP_SENDER_MAIN, 184137,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_MAILBOX, player),
            GuildHouseMailBox, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_STABLE_MASTER, player),
            GOSSIP_SENDER_MAIN, 28690,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_STABLE_MASTER, player),
            GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_CLASS_TRAINER, player),
            GOSSIP_SENDER_MAIN, 2);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_VENDOR_MENU, player),
            GOSSIP_SENDER_MAIN, 3);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_PORTAL_MENU, player),
            GOSSIP_SENDER_MAIN, 4);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_BANK, player),
            GOSSIP_SENDER_MAIN, 30605,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_BANK, player),
            GuildHouseBank, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_AUCTIONEER, player),
            GOSSIP_SENDER_MAIN, 6,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_AUCTIONEER, player),
            GuildHouseAuctioneer, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_NEUTRAL_AUCTIONEER, player),
            GOSSIP_SENDER_MAIN, 9858,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_NEUTRAL_AUCTIONEER, player),
            GuildHouseAuctioneer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_PRIMARY_PROF, player),
            GOSSIP_SENDER_MAIN, 7);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_SECONDARY_PROF, player),
            GOSSIP_SENDER_MAIN, 8);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_SPAWN_SPIRIT_HEALER, player),
            GOSSIP_SENDER_MAIN, 6491,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_SPIRIT_HEALER, player),
            GuildHouseSpirit, false);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {

        switch (action)
        {
        case 2: // Spawn Class Trainer
        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_DEATHKNIGHT, player),
            GOSSIP_SENDER_MAIN, 29195,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_DEATHKNIGHT, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_DRUID, player),
            GOSSIP_SENDER_MAIN, 26324,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_DRUID, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_HUNTER, player),
            GOSSIP_SENDER_MAIN, 26325,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_HUNTER, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_MAGE, player),
            GOSSIP_SENDER_MAIN, 26326,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_MAGE, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_PALADIN, player),
            GOSSIP_SENDER_MAIN, 26327,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PALADIN, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_PRIEST, player),
            GOSSIP_SENDER_MAIN, 26328,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PRIEST, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_ROGUE, player),
            GOSSIP_SENDER_MAIN, 26329,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_ROGUE, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_SHAMAN, player),
            GOSSIP_SENDER_MAIN, 26330,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_SHAMAN, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_WARLOCK, player),
            GOSSIP_SENDER_MAIN, 26331,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_WARLOCK, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_TRAINER_WARRIOR, player),
            GOSSIP_SENDER_MAIN, 26332,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_WARRIOR, player),
            GuildHouseTrainer, false);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_GO_BACK, player),
            GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 3: // Vendors
            ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_VENDOR_TRADE_SUPPLIES, player),
            GOSSIP_SENDER_MAIN, 28692,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_TRADE_SUPPLIES, player),
            GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_VENDOR_TABARD, player),
            GOSSIP_SENDER_MAIN, 28776,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_TABARD, player),
            GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_VENDOR_FOOD_DRINK, player),
            GOSSIP_SENDER_MAIN, 19572,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_FOOD_DRINK, player),
            GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_VENDOR_REAGENT, player),
            GOSSIP_SENDER_MAIN, 29636,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_REAGENT, player),
            GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_VENDOR_AMMO_REPAIR, player),
            GOSSIP_SENDER_MAIN, 29493,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_AMMO_REPAIR, player),
            GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_VENDOR_POISONS, player),
            GOSSIP_SENDER_MAIN, 2622,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_POISONS, player),
            GuildHouseVendor, false);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_GO_BACK, player),
            GOSSIP_SENDER_MAIN, 9);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 4: // Objects & Portals
            ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_OBJECT_FORGE, player),
            GOSSIP_SENDER_MAIN, 1685,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_FORGE, player),
            GuildHouseObject, false);

        AddGossipItemFor(player, GOSSIP_ICON_TALK,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_OBJECT_ANVIL, player),
            GOSSIP_SENDER_MAIN, 4087,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_ANVIL, player),
            GuildHouseObject, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_OBJECT_GUILD_VAULT, player),
            GOSSIP_SENDER_MAIN, 187293,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_GUILD_VAULT, player),
            GuildHouseObject, false);

        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_OBJECT_BARBER_CHAIR, player),
            GOSSIP_SENDER_MAIN, 191028,
            GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_BARBER_CHAIR, player),
            GuildHouseObject, false);

            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                // ALLIANCE players get these options
                AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_IRONFORGE, player),
                    GOSSIP_SENDER_MAIN, GetGameObjectEntry(3),
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_IRONFORGE, player),
                    GuildHousePortal, false);
    
                AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_DARNASSUS, player),
                    GOSSIP_SENDER_MAIN, GetGameObjectEntry(1),
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_DARNASSUS, player),
                    GuildHousePortal, false);
    
                AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_EXODAR, player),
                    GOSSIP_SENDER_MAIN, GetGameObjectEntry(2),
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_EXODAR, player),
                    GuildHousePortal, false);
            }
            else
            {
                // HORDE players get these options
                AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_UNDERCITY, player),
                    GOSSIP_SENDER_MAIN, GetGameObjectEntry(7),
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_UNDERCITY, player),
                    GuildHousePortal, false);
    
                AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_THUNDERBLUFF, player),
                    GOSSIP_SENDER_MAIN, GetGameObjectEntry(6),
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_THUNDERBLUFF, player),
                    GuildHousePortal, false);
    
                AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_SILVERMOON, player),
                    GOSSIP_SENDER_MAIN, GetGameObjectEntry(5),
                    GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_SILVERMOON, player),
                    GuildHousePortal, false);
            }

            // These two portals work for either Team
            AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_SHATTRATH, player),
                GOSSIP_SENDER_MAIN, GetGameObjectEntry(8),
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_SHATTRATH, player),
                GuildHousePortal, false);
    
            AddGossipItemFor(player, GOSSIP_ICON_TAXI,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PORTAL_DALARAN, player),
                GOSSIP_SENDER_MAIN, GetGameObjectEntry(9),
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_ADD_PORTAL_DALARAN, player),
                GuildHousePortal, false);
    
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_GO_BACK, player),
                GOSSIP_SENDER_MAIN, 9);
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
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_ALCH, player),
                GOSSIP_SENDER_MAIN, 28703,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_ALCH, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_BLACKSMITH, player),
                GOSSIP_SENDER_MAIN, 28694,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_BLACKSMITH, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_ENGINEERING, player),
                GOSSIP_SENDER_MAIN, 28697,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_ENGINEERING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_TAILORING, player),
                GOSSIP_SENDER_MAIN, 28699,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_TAILORING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_LEATHERWORKING, player),
                GOSSIP_SENDER_MAIN, 28700,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_LEATHERWORKING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_SKINNING, player),
                GOSSIP_SENDER_MAIN, 28696,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_SKINNING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_MINING, player),
                GOSSIP_SENDER_MAIN, 28698,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_MINING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_HERBALISM, player),
                GOSSIP_SENDER_MAIN, 28704,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_HERBALISM, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_ENCHANTING, player),
                GOSSIP_SENDER_MAIN, 28693,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_ENCHANTING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_JEWELCRAFTING, player),
                GOSSIP_SENDER_MAIN, 28701,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_JEWELCRAFTING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_TRAINER,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_INSCRIPTION, player),
                GOSSIP_SENDER_MAIN, 28702,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_INSCRIPTION, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_GO_BACK, player),
                GOSSIP_SENDER_MAIN, 9);
				
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 8: // Secondary Profession Trainers
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_FIRSTAID, player),
                GOSSIP_SENDER_MAIN, 28706,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_FIRSTAID, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_FISHING, player),
                GOSSIP_SENDER_MAIN, 28742,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_FISHING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_PROF_TRAINER_COOKING, player),
                GOSSIP_SENDER_MAIN, 33587,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_CONFIRM_SPAWN_PROF_COOKING, player),
                GuildHouseProf, false);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                GetGuildHouseLocaleText(GUILDHOUSE_TEXT_GOSSIP_GO_BACK, player),
                GOSSIP_SENDER_MAIN, 9);
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
        case GetCreatureEntry(2): // Innkeeper
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
        case 28694:  // Blacksmithing
        case 28698:  // Mining
        case 28697:  // Engineering
        case 28701: // Jewelcrafting
        case 28693: // Enchanting
        case 18753: // <unused>
        case 28702: // Inscription
        case 30722: // <unused>
        case 28700: // Leatherworking
        case 28696: // Skinning
        case 28703: // Alchemy
        case 28704:   // Herbalism
        case 28699:  // Tailoring
        case 33587: // Cooking
        case 28742:  // Fishing
        case 28706: // First Aid
            cost = GuildHouseProf;
            SpawnNPC(action, player);
            break;
        case 28692: // Trade Supplies
        case 28776: // Tabard Vendor
        case 19572:  // Food & Drink Vendor
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
        case GetGameObjectEntry(1): // Darnassus Portal
        case GetGameObjectEntry(2): // Exodar Portal
        case GetGameObjectEntry(3): // Ironforge Portal
        case GetGameObjectEntry(5): // Silvermoon Portal
        case GetGameObjectEntry(6): // Thunder Bluff Portal
        case GetGameObjectEntry(7): // Undercity Portal
        case GetGameObjectEntry(8): // Shattrath Portal
        case GetGameObjectEntry(9): // Dalaran Portal
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
            ChatHandler(player->GetSession()).PSendSysMessage("%s", GetGuildHouseLocaleText(GUILDHOUSE_TEXT_OBJECT_ALREADY_EXISTS, player).c_str());
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
