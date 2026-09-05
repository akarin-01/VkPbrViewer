# 03. 渲染框架

> **阶段目标**：渲染管线跑通，屏幕上出现一个三角形。

- [x] 实现 SwapChain 类：创建 VkSwapchainKHR + ImageViews，封装 AcquireNextImage / Present / Recreate
  - Recreate 在 Acquire 返回 OUT_OF_DATE 或 Present 返回 OUT_OF_DATE/SUBOPTIMAL 时触发
  - resize 场景（窗口尺寸变化、最小化恢复）由 Recreate 统一处理
- [x] CMake：寻找 glslc，添加 Shader 编译规则（构建时 GLSL → SPIR-V）
- [x] 写一组简单的三角形 Shader（顶点写死在 VS 里，或通过硬编码顶点缓冲传入），验证编译管线
- [x] 实现 Renderer 类
  - 持有 SwapChain（管理呈现链）
  - `DrawFrame()`：acquire → record command buffer → submit → present
  - 处理 swapchain 重建：Acquire/Present 返回 OUT_OF_DATE 时在帧内重建
  - 处理窗口最小化：framebuffer size 为 0 时跳过渲染
- [x] 使用 `VK_KHR_dynamic_rendering`：渲染时通过 `VkRenderingInfo` 指定 color attachment，不创建传统 RenderPass/Framebuffer
- [x] 创建深度 Image + ImageView（`VK_FORMAT_D32_SFLOAT`），作为 dynamic rendering 的深度 attachment
  - resize 时深度图必须同步销毁并按新尺寸重建
- [x] 实现 Command Buffer 录制：每帧 begin → dynamic rendering begin（绑定 swapchain image view + depth）→ draw → end
- [x] 实现 Frame 同步：Fence 控制 CPU-GPU 帧同步（避免同时录制多帧），Semaphore 控制 acquire → submit → present
- [x] 屏幕上出现第一个三角形
