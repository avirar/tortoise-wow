#ifndef PLAYERBOT_AI_OBJECT_H
#define PLAYERBOT_AI_OBJECT_H

#include <string>
#include "PlayerbotAIAware.h"

class AiObjectContext;
class ChatHelper;
class Player;
class PlayerBotAI;

class AiObject : public PlayerbotAIAware
{
public:
    AiObject(PlayerBotAI* botAI);

protected:
    Player* bot;
    Player* GetMaster();
    AiObjectContext* context;
    ChatHelper* chat;
};

class AiNamedObject : public AiObject
{
public:
    AiNamedObject(PlayerBotAI* botAI, std::string const name) : AiObject(botAI), name(name) {}

public:
    virtual std::string const getName() { return name; }

protected:
    std::string const name;
};

//
// TRIGGERS
//

#define BEGIN_TRIGGER(clazz, super)                 \
    class clazz : public super                      \
    {                                               \
    public:                                         \
        clazz(PlayerBotAI* botAI) : super(botAI) {} \
        bool IsActive() override;

#define END_TRIGGER() \
    }                 \
    ;

#define BUFF_TRIGGER(clazz, spell)                               \
    class clazz : public BuffTrigger                             \
    {                                                            \
    public:                                                      \
        clazz(PlayerBotAI* botAI) : BuffTrigger(botAI, spell) {} \
    }

#define BUFF_TRIGGER_A(clazz, spell)                             \
    class clazz : public BuffTrigger                             \
    {                                                            \
    public:                                                      \
        clazz(PlayerBotAI* botAI) : BuffTrigger(botAI, spell) {} \
        bool IsActive() override;                                \
    }

#define BUFF_PARTY_TRIGGER(clazz, spell)                                \
    class clazz : public BuffOnPartyTrigger                             \
    {                                                                   \
    public:                                                             \
        clazz(PlayerBotAI* botAI) : BuffOnPartyTrigger(botAI, spell) {} \
    }

#define DEBUFF_TRIGGER(clazz, spell)                               \
    class clazz : public DebuffTrigger                             \
    {                                                              \
    public:                                                        \
        clazz(PlayerBotAI* botAI) : DebuffTrigger(botAI, spell) {} \
    }

#define DEBUFF_CHECKISOWNER_TRIGGER(clazz, spell)                           \
    class clazz : public DebuffTrigger                                      \
    {                                                                       \
    public:                                                                 \
        clazz(PlayerBotAI* botAI) : DebuffTrigger(botAI, spell, 1, true) {} \
    }

#define DEBUFF_TRIGGER_A(clazz, spell)                             \
    class clazz : public DebuffTrigger                             \
    {                                                              \
    public:                                                        \
        clazz(PlayerBotAI* botAI) : DebuffTrigger(botAI, spell) {} \
        bool IsActive() override;                                  \
    }

#define DEBUFF_ENEMY_TRIGGER(clazz, spell)                                   \
    class clazz : public DebuffOnAttackerTrigger                             \
    {                                                                        \
    public:                                                                  \
        clazz(PlayerBotAI* botAI) : DebuffOnAttackerTrigger(botAI, spell) {} \
    }

#define DEBUFF_ENEMY_TRIGGER_A(clazz, spell)                                 \
    class clazz : public DebuffOnAttackerTrigger                             \
    {                                                                        \
    public:                                                                  \
        clazz(PlayerBotAI* botAI) : DebuffOnAttackerTrigger(botAI, spell) {} \
        bool IsActive() override;                                            \
    }

#define CURE_TRIGGER(clazz, spell, dispel)                                   \
    class clazz : public NeedCureTrigger                                     \
    {                                                                        \
    public:                                                                  \
        clazz(PlayerBotAI* botAI) : NeedCureTrigger(botAI, spell, dispel) {} \
    }

#define CURE_PARTY_TRIGGER(clazz, spell, dispel)                                        \
    class clazz : public PartyMemberNeedCureTrigger                                     \
    {                                                                                   \
    public:                                                                             \
        clazz(PlayerBotAI* botAI) : PartyMemberNeedCureTrigger(botAI, spell, dispel) {} \
    }

