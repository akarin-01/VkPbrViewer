#pragma once

#include "resource/handle.h"
#include "resource/texture.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Skybox
        {
        public:
            using TextureHandle = Resource::Texture::Handle;

            Skybox() = default;
            ~Skybox() = default;

            void SetSkybox(TextureHandle skybox)
            {
                m_skybox = std::move(skybox);
            }
            TextureHandle GetSkybox() const { return m_skybox; }

        private:
            TextureHandle m_skybox{};
        };
    }
}
