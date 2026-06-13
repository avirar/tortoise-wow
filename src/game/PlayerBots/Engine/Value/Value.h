#ifndef _PLAYERBOT_VALUE_H
#define _PLAYERBOT_VALUE_H

#include <time.h>
#include <list>
#include <vector>
#include <sstream>
#include "AiObject.h"
#include "PerfMonitor.h"
#include "Timer.h"

class PlayerbotAI;
class Unit;

class UntypedValue : public AiNamedObject
{
public:
    UntypedValue(PlayerBotAI* botAI, std::string const& name) : AiNamedObject(botAI, name) {}
    virtual ~UntypedValue() {}
    virtual void Update() {}
    virtual void Reset() {}
    virtual std::string const Format() { return "?"; }
    virtual std::string const Save() { return "?"; }
    virtual bool Load([[maybe_unused]] std::string const value) { return false; }
};

template <class T>
class Value
{
public:
    virtual ~Value() {}
    virtual T Get() = 0;
    virtual T LazyGet() = 0;
    virtual T& RefGet() = 0;
    virtual void Reset() {}
    virtual void Set(T value) = 0;
    operator T() { return Get(); }
};

template <class T>
class CalculatedValue : public UntypedValue, public Value<T>
{
public:
    CalculatedValue(PlayerBotAI* botAI, std::string const& name = "value", uint32 checkInterval = 1)
        : UntypedValue(botAI, name),
          checkInterval(checkInterval == 1 ? 1 : (checkInterval < 100 ? checkInterval * 1000 : checkInterval)),
          lastCheckTime(0)
    {
    }

    virtual ~CalculatedValue() {}

    T Get() override
    {
        if (checkInterval < 2)
        {
            value = Calculate();
        }
        else
        {
            time_t now = getMSTime();
            if (!lastCheckTime || now - lastCheckTime >= checkInterval)
            {
                lastCheckTime = now;
                value = Calculate();
            }
        }
        return value;
    }

    T LazyGet() override
    {
        if (!lastCheckTime)
            return Get();
        return value;
    }

    T& RefGet() override
    {
        if (checkInterval < 2)
        {
            value = Calculate();
        }
        else
        {
            time_t now = getMSTime();
            if (!lastCheckTime || now - lastCheckTime >= checkInterval)
            {
                lastCheckTime = now;
                value = Calculate();
            }
        }
        return value;
    }

    void Set(T val) override { value = val; }
    void Update() override {}
    void Reset() override { lastCheckTime = 0; }

protected:
    virtual T Calculate() = 0;

    uint32 checkInterval;
    uint32 lastCheckTime;
    T value;
};

template <class T>
class ManualSetValue : public UntypedValue, public Value<T>
{
public:
    ManualSetValue(PlayerBotAI* botAI, T defaultValue, std::string const& name = "value")
        : UntypedValue(botAI, name), value(defaultValue), defaultValue(defaultValue)
    {
    }

    virtual ~ManualSetValue() {}

    T Get() override { return value; }
    T LazyGet() override { return value; }
    T& RefGet() override { return value; }
    void Set(T val) override { value = val; }
    void Update() override {}
    void Reset() override { value = defaultValue; }

protected:
    T value;
    T defaultValue;
};

class Uint8CalculatedValue : public CalculatedValue<uint8>
{
public:
    Uint8CalculatedValue(PlayerBotAI* botAI, std::string const& name = "value", uint32 checkInterval = 1)
        : CalculatedValue<uint8>(botAI, name, checkInterval) {}

    std::string const Format() override;
};

class Uint32CalculatedValue : public CalculatedValue<uint32>
{
public:
    Uint32CalculatedValue(PlayerBotAI* botAI, std::string const& name = "value", int checkInterval = 1)
        : CalculatedValue<uint32>(botAI, name, checkInterval) {}

    std::string const Format() override;
};

class FloatCalculatedValue : public CalculatedValue<float>
{
public:
    FloatCalculatedValue(PlayerBotAI* botAI, std::string const& name = "value", int checkInterval = 1)
        : CalculatedValue<float>(botAI, name, checkInterval) {}

    std::string const Format() override;
};

class BoolCalculatedValue : public CalculatedValue<bool>
{
public:
    BoolCalculatedValue(PlayerBotAI* botAI, std::string const& name = "value", int checkInterval = 1)
        : CalculatedValue<bool>(botAI, name, checkInterval) {}

    std::string const Format() override { return Calculate() ? "true" : "false"; }
};

class UnitCalculatedValue : public CalculatedValue<Unit*>
{
public:
    UnitCalculatedValue(PlayerBotAI* botAI, std::string const& name = "value", int32 checkInterval = 1);

    std::string const Format() override;
    Unit* Get() override;
};

class UnitManualSetValue : public ManualSetValue<Unit*>
{
public:
    UnitManualSetValue(PlayerBotAI* botAI, Unit* defaultValue, std::string const& name = "value")
        : ManualSetValue<Unit*>(botAI, defaultValue, name) {}

    std::string const Format() override;
    Unit* Get() override;
};

#endif
