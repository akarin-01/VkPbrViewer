# 02. 窗口与 Vulkan 初始化

> **阶段目标**：GLFW 窗口打开，Vulkan Instance/Device/Swapchain 就绪，主循环跑起来（无渲染内容）。

- [x] CMake：添加 GLFW Submodule 到 `third_party/`，链接到目标
- [x] CMake：`find_package(Vulkan REQUIRED)` 查找 Vulkan SDK，链接到目标
- [x] 基于 GLFW 创建窗口，标题 "VkPbrViewer"，设置合适默认尺寸
- [ ] 创建 Vulkan Instance，开启所需的扩展（Surface + 平台对应扩展）
- [ ] Debug 构建下启用 Validation Layers + `VK_EXT_debug_utils`，注册回调输出到控制台
- [ ] 挑选合适的物理设备（独立显卡优先），创建 Logical Device
- [ ] 获取 Graphics + Present Queue，创建 GLFW Surface
- [ ] 创建 Swapchain（选择合适格式、Present Mode、Extent）+ ImageView
- [ ] 实现 resize 回调：窗口大小变化时重建 Swapchain
  - **注意**：若窗口最小化，GLFW 返回的 Framebuffer 宽高为 0，必须在重建前拦截 `if (width == 0 || height == 0) { glfwWaitEvents(); return; }`，否则传入 Vulkan 会导致驱动崩溃
  - 后续阶段 03 创建的深度图也必须在 resize 时同步销毁并重建，否则尺寸不匹配会触发验证层报错
- [ ] 处理窗口最小化：暂停渲染，恢复时正确重建
- [ ] 实现主循环框架：poll events → （暂不渲染）→ sleep，确保窗口能正常打开和关闭
