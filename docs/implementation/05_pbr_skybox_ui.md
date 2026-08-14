# 05. PBR、天空盒与 UI

> **阶段目标**：模型以 PBR 材质渲染（方向光），天空盒作为场景背景，ImGui 面板可实时调节材质与灯光参数。

### 一、PBR 直接光照

- [ ] 替换临时 Blinn-Phong 着色器，实现 Cook-Torrance BRDF（D: GGX, F: Schlick Fresnel, G: Smith GGX），保留现有 TBN / normal map 支持
- [ ] 实现 Tone Mapping（ACES Filmic）+ Gamma 校正，HDR → LDR 正确显示

### 二、天空盒

- [ ] 加载 HDR 环境贴图（`stbi_loadf`，equirect → cubemap 转换），创建 Cubemap Image / ImageView / Sampler
- [ ] 渲染天空盒：全屏三角形 + Cubemap 采样，作为场景背景
  - 引入天空盒 Pass 时调整主渲染循环 Color / Depth Attachment 的 Load / Store 操作（天空盒 Pass 负责 `LOAD_OP_CLEAR`，主 Pass 使用 `LOAD_OP_LOAD`），避免遮挡错误
  - 天空盒深度写 1.0（或深度测试 `LESS` + 深度预置 1.0），保证不遮挡场景
  - 引入多 Pass 结构（天空盒 Pass + 主 Pass）
- [ ] 天空盒 cubemap 供阶段 06 的 IBL 复用（同一张环境图，无需重复加载）

### 三、UI（ImGui）

- [ ] CMake：FetchContent 引入 Dear ImGui（含 Vulkan + GLFW 后端）
- [ ] 集成 ImGui 的 Vulkan + GLFW 后端
  - 本项目使用 `VK_KHR_dynamic_rendering`：初始化 `ImGui_ImplVulkan_InitInfo` 时**必须**将 `UseDynamicRendering` 设为 `true`，并指定 `PipelineRenderingCreateInfo`（填充 Swapchain 颜色格式）
  - 调用 `ImGui_ImplVulkan_RenderDrawData` 时包裹在 `vkCmdBeginRendering` / `vkCmdEndRendering` 之间，作为 Pass 的最后一步绘制
- [ ] 每帧渲染：Begin → 绘制面板 → End → 录制 ImGui Draw Data
- [ ] UI 面板：帧率 / 帧间隔（`Time`）、材质参数滑块（PBR 参数）、灯光颜色 / 强度 / 方向控制
  - UI 修改直接作用于 `Scene::Light` / `Material` 对象，RenderScene 的 dirty 机制下帧自动同步（无需手动更新 UBO）
- [ ] 鼠标悬停 UI 时屏蔽相机输入（面板操作不触发相机旋转 / 平移）
- [ ] （可选）文件选择对话框（ImGuiFileDialog）：模型 / 纹理导入
