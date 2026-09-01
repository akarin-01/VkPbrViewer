#pragma once

namespace Kita::Pbrv
{
    namespace Scene
    {
        class PostProcess
        {
        public:
            PostProcess();
            ~PostProcess();

            /// Writes the exposure into the SceneProxy; called every frame
            void Update() const;

            PostProcess& SetEV(float ev);

            float GetEV() const { return m_ev; }

        private:
            float m_ev{ 0.0f };
        };
    }
}
