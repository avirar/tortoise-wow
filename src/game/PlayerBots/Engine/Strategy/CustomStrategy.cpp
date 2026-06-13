#include "CustomStrategy.h"

CustomStrategy::CustomStrategy(PlayerBotAI* botAI) : Strategy(botAI)
{
}

std::vector<NextAction> CustomStrategy::getDefaultActions()
{
    return {};
}
