#pragma once

#include <cmath>

namespace Kita::Pbrv
{
    namespace Core
    {
        /// Wraps an angle in degrees into [-180, 180).
        inline float WrapDegrees(float degrees)
        {
            float wrapped = std::fmod(degrees, 360.0f);
            if (wrapped >= 180.0f)
            {
                wrapped -= 360.0f;
            }
            else if (wrapped < -180.0f)
            {
                wrapped += 360.0f;
            }
            return wrapped;
        }
    }
}
