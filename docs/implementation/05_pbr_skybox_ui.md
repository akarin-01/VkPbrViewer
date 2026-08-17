# 05. PBR、天空盒与 UI

> **阶段目标**：模型以 PBR 材质渲染（方向光），天空盒作为场景背景，ImGui 面板可实时调节材质与灯光参数。

### 一、PBR 直接光照

- [x] 替换临时 Blinn-Phong 着色器，实现 Cook-Torrance BRDF（D: GGX, F: Schlick Fresnel, G: Smith GGX），保留现有 TBN / normal map 支持
- [x] 实现 Tone Mapping（ACES Filmic），HDR → LDR 正确显示（sRGB 编码由 SRGB Swapchain 硬件承担，无需手动 Gamma 校正）

### 二、天空盒

- [x] 加载 HDR 环境贴图（`stbi_loadf`，equirect → cubemap 转换），创建 Cubemap Image / ImageView / Sampler
- [x] 引入 HDR 中间目标（`R16G16B16A16_SFLOAT`），与 Depth Attachment 一起封装为 `RenderTargets`，由 RenderPipeline 统一创建（随 Swapchain 重建），Pass 通过引用共享
- [x] 引入 RenderPipeline 组合结构：抽象 `RenderPassBase`（`RecreateResources` / `Draw` 生命周期），主 Pass / 天空盒 Pass / PostProcess Pass 继承之，Renderer 只持有 RenderPipeline
- [x] 渲染天空盒：全屏三角形 + Cubemap 采样，作为场景背景
  - 主 Pass（lit）先渲染：Color `LOAD_OP_CLEAR`（清 HDR 目标），Depth CLEAR 到 1.0，正常深度写入
  - 天空盒 Pass 后渲染：Color `LOAD_OP_LOAD`，Depth `LOAD_OP_LOAD` + `LESS_EQUAL` 测试，深度写关闭（只覆盖深度仍为 1.0 的背景像素，不遮挡场景）
  - 两个 Pass 共用同一个 Depth Attachment（`VK_FORMAT_D32_SFLOAT`）
- [x] PostProcess Pass：全屏三角形采样 HDR 目标，ACES Tone Mapping 后输出到 SRGB Swapchain（硬件完成 sRGB 编码），未来 Bloom / FXAA 等效果挂载于此
- [x] 天空盒 cubemap 供阶段 06 的 IBL 复用（同一张环境图，无需重复加载）

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
