# 08. 架构调整

> **阶段目标**：参考成熟引擎（Unreal FPrimitiveSceneProxy、Unity SRP per-object CBUFFER、Filament Engine/资源句柄模型）调整架构与数据流，为多物体（多个 Mesh/Material 实例）支持提供基础。本次只做架构与数据流，不做性能优化（不引入 ECS、GPU 剔除、多线程录制等）。

### 一、分层设计

| 层级        | 职责                                                                                                                         |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------- |
| core        | 工具层：Window/Input/Time/Log/Path/Math（WrapDegrees 等）                                                                    |
| rhi         | Vulkan 封装层：Context/SwapChain/FrameSync、Graphics/ComputePipeline、RenderingScope/OneShotCommand/ShaderModule             |
| resource    | 资源层：AssetManager（CPU 资产缓存）、ResourceManager（GPU 资源缓存 + DescriptorManager 所有权）、句柄系统（通用抽象，见下） |
| render      | 渲染层：SceneProxy（数据流暂存 + 转换）、RenderScene（消费描述、按 id 对账、上传）、RenderPipeline（pass 编排）              |
| scene       | 场景层：Camera/Light/Object/Skybox/PostProcess 各自 `Update()` 写入 SceneProxy 单例；Scene 持有物体池（增删 + 标记清扫）     |
| application | 应用层：程序开始/关闭/渲染循环（app.cpp）；UI（Inspector 面板）与 OrbitCameraController 由本层持有                           |

### 二、句柄系统（AssetManager / ResourceManager / RenderScene 共用）

- 句柄是 **RAII 类而非裸整型**：`Handle<T>` 内部 = 条目索引 + 指向 RefTable 的指针；拷贝/赋值时引用计数 +1，析构时 -1（移动转移所有权，不增减计数），计数归零由管理器执行释放
- AssetManager：`map<path, 资产 + 计数>`；`Load(path)` 返回 RAII 句柄，同 path 复用条目；归零移除并释放 CPU 资产
- ResourceManager：按资源类型分 `HandleTable<T>`（引用计数条目）+ 内容寻址映射（`meshId` / `textureId` / `SamplerDesc` → 条目 id）；句柄归零后拆解为**裸 Vk 句柄**进 `Graveyard`，K 帧后销毁（详见"资源与状态划分"）
- RenderScene：负责 render 状态组合；`MaterialState` 以 `MaterialDesc` 为 key 在 RenderScene 的 `CacheTable` 中共享
- **DescriptorSetRhi**：L0 的资源类型之一（`{ VkDescriptorSet, DescriptorLayoutType }`），随 handle 生命周期走 graveyard，K 帧后经 `DescriptorManager::Recycle` 回收到池的 free list 复用

### 三、数据流

```
scene 层   Camera::Update ──┐   每帧写入区：相机轴/灯光/后处理/物体 transform+材质参数
          Light::Update   ──┼─▶ SceneProxy（局部 static 单例）[已实现]
          Object::Update  ──┘   
          变更记录区：UpdateMesh/UpdateMaterial（脏标记检测，变化才写，带 object id）
          删除通道：Scene::DestroyObject → 标记 → Scene::Update 清扫时 DeleteObject(id)

render 层  RenderScene.Update() ◀─ BuildSceneProxy(aspect) 将原始输入转换为 GPU 数据
           （输出区：ObjectData{id, PerObject} 等；转换后清 scene 输入区）
           消费顺序：删除 → mesh/material 记录（find-or-add）→ UBO 写入
           Reset() 清输出区与记录
           对账：RenderScene 内部 unordered_map<ResourceId, size_t> 定位，
                 外部只暴露 vector<RenderObject>
           LitPass：每帧收集 RenderObject → 按材质/网格两级排序 → 换绑跳过 → 绘制

resource 层 ResourceManager（整型键）──懒创建 + L0/L1 缓存 + 引用计数 + 延迟销毁
           ▲ 统计查询（UI 经句柄）
```

依赖方向（显式约束）：`core ← rhi ← resource ← render`；`scene → render` **仅通过 SceneProxy 接口头**；render 层不 include 任何 scene 层类型。

### 四、设计决策

