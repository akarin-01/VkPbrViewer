#pragma once

#include <chrono>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Time
        {
        public:
            Time() : m_startTime(Clock::now()), m_lastTime(Clock::now())
            {
            }

            void Update()
            {
                TimePoint now = Clock::now();
                m_deltaTime = std::chrono::duration<float>(now - m_lastTime).count();
                m_lastTime = now;
            }

            float GetDeltaTime() const { return m_deltaTime; }
            float GetElapsedTime() const { return std::chrono::duration<float>(Clock::now() - m_startTime).count(); }

        private:
            using Clock = std::chrono::steady_clock;
            using TimePoint = Clock::time_point;

            TimePoint m_startTime;
            TimePoint m_lastTime;
            float m_deltaTime{ 0.0f };
        };
    }
}
