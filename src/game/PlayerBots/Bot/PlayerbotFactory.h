#ifndef _PLAYERBOT_FACTORY_H
#define _PLAYERBOT_FACTORY_H

#include "Common.h"
#include <vector>

class PlayerbotFactory
{
public:
    static void GenerateBots(uint32 count, std::string const& accountPrefix);

private:
    static uint32 CreateBotAccount(uint32 index, std::string const& prefix);
    static uint32 CreateBotCharacter(uint32 accountId);
    static std::string GenerateName();
    static bool IsNameTaken(std::string const& name);
    static void RegisterInPlayerbotTable(uint32 charGuid, uint32 chance, std::string const& aiName);
    static void CleanupOldBots(std::string const& prefix);

    // Load valid race/class combinations from playercreateinfo
    static void LoadValidRaceClassCombinations();
    static std::vector<std::pair<uint8, uint8>> s_validRaceClass;
};

#endif
