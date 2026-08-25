# 08. 架构调整

> **阶段目标**：参考成熟引擎（Unreal FPrimitiveSceneProxy、Unity SRP per-object CBUFFER、Filament Engine/资源句柄模型）调整架构与数据流，为多物体（多个 Mesh/Material 实例）支持提供基础。本次只做架构与数据流，不做性能优化（不引入 ECS、GPU 剔除、多线程录制等）。

### 一、分层设计

| 层级 | 职责 |
| --- | --- |
| core | 工具层：Window/Input/Time/Log（现有代码迁入） |
| rhi | Vulkan 封装层：RenderContext/SwapChain/FrameSync、Graphics/ComputePipeline、RenderingScope/OneShotCommand/ShaderModule（现有代码迁入） |
| resource | 资源层：AssetManager（CPU 资产缓存）、ResourceManager（GPU 资源缓存）、DescriptorManager（layout/pool 统一管理）、句柄系统（通用抽象，见下） |
| render | 渲染层：SceneProxy（纯数据容器 + 写入方法）、RenderScene（消费描述、对账、上传）、RenderGraph（RenderPipeline 改名） |
| scene | 场景层：Camera/Light/Object 自写 `Update(SceneProxy&)`，不引入组件机制 |
| application | 应用层：程序开始/关闭/渲染循环（现有 application.cpp）；UI 由本层持有 |

### 二、句柄系统（AssetManager / ResourceManager 共用）

- 句柄是 **RAII 类而非裸整型**：`Handle<T>` 内部 = 条目索引 + 指向管理器的指针；拷贝/赋值时引用计数 +1，析构时 -1（移动转移所有权，不增减计数），计数归零由管理器执行释放
- AssetManager：`map<path, 资产 + 计数>`；`Load(path)` 返回 RAII 句柄，同 path 复用条目；归零移除并释放 CPU 资产
- ResourceManager：`map<uint32_t, GPU 资源 + 计数>`；键为整型（资产 ID / MaterialDesc 哈希）；值为 pass 渲染所需的东西（buffer/image/descriptor set 等），可直接按整型键取用；计数归零进**延迟销毁队列**，帧间 fence 之后销毁
- 两管理器复用同一句柄模板——句柄类自身完成引用计数控制（RAII），管理器只提供条目表与释放回调

### 三、数据流

```
scene 层   Camera::Update ──┐   每帧数据（矩阵/参数）每帧写入；
           Light::Update  ──┼─▶ SceneProxy   资产引用（WriteMesh/WriteTexture，空句柄=删除）
           Object::Update ──┘   只在变更时调用一次
render 层  RenderScene.Update(proxy) ◀─ 消费描述并对账渲染列表；经 ResourceManager 上传 / 取用 GPU 资源
           RenderGraph 的 passes 遍历 draw items，绑定 object set + 材质 set
resource 层 ResourceManager（整型键）──懒创建 + 缓存 + 引用计数 + 延迟销毁
           ▲ 统计查询（UI 经句柄）
```

依赖方向（显式约束）：`core ← rhi ← resource ← render`；`scene → render` **仅通过 SceneProxy 接口头**；render 层不 include 任何 scene 层类型。

### 四、设计决策

1. **SceneProxy 纯数据 + 描述流**：持有 shader 对应结构体（FrameUbo / PerObjectData，MaterialPC 并入后者）；写入方法做"场景参数 → 结构体"转换。资产引用不下传数据：`WriteMesh(objectId, assetHandle)` 等在变更时调用一次，空句柄 = 删除该引用；未变更不调用。
2. **RenderScene 对账**：渲染物体列表由 RenderScene 自己维护；消费 proxy 描述后比对——缺则经 ResourceManager 懒建、有则复用、空句柄或注销则回收引用。
3. **per-object 数据**：每物体一份 ObjectData = `kMaxFramesInFlight` 个 UBO + 每帧一个描述集，内容为 `mat4 model` + 材质参数（albedo / metallic-roughness-ao / emissive）。材质参数属物体实例（对应 Unity MaterialPropertyBlock 的材质实例分离），材质本身共享。
4. **材质内容寻址**：GPU 材质 = 纯贴图集 + descriptor set（不含参数），以 MaterialDesc（贴图资产句柄 + 采样配置）哈希为 key 去重，碰撞时比较完整 desc。
5. **DescriptorManager**：集中管理全部 descriptor set layout 与 pool；layout 按内容去重；pool 按 layout 分组持有、池满自动扩容（无手工预算）。获取入口待设计。
6. **RenderGraph**：现有 RenderPipeline（pass 编排器）改名，职责不变，不引入 barrier 自动化。
7. **scene 不用组件**：Camera/Light/Object 各自 `Update(SceneProxy&)` 自写数据；不引入 GameObject-Component（物体行为同质，组件是纯负担）。未来行为分化时再演进，proxy 接口不变、render 层零改动。