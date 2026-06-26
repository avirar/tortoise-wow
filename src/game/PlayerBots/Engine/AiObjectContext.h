#ifndef _PLAYERBOT_AI_OBJECT_CONTEXT_H
#define _PLAYERBOT_AI_OBJECT_CONTEXT_H

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "NamedObjectContext.h"
#include "Action/Action.h"
#include "Trigger/Trigger.h"
#include "Value/Value.h"
#include "Spells/SpellMgr.h"
#include "Objects/Player.h"
#include "PlayerBotAI.h"
class UntypedValue;
class Player;

// SpellIdValue: resolve spell name to highest known rank (AC pattern)
class SpellIdValue : public CalculatedValue<uint32>
{
public:
    SpellIdValue(PlayerBotAI* botAI, std::string const& spellName)
        : CalculatedValue<uint32>(botAI, "spell id", 20 * 1000)
        , m_spellName(spellName)
    {
        std::transform(m_spellName.begin(), m_spellName.end(), m_spellName.begin(), ::tolower);
    }

    uint32 Calculate() override
    {
        if (!botAI || !botAI->me)
            return 0;

        Player* bot = botAI->me;
        uint32 bestSpellId = 0;
        uint8 bestLevel = 0;

        for (PlayerSpellMap::const_iterator itr = bot->GetSpellMap().begin();
             itr != bot->GetSpellMap().end(); ++itr)
        {
            uint32 spellId = itr->first;
            if (itr->second.state == PLAYERSPELL_REMOVED || !itr->second.active)
                continue;

            SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellId);
            if (!spellInfo || (spellInfo->Attributes & SPELL_ATTR_PASSIVE))
                continue;

            std::string spellName(spellInfo->SpellName[0]);
            std::transform(spellName.begin(), spellName.end(), spellName.begin(), ::tolower);

            if (spellName == m_spellName)
            {
                if (!bestSpellId || spellInfo->spellLevel > bestLevel)
                {
                    bestSpellId = spellId;
                    bestLevel = spellInfo->spellLevel;
                }
            }
        }
        return bestSpellId;
    }

private:
    std::string m_spellName;
};

class AiObjectContext
{
public:
    AiObjectContext();
    virtual ~AiObjectContext() {}

    virtual void Init(PlayerBotAI* botAI);
    bool IsInitialized() const { return botAI != nullptr; }
    virtual void Reset();

    UntypedValue* GetUntypedValue(std::string const& name);
    Action* GetAction(std::string const& name);
    Trigger* GetTrigger(std::string const& name);

    template <class T>
    Value<T>* GetValue(std::string const& name)
    {
        return dynamic_cast<Value<T>*>(GetUntypedValue(name));
    }

    // Qualified value lookup (AC AI_VALUE2 pattern): creates composite key "name: qualifier"
    template <class T>
    Value<T>* GetValue(std::string const& name, std::string const& qualifier)
    {
        std::string key = name + ": " + qualifier;
        Value<T>* val = dynamic_cast<Value<T>*>(GetUntypedValue(key));
        if (!val)
        {
            // Create on-demand via the shared context creator
            val = CreateValue<T>(name, qualifier);
            if (val)
                AddValue(val, key);
        }
        return val;
    }

    template <class T>
    Value<T>* CreateValue(std::string const& name, std::string const& qualifier)
    {
        if (name == "spell id")
            return static_cast<Value<T>*>(new SpellIdValue(botAI, qualifier));
        return nullptr;
    }

    template <class T>
    void AddValue(Value<T>* value, std::string const& name)
    {
        values[name] = value;
    }

    template <class T>
    void RemoveValue(std::string const& name)
    {
        std::map<std::string, UntypedValue*>::iterator it = values.find(name);
        if (it != values.end())
        {
            delete it->second;
            values.erase(it);
        }
    }

    std::string const Format();

    static void BuildAllSharedContexts();

    std::map<std::string, UntypedValue*> values;
    std::vector<std::string> performanceStack;

protected:
    NamedObjectContextList<Action> actionContexts;
    NamedObjectContextList<Trigger> triggerContexts;

private:
    PlayerBotAI* botAI;
    static SharedNamedObjectContextList<Action> sharedActionContexts;
    static SharedNamedObjectContextList<Trigger> sharedTriggerContexts;
};

#endif
