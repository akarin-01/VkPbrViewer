#pragma once

#include "imgui.h"

#include <optional>
#include <string>

namespace Kita::Pbrv
{
    namespace Application
    {
        template <typename Body>
        void DrawBox(const char* id, const char* title, Body&& body)
        {
            if (!ImGui::BeginChild(id, ImVec2(0, 0),
                ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
            {
                ImGui::EndChild();
                return;
            }

            ImGui::TextUnformatted(title);
            std::forward<Body>(body)();
            ImGui::EndChild();
            ImGui::Spacing();
        }

        std::optional<std::string> OpenFileDialog(const char* filter, const char* title = nullptr);
    }
}
