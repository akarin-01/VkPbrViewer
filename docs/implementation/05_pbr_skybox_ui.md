# 05. PBR、天空盒与 UI

> **阶段目标**：模型以 PBR 材质渲染（方向光），天空盒作为场景背景，ImGui 面板可实时调节材质与灯光参数。

### 一、PBR 直接光照

- [x] 替换临时 Blinn-Phong 着色器，实现 Cook-Torrance BRDF（D: GGX, F: Schlick Fresnel, G: Smith GGX），保留现有 TBN / normal map 支持
- [x] 实现 Tone Mapping（ACES Filmic），HDR → LDR 正确显示（sRGB 编码由 SRGB Swapchain 硬件承担，无需手动 Gamma 校正）
- [x] 后处理曝光控制：新增 `Scene::PostProcess`（EV 参数，exposure = 2^EV），在 ACES 前乘入，UI 可调

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
- [x] 修复天空盒模糊：改用更清晰的 4K HDR 源、cubemap 面尺寸 1024→2048、equirect 专用采样器（U: REPEAT / V: CLAMP_TO_EDGE）

### 三、UI（ImGui）

- [x] CMake：FetchContent 引入 Dear ImGui（含 Vulkan + GLFW 后端）
- [x] 集成 ImGui 的 Vulkan + GLFW 后端
  - 本项目使用 `VK_KHR_dynamic_rendering`：初始化 `ImGui_ImplVulkan_InitInfo` 时**必须**将 `UseDynamicRendering` 设为 `true`，并指定 `PipelineRenderingCreateInfo`（填充 Swapchain 颜色格式）
  - 调用 `ImGui_ImplVulkan_RenderDrawData` 时包裹在 `vkCmdBeginRendering` / `vkCmdEndRendering` 之间，作为 Pass 的最后一步绘制
- [x] 每帧渲染：`Renderer::NewFrame()`（ImGui backend 开帧）→ `UI::Update`（绘制面板 + 修改 Scene）→ `UIPass::Draw`（渲染管线末位 Pass，录制 ImGui Draw Data）
- [x] UI 面板：帧率 / 帧间隔（`Time`）、材质参数滑块（PBR 参数）、灯光颜色 / 强度 / 位置（原 Direction 已重构为 Position）
  - UI 修改直接作用于 `Scene::Light` / `Material` 对象，RenderScene 的 revision 机制下帧自动同步（无需手动更新 UBO）
- [x] 鼠标悬停 UI 时屏蔽相机输入（`WantCaptureMouse` 守卫，面板操作不触发相机旋转 / 平移）
- [x] 文件选择对话框（Win32 `GetOpenFileNameA`，封装于 `ui_utils::OpenFileDialog`）：模型 / 天空盒 / 纹理导入
- [x] Light 参数由 Direction 重构为 Position：`Scene::Light` 存储原始位置向量，shader 内 `normalize(frame.lightPos.xyz)`

### 四、材质贴图与切线生成（补充）

- [ ] AssetLoader：glTF 缺少 `TANGENT` 时按三角形生成切线（标准 MikkTSpace 思路），避免法线贴图 TBN 退化
- [ ] 贴图槽位重构：Metallic / Roughness 两个槽合并为 `MetallicRoughness`（G=roughness, B=metallic），新增 Emissive 槽
- [ ] `lit.frag` 同步：从 MR 贴图取 roughness/metallic，新增 `emissiveFactor * emissiveTexture`（ACES 前加自发光）
- [ ] UI / fallback / 描述符预算同步：槽位总数保持 5，descriptor pool 数量不变
