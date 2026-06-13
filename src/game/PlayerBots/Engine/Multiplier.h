#ifndef PLAYERBOT_MULTIPLIER_H
#define PLAYERBOT_MULTIPLIER_H

#include "AiObject.h"

class Action;
class PlayerBotAI;

class Multiplier : public AiNamedObject
{
public:
    Multiplier(PlayerBotAI* botAI, std::string const name) : AiNamedObject(botAI, name) {}
    virtual ~Multiplier() {}

    virtual float GetValue(Action* action) { return 1.0f; }
};

#endif