#define CAN_CAST_TRIGGER(clazz, spell)                                     \
    class clazz : public SpellCanBeCastTrigger                             \
    {                                                                      \
    public:                                                                \
        clazz(PlayerBotAI* botAI) : SpellCanBeCastTrigger(botAI, spell) {} \
    }

#define CAN_CAST_TRIGGER_A(clazz, spell)                                   \
    class clazz : public SpellCanBeCastTrigger                             \
    {                                                                      \
    public:                                                                \
        clazz(PlayerBotAI* botAI) : SpellCanBeCastTrigger(botAI, spell) {} \
        bool IsActive() override;                                          \
    }

#define CD_TRIGGER(clazz, spell)                                            \
    class clazz : public SpellNoCooldownTrigger                             \
    {                                                                       \
    public:                                                                 \
        clazz(PlayerBotAI* botAI) : SpellNoCooldownTrigger(botAI, spell) {} \
    }

#define INTERRUPT_TRIGGER(clazz, spell)                                    \
    class clazz : public InterruptSpellTrigger                             \
    {                                                                      \
    public:                                                                \
        clazz(PlayerBotAI* botAI) : InterruptSpellTrigger(botAI, spell) {} \
    }

#define INTERRUPT_TRIGGER_A(clazz, spell)                                  \
    class clazz : public InterruptSpellTrigger                             \
    {                                                                      \
    public:                                                                \
        clazz(PlayerBotAI* botAI) : InterruptSpellTrigger(botAI, spell) {} \
        bool IsActive() override;                                          \
    }

#define HAS_AURA_TRIGGER(clazz, spell)                              \
    class clazz : public HasAuraTrigger                             \
    {                                                               \
    public:                                                         \
        clazz(PlayerBotAI* botAI) : HasAuraTrigger(botAI, spell) {} \
    }

#define HAS_AURA_TRIGGER_A(clazz, spell)                            \
    class clazz : public HasAuraTrigger                             \
    {                                                               \
    public:                                                         \
        clazz(PlayerBotAI* botAI) : HasAuraTrigger(botAI, spell) {} \
        bool IsActive() override;                                   \
    }

#define SNARE_TRIGGER(clazz, spell)                                     \
    class clazz : public SnareTargetTrigger                             \
    {                                                                   \
    public:                                                             \
        clazz(PlayerBotAI* botAI) : SnareTargetTrigger(botAI, spell) {} \
    }

#define SNARE_TRIGGER_A(clazz, spell)                                   \
    class clazz : public SnareTargetTrigger                             \
    {                                                                   \
    public:                                                             \
        clazz(PlayerBotAI* botAI) : SnareTargetTrigger(botAI, spell) {} \
        bool IsActive() override;                                       \
    }

#define PROTECT_TRIGGER(clazz, spell)                                   \
    class clazz : public ProtectPartyMemberTrigger                      \
    {                                                                   \
    public:                                                             \
        clazz(PlayerBotAI* botAI) : ProtectPartyMemberTrigger(botAI) {} \
    }

#define DEFLECT_TRIGGER(clazz, spell)                                    \
    class clazz : public DeflectSpellTrigger                             \
    {                                                                    \
    public:                                                              \
        clazz(PlayerBotAI* botAI) : DeflectSpellTrigger(botAI, spell) {} \
    }

#define BOOST_TRIGGER(clazz, spell)                               \
    class clazz : public BoostTrigger                             \
    {                                                             \
    public:                                                       \
        clazz(PlayerBotAI* botAI) : BoostTrigger(botAI, spell) {} \
    }

#define BOOST_TRIGGER_A(clazz, spell)                             \
    class clazz : public BoostTrigger                             \
    {                                                             \
    public:                                                       \
        clazz(PlayerBotAI* botAI) : BoostTrigger(botAI, spell) {} \
        bool IsActive() override;                                 \
    }

#define INTERRUPT_HEALER_TRIGGER(clazz, spell)                                   \
    class clazz : public InterruptEnemyHealerTrigger                             \
    {                                                                            \
    public:                                                                      \
        clazz(PlayerBotAI* botAI) : InterruptEnemyHealerTrigger(botAI, spell) {} \
    }

