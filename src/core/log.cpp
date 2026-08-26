#include "log.h"

#include <atomic>
#include <iostream>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#include <unistd.h>
#endif

namespace Kita::Pbrv
{
    namespace Core
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

                constexpr char* kDefaultColor = "\033[0m";          // default

                const char* ToColor(Level level)
                {
                    switch (level)
                    {
                    case Level::Debug:      return kDefaultColor;
                    case Level::Info:       return "\033[32m";      // green
                    case Level::Warning:    return "\033[33m";      // yellow
                    case Level::Error:      return "\033[31m";      // red
                    default:                return kDefaultColor;
                    }
                }

                bool EnableColors()
                {
#ifdef _WIN32
                    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                    if (hOut == INVALID_HANDLE_VALUE)
                    {
                        return false;
                    }

                    DWORD mode = 0;
                    if (!GetConsoleMode(hOut, &mode))
                    {
                        return false;   // Not a console (redirected output)
                    }

                    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                    return true;
#else
                    return isatty(fileno(stdout)) != 0;
#endif
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
                static const bool colorEnabled = EnableColors();

                std::lock_guard<std::mutex> lock(g_mutex);
                std::ostream& os = (level >= Level::Warning) ? std::cerr : std::cout;

                const char* color = colorEnabled ? ToColor(level) : "";
                const char* reset = colorEnabled ? kDefaultColor : "";
                os << color << "[" << ToString(level) << "] " << message << reset << "\n";
            }
        }
    }
}