1. **SceneProxy 纯数据 + 描述流**【已实现】：持有 shader 对应结构体（PerFrame / PostProcess / PerObject），写入方法做"场景参数 → 结构体"转换；资产引用以变更记录下发：`UpdateMesh(objectId, meshId)` / `UpdateMaterial(objectId, 全组贴图 id)`（无效 id = 删除），未变更不调用。**多物体**：每物体一个 `ResourceId`，输入为 `vector<ObjectInput>`、输出为 `vector<ObjectData{id, PerObject}>`，记录带 object id（MeshRecord / MaterialRecord 各为 vector）。
2. **RenderScene 无 GlobalState**【已实现】：RenderScene 直接持有具体状态/纹理集合/对象，例如 `FrameState`、`TargetTextures`、`ShadowTextures`、`LitState`、`PostProcessState`、`MaterialState`、`ObjectState`。pass 按需获取对应的 state / textures。
3. **RenderScene 对账**【已实现】：外部只暴露 `std::vector<RenderObject>`；内部用 `std::unordered_map<ResourceId, size_t>` 定位。`RenderObject` = `id + MeshResource::Handle + MaterialState::Handle + ObjectState`。mesh/material 记录到达时 find-or-add 再应用；`GetDeletedObjects()` 通道驱动 remove。
4. **per-object 数据**【已实现】：每个场景对象的动态 set 为 `ObjectState` = `kMaxFramesInFlight` 个 UBO + K 个描述集，内容为 `mat4 model` + 材质参数（albedo / metallic-roughness-ao / emissive + emissive intensity）。
5. **材质内容寻址**【已实现】：GPU 材质 = 纯贴图集 + descriptor set（不含参数），以 `MaterialDesc`（全组贴图 id）哈希去重；`MaterialState` 在 RenderScene 的 `CacheTable` 中共享。
6. **DescriptorManager**【已实现】：集中管理全部 descriptor layout 与 pool；layout 按内容去重、pool 按 layout 分组持有、池满自动扩容；layout 为**唯一真源**（Set 内不再存 m_layout），经 `ResourceManager::GetDescriptorSetLayout(Type)` 查询，pass 的 CreatePipeline 收 `std::vector<VkDescriptorSetLayout>`；**回收**：DescriptorSetRhi → graveyard 延迟 K 帧 → `DescriptorManager::Recycle` → 池内 free list（reuse-only，不调用 vkFreeDescriptorSets）。
7. **RenderGraph**【未做】：现有 RenderPipeline（pass 编排器）可改名，职责不变。
8. **scene 不用组件**【已实现】：Camera/Light/Object/Skybox/PostProcess 各自 `Update()` 自写数据到 SceneProxy；Camera 为纯视图状态（position + yaw/pitch，轴/矩阵按需派生），orbit 逻辑在 application 层 OrbitCameraController；Object 持 id + deletePending（标记清扫）+ 脏标记（mesh 变更 + 材质贴图变更）。
9. **SceneProxy 为局部 static 单例**【已实现】：`SceneProxy::Get()`；消费顺序 scene 写入 → RenderScene（Build + 消费 + Reset）；未来多场景/多线程再改显式参数/依赖注入。

### 五、资源与状态划分

Resource 层只保留 L0/L1 资源；与 shader Set 对应的宿主容器不再属于 Resource 层，而是 render 层的状态。命名规则：

- descriptor set 状态：`XxxState`
- 纯纹理/attachment 集合：`XxxTextures`
- 场景对象组合：`RenderObject`

共享规则：

1. 低等级资源统一用句柄（生命周期统一走表 + 延迟销毁）；
2. 被共享的用句柄；不共享的用值；
3. **每层封装**：Resource 与 render state 都不是可穿透的纯容器；外部只使用语义方法（如 `UboResource::Write/GetBuffer`、`FrameState::GetSet`、`RenderObject::GetMaterialSet`），调用方不得穿透到内部成员/句柄链（禁止 `a.xx.yy` 式访问）。state 的字段是 RenderScene 等 owner 的装配细节，不是对外容器。

#### Resource 层资源表

| 资源名           | 简要描述                         | 等级 | 是否共享                | 值或句柄 |
| ---------------- | -------------------------------- | ---- | ----------------------- | -------- |
| BufferRhi        | 缓冲 + 内存                      | L0   | 否（匿名）              | 句柄     |
| ImageRhi         | 图像 + 内存（TextureAsset 内容） | L0   | 是（asset 寻址）        | 句柄     |
| ImageViewRhi     | 图像视图                         | L0   | 否（装配件）            | 句柄     |
| SamplerRhi       | 采样器                           | L0   | 是（desc 去重）         | 句柄     |
| DescriptorSetRhi | 描述集 + 所属 layout 类型        | L0   | 否（随句柄生命周期）    | 句柄     |
| MeshResource     | 顶点/索引缓冲组合                | L1   | 是（asset 寻址）        | 句柄     |
| UboResource      | 单一缓冲的 UBO 槽                | L1   | 否                      | 值       |
| TextureResource  | 图像 + 视图 + 采样器组合         | L1   | 否（装配）              | 值       |

#### Render 层状态/纹理/对象

| 名称             | 简要描述                                             | 共享方式           |
| ---------------- | ---------------------------------------------------- | ------------------ |
| FrameState       | 帧 set（UBO + env 图 + K 个描述集）                  | RenderScene 持有    |
| PostProcessState | 后处理 set（UBO + K 个描述集）                       | RenderScene 持有    |
| LitState         | Lit set（描述集，引用 ShadowTextures）               | RenderScene 持有    |
| MaterialState    | 材质 set（贴图组合 + 描述集）                        | CacheTable 共享     |
| ObjectState      | 对象 set（K×UBO + K 描述集）                         | RenderObject 持有   |
| TargetTextures   | HDR target：color + resolve + depth                  | RenderScene 持有    |
| ShadowTextures   | Shadow map texture                                   | RenderScene 持有    |
| RenderObject     | 场景对象组合：id + mesh + MaterialState::Handle + ObjectState | vector 持有 |
