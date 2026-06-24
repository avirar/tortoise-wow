#include "PlayerbotAIBase.h"

#include "Logging.h"
#include "Timer.h"
#include "Log.h"
#include "NonCombatStrategy.h"
#include "CombatStrategy.h"
#include "MeleeCombatStrategy.h"
#include "RangedCombatStrategy.h"

PlayerbotAIBase::PlayerbotAIBase(PlayerBotAI* botAI)
    : botAI(botAI),
      engine(nullptr),
      enabled(true),
      lastUpdate(0)
{
    engine = new Engine(botAI);
}

PlayerbotAIBase::~PlayerbotAIBase()
{
    delete engine;
}

void PlayerbotAIBase::Initialize()
{
    engine->Init();

    NonCombatStrategy* nonCombatStrategy = new NonCombatStrategy(botAI);
    engine->AddStrategy(nonCombatStrategy);

    uint8 botClass = botAI->me->GetClass();
    if (botClass == CLASS_WARRIOR || botClass == CLASS_ROGUE ||
        botClass == CLASS_PALADIN || botClass == CLASS_DRUID)
    {
        MeleeCombatStrategy* meleeStrategy = new MeleeCombatStrategy(botAI);
        engine->AddStrategy(meleeStrategy);
    }
    else if (botClass == CLASS_MAGE || botClass == CLASS_PRIEST ||
             botClass == CLASS_WARLOCK || botClass == CLASS_HUNTER ||
             botClass == CLASS_SHAMAN)
    {
        RangedCombatStrategy* rangedStrategy = new RangedCombatStrategy(botAI);
        engine->AddStrategy(rangedStrategy);
    }
    else
    {
        CombatStrategy* combatStrategy = new CombatStrategy(botAI);
        engine->AddStrategy(combatStrategy);
    }

    engine->Init();
}

void PlayerbotAIBase::UpdateAI(uint32 diff)
{
    if (!enabled || !botAI)
        return;

    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
        return;

    LOG_DEBUG("playerbots", "[PlayerbotAIBase::UpdateAI] calling engine->Update");
    engine->Update(diff);
}

void PlayerbotAIBase::Reset()
{
    if (engine)
        engine->Reset();
}

void PlayerbotAIBase::SetEnabled(bool enable)
{
    enabled = enable;
}
