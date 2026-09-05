# core 层

> core 是与图形无关的基础设施层：窗口与输入、时间、日志、路径与数学小工具。它不依赖本仓库其他任何层（GLFW 等第三方库除外），被其余所有层依赖。模块刻意保持"薄"——只封装平台与库差异，不做任何渲染或场景语义的假设。

| 文件 | 模块 | 一句话 |
| --- | --- | --- |
| window.h / .cpp | Window | GLFW 窗口生命周期 + Vulkan 互操作，PIMPL 隔离 GLFW |
| input.h / .cpp | Input | 键鼠输入的每帧状态抽象（拉模型 + 边沿检测） |
| time.h | Time | 帧计时：deltaTime 与累计运行时长 |
| log.h / .cpp | Log | 四级日志：变参拼接、级别门控、彩色输出 |
| path.h | Path::Normalize | 路径归一化，保证缓存 key 稳定 |
| math.h | WrapDegrees | 角度归一化到 [-180, 180) |

## Window（window.h / window.cpp）

GLFW 窗口的封装，也是全工程唯一接触 GLFW 的地方。

- **PIMPL 隔离**：`GLFW/glfw3.h` 只出现在 .cpp，头文件对外不泄漏任何 GLFW 类型；构造时 `glfwInit` + 创建窗口，析构对称销毁。窗口创建带 `GLFW_CLIENT_API = GLFW_NO_API`——Vulkan 不需要 GL 上下文。
- **Vulkan 互操作**：`GetRequiredInstanceExtensions()`（rhi 层建 instance 时用）与 `CreateSurface(instance)`（创建 presentation surface），这是窗口与渲染管线仅有的两个交点。
- **事件模型是轮询而非回调驱动**：主循环每帧调 `PollEvents()` + `ShouldClose()`。两个异步事件被"降级"为可轮询的状态：
  - **窗口尺寸变化**：回调只置一个 `m_framebufferResized` 标志，渲染层在方便的时机查询并 `ResetFramebufferResized()`——何时重建交换链由渲染层决定，不在窗口回调里做任何渲染相关的事；
  - **滚轮**：单一回调槽 `SetScrollCallback`，当前由 Input 的内部后端注册，用于累积滚动增量。

## Input（input.h / input.cpp）

键鼠输入的每帧状态抽象。核心取舍是**拉模型**：不维护事件队列，消费方每帧主动采样当前状态。

- **只枚举用到的键**：`Key`（W/S/A/D/Q/E/Escape）、`MouseButton`（左/中/右）、`CursorMode`（正常/锁定）三个枚举，内部用 constexpr 表映射到 GLFW 常量。枚举尾部的 `Count` 用于定长状态数组。
- **三态查询**：`IsKeyDown`（持续按住）/ `IsKeyPressed`（本帧刚按下）/ `IsKeyReleased`（本帧刚抬起）。边沿在每帧 `Update()` 里由"上一帧状态 vs 当前状态"对比得出，三个布尔数组（按下态 / 本帧按下 / 本帧抬起）随帧刷新。
- **光标与滚轮**：`GetCursorPosition` 返回窗口坐标（原点左上、y 向下），`GetCursorDelta` 是本帧位移（FPS 环视的输入）；滚轮增量由 Window 回调在 InputBackend 里累积，`Update()` 时取出并清零（一帧内多次滚动合并为一个值）。
- **InputBackend：GLFW 实现隔离**（本模块的设计重点）：`Input` 本身不 include 任何 GLFW 头，所有 GLFW 调用（`glfwGetKey` / `glfwGetMouseButton` / `glfwGetCursorPos` / `glfwSetInputMode`）全部收敛在 .cpp 内部类 `InputBackend` 中。对外的键鼠枚举与查询接口因此与平台库完全解耦——将来更换窗口/输入库只需重写这一个类，消费方（app、相机控制器、UI）零改动。它通过 `Window::GetNativeHandle()` 拿到裸窗口句柄再转回 `GLFWwindow*`：这个 `void*` 是 PIMPL 封装有意留的唯一缺口，GLFW 类型转换被限制在内部类内部。

拉模型的代价是单帧内"按下又抬起"会被合并消失——对查看器场景无影响，换来的是消费端无缓冲、无时序问题的简单查询接口。

## Time（time.h）

header-only 的帧计时。内部用 `std::chrono::steady_clock`（单调时钟，不受系统时间调整影响）；每帧 `Update()` 计算与上一帧的间隔 `GetDeltaTime()`（相机移动、UI 动画都基于它），`GetElapsedTime()` 返回启动以来的累计秒数。

## Log（log.h / log.cpp）

四级日志（Debug / Info / Warning / Error），命名空间 + 变参函数而非类。

- **先门控再格式化**：`Log(level, args...)` 模板先比对级别阈值，被过滤的日志连字符串拼接都不发生；消息用 fold expression `(oss << ... << args)` 拼接，调用点可以写 `Log::Info("[Resource] Create skybox: ", size, "x", size)` 而不需要手拼字符串。
- **输出分流**：Warning 及以上走 stderr，其余走 stdout；互斥锁保护输出。
- **彩色输出**：ANSI 转义码（Info 绿 / Warning 黄 / Error 红）；Windows 上开启控制台的 `ENABLE_VIRTUAL_TERMINAL_PROCESSING`，POSIX 上用 `isatty` 判断——输出被重定向到文件时自动退化为无色。
- **编译期剔除**：`KITA_LOG_DEBUG` 宏在 NDEBUG（Release）下展开为空，高频的 Debug 日志在发布版零开销。资源层的日志分级使用约定见 [../dev-history/architecture.md](../dev-history/architecture.md)。

## Path（path.h）

`Path::Normalize`：`absolute` + `lexically_normal` 归一化路径（统一分隔符、消解 `..`），**全程不触碰文件系统**。用途单一但关键：AssetManager 以归一化路径作为缓存 key——同一个文件无论以什么写法传入（`./assets/a.hdr`、`assets/../assets/a.hdr`、绝对路径），都会命中同一条缓存。

## Math（math.h）

目前只有 `WrapDegrees`：把角度值归一化到 `[-180, 180)`。旋转量存储前先归一化（`Object::SetRotation` 使用），避免 UI 滑块和比较逻辑被任意大的角度值干扰。
