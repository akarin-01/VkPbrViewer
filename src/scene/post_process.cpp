#include "post_process.h"

#include "render/scene_proxy.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        PostProcess::PostProcess() = default;

        PostProcess::~PostProcess() = default;

        void PostProcess::Update() const
        {
            Render::SceneProxy::Get().WritePostProcessData(m_ev);
        }

        PostProcess& PostProcess::SetEV(float ev)
        {
            m_ev = ev;
            return *this;
        }
    }
}
