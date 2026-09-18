#ifndef MANGOS_PLAYERBOTAI_H
#define MANGOS_PLAYERBOTAI_H

#include "PlayerAI.h"
#include "WorldSession.h"
#include "PlayerRpgInfo.h"
#include "Event.h"
#include <map>
#include <queue>
#include <unordered_set>

struct PlayerBotEntry;
class WorldSession;
class PlayerBotAI;
class PlayerbotAIBase;
class Engine;
class AiObjectContext;
class WorldPacket;
class ExternalEventHelper;

PlayerBotAI* CreatePlayerBotAI(std::string ainame);

class PlayerBotAI: public PlayerAI
{
    public:
        explicit PlayerBotAI(Player* pPlayer = nullptr) : PlayerAI(pPlayer), botEntry(nullptr), _lastLevel(0), engine(nullptr) {}
        virtual ~PlayerBotAI();
        void Remove() override;

        virtual bool OnSessionLoaded(PlayerBotEntry* entry, WorldSession* sess);
        virtual void OnBotEntryLoad(PlayerBotEntry* entry) {}
        virtual void OnPacketReceived(WorldPacket const* packet);

        virtual void SendFakePacket(uint16 /*opcode*/) {}
        virtual void UpdateAI(const uint32 diff) override;
        virtual void OnPlayerLogin();
        virtual void OnLevelUp();
        virtual void BeforeAddToMap(Player* player) {}

        void Initialize();
        void Reset();
        Engine* GetEngine();
        void SetNextCheckDelay(uint32 delay);
        AiObjectContext* GetAiObjectContext();
        uint32 SelectOffensiveSpell(Unit* target) const;

        // Packet handling (AC ExternalEventHelper pattern)
        void HandlePacket(WorldPacket const& packet);
        void ProcessQueuedPackets();

        // AC pattern: run one registered action by name on any engine
        // (PlayerbotAI::DoSpecificAction, Bot/PlayerbotAI.cpp) — the R6.1
        // agent interface drives spell/loot/follow commands through the
        // existing engine actions.
        bool DoSpecificAction(std::string const& name, Event event = Event(), bool silent = true);

        // Engine state switching (called from actions, matches AC pattern)
        void ChangeEngine(uint8 state);

        // AC PlayerbotAI::ChangeStrategy/ClearStrategies (mod-playerbots
        // Bot/PlayerbotAI.cpp:1583) — runtime strategy add/remove/toggle for a
        // given engine state, driven by the agent interface.
        void ChangeStrategy(std::string const& names, uint8 state);
        void ClearStrategies(uint8 state);
        std::string ListStrategies(uint8 state);

        bool SpawnNewPlayer(WorldSession* sess, uint8 _class, uint32 _race, uint32 mapId, uint32 instanceId, float dx, float dy, float dz, float o);
        PlayerBotEntry* botEntry;
    protected:
        uint8 _lastLevel;
        void AutoLearnSpellsForLevel();
        void AutoEquipForLevel();
        void EquipBags();
        void GiveFoodDrink();

        // Packet handlers (AC pattern: botOutgoingPacketHandlers)
        std::map<uint16, std::string> m_packetHandlers;
        std::queue<std::shared_ptr<WorldPacket>> m_packetQueue;

    public:
        bool CanMove();
        uint32 _gearMaxDiff = 9;
        uint32 GetHighestKnownSpell(uint32 spellId) const;
        bool TargetHasAuraFromChain(Unit* target, uint32 spellId) const;

        // AC NewRpgInfo (per-bot RPG state machine + MoveFarTo stuck tracking)
        PlayerRpgInfo rpgInfo;
        // AC PlayerbotAI::lowPriorityQuest — quests abandoned for no progress
        // (do-quest state machine avoids re-selecting them).
        std::unordered_set<uint32> rpgLowPriorityQuest;

        PlayerbotAIBase* engine;
};

class PlayerCreatorAI: public PlayerBotAI
{
    public:
        explicit PlayerCreatorAI(Player* pPlayer, uint8 _race_, uint8 _class_, uint32 mapId, uint32 instanceId, float x, float y, float z, float o) :
            PlayerBotAI(pPlayer), _race(_race_), _class(_class_), _mapId(mapId), _instanceId(instanceId), _x(x), _y(y), _z(z), _o(o) { }
        virtual ~PlayerCreatorAI() {}
        bool OnSessionLoaded(PlayerBotEntry* entry, WorldSession* sess) override
        {
            return SpawnNewPlayer(sess, _class, _race, _mapId, _instanceId, _x, _y, _z, _o);
        }
    protected:
        uint8 _race;
        uint8 _class;
        uint32 _mapId;
        uint32 _instanceId;
        float _x;
        float _y;
        float _z;
        float _o;
};

class PlayerBotFleeingAI : public PlayerBotAI
{
    public:
        PlayerBotFleeingAI() : PlayerBotAI() {}
        void OnPlayerLogin() override;
};

class MageOrgrimmarAttackerAI: public PlayerBotAI
{
    public:
        explicit MageOrgrimmarAttackerAI(Player* pPlayer = nullptr) : PlayerBotAI(pPlayer) {}
        virtual ~MageOrgrimmarAttackerAI() {}
        bool OnSessionLoaded(PlayerBotEntry* entry, WorldSession* sess) override;
        void UpdateAI(const uint32 /*diff*/) override;
};

class PopulateAreaBotAI: public PlayerBotAI
{
    public:
        explicit PopulateAreaBotAI(uint32 map, float x, float y, float z, uint32 team, float radius, Player* pPlayer = nullptr) : PlayerBotAI(pPlayer), _map(map), _x(x), _y(y), _z(z), _radius(radius), _team(team) {}
        virtual ~PopulateAreaBotAI() {}
        void BeforeAddToMap(Player* player) override; // me=nullptr at call
        void OnPlayerLogin() override;
    protected:
        uint32 _map;
        float _x, _y, _z;
        float _radius;
        uint32 _team;
};
#endif
