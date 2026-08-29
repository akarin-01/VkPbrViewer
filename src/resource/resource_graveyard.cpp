#include "resource_graveyard.h"

#include "core/log.h"
#include "resource/resource_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        ResourceGraveyard::ResourceGraveyard(const Rhi::Context& context)
            : m_context(context),
            m_bufferQueue([this](BufferResource& buffer)
                {
                    ResourceUtils::DestroyBufferResource(m_context, buffer);
                }),
            m_textureQueue([this](TextureResource& tex)
                {
                    // TODO: Destroy texture
                })
        {
        }

        ResourceGraveyard::~ResourceGraveyard() = default;

        void ResourceGraveyard::Flush()
        {
            size_t bufferCount = m_bufferQueue.Flush();
            size_t textureCount = m_textureQueue.Flush();

            bool empty = (bufferCount == 0 && textureCount == 0);
            if (!empty)
            {
                KITA_LOG_DEBUG("[Resource] Graveyard flush: ", bufferCount, " buffers, ",
                    textureCount, " textures");
            }
        }

        void ResourceGraveyard::PushBuffer(BufferResource&& buffer)
        {
            m_bufferQueue.Push(std::move(buffer));
        }

        void ResourceGraveyard::PushTexture(TextureResource&& tex)
        {
            m_textureQueue.Push(std::move(tex));
        }
    }
}
