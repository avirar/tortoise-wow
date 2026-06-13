#ifndef _PLAYERBOT_AI_OBJECT_CONTEXT_H
#define _PLAYERBOT_AI_OBJECT_CONTEXT_H

#include <map>
#include <vector>
#include <string>

class PlayerBotAI;
template <class T> class Value;

class AiObjectContext
{
public:
    AiObjectContext();
    virtual ~AiObjectContext() {}

    virtual void Init(PlayerBotAI* botAI);
    virtual void Reset();

    template <class T>
    Value<T>* GetValue(std::string const& name)
    {
        typename std::map<std::string, Value<T>*>::iterator it = valuesT<T>().find(name);
        if (it != valuesT<T>().end())
            return it->second;
        return nullptr;
    }

    template <class T>
    void AddValue(Value<T>* value)
    {
        valuesT<T>()[value->getName()] = value;
    }

    template <class T>
    void RemoveValue(std::string const& name)
    {
        valuesT<T>().erase(name);
    }

    template <class T>
    std::map<std::string, Value<T>*> &valuesT()
    {
        static std::map<std::string, Value<T>*> m;
        return m;
    }

    std::string const Format();

    std::map<std::string, void*> values;
    std::vector<std::string> performanceStack;
};

#endif
