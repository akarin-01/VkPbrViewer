#include "log.h"

#include <atomic>
#include <iostream>
#include <mutex>

namespace Kita::Pbrv
{
    namespace Log
    {
        namespace
        {
            std::atomic<Level> g_minLevel{ Level::Debug };
            std::mutex g_mutex;

            const char* ToString(Level level)
            {
                switch (level)
                {
                case Level::Debug:      return "Debug";
                case Level::Info:       return "Info";
                case Level::Warning:    return "Warning";
                case Level::Error:      return "Error";
                default:                return "Unknown";
                }
            }
        }

        void SetLevel(Level level)
        {
            g_minLevel.store(level);
        }

        Level GetLevel()
        {
            return g_minLevel.load();
        }

        void Write(Level level, const std::string& message)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            std::ostream& os = (level >= Level::Warning) ? std::cerr : std::cout;
            os << "[" << ToString(level) << "] " << message << "\n";
        }
    }
}

