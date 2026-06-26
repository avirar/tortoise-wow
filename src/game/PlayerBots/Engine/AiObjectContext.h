#ifndef _PLAYERBOT_AI_OBJECT_CONTEXT_H
#define _PLAYERBOT_AI_OBJECT_CONTEXT_H

#include <map>
#include <vector>
#include <string>

#include "NamedObjectContext.h"
#include "Action/Action.h"
#include "Trigger/Trigger.h"
#include "Value/Value.h"

class PlayerBotAI;
class UntypedValue;

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
