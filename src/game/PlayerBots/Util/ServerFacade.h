#ifndef PLAYERBOT_SERVER_FACADE_H
#define PLAYERBOT_SERVER_FACADE_H

#include "CommonTypes.h"
#include <cmath>

class ServerFacade
{
public:
    static ServerFacade& instance()
    {
        static ServerFacade instance;
        return instance;
    }

    float GetDistance2d(Unit* unit, WorldObject* wo);
    float GetDistance2d(Unit* unit, float x, float y);
    bool IsDistanceLessThan(float dist1, float dist2);
    bool IsDistanceGreaterThan(float dist1, float dist2);
    bool IsDistanceGreaterOrEqualThan(float dist1, float dist2);
    bool IsDistanceLessOrEqualThan(float dist1, float dist2);
    void SetFacingTo(Player* bot, WorldObject* wo, bool force = false);
    Unit* GetChaseTarget(Unit* target);
    void SendPacket(Player* player, WorldPacket* packet);
    Unit* SelectNearestHostileTarget(Unit* unit, float range);
    void DebugNearbyCreatures(Unit* unit, float range, const char* caller);

    // Check how many group members are already targeting this unit (AC GrindTargetValue pattern)
    uint32 GetTargetingPlayerCount(Player* bot, Unit* target);

    // Find nearest hostile target that no other group member is targeting (AC pattern)
    Unit* SelectNearestSafeTarget(Player* bot, float range);

private:
    ServerFacade() = default;
    ~ServerFacade() = default;
    ServerFacade(const ServerFacade&) = delete;
    ServerFacade& operator=(const ServerFacade&) = delete;
};

#define sServerFacade ServerFacade::instance()

#endif
