#include "CurrentTargetValue.h"
#include "Player.h"
#include "Unit.h"

CurrentTargetValue::CurrentTargetValue(PlayerBotAI* botAI)
    : UnitManualSetValue(botAI, nullptr, "current target"), m_guid()
{
}

Unit* CurrentTargetValue::Get()
{
    Unit* ptr = UnitManualSetValue::Get();
    if (!ptr)
        return nullptr;

    // Validate pointer is still a live object (not dangling from despawn)
    // Check IsInWorld() first (cheapest), then IsDeleted() as backup
    if (!ptr->IsInWorld() || ptr->IsDeleted())
    {
        // Object is being removed or already removed — clear stale pointer
        UnitManualSetValue::Set(nullptr);
        m_guid.Clear();
        return nullptr;
    }

    // Verify GUID still matches (catches case where memory was reused)
    if (m_guid && ptr->GetObjectGuid() != m_guid)
    {
        UnitManualSetValue::Set(nullptr);
        m_guid.Clear();
        return nullptr;
    }

    return ptr;
}

void CurrentTargetValue::Set(Unit* val)
{
    UnitManualSetValue::Set(val);
    if (val)
        m_guid = val->GetObjectGuid();
    else
        m_guid.Clear();
}
