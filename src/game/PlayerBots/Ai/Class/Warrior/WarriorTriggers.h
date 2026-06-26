/**
 * @file WarriorTriggers.h
 * @brief Warrior class-specific combat triggers
 *
 * Modular pattern matching AC mod-playerbots: each trigger checks a specific
 * condition (cooldown ready, buff expired, rage level, stance, etc.) and fires
 * when the condition is met.
 */
#ifndef _PLAYERBOT_WARRIOR_TRIGGERS_H
#define _PLAYERBOT_WARRIOR_TRIGGERS_H

#include "Trigger/Trigger.h"

class PlayerBotAI;

// ============================================================================
// Cooldown triggers
// ============================================================================

class CooldownReadyTrigger : public Trigger
{
public:
    CooldownReadyTrigger(PlayerBotAI* botAI);
    virtual ~CooldownReadyTrigger() {}
    virtual bool IsActive() override;
    virtual std::string const getName() override;
protected:
    std::string spellName;
};

// ============================================================================
// Buff triggers
// ============================================================================

class BattleShoutExpiredTrigger : public Trigger
{
public:
    BattleShoutExpiredTrigger(PlayerBotAI* botAI);
    virtual ~BattleShoutExpiredTrigger() {}
    virtual bool IsActive() override;
};

class SweepingStrikesExpiredTrigger : public Trigger
{
public:
    SweepingStrikesExpiredTrigger(PlayerBotAI* botAI);
    virtual ~SweepingStrikesExpiredTrigger() {}
    virtual bool IsActive() override;
};

// ============================================================================
// Debuff triggers
// ============================================================================

class RendExpiredTrigger : public Trigger
{
public:
    RendExpiredTrigger(PlayerBotAI* botAI);
    virtual ~RendExpiredTrigger() {}
    virtual bool IsActive() override;
};

class SunderArmorExpiredTrigger : public Trigger
{
public:
    SunderArmorExpiredTrigger(PlayerBotAI* botAI);
    virtual ~SunderArmorExpiredTrigger() {}
    virtual bool IsActive() override;
};

class ThunderClapExpiredTrigger : public Trigger
{
public:
    ThunderClapExpiredTrigger(PlayerBotAI* botAI);
    virtual ~ThunderClapExpiredTrigger() {}
    virtual bool IsActive() override;
};

// ============================================================================
// Rage triggers
// ============================================================================

class LowRageTrigger : public Trigger
{
public:
    LowRageTrigger(PlayerBotAI* botAI);
    virtual ~LowRageTrigger() {}
    virtual bool IsActive() override;
};

class MediumRageTrigger : public Trigger
{
public:
    MediumRageTrigger(PlayerBotAI* botAI);
    virtual ~MediumRageTrigger() {}
    virtual bool IsActive() override;
};

class HighRageTrigger : public Trigger
{
public:
    HighRageTrigger(PlayerBotAI* botAI);
    virtual ~HighRageTrigger() {}
    virtual bool IsActive() override;
};

// ============================================================================
// Stance triggers
// ============================================================================

class NotInBattleStanceTrigger : public Trigger
{
public:
    NotInBattleStanceTrigger(PlayerBotAI* botAI);
    virtual ~NotInBattleStanceTrigger() {}
    virtual bool IsActive() override;
};

class NotInDefensiveStanceTrigger : public Trigger
{
public:
    NotInDefensiveStanceTrigger(PlayerBotAI* botAI);
    virtual ~NotInDefensiveStanceTrigger() {}
    virtual bool IsActive() override;
};

class NotInBerserkerStanceTrigger : public Trigger
{
public:
    NotInBerserkerStanceTrigger(PlayerBotAI* botAI);
    virtual ~NotInBerserkerStanceTrigger() {}
    virtual bool IsActive() override;
};

// ============================================================================
// Target state triggers
// ============================================================================

class TargetBelow20PercentTrigger : public Trigger
{
public:
    TargetBelow20PercentTrigger(PlayerBotAI* botAI);
    virtual ~TargetBelow20PercentTrigger() {}
    virtual bool IsActive() override;
};

class TargetBelow35PercentTrigger : public Trigger
{
public:
    TargetBelow35PercentTrigger(PlayerBotAI* botAI);
    virtual ~TargetBelow35PercentTrigger() {}
    virtual bool IsActive() override;
};

class TargetDodgedTrigger : public Trigger
{
public:
    TargetDodgedTrigger(PlayerBotAI* botAI);
    virtual ~TargetDodgedTrigger() {}
    virtual bool IsActive() override;
};

// ============================================================================
// Special triggers
// ============================================================================

class DeathWishReadyTrigger : public Trigger
{
public:
    DeathWishReadyTrigger(PlayerBotAI* botAI);
    virtual ~DeathWishReadyTrigger() {}
    virtual bool IsActive() override;
};

class ShieldBlockReadyTrigger : public Trigger
{
public:
    ShieldBlockReadyTrigger(PlayerBotAI* botAI);
    virtual ~ShieldBlockReadyTrigger() {}
    virtual bool IsActive() override;
};

class IntimidatingShoutReadyTrigger : public Trigger
{
public:
    IntimidatingShoutReadyTrigger(PlayerBotAI* botAI);
    virtual ~IntimidatingShoutReadyTrigger() {}
    virtual bool IsActive() override;
};

#endif // _PLAYERBOT_WARRIOR_TRIGGERS_H
