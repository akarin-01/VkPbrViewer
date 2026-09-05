# render 层

> render 层消费 SceneProxy 的数据流，把它装配成 GPU 侧状态（描述集、UBO、离屏目标），并编排五个 pass 完成绘制。数据怎么流动（一帧时序、同步、重建路径）见 [../data-flow.md](../data-flow.md)；本文讲层内结构——状态怎么组织、pass 怎么协作。

| 文件 | 模块 | 一句话 |
| --- | --- | --- |
| renderer.h / .cpp | Renderer | 对外门面：组装各子系统，NewFrame / DrawFrame |
| scene_proxy.h / .cpp | SceneProxy | scene → render 的纯数据桥（三段式） |
| render_scene.h / .cpp | RenderScene | GPU 场景状态的唯一持有者与消费者 |
| scene_state.h | 状态容器 | FrameState / MaterialState / RenderObject… |
| render_pipeline.h / .cpp | RenderPipeline | pass 编排 + 屏障序列 |
| render_pass_base.h | RenderPassBase | pass 抽象基类 |
| passes/*.h / .cpp | 五个 pass | shadow / lit / skybox / post / ui |
| vertex_input.h | VertexInput | 顶点输入描述（与顶点着色器接口对应） |

## Renderer：对外门面（renderer.h / renderer.cpp）

组装全部子系统（Context → SwapChain → FrameSync → ResourceManager → RenderScene → RenderPipeline），并定义帧边界：`NewFrame()`（ImGui 帧开始）与 `DrawFrame()`（wait fence → graveyard flush → 重建分支 → RenderScene::Update → RenderPipeline::Draw → submit + present）。析构 `vkDeviceWaitIdle` 兜底，保证销毁时 GPU 空闲。完整时序见 [../data-flow.md](../data-flow.md)。

## SceneProxy：scene → render 的桥（scene_proxy.h / scene_proxy.cpp）

三段式数据容器（场景输入区 / 变更记录区 / 渲染输出区），生命周期的精确一帧。写入端方法做"场景参数 → GPU 结构"的转换（如 model 矩阵合成、EV → 曝光系数、光空间矩阵计算）；`Build` 与 `Reset` 由 RenderScene 在消费前后调用。三段式的动机、时序与设计问答见 [../data-flow.md](../data-flow.md)。

## RenderScene：GPU 状态的唯一持有者（render_scene.h / render_scene.cpp）

> **亮点：RenderScene 无全局状态对象，直接持有具体状态。** FrameState、TargetTextures、LitState、MaterialState、ObjectState、RenderObject 都是它的直接成员，pass 按需索取——没有一个打包一切的"GlobalState"。好处是每份状态的归属一目了然，pass 只依赖自己用到的那几份。

职责三块：

1. **消费**：`Update(frameInfo)` = Build → 更新 Frame/PostProcess/Object 状态 → `proxy.Reset()`。物体按 id find-or-add（IdVector），变更记录只处理增量，删除走 `GetDeletedObjects` 通道。
2. **装配**：`GetOrCreateMaterialState(MaterialDesc)` 走 CacheTable 内容寻址去重；装配时 spelled out 采样器策略（材质贴图统一 REPEAT + mipmap + 可选各向异性）。空贴图 id → per-slot 1×1 fallback，缺贴图不崩。
3. **持有离屏目标**：TargetTextures（MSAA color + resolve + depth，随交换链尺寸）与 ShadowTextures（2048²）由它创建并持有所有权，pass 只引用。

**重建安全依赖一条引用规则**：各 pass 与 RenderPipeline 持有的是 RenderScene **成员的引用**（`const TargetTextures&`、`const std::vector<RenderObject>&`…），而 `Recreate()` 对成员做的是**整体内容替换**（新 TargetTextures move 赋值给成员）。引用始终指向同一个成员对象，自动看到新内容——不需要通知任何 pass，重建即生效。

## 状态容器（scene_state.h）

与 shader 的 descriptor set 一一对应的宿主容器，命名规则（XxxState / XxxTextures）见 [../dev-history/architecture.md](../dev-history/architecture.md)。

| 容器 | 内容 | 说明 |
| --- | --- | --- |
| FrameState | K×UBO + K×set（含 BRDF LUT + 三张环境图） | set 0；环境图在创建时绑定，之后只写 UBO |
| PostProcessState | K×UBO + K×set | set 1；exposure |
| LitState | 单 set（shadow map 采样器） | set 1（lit 管线内）；引用 ShadowTextures，不复制 |
| MaterialState | 5 张贴图 + set，RAII Handle | set 2；`MaterialDesc`（5 贴图 id）哈希去重共享 |
| ObjectState | K×UBO + K×set | set 3；per-object model/normal/材质参数 |
| TargetTextures / ShadowTextures | TextureResource 值成员 | 离屏目标与 shadow map 的所有权 |

K = `kMaxFramesInFlight`（2）：所有"每帧写"的 UBO 与 set 都双份，按 frameIndex 选取。`RenderObject` = id + MeshResource::Handle + MaterialState::Handle + ObjectState，暴露语义 getter（`GetMaterialSet()` / `GetObjectSet(frameIndex)` / `GetVertexBuffer()`）——pass 只用 getter，不穿透到句柄链。

## RenderPipeline 与五个 pass（render_pipeline / passes/）

`RenderPassBase` 只约定两件事：`Draw(frameInfo)` 与 `RecreateResources()`，外加 context/swapchain 引用。`RenderPipeline::Draw` 录制固定的命令序列（shadow → lit → barrier → skybox → post → barrier → ui，外加各组 layout transition），屏障原语来自 rhi 的 `TransitionImageLayout` / `ImageMemoryBarrier`——完整序列与依赖分析见 [../data-flow.md](../data-flow.md)。

- **ShadowPass**：深度-only 管线（2048²），深度偏移（constant 1.25 / slope 1.75）抗阴影痤疮；管线 layout 用 rhi 的 `Empty` 布局占位 set 1/2——**与 LitPass 共享 set 编号约定**，两个 pass 的顶点输入与 set 3 完全一致。
- **LitPass**：全部 RenderObject 先**按 MaterialState id、再按 mesh id 两级排序**，绘制循环记录 last id，只在变化时 bind 材质 set / 重绑顶点索引缓冲——bind 次数从"每物体 3 次"降到"每材质 + 每网格 + 每物体 1 次"。
- **SkyboxPass**：36 顶点的 cube（非索引 draw），color/depth 都 LOAD，深度 **LEQUAL 只测不写**（store DONT_CARE）——只填充 lit 没覆盖的像素，然后随 scope 再 resolve 一次。
- **PostProcessPass**：3 顶点全屏三角形（无顶点缓冲，`vkCmdDraw(3)`），对交换链图像 loadOp DONT_CARE（整屏覆写），采样 resolve 图做曝光 + ACES 后直接写交换链。
- **UIPass**：拥有 ImGui 的完整生命周期（context + GLFW/Vulkan 双后端），以 `UseDynamicRendering` 模式初始化——动态渲染下后端不拥有 framebuffer，交换链重建时只需 `SetMinImageCount`。绘制时 LOAD 交换链图像叠加 ImGui 数据。

顶点输入（vertex_input.h）与 `Resource::Vertex` 一一对应：position / normal / texCoord / tangent（vec4，w 存手性），location 顺序即顶点着色器接口。
