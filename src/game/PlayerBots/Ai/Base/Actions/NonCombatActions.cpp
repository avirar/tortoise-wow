#include "NonCombatActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Item.h"
#include "ItemPrototype.h"
#include "ServerFacade.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "ObjectMgr.h"
#include "Logging.h"
#include "SharedDefines.h"
#include "PlayerbotAIConfig.h"

// SetFacingAction
SetFacingAction::SetFacingAction(PlayerBotAI* botAI) : Action(botAI, "set facing") {}

bool SetFacingAction::Execute(Event /*event*/)
{
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive())
        return false;

    sServerFacade.SetFacingTo(bot, target);
    return true;
}

// SetBehindAction
SetBehindAction::SetBehindAction(PlayerBotAI* botAI) : MovementAction(botAI, "set behind") {}

bool SetBehindAction::Execute(Event /*event*/)
{
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive())
        return false;

    float angle = target->GetAngle(bot) - M_PI;
    float dist = 5.0f;
    float x = target->GetPositionX() + dist * cos(angle);
    float y = target->GetPositionY() + dist * sin(angle);
    float z = target->GetPositionZ();

    return MoveTo(x, y, z);
}

bool SetBehindAction::isUseful()
{
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    return target && target->IsAlive();
}

// EatAction: find food item in inventory and use it
// AC pattern: spellcategory_1 == 11 for food
EatAction::EatAction(PlayerBotAI* botAI) : Action(botAI, "food") {}

bool EatAction::Execute(Event /*event*/)
{
    if (bot->IsInCombat() || bot->IsMounted())
        return false;

    // Find food item in inventory (spellcategory_1 == 11 for food, AC pattern)
    Item* foodItem = nullptr;
    for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        ItemPrototype const* proto = item->GetProto();
        if (proto && proto->Spells[0].SpellId && proto->Spells[0].SpellCategory == 11)
        {
            foodItem = item;
            break;
        }
    }

    if (!foodItem)
    {
        LOG_DEBUG("playerbots", "%s [EatAction] no food item found", bot->GetName());
        return false;
    }

    // Sit down before eating
    bot->SetStandState(UNIT_STAND_STATE_SIT);

    // Build and send CMSG_USE_ITEM packet (AC UseItemAction pattern)
    WorldPacket packet(CMSG_USE_ITEM);
    packet << (uint8)foodItem->GetBagSlot() << (uint8)foodItem->GetSlot() << (uint8)1;

    bot->GetSession()->HandleUseItemOpcode(packet);

    // Set delay for eating duration (~18 seconds to full health, AC pattern)
    float hp = bot->GetHealthPercent();
    botAI->SetNextCheckDelay(std::max(10000u, static_cast<uint32>(27000.0f * (100 - hp) / 100.0f)));

    LOG_DEBUG("playerbots", "%s [EatAction] using item %u '%s' (%.0f%% hp)", bot->GetName(), foodItem->GetEntry(), foodItem->GetProto()->Name1, hp);
    return true;
}

bool EatAction::isUseful()
{
    if (!bot || bot->IsInCombat() || bot->IsMounted())
        return false;

    // Check health percentage
    uint8 health = GetAiObjectContext()->GetValue<uint8>("health")->Get();
    return health < sPlayerbotAIConfig.lowHealth;
}

bool EatAction::isPossible()
{
    return !bot->IsInCombat() && !bot->IsMounted();
}

// DrinkAction: find drink item in inventory and use it
// AC pattern: spellcategory_1 == 59 for drinks
DrinkAction::DrinkAction(PlayerBotAI* botAI) : Action(botAI, "drink") {}

bool DrinkAction::Execute(Event /*event*/)
{
    if (bot->IsInCombat() || bot->IsMounted())
        return false;

    // Find drink item in inventory (spellcategory_1 == 59 for drinks, AC pattern)
    Item* drinkItem = nullptr;
    for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        ItemPrototype const* proto = item->GetProto();
        if (proto && proto->Spells[0].SpellId && proto->Spells[0].SpellCategory == 59)
        {
            drinkItem = item;
            break;
        }
    }

    if (!drinkItem)
    {
        LOG_DEBUG("playerbots", "%s [DrinkAction] no drink item found", bot->GetName());
        return false;
    }

    // Sit down before drinking
    bot->SetStandState(UNIT_STAND_STATE_SIT);

    // Build and send CMSG_USE_ITEM packet (AC UseItemAction pattern)
    WorldPacket packet(CMSG_USE_ITEM);
    packet << (uint8)drinkItem->GetBagSlot() << (uint8)drinkItem->GetSlot() << (uint8)1;

    bot->GetSession()->HandleUseItemOpcode(packet);

    // Set delay for drinking duration (~18 seconds to full mana, AC pattern)
    float mp = bot->GetPowerPercent(POWER_MANA);
    botAI->SetNextCheckDelay(std::max(10000u, static_cast<uint32>(27000.0f * (100 - mp) / 100.0f)));

    LOG_DEBUG("playerbots", "%s [DrinkAction] using item %u '%s' (%.0f%% mana)", bot->GetName(), drinkItem->GetEntry(), drinkItem->GetProto()->Name1, mp);
    return true;
}

bool DrinkAction::isUseful()
{
    if (!bot || bot->IsInCombat() || bot->IsMounted())
        return false;

    // Check if bot has mana and mana is low
    bool hasMana = GetAiObjectContext()->GetValue<bool>("has mana")->Get();
    if (!hasMana)
        return false;

    uint8 mana = GetAiObjectContext()->GetValue<uint8>("mana")->Get();
    return mana < sPlayerbotAIConfig.lowMana;
}

bool DrinkAction::isPossible()
{
    return !bot->IsInCombat() && !bot->IsMounted();
}
