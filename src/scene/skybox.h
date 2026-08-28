#pragma once

#include "resource/asset_types.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Skybox
        {
        public:
            Skybox() = default;
            ~Skybox() = default;

            void SetSkybox(Resource::TextureAsset::Handle skybox)
            {
                m_skybox = std::move(skybox);
            }
            Resource::TextureAsset::Handle GetSkybox() const { return m_skybox; }

        private:
            Resource::TextureAsset::Handle m_skybox{};
        };
    }
}
