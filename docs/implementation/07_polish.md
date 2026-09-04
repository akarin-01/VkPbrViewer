# 07. 收尾优化

> **阶段目标**：错误处理与日志通道统一，常用渲染功能补全（自由相机、MSAA、glTF 材质、阴影）。

- [ ] 【错误处理】统一错误通道：明确各错误场景的处置方式（约定见文末专题），异常信息包含文件路径与失败原因
- [x] 【验证层回调】`VK_EXT_debug_utils`：注册 debug messenger，校验信息通过 `Log` 模块输出
- [x] 【自由相机】相机从 Orbit 改为 FPS 风格自由飞行：按住鼠标右键时激活，WASD 前后左右平移、Q/E 上下平移、鼠标移动旋转视角
- [x] 【MSAA】多采样抗锯齿：Dynamic Rendering 下通过 `VkRenderingAttachmentInfo::resolveMode` + `pResolveImageView` + 多采样临时 Color Image 实现（RenderContext 已提供 `SampleCount()` / `MaxSampleCount()`）
- [ ] 【glTF 材质】加载 glTF 模型时顺便读取材质参数与贴图：factor 映射（baseColor → albedo、metallic/roughness → 标量）；metallicRoughness 贴图直接复用现有 MR 槽位（G = roughness、B = metallic，与 shader 通道一致）。详细范围、接口与生命周期约定见文末“glTF 一键加载约定”。
- [x] 【阴影】方向光 Shadow Map：
  - 阴影贴图：独立的深度 Image + ImageView 存储 Shadow Map
  - Descriptor 布局增加阴影贴图采样器 Binding 槽位
  - 阴影 Map 经 Descriptor 传入 PBR 主 Pass

### glTF 一键加载约定（后续阶段一律参考）

#### 目标定位

- 项目目标是做一个简单的 Vulkan PBR 渲染器。
- 用户选择一个 glTF/GLB 文件后，可一键加载创建 `Scene::Object` 所需的全部信息。
- 公共 API 不暴露内部实现是 glTF 还是其它模型格式。

#### 加载范围与交互约束

- 模型级加载（`LoadModel` / 一键加载）：
  - 一个 glTF/GLB 文件可提供多个 `Scene::Object` 所需信息。
  - 该路径允许支持多 mesh、多 primitive、多 material。
  - 当前贴图来源仍只支持外部 URI。
- 单 Object 替换（Replace Mesh / Replace Texture）：
  - `Replace Mesh` 只允许加载“单 primitive glTF/GLB”。
  - `Replace Texture` 只允许加载“外部贴图文件”。
  - 手动替换不支持从 glTF 内部选择某个 primitive 或内部贴图。
- Replace Mesh 降级规则：
  - 遇到多 primitive / 多 mesh 文件时，默认加载“第一个 mesh 的第一个有效 primitive”。
  - 同时 `Log::Warning` 提示只加载了第一个可绘制部分。
  - 不做多 primitive 合并或展开。
- 当前最小验证模型：
  - `DamagedHelmet.gltf`
  - `Suzanne.gltf`
  - 两者都是单 mesh、单 primitive、单 material、外部贴图。
- 模型级多 Object 作为后续扩展目标。

#### 贴图来源

- 当前只支持外部 URI。
- data URI / GLB bufferView 内置贴图暂不考虑；后续需要时再设计。
- 外部贴图在解析时转换为绝对路径，并以 `TextureKey`（绝对路径 + `TextureAsset::Type`）缓存。
- 外部贴图可以独立加载，不与 glTF 生命周期强绑定。

#### AssetManager 公共 API

```cpp
MeshView LoadMesh(const std::string& path);
TextureView LoadTexture(const std::string& path, TextureAsset::Type type);
ModelLoadResult LoadModel(const std::string& path);
```

- `LoadMesh`：只加载 mesh，返回第一个可绘制部分的 `MeshView`。
- `LoadTexture`：只加载一张独立外部贴图，返回 `TextureView`。
- `LoadModel`：一键加载模型，返回多个可创建 `Scene::Object` 的实例。

#### 公共类型

- `MeshView`
  - 直接持有 `Handle<GltfAsset> m_asset` 与 `uint32_t m_objectIndex`，不使用 pimpl。
  - 对外只暴露 mesh 访问，例如 `GetMesh()`、`IsValid()`、`GetId()`。
  - 不暴露 `GltfObject`。
- `TextureView`
  - 持有 `TextureAsset::Handle m_texture`。
- `MaterialParams`
  - 通用材质参数结构体，不含 glTF 前缀，Scene 层可直接使用。
- `Transform`
  - 通用变换结构体，不含 glTF 前缀，Scene 层可直接使用。
- `ModelInstance`
  - `MeshView m_mesh`
  - `std::array<TextureView, kMaterialSlotCount> m_textures`
  - `MaterialParams m_material`
  - `Transform m_transform`
- `ModelLoadResult`
  - `std::vector<ModelInstance> m_instances`

#### 内部结构（不对外暴露）

- `GltfAsset` 持有 `std::vector<GltfObject>`。
- 每个 `GltfObject` 持有：
  - `MeshAsset m_mesh`
  - `MaterialParams m_material`
  - `std::array<TextureKey, kMaterialSlotCount> m_textureKeys`
  - `Transform m_transform`
- `GltfAsset` / `GltfObject` 作为内部实现，不进入 Scene 层公共 API。
- `AssetUtils` 负责实际 glTF 解析，返回可被 `GltfAsset` 缓存的数据。

#### 生命周期与缓存

- `GltfAsset` 是 Resource 层内部缓存单位，路径缓存，自动引用计数释放。
- `MeshView` 持有 `GltfAsset::Handle`，因此 Object 使用 mesh 时 `GltfAsset` 不会提前释放。
- `TextureAsset` 继续使用现有 `m_textureCache` 按 `TextureKey` 缓存。
- `LoadModel` 与 `LoadMesh` 共用同一个 `GltfAsset` 缓存，避免同一 glTF 文件重复解析。
- `LoadTexture` 独立使用 `m_textureCache`，与 glTF 文件缓存解耦。

### 错误处理约定（后续阶段一律参考）

| 错误类别                                 | 方法                                                                              | 举例                                                               |
| ---------------------------------------- | --------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| 外部输入失败：文件不存在、损坏、解码失败 | 抛异常，消息含路径 + 底层原因；边界（UI 回调）try/catch → `Log::Error` 后继续运行 | `Failed to load texture 'xxx.png': <stbi 原因>`（glTF / HDR 同理） |
| 解析警告、可降级项                       | `Log::Warning` + 跳过 / 自动计算，继续渲染                                        | tinygltf warning；不支持的 primitive mode；缺失切线                |
| 枚举 / 传参非法（代码错误）              | 抛异常，初始化期暴露                                                              | `ToFormat` / `GetTextureChannels` 的 default 分支                  |
| GPU 资源创建 / 命令提交失败（致命）      | 抛异常，消息含资源名 + VkResult；传播到 main 的 catch 兜底终止                    | `CreateImage` / `CreateBuffer` 失败；`OneShotCommand` 提交失败     |
| 内部不变量（句柄 / 指针无效）            | assert 中断（debug），不继续使用                                                  | `GetImage` 返回 null                                               |
| 操作取消（正常流程）                     | `return nullopt`                                                                  | 文件对话框取消                                                     |
