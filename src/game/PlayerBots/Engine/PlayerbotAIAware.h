#ifndef PLAYERBOT_PLAYERBOT_AI_AWARE_H
#define PLAYERBOT_PLAYERBOT_AI_AWARE_H

class PlayerBotAI;

class PlayerbotAIAware
{
public:
    PlayerbotAIAware(PlayerBotAI* botAI) : botAI(botAI) {}

protected:
    PlayerBotAI* botAI;
};

#endif
