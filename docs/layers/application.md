# application 层

> application 层是程序的组装与编排：创建各层子系统、驱动主循环、提供 Inspector UI 与相机控制器。它只做"把谁按什么顺序串起来"，不拥有任何渲染或场景语义——所以它是唯一允许知道所有层存在的层。

| 文件 | 模块 | 一句话 |
| --- | --- | --- |
| app.h / .cpp | App | 子系统组装 + 主循环 |
| ui.h / .cpp | UI | ImGui Inspector：环境 / 物体 / 后处理 / 统计 |
| ui_utils.h / .cpp | DrawBox / OpenFileDialog | UI 小工具：盒子模板 + Win32 文件对话框 |
| fps_camera_controller.h / .cpp | FpsCameraController | FPS 相机（当前默认） |
| orbit_camera_controller.h / .cpp | OrbitCameraController | 轨道相机（保留，可切换） |

## App：组装与主循环（app.h / app.cpp）

**成员声明顺序即依赖顺序**：Window → Input → Time → AssetManager → Renderer → Scene → UI——构造按序进行，UI 最后（它要引用场景与两个资源管理器）。启动时检查 `assets/` 目录存在性，随后 `InitScene` 装配默认场景（DamagedHelmet + qwantani 天空盒 + EV -1），走的正是 `Utils::SpawnModel` 统一入口。

主循环每帧的固定次序：

1. `PollEvents` → `Input::Update` → `Time::Update`；ESC 退出；
2. `renderer->NewFrame()`（ImGui 帧开始）；
3. **UI 悬停分支**：`IsMouseHovered()`（= ImGui 的 `WantCaptureMouse`）决定右键是否禁用光标进入 FPS 环视、相机控制器是否生效——鼠标在面板上时，相机输入完全让位；
4. `cameraController.Update(delta)`（仅在非悬停时）；
5. `ui->Update(delta)`——即时模式编辑，直接写场景实体；
6. `scene->Update()`——实体把状态推入 SceneProxy；
7. `renderer->DrawFrame()`。

这个次序保证了一帧内的因果链：**输入影响相机 → UI 编辑落到实体 → 实体推数据 → 渲染消费**，UI 改的参数同帧就会出现在画面里。

## UI：Inspector（ui.h / ui.cpp）

单个 `NoMove` 的 Inspector 窗口，四个 Collapsing 面板：

| 面板 | 内容 |
| --- | --- |
| Stats | Perf（FPS / 帧毫秒）、Assets（模型/网格/贴图计数）、Resources（L0 各类资源计数 + 材质/物体数） |
| Environment | Skybox（选择/删除 HDR）、Light（位置/颜色/强度）、Camera（只读位置朝向 + FOV 滑块） |
| Objects | 逐物体 Transform / Mesh / Material 三个盒子 + Delete；底部 Import Objects |
| Post Process | EV 滑块 |

> **亮点：即时模式 UI 与场景实体直连，没有绑定层。** ImGui 每帧重画，控件直接读写实体 getter/setter——拖动滑块就是 `mat.SetRoughness(...)`，不存在"UI 状态 → 事件 → 更新场景"的中间层。写路径与 `SpawnModel` 完全相同，UI 不会开辟第二条修改通道。

- **读取**：资产信息（名称、尺寸、顶点数）直接经 View 句柄读——CPU 侧资产数据随 View 常驻内存正是为此设计（见 [resource.md](resource.md)）；资源统计经 ResourceManager / RenderScene 的只读计数接口。
- **错误边界**：所有导入动作（模型 / 贴图 / 天空盒）都包在 try/catch 里，失败 `Log::Error` 并保持原状态不变——加载失败不会让任何一条已有引用悬空。
- 小技巧：Objects 面板的折叠头用 `###ObjectsHeader` 稳定 id，数量变化时状态不丢失；每个物体的控件用 `PushID(object id)` 隔离。

## 相机控制器：可替换策略（fps_camera_controller / orbit_camera_controller）

控制器是"输入 → 相机"的纯映射：构造时绑定 `const Input&` 与 `Camera&`，自身只有转速/移速两个参数，`Update(delta)` 统一签名——**换控制器就是换一个构造调用**（`app.cpp` 里注释切换，Orbit 保留且支持滚轮推拉）。

- FPS：右键按住时以光标 delta 转动 yaw/pitch；WASD 沿本地轴水平移动（对角线归一化，避免斜走更快），Q/E 沿世界 Y 轴升降；全部乘 delta 保证与帧率无关。
- Orbit：滚轮改视距（推拉相机位置而非改 FOV，避免透视畸变）。

## UI 工具（ui_utils.h / ui_utils.cpp)

- `DrawBox(id, title, body)`：模板函数封装"带边框的自适应子窗口 + 标题 + 内容 + 间隔"——面板里所有盒子共用一个视觉结构；
- `OpenFileDialog(filter, title)`：Win32 `GetOpenFileNameA`，返回 `optional<string>`（取消为 nullopt）；**非 Windows 平台编译通过但返回 nullopt**，按钮成为 no-op——平台差异收敛在这一个函数里（与 CMake 的警告对应）。
