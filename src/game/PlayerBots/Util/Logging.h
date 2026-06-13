#ifndef PLAYERBOT_LOGGING_H
#define PLAYERBOT_LOGGING_H

#include "Log.h"

#define LOG_ERROR(category, fmt, ...) sLog.outError("playerbots: " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(category, fmt, ...) sLog.outDebug("playerbots: " fmt, ##__VA_ARGS__)
#define LOG_INFO(category, fmt, ...)  sLog.outString("playerbots: " fmt, ##__VA_ARGS__)
#define LOG_WARN(category, fmt, ...)  sLog.outError("playerbots: " fmt, ##__VA_ARGS__)
#define LOG_TRACE(category, fmt, ...) sLog.outDebug("playerbots: " fmt, ##__VA_ARGS__)

#endif
