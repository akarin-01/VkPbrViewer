# resource 层

> resource 层管理两类资源的所有权、缓存与生命周期：**CPU 资产**（glTF 模型、贴图的解析结果）归 AssetManager，**GPU 资源**（buffer / image / sampler / descriptor set 及其组合）归 ResourceManager。它不了解场景结构——只认 id 和 key；也直接使用 Vulkan 类型（见 [rhi 层](rhi.md) 的设计立场）。本层是全工程基础设施最密集的一层：句柄系统、内容寻址缓存、延迟销毁队列都定义在这里，被 render 层复用。

| 文件 | 模块 | 一句话 |
| --- | --- | --- |
| handle.h | RefTable / Handle / HandleTable | RAII 引用计数句柄系统（三件套） |
| id_vector.h | IdVector | 稠密 id 向量：FindOrAdd + O(1) 移除 |
| cache_table.h | CacheTable | 内容寻址去重缓存（覆盖在 HandleTable 上） |
| graveyard.h / .cpp | Graveyard | 延迟销毁队列：K 帧后按依赖序处置 |
| resource_id.h | ResourceId | 资源 id（uint64，0 为无效值） |
| asset_manager.h / .cpp | AssetManager | CPU 资产：解析、缓存、View 铸造 |
| asset_types.h | ModelAsset / MeshView / TextureView… | 资产数据结构与视图 |
| asset_utils.h / .cpp | 解析实现 | tinygltf 解析、accessor 读取、贴图解码 |
| resource_manager.h / .cpp | ResourceManager | GPU 资源：懒创建、句柄表、销毁者注入 |
| resource_types.h | BufferRhi / MeshResource / TextureResource… | L0/L1 资源结构与 Desc |
| descriptor_manager.h / .cpp | DescriptorManager | descriptor layout 唯一真源 + 池管理 |
| descriptor_writer.h / .cpp | DescriptorWriter | 单个 set 的批量写入器 |
| environment_baker.h / .cpp | EnvironmentBaker | IBL 四步烘焙（见 data-flow） |
| compute_conversion.h / .cpp | ComputeConversion | compute dispatch 的封装 |
| gpu_layouts.h | Gpu::PerFrame / PerObject… | shader UBO/推常量的 CPU 镜像 + 静态断言 |
| resource_utils.h / .cpp | ResourceUtils | buffer/image 创建、mip 生成等底层杂项 |
| constants.h / vertex.h | 常量与顶点布局 | IBL 尺寸、材质槽位、顶点属性 |

## 基础设施：句柄系统（handle.h）

本层的设计核心，三个类各司一职：

- **RefTable**（叶）：`unordered_map<ResourceId, Entry>`，Entry = 资源 + 引用计数。`Add` 插入**引用计数为 0 的"无主条目"**，由句柄机器"收养"——`Handle` 从表里诞生时构造函数统一 +1，规则只有一个入口。
- **Handle\<T\>**（RAII 句柄）：完整五法则——拷贝 +1、析构 -1、移动**转移**所有权（计数不变）。`Get()` 返回的裸指针生命周期与句柄绑定；`GetId()` 用于跨层传递（SceneProxy 变更记录里传的就是 id）。
- **HandleTable**（门面）：铸造句柄的唯一表面——`Create`（插入无主条目并收养）与 `GetShared`（对既有条目再借一份，条目不存在则抛异常）。

> **亮点：句柄不变量。** 对一切 `Handle<T>` 成立：valid ⇒ entry 存活且数据有效。"数据有效"由铸造路径保证（非法数据在创建时就被拒绝，例如空模型抛异常），消费方只查 `IsValid()` 即可，不需要二次校验。这让"资源还活着吗"从一个运行时风险变成一个类型系统问题。

引用归零时 RefTable 调用注入的 **destroyer**——这是资源层生命周期的关键接缝，见下文 ResourceManager。

## 基础设施：IdVector（id_vector.h）

稠密 vector + `unordered_map<id, 下标>` 索引的混合结构：迭代走 vector（缓存友好），定位走哈希。

