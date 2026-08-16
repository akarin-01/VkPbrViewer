#include "skybox.h"

#include "core/log.h"

namespace Kita::Pbrv
{
    Skybox::Skybox() = default;

    Skybox::~Skybox() = default;

    void Skybox::SetData(const std::string& name, std::vector<float>&& pixels, uint32_t width, uint32_t height)
    {
        bool unchanged = m_name == name
            && m_pixels == pixels
            && m_width == width
            && m_height == height;
        if (unchanged)
        {
            KITA_LOG_DEBUG("[Scene] Set skybox data unchanged, skip: ", name);
            return;
        }

        m_name = name;
        m_pixels = std::move(pixels);
        m_width = width;
        m_height = height;
        m_isDirty = true;

        KITA_LOG_DEBUG("[Scene] Set skybox data: ", m_name, ", ",
            m_pixels.size(), " pixels, ",
            m_width, " * ", m_height);
    }

    void Skybox::SetEmpty()
    {
        SetData("empty", {}, 0, 0);
    }
}