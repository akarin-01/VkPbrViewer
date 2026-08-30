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
            m_imageQueue([this](ImageResource& image)
                {
                    ResourceUtils::DestroyImageResource(m_context, image);
                })
        {
        }

        ResourceGraveyard::~ResourceGraveyard() = default;

        void ResourceGraveyard::Flush()
        {
            size_t bufferCount = m_bufferQueue.Flush();
            size_t imageCount = m_imageQueue.Flush();

            bool empty = (bufferCount == 0 && imageCount == 0);
            if (!empty)
            {
                KITA_LOG_DEBUG("[Resource] Graveyard flush: ", bufferCount, " buffers, ",
                    imageCount, " images");
            }
        }

        void ResourceGraveyard::PushBuffer(BufferResource&& buffer)
        {
            m_bufferQueue.Push(std::move(buffer));
        }

        void ResourceGraveyard::PushImage(ImageResource&& image)
        {
            m_imageQueue.Push(std::move(image));
        }
    }
}
