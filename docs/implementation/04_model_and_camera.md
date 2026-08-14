# 04. 模型加载与基础渲染

> **阶段目标**：glTF 模型正确显示在屏幕上，Orbit 相机可交互。本阶段同时落地 GPU 资源组织架构（见下"架构设计"），后续阶段（材质、灯光、多 Pass）都在此骨架上扩展。

- [x] CMake：引入 glm（`GLM_FORCE_DEPTH_ZERO_TO_ONE`）、stb_image
- [x] CMake：引入 tinygltf（替换 tinyobjloader）

### 一、CPU 端参数设计

定义场景侧纯 CPU 数据结构（`src/scene/`），不依赖 Vulkan 头文件，是"事实源"：

- [x] 顶点结构体：Position、Normal、TexCoord
- [x] 顶点结构体补充 Tangent（`vec4`，w 存 handedness）——构建 TBN，normal map 的前置
- [x] 材质参数结构体：BaseColor、Metallic、Roughness、AO 等
- [x] 灯光结构体：方向、颜色、强度（CPU 侧保持单一 `Light` 概念）
- [x] 纹理结构体：像素数据 / 宽高 / 格式类型，纯数据类（`SetData` / `SetEmpty`）
- [x] 相机结构体：Orbit 参数（fov、aspect、zNear/zFar）

### 二、glTF 加载（`AssetLoader`）

本阶段只使用 glTF 的**模型数据**（几何 + 切线）；材质与纹理数据留待阶段 08（可选实现）。

- [x] `AssetLoader::LoadGltfMesh`：解析 `.gltf`/`.glb` → 填充 Mesh（几何 + 切线）
- [x] 几何：取 **mesh[0] 的全部 primitive**，同材质时合并（顶点拼接 + 索引重基址）；非索引 primitive 生成顺序索引；非 TRIANGLES mode 警告跳过
- [x] Interleaved 属性解包：按 bufferView 的 stride/offset 解出 Vertex（AoS）
- [x] 索引类型统一转换：uint8 / uint16 / uint32 → uint32
- [x] 切线：读取 glTF 的 `TANGENT` 属性（`vec4`，w 存 handedness），缺失时给默认值

**能力边界**（明确记录——是触发点，不是本次目标）：
- 渲染器只消费 **mesh[0]**（含其全部同材质 primitive）：primitive 共享 mesh 局部坐标系，合并不会错位
- glTF 的材质 / 纹理读取（factor 映射、metallicRoughness 拆双通道、三来源纹理、Mikktspace 自动切线）→ 阶段 08 可选实现
- 多 mesh / 多材质 / node 变换不在范围：无变换时不同 mesh 的顶点会叠在原点（变换在 node 上）；三者同属 DrawItem 化（`RenderList` → `std::vector<DrawItem>`），是同一个结构的三个面，出现"必须同时显示多个部分"的模型时一起做

### 三、GPU 端资源管理（`RenderResources`）

所有 GPU 资源统一托管，只认业务无关的 Buffer/Image 类型。外部通过 handle 操作，不持有 Vk 裸句柄：

- [x] `RenderResources` 类骨架：构造函数、析构函数
- [x] `CreateBuffer`：纯创建，给定 `VkBufferCreateInfo` + 内存属性，返回 `RenderBufferHandle`
- [x] `CreateBufferWithData`：附带初始数据创建，内部处理 staging → device local 上传
- [x] `WriteBuffer`：向已 mapped buffer 写入数据
- [x] `GetBuffer`：句柄 → `RenderBuffer*` 解析
- [x] `DestroyBuffer`：销毁 buffer 资源
- [x] `CreateImage` / `GetImage` / `DestroyImage`：Image 资源的同等操作
- [x] 延迟销毁队列：旧资源可能正被 in-flight 帧引用，需排队延迟释放
- [x] 数据契约 `FrameUbo`（相机 + 灯光）与 shader 共享布局定义；材质参数走 Push Constants（不占 UBO）
- [x] 创建 per-frame Uniform Buffer（ring × `kMaxFramesInFlight`），每帧更新
- [x] `DescriptorAllocator`：共享 descriptor pool 的 RAII 封装（Renderer 持有，RenderScene / RenderPass 借其分配 set）

