#pragma once

namespace Kita::Pbrv
{
    class Scene;

    class UI
    {
    public:
        UI(Scene& scene);
        ~UI();

        void Update(float deltaTime);

        bool IsMouseHovered() const;

    private:
        void DrawPanel(float deltaTime);

    private:
        Scene& m_scene;
    };
}
