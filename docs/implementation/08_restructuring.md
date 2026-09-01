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

### 二、句柄系统（AssetManager / ResourceManager 共用）

- 句柄是 **RAII 类而非裸整型**：`Handle<T>` 内部 = 条目索引 + 指向 RefTable 的指针；拷贝/赋值时引用计数 +1，析构时 -1（移动转移所有权，不增减计数），计数归零由管理器执行释放
- AssetManager：`map<path, 资产 + 计数>`；`Load(path)` 返回 RAII 句柄，同 path 复用条目；归零移除并释放 CPU 资产
- ResourceManager：按资源类型分 `HandleTable<T>`（引用计数条目）+ 内容寻址映射（`meshId` / `textureId` / `SamplerDesc` / `MaterialDesc` → 条目 id）；句柄归零后拆解为**裸 Vk 句柄**进 `Graveyard`，K 帧后销毁（详见"资源划分维度"）
- **DescriptorSetRhi**：L0 的资源类型之一（`{ VkDescriptorSet, DescriptorLayoutType }`），随 handle 生命周期走 graveyard，K 帧后经 `DescriptorManager::Recycle` 回收到池的 free list 复用

### 三、数据流

```
scene 层   Camera::Update ──┐   每帧写入区：相机轴/灯光/后处理/物体 transform+材质参数
          Light::Update   ──┼─▶ SceneProxy（局部 static 单例）[已实现]
          Object::Update  ──┘   
          变更记录区：WriteMesh/WriteMaterialTextures（脏标记检测，变化才写，带 object id）
          删除通道：Scene::DestroyObject → 标记 → Scene::Update 清扫时 DeleteObject(id)

render 层  RenderScene.Update() ◀─ BuildSceneProxy(aspect) 将原始输入转换为 GPU 数据
           （输出区：ObjectData{id, PerObject} 等；转换后清 scene 输入区）
           消费顺序：删除 → mesh/material 记录（find-or-create 状态）→ UBO 写入
           Reset() 清输出区与记录
           对账：std::unordered_map<ResourceId, ObjectState>，按 id 键控
           LitPass：每帧收集状态 → 按材质/网格两级排序 → 换绑跳过 → 绘制

resource 层 ResourceManager（整型键）──懒创建 + 缓存 + 引用计数 + 延迟销毁
           ▲ 统计查询（UI 经句柄）
```

依赖方向（显式约束）：`core ← rhi ← resource ← render`；`scene → render` **仅通过 SceneProxy 接口头**；render 层不 include 任何 scene 层类型。

### 四、设计决策

1. **SceneProxy 纯数据 + 描述流**【已实现】：持有 shader 对应结构体（PerFrame / PostProcess / PerObject），写入方法做"场景参数 → 结构体"转换；资产引用以变更记录下发：`WriteMesh(objectId, assetId)` / `WriteMaterialTextures(objectId, 全组 id)`（空 id = 删除），未变更不调用。**多物体**：每物体一个 `ResourceId`，输入为 `vector<ObjectInput>`、输出为 `vector<ObjectData{id, PerObject}>`，记录带 object id（MeshRecord / MaterialRecord 各为 vector）。
2. **RenderScene 对账**【已实现】：`std::unordered_map<ResourceId, ObjectState>` 按 id 键控；mesh/material 记录到达时 find-or-create 状态再应用；`GetDeletedObjects()` 通道驱动 erase；每帧 UBO 写入遍历输出流。
3. **per-object 数据**【已实现】：每物体一份 ObjectData = `kMaxFramesInFlight` 个 UBO + K 个描述集（PerObjectSet），内容为 `mat4 model` + 材质参数（albedo / metallic-roughness-ao / emissive + emissive intensity）。
4. **材质内容寻址**【已实现】：GPU 材质 = 纯贴图集 + descriptor set（不含参数），以 MaterialDesc（全组贴图 id）哈希去重；结果经 PerMaterialSet Handle 共享。
5. **DescriptorManager**【已实现】：集中管理全部 descriptor layout 与 pool；layout 按内容去重、pool 按 layout 分组持有、池满自动扩容；layout 为**唯一真源**（Set 内不再存 m_layout），经 `ResourceManager::GetDescriptorSetLayout(Type)` 查询，pass 的 CreatePipeline 收 `std::vector<VkDescriptorSetLayout>`；**回收**：DescriptorSetRhi → graveyard 延迟 K 帧 → `DescriptorManager::Recycle` → 池内 free list（reuse-only，不调用 vkFreeDescriptorSets）。
6. **RenderGraph**【未做】：现有 RenderPipeline（pass 编排器）可改名，职责不变。
7. **scene 不用组件**【已实现】：Camera/Light/Object/Skybox/PostProcess 各自 `Update()` 自写数据到 SceneProxy；Camera 为纯视图状态（position + yaw/pitch，轴/矩阵按需派生），orbit 逻辑在 application 层 OrbitCameraController；Object 持 id + active（停用保留容器）+ deletePending（标记清扫）+ 脏标记。
8. **SceneProxy 为局部 static 单例**【已实现】：`SceneProxy::Get()`；消费顺序 scene 写入 → RenderScene（Build + 消费 + Reset）；未来多场景/多线程再改显式参数/依赖注入。

### 五、资源划分维度

资源按两个正交维度划分：**等级**（L0 Vk 句柄 → L1 渲染资源 → L2 Set）与**是否共享**。规则：

1. 低等级统一用句柄（生命周期统一走表 + 延迟销毁）；
2. 被共享的用句柄；不共享的用值；
3. **每层封装**：资源只暴露语义方法（如 `UboResource::Write/GetBuffer`、`PerObjectSet::WriteData/GetSet`），调用方不得穿透句柄链（禁止 `a.xx.yy` 式访问；render state 作为纯容器除外）。

L2 的 `PerXxxSet` 与 shader 侧的 descriptor set 一一对应（`shader_sets.glsl` 的 `SET_PER_FRAME / SET_PER_PASS / SET_PER_MATERIAL / SET_PER_OBJECT`）——它们是 GPU 描述符集的宿主容器，等级划分的"使用方"即指 shader 侧。

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
| PerMaterialSet   | 材质 Set（贴图组合 + 描述集）    | L2   | 是（MaterialDesc 寻址） | 句柄     |
| PerObjectSet     | 对象 Set（K×UBO + K 描述集）     | L2   | 否                      | 值       |
| PerFrameSet      | 帧 Set（UBO + env 图 + 描述集）  | L2   | 否                      | 值       |
| PerPassSet       | Pass Set（UBO + 屏幕纹理）       | L2   | 否                      | 值       |
