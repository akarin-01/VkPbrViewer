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

            /// Writes the environment record when the skybox changes
            void Update();

            Skybox& SetSkybox(Resource::TextureAsset::Handle skybox);

            Resource::TextureAsset::Handle GetSkybox() const { return m_skybox; }

        private:
            Resource::TextureAsset::Handle m_skybox{};
            bool m_skyboxDirty{ true };
        };
    }
}