#define INTERRUPT_HEALER_TRIGGER_A(clazz, spell)                                 \
    class clazz : public InterruptEnemyHealerTrigger                             \
    {                                                                            \
    public:                                                                      \
        clazz(PlayerBotAI* botAI) : InterruptEnemyHealerTrigger(botAI, spell) {} \
        bool IsActive() override;                                                \
    }

#define CC_TRIGGER(clazz, spell)                                        \
    class clazz : public HasCcTargetTrigger                             \
    {                                                                   \
    public:                                                             \
        clazz(PlayerBotAI* botAI) : HasCcTargetTrigger(botAI, spell) {} \
    }

//
// ACTIONS
//

#define MELEE_ACTION(clazz, spell)                                        \
    class clazz : public CastMeleeSpellAction                             \
    {                                                                     \
    public:                                                               \
        clazz(PlayerBotAI* botAI) : CastMeleeSpellAction(botAI, spell) {} \
    }

#define MELEE_ACTION_U(clazz, spell, useful)                              \
    class clazz : public CastMeleeSpellAction                             \
    {                                                                     \
    public:                                                               \
        clazz(PlayerBotAI* botAI) : CastMeleeSpellAction(botAI, spell) {} \
        bool isUseful() override { return useful; }                       \
    }

#define SPELL_ACTION(clazz, spell)                                   \
    class clazz : public CastSpellAction                             \
    {                                                                \
    public:                                                          \
        clazz(PlayerBotAI* botAI) : CastSpellAction(botAI, spell) {} \
    }

#define SPELL_ACTION_U(clazz, spell, useful)                         \
    class clazz : public CastSpellAction                             \
    {                                                                \
    public:                                                          \
        clazz(PlayerBotAI* botAI) : CastSpellAction(botAI, spell) {} \
        bool isUseful() override { return useful; }                  \
    }

#define HEAL_ACTION(clazz, spell)                                           \
    class clazz : public CastHealingSpellAction                             \
    {                                                                       \
    public:                                                                 \
        clazz(PlayerBotAI* botAI) : CastHealingSpellAction(botAI, spell) {} \
    }

#define HEAL_PARTY_ACTION(clazz, spell, estAmount, manaEfficiency)                                    \
    class clazz : public HealPartyMemberAction                                                        \
    {                                                                                                 \
    public:                                                                                           \
        clazz(PlayerBotAI* botAI) : HealPartyMemberAction(botAI, spell, estAmount, manaEfficiency) {} \
    }

#define AOE_HEAL_ACTION(clazz, spell, estAmount, manaEfficiency)            \
    class clazz : public CastAoeHealSpellAction                             \
    {                                                                       \
    public:                                                                 \
        clazz(PlayerBotAI* botAI) : CastAoeHealSpellAction(botAI, spell) {} \
    }

#define BUFF_ACTION(clazz, spell)                                        \
    class clazz : public CastBuffSpellAction                             \
    {                                                                    \
    public:                                                              \
        clazz(PlayerBotAI* botAI) : CastBuffSpellAction(botAI, spell) {} \
    }

#define BUFF_ACTION_U(clazz, spell, useful)                              \
    class clazz : public CastBuffSpellAction                             \
    {                                                                    \
    public:                                                              \
        clazz(PlayerBotAI* botAI) : CastBuffSpellAction(botAI, spell) {} \
        bool isUseful() override { return useful; }                      \
    }

#define BUFF_PARTY_ACTION(clazz, spell)                                \
    class clazz : public BuffOnPartyAction                             \
    {                                                                  \
    public:                                                            \
        clazz(PlayerBotAI* botAI) : BuffOnPartyAction(botAI, spell) {} \
    }

#define CURE_ACTION(clazz, spell)                                        \
    class clazz : public CastCureSpellAction                             \
    {                                                                    \
    public:                                                              \
        clazz(PlayerBotAI* botAI) : CastCureSpellAction(botAI, spell) {} \
    }

#define CURE_PARTY_ACTION(clazz, spell, dispel)                                    \
    class clazz : public CurePartyMemberAction                                     \
    {                                                                              \
    public:                                                                        \
        clazz(PlayerBotAI* botAI) : CurePartyMemberAction(botAI, spell, dispel) {} \
    }

#define RESS_ACTION(clazz, spell)                                               \
    class clazz : public ResurrectPartyMemberAction                             \
    {                                                                           \
    public:                                                                     \
        clazz(PlayerBotAI* botAI) : ResurrectPartyMemberAction(botAI, spell) {} \
    }

