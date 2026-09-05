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
  - `Replace Mesh` 只允许加载“单 primitive glTF/GLB”，默认使用 part index 0。
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

#### Asset / View 分层

- **实际数据类**：
  - `ModelAsset`：glTF 文件级数据，包含多个 `Part`
  - `MeshAsset`：实际几何数据
  - `TextureAsset`：实际解码后的图片数据
- **外部访问器（本身是 handle，有 id）**：
  - `MeshView`：指向 `ModelAsset` 中某个 `Part`
  - `TextureView`：指向某个 `TextureAsset`
- Scene 层持有的是 view handle，不直接穿透访问 asset 内部字段。
- View / Asset 都应提供便捷访问方法；禁止外部调用时出现 `a.xx.yy` 式穿透。

#### AssetManager 公共 API

```cpp
MeshView::Handle LoadMesh(const std::string& path, uint32_t partIndex = 0);
TextureView::Handle LoadTexture(const std::string& path, TextureAsset::Type type);
ModelLoadResult LoadModel(const std::string& path);
```

- `LoadMesh`：返回 `MeshView::Handle`，默认 part index 0。
- `LoadTexture`：返回 `TextureView::Handle`。
- `LoadModel`：返回多个可创建 `Scene::Object` 的 `ModelInstance`。

#### 公共类型

- `MeshView::Handle`
  - 外部持有的 mesh 访问器
  - 有独立 `ResourceId`
  - 同一个 `(path, partIndex)` 共享同一个 view
- `TextureView::Handle`
  - 外部持有的贴图访问器
  - 有独立 `ResourceId`
  - 同一个 `TextureKey` 共享同一个 view
- `MaterialParams`
  - 通用材质参数结构体，Scene 层可直接使用。
- `Transform`
  - 通用变换结构体，Scene 层可直接使用。
- `ModelInstance`
  - `MeshView::Handle m_mesh`
  - `std::array<TextureView::Handle, kMaterialSlotCount> m_textures`
  - `MaterialParams m_material`
  - `Transform m_transform`
- `ModelLoadResult`
  - `std::vector<ModelInstance> m_instances`

#### 缓存键

```cpp
struct MeshKey
{
    std::string m_path;
    uint32_t m_partIndex{ 0 };
};

struct TextureKey
{
    std::string m_path;
    TextureAsset::Type m_type;
};
```

#### 内部缓存结构

```cpp
CacheTable<ModelAsset, std::string>                  m_modelCache;
CacheTable<MeshView, MeshKey, MeshKey::Hash>          m_meshViewCache;
CacheTable<TextureAsset, TextureKey, TextureKey::Hash> m_textureAssetCache;
CacheTable<TextureView, TextureKey, TextureKey::Hash> m_textureViewCache;
```

- `m_modelCache`：实际 glTF 数据缓存
- `m_meshViewCache`：MeshView 句柄缓存
- `m_textureAssetCache`：实际贴图数据缓存
- `m_textureViewCache`：TextureView 句柄缓存

#### 生命周期

- `Scene::Object` 持有 `MeshView::Handle`
  - MeshView 存活
  - MeshView 持有 `ModelAsset::Handle`
  - ModelAsset 存活
- `Scene::Material` 持有 `TextureView::Handle`
  - TextureView 存活
  - TextureView 持有 `TextureAsset::Handle`
  - TextureAsset 存活
- 没有外部引用时，view 和数据都会按引用计数自动释放。
- **有效性不变量**：一个有效 view 必然持有有效且存活的 asset handle；AssetManager 只会从有效 asset 创建 view。默认构造 / 空路径返回的 view 无效，且不持有任何 asset。

#### 访问规则

- 外部通过 `MeshView::Handle` 使用便捷方法：
  - `GetName()`
  - `GetVertexCount()`
  - `GetVertexData()`
  - `GetIndexCount()`
  - `GetIndexData()`
- 外部通过 `TextureView::Handle` 使用便捷方法：
  - `GetName()`
  - `GetWidth()`
  - `GetHeight()`
  - `GetType()`
  - `GetByteCount()`
- 禁止从外部穿透访问 `view->m_model->...` 或 `view->m_texture->...` 等内部字段。

### 错误处理约定（后续阶段一律参考）

| 错误类别                                 | 方法                                                                              | 举例                                                               |
| ---------------------------------------- | --------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| 外部输入失败：文件不存在、损坏、解码失败 | 抛异常，消息含路径 + 底层原因；边界（UI 回调）try/catch → `Log::Error` 后继续运行 | `Failed to load texture 'xxx.png': <stbi 原因>`（glTF / HDR 同理） |
| 解析警告、可降级项                       | `Log::Warning` + 跳过 / 自动计算，继续渲染                                        | tinygltf warning；不支持的 primitive mode；缺失切线                |
| 枚举 / 传参非法（代码错误）              | 抛异常，初始化期暴露                                                              | `ToFormat` / `GetTextureChannels` 的 default 分支                  |
| GPU 资源创建 / 命令提交失败（致命）      | 抛异常，消息含资源名 + VkResult；传播到 main 的 catch 兜底终止                    | `CreateImage` / `CreateBuffer` 失败；`OneShotCommand` 提交失败     |
| 内部不变量（句柄 / 指针无效）            | assert 中断（debug），不继续使用                                                  | `GetImage` 返回 null                                               |
| 操作取消（正常流程）                     | `return nullopt`                                                                  | 文件对话框取消                                                     |
