#include "ui_utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <filesystem>

namespace Kita::Pbrv
{
    std::optional<std::string> OpenFileDialog(const char* filter, const char* title)
    {
        char path[MAX_PATH]{};
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = path;
        ofn.nMaxFile = sizeof(path);
        ofn.lpstrTitle = title;

        // Default directory
        std::string initDir = std::filesystem::current_path().string();
        ofn.lpstrInitialDir = initDir.c_str();

        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameA(&ofn))
            return std::string(path);
        return std::nullopt;
    }
}
