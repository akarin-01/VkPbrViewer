#pragma once

#include <sstream>
#include <string>
#include <utility>

namespace Kita::Pbrv
{
    namespace Core
    {
        namespace Log
        {
            enum class Level
            {
                Debug,
                Info,
                Warning,
                Error,
            };

            void SetLevel(Level level);
            Level GetLevel();
            void Write(Level level, const std::string& message);

            template <typename... Args>
            void Log(Level level, Args&&... args)
            {
                if (level < GetLevel())
                {
                    return;
                }

                std::ostringstream oss;
                (oss << ... << std::forward<Args>(args));
                Write(level, oss.str());
            }

            template <typename... Args>
            void Debug(Args&&... args)
            {
                Log(Level::Debug, std::forward<Args>(args)...);
            }

            template <typename... Args>
            void Info(Args&&... args)
            {
                Log(Level::Info, std::forward<Args>(args)...);
            }

            template <typename... Args>
            void Warning(Args&&... args)
            {
                Log(Level::Warning, std::forward<Args>(args)...);
            }

            template <typename... Args>
            void Error(Args&&... args)
            {
                Log(Level::Error, std::forward<Args>(args)...);
            }
        }
    }
}

#ifdef NDEBUG
#define KITA_LOG_DEBUG(...) ((void)0)
#else
#define KITA_LOG_DEBUG(...) ::Kita::Pbrv::Core::Log::Debug(__VA_ARGS__)
#endif
