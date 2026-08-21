#pragma once

namespace Kita::Pbrv
{
    class PostProcess
    {
    public:
        PostProcess();
        ~PostProcess();

        PostProcess& SetEV(float ev);

        float GetEV() const { return m_ev; }

    private:
        float m_ev{ 0.0f };
    };
}
