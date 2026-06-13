#ifndef _PLAYERBOT_EVENT_H
#define _PLAYERBOT_EVENT_H

#include <string>

class PlayerbotAI;
class Unit;

class Event
{
public:
    Event() : name(""), eventUnit(nullptr) {}
    Event(std::string const& name, Unit* unit = nullptr) : name(name), eventUnit(unit) {}

    std::string const& GetName() const { return name; }
    Unit* GetUnit() const { return eventUnit; }

    bool IsEmpty() const { return name.empty(); }

    bool operator==(Event const& event) const { return name == event.name && eventUnit == event.eventUnit; }
    bool operator!=(Event const& event) const { return !(*this == event); }

private:
    std::string name;
    Unit* eventUnit;
};

#endif
