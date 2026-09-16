#ifndef PLAYERBOT_CURRENT_TARGET_VALUE_H
#define PLAYERBOT_CURRENT_TARGET_VALUE_H

#include "Value/Value.h"
#include "ObjectGuid.h"

class PlayerBotAI;

class CurrentTargetValue : public UnitManualSetValue
{
public:
    CurrentTargetValue(PlayerBotAI* botAI);

    Unit* Get() override;  // Validates pointer via stored GUID
    void Set(Unit* val) override;  // Stores GUID alongside pointer

private:
    ObjectGuid m_guid;  // Tracks target GUID for dangling pointer detection
};

#endif
