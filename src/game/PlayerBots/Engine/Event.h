#ifndef _PLAYERBOT_EVENT_H
#define _PLAYERBOT_EVENT_H

#include <string>
#include <memory>

class PlayerbotAI;
class Unit;
class WorldPacket;

class Event
{
public:
    Event() : name(""), eventUnit(nullptr) {}
    Event(std::string const& name, Unit* unit = nullptr) : name(name), eventUnit(unit) {}
    Event(std::string const& name, std::shared_ptr<WorldPacket> packet) : name(name), eventUnit(nullptr), packetData(std::move(packet)) {}

    std::string const& GetName() const { return name; }
    Unit* GetUnit() const { return eventUnit; }
    std::shared_ptr<WorldPacket> const& GetPacket() const { return packetData; }

    bool IsEmpty() const { return name.empty(); }

    bool operator==(Event const& event) const { return name == event.name && eventUnit == event.eventUnit; }
    bool operator!=(Event const& event) const { return !(*this == event); }

private:
    std::string name;
    Unit* eventUnit;
    std::shared_ptr<WorldPacket> packetData;
};

#endif
