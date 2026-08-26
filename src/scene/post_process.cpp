#include "post_process.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        PostProcess::PostProcess() = default;

        PostProcess::~PostProcess() = default;

        PostProcess& PostProcess::SetEV(float ev)
        {
            m_ev = ev;
            return *this;
        }
    }
}
