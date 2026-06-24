#include "CastSpellAction.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "AiObjectContext.h"
#include "Value.h"

CastSpellAction::CastSpellAction(PlayerBotAI* botAI)
    : Action(botAI, "cast spell"),
      lastCastTime(0)
{
}

bool CastSpellAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();

    if (!target || !target->IsAlive())
        return false;

    uint32 spellId = botAI->SelectOffensiveSpell(target);

    if (spellId > 0 && bot->HasSpell(spellId) && !bot->HasSpellCooldown(spellId))
    {
        SpellEntry const* info = sSpellMgr.GetSpellEntry(spellId);
        if (info && bot->GetPower((Powers)info->powerType) >= (uint32)info->manaCost)
        {
            bot->CastSpell(target, spellId, false);
            lastCastTime = getMSTime();
            return true;
        }
    }

    bot->Attack(target, true);
    return true;
}

bool CastSpellAction::isUseful()
{
    Unit* target = GetTarget();

    if (!target || !target->IsAlive())
        return false;

    float dist = sServerFacade.GetDistance2d(bot, target);
    return sServerFacade.IsDistanceLessOrEqualThan(dist, sPlayerbotAIConfig.spellDistance);
}

Unit* CastSpellAction::GetTarget()
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (targetValue)
    {
        Unit* target = targetValue->Get();
        if (target && target->IsAlive() && target->IsInWorld())
            return target;
    }

    return nullptr;
}
