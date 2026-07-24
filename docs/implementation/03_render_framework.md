# 03. 渲染框架

- [ ] CMake：添加 VMA Submodule 到 `third_party/`，链接到目标
- [ ] 实现 RenderContext：管理 Device、Swapchain、Command Pool、Descriptor Pool 等全局对象
- [ ] 实现 PipelineManager：管理 Shader Module 创建/销毁、Pipeline Layout 和 Graphics Pipeline 的构建
- [ ] 实现 ResourceManager：封装 VMA 进行 Buffer / Image 的分配与释放
- [ ] 使用 `VK_KHR_dynamic_rendering`：渲染时直接通过 `VkRenderingInfo` 指定 attachment，不创建传统 RenderPass 和 Framebuffer
- [ ] 创建深度 Image + ImageView（`VK_FORMAT_D32_SFLOAT`），作为 dynamic rendering 的深度 attachment
- [ ] 实现 Command Buffer 录制框架：每帧 begin → dynamic rendering begin → draw → end → submit
- [ ] 实现基本的 Frame 同步：Fence 控制 CPU-GPU 帧同步，Semaphore 控制 acquire → submit → present
- [ ] 写一个硬编码三角形（顶点写死在 Shader 里），验证整条链路能画出东西
