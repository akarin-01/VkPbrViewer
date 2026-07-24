# 07. UI 与灯光

> **阶段目标**：ImGui 面板可用，灯光参数可通过 UI 实时调节。

- [ ] CMake：通过 FetchContent 引入 Dear ImGui（含 Vulkan + GLFW 后端）
- [ ] 集成 Dear ImGui 的 Vulkan + GLFW 后端，创建 ImGui Context
  - ImGui 官方 Vulkan 后端默认基于传统 `VkRenderPass`，由于本项目使用 `VK_KHR_dynamic_rendering`，初始化 `ImGui_ImplVulkan_InitInfo` 时**必须**将 `UseDynamicRendering` 设为 `true`，并指定 `PipelineRenderingCreateInfo`（填充 Swapchain 颜色格式，如 `VK_FORMAT_B8G8R8A8_UNORM`）
  - 调用 `ImGui_ImplVulkan_RenderDrawData` 时，需将其包裹在 `vkCmdBeginRendering` / `vkCmdEndRendering` 之间，作为 Pass 的最后一步绘制
- [ ] ImGui 每帧渲染：Begin → 绘制面板 → End → 录制 ImGui Draw Data
- [ ] UI 面板：帧率/帧间隔、材质参数滑块/颜色拾取器、模型选择、贴图导入按钮
- [ ] 实现文件选择对话框（模型导入、纹理导入），如使用 ImGuiFileDialog
- [ ] 将 UI 面板的滑块/拾取器数值同步更新至阶段 04 已创建的灯光 Uniform Buffer，实现 UI 对灯光颜色/强度/方向的动态控制
- [ ] 天空盒在 UI 中切换
