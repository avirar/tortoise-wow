#ifndef PLAYERBOT_TIMER_H
#define PLAYERBOT_TIMER_H

#include <ctime>
#include "Common.h"

inline uint32 getMSTime()
{
    return (uint32)(time(nullptr) * 1000);
}

#endif
