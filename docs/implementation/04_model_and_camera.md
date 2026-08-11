# 04. 模型加载与基础渲染

> **阶段目标**：`.obj` 模型正确显示在屏幕上，Orbit 相机可交互。本阶段同时落地 GPU 资源组织架构（见下"架构设计"），后续阶段（材质、灯光、多 Pass）都在此骨架上扩展。

- [x] CMake：引入 glm（`GLM_FORCE_DEPTH_ZERO_TO_ONE`）、tinyobjloader、stb_image

### 一、CPU 端参数设计

定义场景侧纯 CPU 数据结构（`src/scene/`），不依赖 Vulkan 头文件，是"事实源"：

- [x] 顶点结构体：Position、Normal、TexCoord、Tangent
- [x] 材质参数结构体：BaseColor、Metallic、Roughness、AO 等
- [x] 灯光结构体：方向、颜色、强度（CPU 侧保持单一 `Light` 概念）
- [x] 纹理结构体：文件路径 / 像素数据 / 宽高 / 格式类型，支持 LoadFromFile 与 SetData（程序化）
- [x] 相机结构体：位置、朝向、透视参数（fov、aspect、zNear/zFar）
- [x] 使用 tinyobjloader 加载 `.obj`，提取顶点和索引数据
- [ ] 加载后计算法线和 Tangent（如果模型缺少）

### 二、GPU 端资源管理（`RenderResources`）

所有 GPU 资源统一托管，只认业务无关的 Buffer/Image 类型。外部通过 handle 操作，不持有 Vk 裸句柄：

- [x] `RenderResources` 类骨架：构造函数、析构函数
- [x] `CreateBuffer`：纯创建，给定 `VkBufferCreateInfo` + 内存属性，返回 `RenderBufferHandle`
- [x] `CreateBufferWithData`：附带初始数据创建，内部处理 staging → device local 上传
- [x] `WriteBuffer`：向已 mapped buffer 写入数据
- [x] `GetBuffer`：句柄 → `RenderBuffer*` 解析
- [x] `DestroyBuffer`：销毁 buffer 资源
- [x] `CreateImage` / `GetImage` / `DestroyImage`：Image 资源的同等操作
- [ ] 延迟销毁队列：旧资源可能正被 in-flight 帧引用，需排队延迟释放
- [x] 定义数据契约（`FrameUBO`：相机 + 灯光数组；`ShadowUBO`：光空间矩阵），与 shader 共享布局定义
- [x] 创建 per-frame Uniform Buffer（ring × `kMaxFramesInFlight`），每帧更新

### 三、`RenderScene` 数据处理

CPU 场景 → GPU 资源映射的唯一入口，持有 `RenderResources` 引用（不拥有），负责 diff + 打包 + 产出 RenderList：

- [x] `RenderMesh` 缩为句柄容器：`{ vertexBufferHandle, indexBufferHandle, indexCount }`，与 CPU `Mesh` 形成镜像
- [x] `RenderScene::Update`：逐帧检查 dirty → 销毁旧资源 → 创建新 GPU 资源
- [x] 顶点/索引 buffer 通过 `CreateBufferWithData` 上传
- [x] 打包器：相机 + 灯光数据结构 → GPU UBO 数据写入（唯一知道 CPU → GPU 布局映射的地方）
- [x] 产出 `RenderList`：有序 draw 命令（几何/材质/变换），存句柄不存 Vk 裸句柄

### 四、渲染与交互

- [ ] 实现 Orbit 相机：左键旋转、滚轮缩放、中键平移
- [x] 写临时 Lambert 着色器，确保模型正确显示

---

## 架构设计：数据流概览

目标：资源所有权单一，场景与渲染解耦。

```
Scene (CPU 数据) ──► RenderScene ──► RenderList ──► Pass
                         │  ▲                         │
                         ▼  │                         ▼
                      RenderResources ◄─────── handle 查询
```

- **CPU 层**（`src/scene/`）：`Mesh`/`Camera`/`Light`/`Material` 纯数据，不依赖 Vulkan，通过 dirty 标记暴露变更
- **RenderResources**：只认 Buffer/Image 基础 GPU 类型，不感知业务概念。提供 create/destroy/write + 延迟销毁队列，外部通过 handle 操作
- **RenderScene**：唯一的 CPU→GPU 映射入口。读 dirty → 调 RenderResources 创建/销毁资源 → 打包 UBO → 产出 RenderList
- **数据契约**：`FrameUBO`（相机 + 灯光数组）、`ShadowUBO`（光空间矩阵），与 shader 共享布局，打包器是唯一知道 CPU→GPU 布局映射的地方
- **RenderList**：有序 draw 命令，存 handle 不存 Vk 裸句柄；Pass 消费时通过 RenderResources 解析
