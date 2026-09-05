# rhi 层

> 本层没有做常规 RHI 的 API 隔离——本项目是 Vulkan 专用渲染器，上层（resource / render）直接使用 Vulkan 类型。层内内容就是两类：**样板代码封装**（Context / FrameSync / SwapChain）与**工具类**（RAII 封装或工具函数）。

| 文件 | 模块 | 一句话 |
| --- | --- | --- |
| context.h / .cpp | Context | Vulkan 全局状态持有者 + 物理设备能力缓存 |
| swap_chain.h / .cpp | SwapChain | 交换链及其图像视图、完整重建流程 |
| frame_sync.h / .cpp | FrameSync | frames in flight 的三组同步对象与帧边界 |
| graphics_pipeline.h / .cpp | GraphicsPipeline | Config + Builder + RAII 句柄 |
| compute_pipeline.h / .cpp | ComputePipeline | 同上，单 compute stage |
| rendering_scope.h / .cpp | RenderingScope | `vkCmdBeginRendering` / `End` 的 RAII |
| one_shot_command.h / .cpp | OneShotCommand | 一次性提交：构造即开始录制，析构即提交等待 |
| shader_module.h / .cpp | ShaderModule | SPIR-V 文件加载 |
| utils.h / .cpp | 自由函数 | 队列族/交换链查询、两种 image barrier 原语 |

## 样板代码封装

### Context（context.h / context.cpp）

Vulkan 全局状态的单一持有者，构造序列即 Vulkan 初始化序列：instance（Debug 构建附带 debug messenger）→ surface（经 core 的 Window）→ 物理设备选择 → 逻辑设备与队列 → command pool → **能力缓存**。

- **能力缓存**是 Context 的实用价值所在：`DepthFormat / HdrFormat / ShadowMapFormat`（带格式的支持性检测）、`MaxAnisotropy`、支持的/最大的/实际选定的 MSAA 采样数。建管线、建离屏目标时直接查询，不必各处重复枚举与挑选。
- **头文件直接暴露 Vk 句柄 getter**（`Instance()` / `Device()` / `GraphicsQueue()`…）——这就是"上层直接用 Vulkan 类型"在最基础模块上的体现：Context 管的是"创建出来、统一持有"，不是"藏起来"。

### SwapChain（swap_chain.h / swap_chain.cpp）

交换链封装，包含它最容易出错的部分——重建。

- **图像视图自持**：交换链图像由驱动拥有（`vkGetSwapchainImagesKHR` 只是借用），因此它们的视图不经 ResourceManager，在类内创建与销毁，重建时一起走。
- **选择策略**：格式优先 `B8G8R8A8_SRGB` + sRGB 非线性色彩空间；present mode 优先 MAILBOX（低延迟三缓冲）、FIFO 兜底（保证可用）；图像数取 `minImageCount + 1`；graphics 与 present 队列族不同时用 CONCURRENT 共享模式。
- **重建的三个触发点**统一收口：`AcquireNextImage` 返回 `VK_ERROR_OUT_OF_DATE`、`QueuePresent` 返回 OUT_OF_DATE / SUBOPTIMAL、窗口 resize 标志置位（在 present 时检查）。重建流程：先等窗口尺寸非零（`WaitEvents` 循环，处理窗口最小化），`vkDeviceWaitIdle` 后销毁重建。
- `Acquire / QueuePresent` 返回 bool"是否已重建"，是否走重建路径由上层（renderer）决定——rhi 只负责把交换链修好。

### FrameSync（frame_sync.h / frame_sync.cpp）

frames in flight 的同步对象持有者，三类对象三种索引方式：

- per-frame 的 imageAvailable 信号量 ×2 与 inFlight fence ×2（fence 初始即 signaled，保证首帧不卡死），按 frameIndex 索引；
- per-image 的 present 信号量，数量等于交换链图像数，按 imageIndex 索引，交换链重建时销毁重建；
- `BeginFrame`（等 fence → acquire → 重置 fence 与命令缓冲）与 `EndFrame`（提交 → present → frameIndex 轮转）的完整时序见 [../data-flow.md](../data-flow.md) 的同步模型一节。

## 工具类（RAII 封装 / 工具函数）

### GraphicsPipeline / ComputePipeline（graphics_pipeline / compute_pipeline）

三件套结构：`XxxConfig`（纯数据结构）+ `XxxBuilder`（流式 setter，链式调用）+ `XxxPipeline`（RAII 持有 pipeline 与 layout）。

- **固定状态取合理默认**：triangle list、前向 CCW、FILL 填充、混合关闭（但全通道写掩码）、关闭 depth clamp / stencil；调用方只声明差异项（cull、深度、MSAA、深度偏移）。
- **viewport / scissor 声明为动态态**：管线不绑定尺寸，由 RenderingScope 在每帧 begin 后设置。
- **dynamic rendering 接入点**：附件格式通过 `VkPipelineRenderingCreateInfo`（pNext）声明——没有 RenderPass 也能创建管线，这是本工程"无经典 RenderPass"路线的落点。
- **着色器以 spv 路径传入**：ShaderModule 在管线构造函数内部创建、用完即毁，调用方不接触 VkShaderModule。
- ComputePipeline 是同构的简化版：单 stage、无顶点输入、无动态渲染信息。

### RenderingScope（rendering_scope.h / rendering_scope.cpp）

`vkCmdBeginRendering` ↔ `vkCmdEndRendering` 的 RAII：构造时 begin，析构时 end，作用域即渲染范围，不可拷贝不可移动。

- `RenderingAttachmentDesc` 描述每个附件：view、layout、loadOp（**默认 LOAD**——本工程多数 scope 靠显式 barrier 而非 clear 保证正确性）、storeOp、clearValue，以及可选的 resolve view；
- 给了 resolve view 就自动取 `VK_RESOLVE_MODE_AVERAGE_BIT`——MSAA resolve 由动态渲染的 resolve attachment 完成（lit 与 skybox 各自 resolve 一次）；
- begin 后顺手设置与 extent 匹配的 viewport / scissor（与管线的动态态声明配套）。

### OneShotCommand（one_shot_command.h / one_shot_command.cpp）

一次性命令提交的 RAII：构造 = 分配命令缓冲 + begin（`ONE_TIME_SUBMIT`）；析构 = end → submit（无 fence）→ `vkQueueWaitIdle` → free。

同步语义简单直接：析构返回时，作用域内录制的命令**已经执行完毕**。网格顶点/索引上传、IBL 四步烘焙全部走它。代价是 CPU 阻塞等待——对启动期的一次性操作可接受，换来的是调用方完全不用操心同步。

### utils（utils.h / utils.cpp）

四个自由函数，都是"原语"而非"编排"：

- `FindQueueFamilies` / `QuerySwapChainSupport`：物理设备查询（Context 选设备、SwapChain 建交换链时用）；
- `TransitionImageLayout`：layout 转换的 `vkCmdPipelineBarrier2` 封装；
- `ImageMemoryBarrier`：**同步专用** barrier——不改变 layout，只声明"srcStage/access 的写入对 dstStage/access 可见"。动态渲染 scope 之间没有隐式依赖，lit→skybox、post→ui 的可见性就靠它；
- 后两者共享同一个 `BuildImageBarrier` 核心，stage/access 掩码由调用方给出——rhi 提供原语，屏障编排知识留在 render 层的 pass 编排里。