### 四、`RenderScene` 数据处理

CPU 场景 → GPU 资源映射的唯一入口，持有各层引用（不拥有），负责 diff + 打包 + 产出 RenderList：

- [x] 资源镜像：`RenderMesh` / `RenderMaterial` / `RenderTexture` 为句柄容器，与 CPU 侧形成镜像
- [x] Create / Update / Destroy 三段式：每类资源生命周期闭环，逐帧检查 dirty → 销毁旧 → 创建新
- [x] 打包器：相机 + 灯光 → UBO 写入（唯一知道 CPU → GPU 布局映射的地方）
- [x] **材质 descriptor 所有权**：layout + set 由 RenderScene 创建与更新；纹理变化时倒计时刷新各帧槽位 set（只写当前帧、未 submit 的 set，满足 VUID 03047）
- [x] 产出 `RenderList`：材质 set / layout、Push Constants、句柄——Pass 消费的完整契约
- [x] Fallback 兜底机制：
  - **Texture 为空**（未加载 / 被 Reset）：材质槽位自动回退到 `RenderScene` 持有的共享 Fallback 纹理（每种类型 1×1 中性值：albedo=白、normal=flat、metallic/roughness/ao=白），保证 descriptor 永远绑定合法资源；`RenderScene` 析构时统一销毁，材质自身持有的是句柄拷贝（共享，不随材质销毁）
  - **Mesh 为空**：`indexCount=0` 触发空绘制（`vkCmdDrawIndexed` count=0 合法），无需兜底资源，留空即可
  - **所有权**：槽位资源只有两类——共享 Fallback（句柄与 `m_fallbackTextures[i]` 相等）或自身上传纹理；销毁路径（热替换 / 材质销毁）只销毁自有的，Fallback 仅由 `RenderScene` 统一释放

### 五、渲染与交互

- [ ] 实现 Orbit 相机：左键旋转、滚轮缩放、中键平移
- [x] 写临时 Lambert 着色器，确保模型正确显示
- [x] normal map 生效（TBN 变换 + 采样解码 `*2-1`）

---

## 架构设计：数据流概览

目标：资源所有权单一，场景与渲染解耦。

```
                  ┌────────────────────────────┐
                  │  GltfLoader（装配层）       │
                  │  .gltf/.glb → Scene        │
                  └─────────────┬──────────────┘
                                ▼
Scene (CPU 数据) ──► RenderScene ──► RenderList ──► Pass
                         │  ▲                         │
                         ▼  │                         ▼
                      RenderResources ◄─────── handle 查询
```

- **CPU 层**（`src/scene/`）：`Mesh`/`Camera`/`Light`/`Material` 纯数据，不依赖 Vulkan，通过 dirty 标记暴露变更
- **GltfLoader**：装配层，与数据类分离——解析 glTF 文件 → 填充 Scene 的 mesh / material / textures
- **RenderResources**：只认 Buffer/Image 基础 GPU 类型，不感知业务概念。提供 create/destroy/write + 延迟销毁队列，外部通过 handle 操作
- **DescriptorAllocator**：共享 descriptor pool 的 RAII 封装，业务无关（不碰 set 内容）；配额在组合根（Renderer）定义
- **RenderScene**：唯一的 CPU→GPU 映射入口。读 dirty → 调 RenderResources 创建/销毁资源 → 打包 UBO → 更新材质 descriptor → 产出 RenderList
- **数据契约**：`FrameUbo`（相机 + 灯光）与 shader 共享布局，打包器是唯一知道 CPU→GPU 布局映射的地方
- **RenderList**：材质 set / layout、Push Constants、句柄——Pass 只消费契约，不接触场景；`Draw` 为 const（纯录制）
