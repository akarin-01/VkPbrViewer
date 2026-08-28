#pragma once

#include <filesystem>
#include <string>

namespace Kita::Pbrv
{
    namespace Core
    {
        /// Lexically resolves a path against the working directory and cleans
        /// it up (separators, `..`), so the same file always yields the same
        /// cache key — no filesystem access involved.
        namespace Path
        {
            inline std::string Normalize(const std::string& path)
            {
                return std::filesystem::absolute(std::filesystem::path(path)).lexically_normal().string();
            }
        }
    }
}
