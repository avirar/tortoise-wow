#ifndef _PLAYERBOT_LOOTACTION_H
#define _PLAYERBOT_LOOTACTION_H

#include "Action.h"
#include "MovementActions.h"

class PlayerBotAI;

class LootAction : public MovementAction
{
public:
    LootAction(PlayerBotAI* botAI) : MovementAction(botAI, "loot") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class OpenLootAction : public MovementAction
{
public:
    OpenLootAction(PlayerBotAI* botAI) : MovementAction(botAI, "open loot") {}
    bool Execute(Event event) override;
};

class StoreLootAction : public Action
{
public:
    StoreLootAction(PlayerBotAI* botAI) : Action(botAI, "store loot"), lastStoredGuid(ObjectGuid()) {}
    bool Execute(Event event) override;

private:
    ObjectGuid lastStoredGuid;  // one-shot: prevent re-processing same lootGuid
};

class EquipUpgradesAction : public Action
{
public:
    EquipUpgradesAction(PlayerBotAI* botAI) : Action(botAI, "equip upgrades") {}
    bool Execute(Event event) override;
};

class AddAllLootAction : public Action
{
public:
    AddAllLootAction(PlayerBotAI* botAI) : Action(botAI, "add all loot") {}
    bool Execute(Event event) override;
};

class MoveToLootAction : public MovementAction
{
public:
    MoveToLootAction(PlayerBotAI* botAI) : MovementAction(botAI, "move to loot") {}
    bool Execute(Event event) override;
};

#endif
