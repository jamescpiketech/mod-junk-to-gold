#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "ItemTemplate.h"
#include "Transmogrification.h"

class JunkToGold : public PlayerScript
{
public:
    JunkToGold() : PlayerScript("JunkToGold") {}

    void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid /*lootguid*/) override
    {
        if (!item || !item->GetTemplate())
        {
            return;
        }

        if (item->GetTemplate()->Quality == ITEM_QUALITY_POOR)
        {
            SendTransactionInformation(player, item, count);

            // If this junk item is armor/weapon, record the appearance for transmogrification collection
            ItemTemplate const* proto = item->GetTemplate();
            if (proto && (proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON))
            {
                uint32 accountId = 0;
                if (player->GetSession())
                    accountId = player->GetSession()->GetAccountId();
                else
                    accountId = sCharacterCache->GetCharacterAccountIdByGuid(player->GetGUID());

                if (accountId)
                {
                    CharacterDatabase.Execute("INSERT IGNORE INTO custom_unlocked_appearances (account_id, item_template_id) VALUES ({}, {})", accountId, proto->ItemId);
                    sTransmogrification->AddCollectedAppearance(accountId, proto->ItemId);
                }
            }

            player->ModifyMoney(item->GetTemplate()->SellPrice * count);
            player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        }
    }

private:
    void SendTransactionInformation(Player* player, Item* item, uint32 count)
    {
        std::string name;
        if (count > 1)
        {
            name = Acore::StringFormat("|cff9d9d9d|Hitem:{}::::::::80:::::|h[{}]|h|rx{}", item->GetTemplate()->ItemId, item->GetTemplate()->Name1, count);
        }
        else
        {
            name = Acore::StringFormat("|cff9d9d9d|Hitem:{}::::::::80:::::|h[{}]|h|r", item->GetTemplate()->ItemId, item->GetTemplate()->Name1);
        }

        uint32 money = item->GetTemplate()->SellPrice * count;
        uint32 gold = money / GOLD;
        uint32 silver = (money % GOLD) / SILVER;
        uint32 copper = (money % GOLD) % SILVER;

        std::string info;
        if (money < SILVER)
        {
            info = Acore::StringFormat("{} sold for {} copper.", name, copper);
        }
        else if (money < GOLD)
        {
            if (copper > 0)
            {
                info = Acore::StringFormat("{} sold for {} silver and {} copper.", name, silver, copper);
            }
            else
            {
                info = Acore::StringFormat("{} sold for {} silver.", name, silver);
            }
        }
        else
        {
            if (copper > 0 && silver > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold, {} silver and {} copper.", name, gold, silver, copper);
            }
            else if (copper > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold and {} copper.", name, gold, copper);
            }
            else if (silver > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold and {} silver.", name, gold, silver);
            }
            else
            {
                info = Acore::StringFormat("{} sold for {} gold.", name, gold);
            }
        }

        ChatHandler(player->GetSession()).SendSysMessage(info);
    }
};

void Addmod_junk_to_goldScripts()
{
    new JunkToGold();
}
