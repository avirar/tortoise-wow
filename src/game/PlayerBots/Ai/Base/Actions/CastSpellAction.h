#ifndef _PLAYERBOT_CAST_SPELL_ACTION_H
#define _PLAYERBOT_CAST_SPELL_ACTION_H

#include "Action.h"
#include "Timer.h"

class PlayerBotAI;
class Unit;

class CastSpellAction : public Action
{
public:
    CastSpellAction(PlayerBotAI* botAI);
    virtual ~CastSpellAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
    virtual std::string const GetTargetName() { return "current target"; }

protected:
    Unit* GetTarget();

    uint32 lastCastTime;
};

#endif
