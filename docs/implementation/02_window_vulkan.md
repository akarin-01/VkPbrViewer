# 02. 窗口与 Vulkan 初始化

- [x] CMake：添加 GLFW Submodule 到 `third_party/`，链接到目标
- [x] CMake：`find_package(Vulkan REQUIRED)` 查找 Vulkan SDK，链接到目标
- [x] 基于 GLFW 创建窗口，标题 "VkPbrViewer"，设置合适默认尺寸
- [ ] 创建 Vulkan Instance，开启所需的扩展（Surface + 平台对应扩展）
- [ ] Debug 构建下启用 Validation Layers + `VK_EXT_debug_utils`，注册回调输出到控制台
- [ ] 挑选合适的物理设备（独立显卡优先），创建 Logical Device
- [ ] 获取 Graphics + Present Queue，创建 GLFW Surface
- [ ] 创建 Swapchain（选择合适格式、Present Mode、Extent）+ ImageView
- [ ] 实现 resize 回调：窗口大小变化时重建 Swapchain
- [ ] 处理窗口最小化：暂停渲染，恢复时正确重建
- [ ] 实现主循环框架：poll events → （暂不渲染）→ sleep，确保窗口能正常打开和关闭
