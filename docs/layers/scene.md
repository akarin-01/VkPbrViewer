# scene 层

> scene 层是场景实体：Camera / Light / Skybox / PostProcess / Object（含 Material），纯业务数据、零 Vulkan 类型。实体不订阅不注册，各自 `Update()` 把状态写进 SceneProxy（本层 include 的唯一 render 头就是 `render/scene_proxy.h`）——这是 scene → render 的唯一通道。本层不决定"怎么画"，只决定"场景现在是什么"。

| 文件 | 模块 | 一句话 |
| --- | --- | --- |
| scene.h / .cpp | Scene | 物体池 + 全局实体持有 + 每帧更新与清扫 |
| object.h / .cpp | Object | 场景物体：id、transform、材质、网格 + 脏标记 |
| material.h / .cpp | Material | 材质标量参数 + 五个贴图槽 + 贴图脏标记 |
| camera.h / .cpp | Camera | 纯视图状态：位置 + yaw/pitch，轴按需派生 |
| light.h / .cpp | Light | 平行光参数（位置即方向，指向原点） |
| skybox.h / .cpp | Skybox | 环境贴图 + 变更记录 |
| post_process.h / .cpp | PostProcess | EV 曝光值 |
| utils.h / .cpp | Utils::SpawnModel | 统一的模型导入装配入口 |

## Scene：物体池与每帧驱动（scene.h / scene.cpp）

`Scene::Update()` 每帧做两件事：

1. 四个全局实体（Camera / Light / Skybox / PostProcess）依次 `Update()` 写 SceneProxy；
2. 物体池清扫：先处理 `deletePending` 的物体（`proxy.DeleteObject(id)` 下发删除 + 从池中 erase），其余正常 `Update()`。

物体 id 由 Scene 递增分配。删除采用**标记 + 清扫**而非立即擦除：`DestroyObject` 只置标记，真正的删除发生在下一帧 Update 的遍历中——删除通道与渲染侧的对账节奏保持一致（详见 [../data-flow.md](../data-flow.md)）。这里的 erase 是保序删除（物体会被 UI 遍历展示），与渲染侧 IdVector 的 swap-and-pop 各行其道。

## Object：id + 脏标记（object.h / object.cpp）

> **亮点：两种写入节奏分治"每帧变的"和"偶尔变的"。** `Object::Update()` 里，**逐帧数据**（transform + 材质标量参数）每帧全量写 SceneProxy——成本只是几个字节的赋值；**变更记录**（mesh、贴图组合）只在变化发生的那一帧写一次，靠两个脏标记驱动：`m_meshDirty`（SetMesh 时置位）与 `Material::ConsumeTexturesDirty()`（读后即清）。

`SetMesh` 以 Handle 移动接管网格引用，同 id 重复设置直接短路。Mesh 记录用**无效 id 表示移除**：`UpdateMesh(id, kInvalidId)` 是合法语义。

## Material：参数与贴图槽（material.h / material.cpp）

标量参数（albedo 因子、metallic、roughness、AO、emissive + 强度）与五个贴图槽位的 View Handle 分开存：标量走每帧全量通道（改滑块零成本生效），贴图走脏标记变更通道（触发渲染侧 MaterialState 的内容寻址去重重建）。`SetTexture(slot, handle)` 按 `MaterialSlot` 枚举统一入口。贴图脏标记初值为 true——物体诞生的第一帧就会下发一次完整贴图组合。

## Camera：纯视图状态（camera.h / camera.cpp）

> **亮点：Camera 不存任何矩阵，只有位置 + yaw/pitch + 投影参数。** front/right/up 轴由 yaw/pitch 按需派生（yaw = pitch = 0 朝 -Z），view/proj 矩阵在 SceneProxy 的 Build 阶段合成。存"最小状态"而非派生结果，天然杜绝了矩阵与角度不同步一类的一致性 bug。

边界处理都在 setter 内收敛：pitch 钳制 ±89°（避免万向锁奇异区）、fov 钳制 30–90°、yaw 经 `WrapDegrees` 归一化。`MoveLocal` 沿派生轴移动（FPS 控制器的基础）。`Update()` 是 const 的——写的是 SceneProxy 单例，相机自身状态不变。

## 全局实体：Light / Skybox / PostProcess

- **Light**：position + color + intensity 三项参数。注意**位置即方向**——光源是从该位置指向原点的平行光，方向在 SceneProxy Build 时换算。
- **Skybox**：持环境贴图（equirect HDR）的 View Handle，脏标记初值 true——首帧必下发一次 EnvironmentRecord，渲染侧据此完成 IBL 烘焙与 FrameState 装配（见 [../data-flow.md](../data-flow.md)）。
- **PostProcess**：只有一个 EV 值，`Update()` 每帧写（曝光是连续量，走全量通道）。

## Utils::SpawnModel：统一导入入口（utils.h / utils.cpp)

模型导入只有一条路：`SpawnModel(scene, assets, path)`——`LoadModel` 后逐 part `CreateObject` + SetName/SetMesh/SetTransform/SetParams，有效贴图逐槽位设置。程序初始化的默认场景与 UI 的导入按钮共用它，行为完全一致。

失败边界清晰：加载失败（文件不存在、解析错误）**抛异常**——app 初始化期直接上抛终止，UI 回调边界 try/catch 捕获后 Log::Error 并保持原资源不变（错误处理约定的"外部输入失败"类，见 [../dev-history/architecture.md](../dev-history/architecture.md)）。单个贴图缺失不抛——View 无效就不设置该槽位，交给 GPU 侧的 fallback 降级。
