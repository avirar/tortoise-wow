#include "ChatHelper.h"
#include "Player.h"
#include "Chat.h"
#include "Helpers.h"
#include "PlayerBotAI.h"

ChatHelper::ChatHelper(PlayerBotAI* botAI)
    : _botAI(botAI), _name("")
{
}

void ChatHelper::HandleCommand(std::string const& command, Player* sender)
{
    if (!_botAI || !sender)
        return;

    std::string cmd = command;
    trim(cmd);

    if (cmd.empty())
        return;

    std::vector<std::string> args;
    split(args, cmd, " ");

    std::string action = args[0];

    if (action == "follow" || action == "come" || action == "come to me")
    {
    }
    else if (action == "stay" || action == "wait")
    {
    }
    else if (action == "go")
    {
    }
    else if (action == "help")
    {
        Reply("Available commands: follow, stay, go, loot, attack, help");
    }
    else if (action == "loot")
    {
    }
    else if (action == "attack")
    {
    }
    else
    {
        Reply("Unknown command. Type 'help' for available commands.");
    }
}

void ChatHelper::Reply(std::string const& text)
{
    if (!_botAI)
        return;
}

bool ChatHelper::parseableItem(std::string const& command)
{
    return false;
}
