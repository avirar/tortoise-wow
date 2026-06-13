#ifndef PLAYERBOT_CHAT_HELPER_H
#define PLAYERBOT_CHAT_HELPER_H

#include "CommonTypes.h"
#include <string>

class PlayerbotAI;

class ChatHelper
{
public:
    ChatHelper(PlayerbotAI* botAI);
    ~ChatHelper() = default;

    void HandleCommand(std::string const& command, Player* sender);
    std::string GetName() const { return _name; }
    void Reply(std::string const& text);
    static bool parseableItem(std::string const& command);

private:
    PlayerbotAI* _botAI;
    std::string _name;
};

#endif
