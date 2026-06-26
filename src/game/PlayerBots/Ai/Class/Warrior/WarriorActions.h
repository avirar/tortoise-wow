/**
 * @file WarriorActions.h
 * @brief Warrior class-specific combat actions (Turtle WoW 1.18.1)
 *
 * Modular pattern matching AC mod-playerbots: each action is a named spell-cast
 * action that resolves the highest known rank via spell_chain and checks
 * cooldowns, rage cost, range, and stance requirements.
 */
#ifndef _PLAYERBOT_WARRIOR_ACTIONS_H
#define _PLAYERBOT_WARRIOR_ACTIONS_H

#include "Action/Action.h"
#include "Timer.h"

class PlayerBotAI;
class Unit;

// ============================================================
// Stance actions (self-cast, stance requirement checks)
// ============================================================

class CastBattleStanceAction : public Action
{
public:
    CastBattleStanceAction(PlayerBotAI* botAI);
    virtual ~CastBattleStanceAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastDefensiveStanceAction : public Action
{
public:
    CastDefensiveStanceAction(PlayerBotAI* botAI);
    virtual ~CastDefensiveStanceAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastBerserkerStanceAction : public Action
{
public:
    CastBerserkerStanceAction(PlayerBotAI* botAI);
    virtual ~CastBerserkerStanceAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

// ============================================================
// Buff actions (self-buffs)
// ============================================================

class CastBattleShoutAction : public Action
{
public:
    CastBattleShoutAction(PlayerBotAI* botAI);
    virtual ~CastBattleShoutAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastBloodrageAction : public Action
{
public:
    CastBloodrageAction(PlayerBotAI* botAI);
    virtual ~CastBloodrageAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastBerserkerRageAction : public Action
{
public:
    CastBerserkerRageAction(PlayerBotAI* botAI);
    virtual ~CastBerserkerRageAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastDeathWishAction : public Action
{
public:
    CastDeathWishAction(PlayerBotAI* botAI);
    virtual ~CastDeathWishAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastRecklessnessAction : public Action
{
public:
    CastRecklessnessAction(PlayerBotAI* botAI);
    virtual ~CastRecklessnessAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastShieldBlockAction : public Action
{
public:
    CastShieldBlockAction(PlayerBotAI* botAI);
    virtual ~CastShieldBlockAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

class CastIntimidatingShoutAction : public Action
{
public:
    CastIntimidatingShoutAction(PlayerBotAI* botAI);
    virtual ~CastIntimidatingShoutAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

// ============================================================
// Melee offensive actions (target required, melee range)
// ============================================================

class CastChargeAction : public Action
{
public:
    CastChargeAction(PlayerBotAI* botAI);
    virtual ~CastChargeAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastHeroicStrikeAction : public Action
{
public:
    CastHeroicStrikeAction(PlayerBotAI* botAI);
    virtual ~CastHeroicStrikeAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastRendAction : public Action
{
public:
    CastRendAction(PlayerBotAI* botAI);
    virtual ~CastRendAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastSunderArmorAction : public Action
{
public:
    CastSunderArmorAction(PlayerBotAI* botAI);
    virtual ~CastSunderArmorAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastThunderClapAction : public Action
{
public:
    CastThunderClapAction(PlayerBotAI* botAI);
    virtual ~CastThunderClapAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastHamstringAction : public Action
{
public:
    CastHamstringAction(PlayerBotAI* botAI);
    virtual ~CastHamstringAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastOverpowerAction : public Action
{
public:
    CastOverpowerAction(PlayerBotAI* botAI);
    virtual ~CastOverpowerAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastWhirlwindAction : public Action
{
public:
    CastWhirlwindAction(PlayerBotAI* botAI);
    virtual ~CastWhirlwindAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastMortalStrikeAction : public Action
{
public:
    CastMortalStrikeAction(PlayerBotAI* botAI);
    virtual ~CastMortalStrikeAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastBloodthirstAction : public Action
{
public:
    CastBloodthirstAction(PlayerBotAI* botAI);
    virtual ~CastBloodthirstAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastExecuteAction : public Action
{
public:
    CastExecuteAction(PlayerBotAI* botAI);
    virtual ~CastExecuteAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastShieldSlamAction : public Action
{
public:
    CastShieldSlamAction(PlayerBotAI* botAI);
    virtual ~CastShieldSlamAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastRevengeAction : public Action
{
public:
    CastRevengeAction(PlayerBotAI* botAI);
    virtual ~CastRevengeAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastTauntAction : public Action
{
public:
    CastTauntAction(PlayerBotAI* botAI);
    virtual ~CastTauntAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastDemoralizingShoutAction : public Action
{
public:
    CastDemoralizingShoutAction(PlayerBotAI* botAI);
    virtual ~CastDemoralizingShoutAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
protected:
    Unit* GetTarget();
};

class CastSweepingStrikesAction : public Action
{
public:
    CastSweepingStrikesAction(PlayerBotAI* botAI);
    virtual ~CastSweepingStrikesAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

#endif // _PLAYERBOT_WARRIOR_ACTIONS_H