#define DEBUFF_ACTION(clazz, spell)                                        \
    class clazz : public CastDebuffSpellAction                             \
    {                                                                      \
    public:                                                                \
        clazz(PlayerBotAI* botAI) : CastDebuffSpellAction(botAI, spell) {} \
    }

#define DEBUFF_CHECKISOWNER_ACTION(clazz, spell)                                 \
    class clazz : public CastDebuffSpellAction                                   \
    {                                                                            \
    public:                                                                      \
        clazz(PlayerBotAI* botAI) : CastDebuffSpellAction(botAI, spell, true) {} \
    }

#define DEBUFF_ACTION_U(clazz, spell, useful)                              \
    class clazz : public CastDebuffSpellAction                             \
    {                                                                      \
    public:                                                                \
        clazz(PlayerBotAI* botAI) : CastDebuffSpellAction(botAI, spell) {} \
        bool isUseful() override { return useful; }                        \
    }

#define DEBUFF_ACTION_R(clazz, spell, distance)                                               \
    class clazz : public CastDebuffSpellAction                                                \
    {                                                                                         \
    public:                                                                                   \
        clazz(PlayerBotAI* botAI) : CastDebuffSpellAction(botAI, spell) { range = distance; } \
    }

#define DEBUFF_ENEMY_ACTION(clazz, spell)                                            \
    class clazz : public CastDebuffSpellOnAttackerAction                             \
    {                                                                                \
    public:                                                                          \
        clazz(PlayerBotAI* botAI) : CastDebuffSpellOnAttackerAction(botAI, spell) {} \
    }

#define REACH_ACTION(clazz, spell, range)                                              \
    class clazz : public CastReachTargetSpellAction                                    \
    {                                                                                  \
    public:                                                                            \
        clazz(PlayerBotAI* botAI) : CastReachTargetSpellAction(botAI, spell, range) {} \
    }

#define ENEMY_HEALER_ACTION(clazz, spell)                                         \
    class clazz : public CastSpellOnEnemyHealerAction                             \
    {                                                                             \
    public:                                                                       \
        clazz(PlayerBotAI* botAI) : CastSpellOnEnemyHealerAction(botAI, spell) {} \
    }

#define SNARE_ACTION(clazz, spell)                                        \
    class clazz : public CastSnareSpellAction                             \
    {                                                                     \
    public:                                                               \
        clazz(PlayerBotAI* botAI) : CastSnareSpellAction(botAI, spell) {} \
    }

#define CC_ACTION(clazz, spell)                                                  \
    class clazz : public CastCrowdControlSpellAction                             \
    {                                                                            \
    public:                                                                      \
        clazz(PlayerBotAI* botAI) : CastCrowdControlSpellAction(botAI, spell) {} \
    }

#define PROTECT_ACTION(clazz, spell)                                        \
    class clazz : public CastProtectSpellAction                             \
    {                                                                       \
    public:                                                                 \
        clazz(PlayerBotAI* botAI) : CastProtectSpellAction(botAI, spell) {} \
    }

#define BEGIN_SPELL_ACTION(clazz, name)  \
    class clazz : public CastSpellAction \
    {                                    \
    public:                              \
        clazz(PlayerBotAI* botAI) : CastSpellAction(botAI, name) {}

#define END_SPELL_ACTION() \
    }                      \
    ;

#define BEGIN_DEBUFF_ACTION(clazz, name)       \
    class clazz : public CastDebuffSpellAction \
    {                                          \
    public:                                    \
        clazz(PlayerBotAI* botAI) : CastDebuffSpellAction(botAI, name) {}

#define BEGIN_RANGED_SPELL_ACTION(clazz, name) \
    class clazz : public CastSpellAction       \
    {                                          \
    public:                                    \
        clazz(PlayerBotAI* botAI) : CastSpellAction(botAI, name) {}

#define BEGIN_MELEE_SPELL_ACTION(clazz, name) \
    class clazz : public CastMeleeSpellAction \
    {                                         \
    public:                                   \
        clazz(PlayerBotAI* botAI) : CastMeleeSpellAction(botAI, name) {}

#endif
