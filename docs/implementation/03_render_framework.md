# 03. 渲染框架

> **阶段目标**：渲染管线跑通，屏幕上出现一个三角形。

- [ ] CMake：添加 VMA Submodule 到 `third_party/`，链接到目标
- [ ] CMake：寻找 glslangValidator，添加 Shader 编译规则（构建时 GLSL → SPIR-V），走通"修改 GLSL → CMake 自动编译 → 运行时加载 SPIR-V"的自动化管线
- [ ] 实现 RenderContext：管理 Device、Swapchain、Command Pool、Descriptor Pool 等全局对象
- [ ] 实现 PipelineManager：管理 Shader Module 的创建/销毁、Pipeline Layout 和 Graphics Pipeline 的构建
- [ ] 实现 ResourceManager：封装 VMA 进行 Buffer / Image 的分配与释放
- [ ] 使用 `VK_KHR_dynamic_rendering`：渲染时直接通过 `VkRenderingInfo` 指定 attachment，不创建传统 RenderPass 和 Framebuffer
  - 封装 `VkRenderingInfo` 构建函数时预留 `VkSampleCountFlagBits` 参数，为后续 MSAA 一键开启铺路
- [ ] 创建深度 Image + ImageView（`VK_FORMAT_D32_SFLOAT`），作为 dynamic rendering 的深度 attachment
  - resize 时深度图必须同步销毁并按新尺寸重建
- [ ] 实现 Command Buffer 录制框架：每帧 begin → dynamic rendering begin → draw → end → submit
- [ ] 实现基本的 Frame 同步：Fence 控制 CPU-GPU 帧同步，Semaphore 控制 acquire → submit → present
  - 为后续多 Pass（阴影 Pass → PBR 主 Pass）留出同步扩展点
- [ ] 写一个硬编码三角形（顶点写死在 Shader 里），验证整条链路能画出东西