- `FindOrAdd(id, factory)`：不存在则用工厂构造——RenderScene 的"按 object id 对账"（find-or-create）就是它；
- `Remove`：swap-and-pop，O(1) 移除（尾部元素换到空洞），不保序——消费方只按 id 访问，无影响。

## 基础设施：CacheTable（cache_table.h）

内容寻址去重缓存 = `key → id` 映射 + HandleTable。`GetOrCreate(key, create)` 的三个细节都值得说：

> **亮点：失败与陈旧的防御性语义。** ① **nullopt 永不缓存**——creator 返回 `nullopt`（如贴图 id 查不到）直接返回无效句柄，下次同 key 仍会重试创建，坏请求不会污染缓存；② **stale id 自愈**——key→id 映射独立于条目存活，条目被释放后 key 还在映射里，下次命中时检测到条目已不存在，自动落到重建路径；③ 命中即 `GetShared`（+1 引用），未命中才执行 creator（约定为纯函数）。

SamplerDesc / MaterialDesc 这类"按内容去重"的 key 都要求自带 `operator==` 与 `Hash`（如 SamplerDesc 用 FNV-1a 逐字段混合）。

## 基础设施：Graveyard（graveyard.h / graveyard.cpp）

延迟销毁队列：六类裸 Vk 句柄各一条队列（memory / image / buffer / sampler / imageView / descriptorSet），入队时 lifetime = `kMaxFramesInFlight`(2)，每帧 `Flush()`（waitFence 之后）整体减一，归零处置。

> **亮点：销毁顺序即依赖序。** Flush 显式按 descriptorSet → imageView → sampler → buffer → image → memory 的顺序进行——视图先于其图像、内存最后释放（vkFreeMemory 要求没有存活的绑定资源）。descriptor set 不是销毁而是 `DescriptorManager::Recycle` 回到池的 free list 复用（reuse-only，从不调 vkFreeDescriptorSets）。为什么 K=2：死亡帧的提交完成由"第 K 次 flush 观察到的 fence"证明。

## AssetManager：CPU 资产（asset_manager.h / asset_types.h / asset_utils.cpp）

**Asset / View 两层访问**：Asset（ModelAsset / TextureAsset）是内部实现，对外只暴露 View（MeshView / TextureView）与 key。

- **缓存粒度**：`ModelAsset` 按**文件路径**缓存（一次 tinygltf 解析，多 part 复用）；`MeshView` 按 `MeshKey{路径, part 序号}`；`TextureAsset`/`TextureView` 按 `TextureKey{路径, 类型}`。**同一文件可以按不同语义加载**——同一张 jpg 既可作 sRGB albedo 也可作 linear 数据，类型进 key，互不串缓存。
- **View 的构成**：`MeshView` = ModelAsset::Handle + partIndex，暴露语义方法（顶点/索引数据、名称）；`TextureView` = TextureAsset::Handle，暴露像素字节与尺寸。两者构造函数私有、friend AssetManager——铸造权单点收口。**View 有效 ⇒ 其 Asset Handle 有效**：资产生命周期唯一归属 scene（Object/Material 持 View 的 Handle），引用归零即从表擦除。
- **借用规则**：resource 侧需要像素/顶点数据时，经 `GetMesh(id)` / `GetTexture(id)` 拿**裸指针借用**，单次调用内用完即弃——ResourceManager 绝不持有 Asset 的 Handle，两套生命周期互不纠缠。stale id 查得 nullptr → 降级路径（见下）。
- 解析实现（asset_utils.cpp）处理 glTF 规范细节：accessor 按规范读取（归一化、字节对齐）、纹理 URI 必须外置（内嵌 data URI 记警告跳过）、每个 part 的材质参数与贴图 key 提取。

## ResourceManager：GPU 资源（resource_manager.h / resource_manager.cpp）

L0（Rhi 句柄表）+ L1（渲染资源组合）的懒创建工厂 + 生命周期中枢。

