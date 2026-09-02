# 架构约定（跨会话唯一真源，改架构前先读）

> 设计过程与细节见 [implementation/08_restructuring.md](implementation/08_restructuring.md)；本节只保留**必须遵守的规则**。

## 分层与依赖

- 层级职责：core（工具：Window/Input/Time/Log/Path/Math）← rhi（Vulkan 封装：Context/SwapChain/Pipeline、RenderingScope/OneShotCommand）← resource（ResourceManager + DescriptorManager 所有权、句柄系统、EnvironmentBaker/ComputeConversion）← render（SceneProxy/RenderScene/RenderPipeline）；scene（场景实体，只经 SceneProxy 与 render 通信）、application（渲染循环、UI 面板、OrbitCameraController）。
- 依赖方向（显式约束）：`core ← rhi ← resource ← render`；`scene → render` 仅通过 SceneProxy 接口头；render 层不得 include 任何 scene 层类型。

## 命名约定

- 直接封装 Vulkan 的 L0 资源统一 `Rhi` 后缀：BufferRhi / ImageRhi / ImageViewRhi / SamplerRhi / DescriptorSetRhi；业务与上层资源不加（Scene、ResourceManager、MeshResource、UboResource…）。
- resource 层基础设施无后缀：CacheTable、Graveyard、DescriptorWriter。

## 资源模型（等级 L0/L1/L2 × 共享/非共享）

- L0 = Vk 句柄封装，统一走句柄 + 延迟销毁；L1 = 渲染资源组合（MeshResource/UboResource/TextureResource）；L2 = 与 shader Set 一一对应的宿主容器（PerFrameSet/PerPassSet/PerMaterialSet/PerObjectSet ↔ `shader_sets.glsl` 的 SET_PER_*）。
- 共享资源用 RAII 句柄 `Handle<T>`（条目索引 + RefTable 指针；拷贝/赋值引用 +1、析构 -1、移动转移所有权），归零后拆解为裸 Vk 句柄进 Graveyard，K 帧后销毁；非共享资源用值。
- 每层封装：资源只暴露语义方法（`UboResource::Write/GetBuffer`、`PerObjectSet::WriteData/GetSet`），禁止 `a.xx.yy` 穿透句柄链访问（render state 纯容器除外）。

## 所有权与关键机制

- DescriptorManager 归 ResourceManager 所有；ResourceManager 单向依赖 EnvironmentBaker / ComputeConversion。
- descriptor layout 是**唯一真源**：layout 按内容去重、pool 按 layout 分组持有、池满自动扩容；Set 内不存 layout，经 `ResourceManager::GetDescriptorSetLayout(Type)` 查询，pass 建 pipeline 收 `vector<VkDescriptorSetLayout>`。
- DescriptorSetRhi 销毁：句柄归零 → graveyard 延迟 K 帧 → `DescriptorManager::Recycle` → 池内 free list 复用（reuse-only，不调 vkFreeDescriptorSets）。
- GPU 材质 = 纯贴图集 + descriptor set（不含参数），MaterialDesc（全组贴图 id）哈希去重，PerMaterialSet 按 desc 共享。

## 数据流

- scene 实体（Camera/Light/Object/Skybox/PostProcess）各自 `Update()` 自写数据到 SceneProxy（局部 static 单例）；变更走脏标记：`WriteMesh` / `WriteMaterialTextures`（空 id = 删除）；删除走 Scen 的 DestroyObject → 标记 → Update 清扫。
- RenderScene.Update() ← BuildSceneProxy(aspect)（转换后清 scene 输入区）；消费顺序：删除 → mesh/material 记录（find-or-create）→ UBO 写入；`Reset()` 清输出区。
- 对账：`std::unordered_map<ResourceId, ObjectState>` 按 id 键控；`GetDeletedObjects()` 驱动 erase。
- LitPass：每帧收集状态 → 按材质/网格两级排序 → 换绑跳过 → 绘制。
- 每物体 ObjectData = kMaxFramesInFlight 个 UBO + K 个描述集（PerObjectSet）：`mat4 model` + 材质参数（albedo / metallic-roughness-ao / emissive+intensity）。
- Camera 为纯视图状态（position + yaw/pitch，轴/矩阵按需派生）；orbit 逻辑在 application 层 OrbitCameraController；Object 持 id + active + deletePending + 脏标记。

## 错误处理约定

六类场景处置（详表见 [07_polish.md](implementation/07_polish.md) 末尾）：外部输入失败 → 抛异常（消息含路径+原因），UI 回调边界 try/catch → Log::Error 后继续；解析警告/可降级 → Log::Warning + 跳过继续；枚举/传参非法（代码错误）→ 抛异常初始化期暴露；GPU 创建/提交失败（致命）→ 抛异常（含资源名 + VkResult）传播到 main 兜底；内部不变量 → assert 中断；操作取消 → `return nullopt`。

## 日志分级（resource 层）

底层资源创建/销毁 → Debug；高层资源创建/销毁 → Info；复用命中 → Debug（其余层保持同一习惯）。