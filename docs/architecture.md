# 架构约定（跨会话唯一真源，改架构前先读）

> 设计过程与细节见 [implementation/08_restructuring.md](implementation/08_restructuring.md)；本节只保留**必须遵守的规则**。

## 分层与依赖

- 层级职责：core（工具：Window/Input/Time/Log/Path/Math）← rhi（Vulkan 封装：Context/SwapChain/Pipeline、RenderingScope/OneShotCommand）← resource（ResourceManager + DescriptorManager 所有权、句柄系统、EnvironmentBaker/ComputeConversion）← render（SceneProxy/RenderScene/RenderPipeline）；scene（场景实体，只经 SceneProxy 与 render 通信；对 resource 只开放其 asset 部分：asset_types / asset_manager / resource_id / constants，不得触碰 GPU 资源侧）、application（渲染循环、UI 面板、OrbitCameraController）。
- 依赖方向（显式约束）：`core ← rhi ← resource ← render`；`scene → render` 仅通过 SceneProxy 接口头；render 层不得 include 任何 scene 层类型。

## 命名约定

- 直接封装 Vulkan 的 L0 资源统一 `Rhi` 后缀：BufferRhi / ImageRhi / ImageViewRhi / SamplerRhi / DescriptorSetRhi；业务与上层资源不加（Scene、ResourceManager、MeshResource、UboResource…）。
- resource 层基础设施无后缀：CacheTable、Graveyard、DescriptorWriter。

## 资源与状态模型（L0/L1 + render state）

- L0 = Vk 句柄封装，统一走句柄 + 延迟销毁。
- L1 = 渲染资源组合（MeshResource / UboResource / TextureResource）。
- render 层不再使用 resource 层的 L2 Set 类型；与 shader descriptor set 对应的宿主容器由 `RenderScene` 维护，命名规则：
  - descriptor set 状态：`XxxState`（`FrameState` / `LitState` / `PostProcessState` / `MaterialState` / `ObjectState`）；
  - 纯纹理/attachment 集合：`XxxTextures`（`TargetTextures` / `ShadowTextures`）。
- 共享资源用 RAII 句柄 `Handle<T>`（条目索引 + RefTable 指针；拷贝/赋值引用 +1、析构 -1、移动转移所有权），归零后拆解为裸 Vk 句柄进 Graveyard，K 帧后销毁；非共享资源用值。句柄不变量（对一切 `Handle<T>` 成立）：valid ⇒ entry 存活且数据有效——数据有效性由铸造方在创建路径保证（非法数据被拒绝，如空模型抛异常），消费方只需检查句柄有效性。
- 每层封装：Resource 与 render state 都不是可穿透的纯容器；外部只使用语义方法（`UboResource::Write/GetBuffer`、`FrameState::GetSet`、`RenderObject::GetMaterialSet`、`TargetTextures::GetColorImageView`）。禁止 `a.xx.yy` 穿透到内部成员/句柄链；state 的字段是 RenderScene 等 owner 的装配细节。

## 所有权与关键机制

- DescriptorManager 归 ResourceManager 所有；ResourceManager 单向依赖 EnvironmentBaker / ComputeConversion。
- descriptor layout 是**唯一真源**：layout 按内容去重、pool 按 layout 分组持有、池满自动扩容；Set 内不存 layout，经 `ResourceManager::GetDescriptorSetLayout(Type)` 查询，pass 建 pipeline 收 `vector<VkDescriptorSetLayout>`。
- DescriptorSetRhi 销毁：句柄归零 → graveyard 延迟 K 帧 → `DescriptorManager::Recycle` → 池内 free list 复用（reuse-only，不调 vkFreeDescriptorSets）。
- GPU 材质 = 纯贴图集 + descriptor set（不含参数），`MaterialDesc`（全组贴图 id）哈希去重，`MaterialState` 按 desc 在 RenderScene 的 `CacheTable` 中共享。
- `TargetTextures` / `ShadowTextures` 是 RenderScene 拥有的纹理所有权；`LitState` / `PostProcessState` 等只引用它们，不复制持有。
- 重建引用同一纹理的 set state 时，应先创建新纹理，再用新纹理重建 set state，最后替换旧纹理，避免 descriptor set 引用已被释放的 image view。
- Asset/View 两层访问：Asset（ModelAsset/TextureAsset/MeshAsset）是 asset 模块内部实现，对外只暴露 View（MeshView/TextureView）与 key；上层经 View 的语义方法访问，不直接解引用 Asset。
- Asset 生命周期唯一归属 scene（Object/Material/Skybox 持 View::Handle）；resource 层对 asset 只做调用内借用（`GetMesh`/`GetTexture` 返回裸指针，单次调用内用完即弃），绝不持有 asset Handle。entry 归零即擦除，stale id 查得 nullptr → 降级 fallback，不会悬空。
- CPU 资产数据（顶点/像素 bytes）随 View 引用常驻内存：UI 直接读 View 展示 asset 信息，GPU 上传不触发二次文件 IO；全部 View 释放后内存随之回收，再次请求从磁盘重新解码。

## 数据流

- scene 实体（Camera/Light/Object/Skybox/PostProcess）各自 `Update()` 自写数据到 SceneProxy（局部 static 单例）；变更走脏标记：`UpdateMesh` / `UpdateMaterial`（无效 id = 删除）；删除走 Scene 的 DestroyObject → 标记 → Update 清扫。
- 模型导入统一走 `Scene::Utils::SpawnModel(scene, assets, path)`（加载 + 逐 part 装配 Object），app 初始化与 UI 导入共用；UI 加载失败（异常或 invalid handle）→ Log::Error 并保持原资源不变。
- RenderScene.Update() ← BuildSceneProxy(aspect)（转换后清 scene 输入区）；消费顺序：删除 → mesh/material 记录（find-or-create）→ UBO 写入；`Reset()` 清输出区。
- 对账：RenderScene 内部用 `unordered_map<ResourceId, size_t>` 定位，对外只暴露 `vector<RenderObject>`；`GetDeletedObjects()` 驱动 remove。
- LitPass：每帧收集 `RenderObject` → 按材质/网格两级排序 → 换绑跳过 → 绘制。
- 阴影投射体为世界坐标系下围绕原点的固定包围盒（SceneProxy 的 `kShadowBounds*`），光空间 ortho 视锥体每帧拟合该包围盒；盒外物体不投影——已知取舍。
- 每个场景对象 = `RenderObject`：`id + MeshResource::Handle + MaterialState::Handle + ObjectState`；`ObjectState` = kMaxFramesInFlight 个 UBO + K 个描述集（`mat4 model` + 材质参数）。
- Camera 为纯视图状态（position + yaw/pitch，轴/矩阵按需派生）；orbit 逻辑在 application 层 OrbitCameraController；Object 持 id + deletePending + 脏标记（mesh / 材质贴图）。

## 错误处理约定

六类场景处置（详表见 [07_polish.md](implementation/07_polish.md) 末尾）：外部输入失败 → 抛异常（消息含路径+原因），UI 回调边界 try/catch → Log::Error 后继续；解析警告/可降级 → Log::Warning + 跳过继续；枚举/传参非法（代码错误）→ 抛异常初始化期暴露；GPU 创建/提交失败（致命）→ 抛异常（含资源名 + VkResult）传播到 main 兜底；内部不变量 → assert 中断；操作取消 → `return nullopt`。

## 日志分级（resource 层）

底层资源创建/销毁 → Debug；高层资源创建/销毁 → Info；复用命中 → Debug（其余层保持同一习惯）。