- **L0 表**：buffer 表、descriptorSet 表、imageCache（**按 ResourceId 内容寻址**——同一张贴图全工程只上传一次）、imageView 表、samplerCache（按 SamplerDesc 去重）。L1：meshCache（按 ResourceId）。
- **销毁者注入**（设计重点）：每张表在构造时注入 destroyer lambda，逻辑归零时被 RefTable 调用——它们的职责就是把资源**拆成裸 Vk 句柄推进 Graveyard**（如 buffer：unmap → PushBuffer → PushMemory）。
- **组合资源靠 RAII 级联自拆**：`MeshResource` 只持两个 `BufferRhi::Handle`，它的 destroyer 只打日志——句柄成员析构时自动级联进 buffer 表的 destroyer 进 Graveyard。`ImageViewRhi` 内嵌 `ImageRhi::Handle`（视图持其图像的引用），结构上不可能出现视图比图像活得久。拆解是组合的、销毁时机是延迟的，两件事正交。
- **懒创建链**（以贴图为例）：`CreateTexture(textureId, viewDesc, samplerDesc)` → `GetOrCreateImage`（miss 时**借用** AssetManager 的 TextureView，经 staging 一次性上传）→ `CreateImageView`（**视图持有图像句柄**）→ `GetOrCreateSampler`（desc 去重）。任何一步无效（stale id / 资产已释放）→ 返回空 TextureResource，由上层装配 fallback——**失败沿调用链逐级降级，不抛异常不崩溃**。
- **成员声明序即生命周期依赖**：descriptorManager 先于 graveyard（其 flush 依赖回收）、graveyard 先于各表（表的 destroyer 推 graveyard）、fallback 资源最后创建最先释放——头文件注释里写明了这条约束。

## DescriptorManager：layout 唯一真源（descriptor_manager.h / descriptor_manager.cpp）

8 种 layout 类型（Empty / PerFrame / PerMaterial / PerObject / PostProcess / Lit / ComputeWrite / ComputeSample）各配一个 `DescriptorSetPool`，Manager 只做 Type → Pool 的路由。

- **binding 号 = BindingDesc 在数组中的位置**：C++ 侧单点定义 layout（与 `shader_sets.glsl` 的 set/binding 约定对应），pass 建管线时经 `GetLayout(type)` 查询，不自行描述。
- **池满自动扩容**：容量从 8 起，`used == capacity` 时**翻倍并追加一个新池**——旧池不销毁（里面的 set 还被引用着），扩容零迁移。
- **回收复用**：`Allocate` 先查 free list，命中即复用（配合 Graveyard 的 K 帧延迟，保证复用的 set 上一份内容已不被 GPU 读取）；`Recycle` 只进 free list，从不 `vkFreeDescriptorSets`。
- `Empty` 类型是个小技巧：无 binding 的 layout，供 ShadowPass 占位 set 1/2，与 LitPass 的 set 编号对齐（见 [layers/render.md](render.md)）。

## DescriptorWriter（descriptor_writer.h / descriptor_writer.cpp)

单个 set 的批量写入器：`WriteBuffer` / `WriteImage` 链式累积（按 binding 聚合，支持数组型 binding），`UpdateSet` 一次性 `vkUpdateDescriptorSets`。只收裸 Vk 句柄——"资源已由调用方解析"，写入器不管生命周期。

## GPU 布局镜像（gpu_layouts.h）

`Gpu::PerFrame`（240B）/ `PerObject`（176B）/ `PostProcess`（16B）是 shader UBO 块的 std140 CPU 镜像，compute 推常量（IrradiancePC / PrefilterPC）是 std430 镜像。

> **亮点：编译期锁死 CPU/GPU 布局一致。** `STD140_ASSERT(T, SIZE)` / `STD430_ASSERT(T, SIZE)` 用 `static_assert(sizeof(T) == SIZE)` 把每个镜像结构体的大小钉死在预期值上——GLSL 侧改了结构（加字段、改对齐），C++ 侧编译立刻报错，而不是运行时拿到错位的 uniform 数据。CPU/GPU 跨语言一致性问题被压缩成一行断言。

## EnvironmentBaker / ComputeConversion（environment_baker / compute_conversion）

IBL 四步烘焙（equirect→cubemap、irradiance、prefilter、BRDF LUT）的执行者，全部走 ComputeConversion 的**同步 dispatch**（内部 OneShotCommand，每步自带到目标 layout 的转换）。尺寸常量与各步方法详见 [../data-flow.md](../data-flow.md) 的 IBL 烘焙链路一节。
