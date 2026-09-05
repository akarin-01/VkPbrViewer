# 02. 窗口与 Vulkan 初始化

> **阶段目标**：GLFW 窗口打开，Vulkan Instance/Device/Surface 就绪，设备层代码分层清晰。

- [x] CMake：添加 GLFW Submodule 到 `third_party/`，链接到目标
- [x] CMake：`find_package(Vulkan REQUIRED)` 查找 Vulkan SDK，链接到目标
- [x] 基于 GLFW 创建窗口，标题 "VkPbrViewer"，设置合适默认尺寸
- [x] 创建 Vulkan Instance，开启所需的扩展（Surface + 平台对应扩展）
- [x] 挑选合适的物理设备（独立显卡优先），创建 Logical Device
- [x] 获取 Graphics + Present Queue，创建 GLFW Surface
- [x] 创建 Command Pool
- [x] 提取共享工具函数到 `render_utils`：`FindQueueFamilies`、`QuerySwapChainSupport` 等
- [x] Validation Layers：当前已启用但暂未注册 `VK_EXT_debug_utils` 回调，Vulkan 诊断信息直接通过控制台输出，后续视需要再决定是否添加结构化回调

## 架构说明

```
Window (PIMPL)          RenderContext           render_utils (纯函数工具)
├── GLFW 细节隐藏        ├── VkInstance          ├── QueueFamilyIndices
├── RequiredExtensions   ├── VkSurfaceKHR        ├── SwapChainSupportDetails
├── CreateSurface()      ├── VkPhysicalDevice    ├── FindQueueFamilies()
├── GetFramebufferSize() ├── VkDevice            └── QuerySwapChainSupport()
                         ├── VkQueue (graphics)
                         ├── VkQueue (present)
                         └── VkCommandPool
```

### 关键设计决策

- **Window 用 PIMPL**：`GLFWwindow*` 隐藏在 `.cpp` 中，头文件只暴露 Vulkan 类型，GLFW `#include` 被关在实现文件里
- **Surface 归 Window**：窗口系统为 Vulkan 创建 surface 是 Window 的天然职责
- **工具函数放 `render_utils`**：被多个类共用的 Vulkan 查询函数提取为纯函数，不绑定到特定类
- **SwapChain 不在此阶段**：留到 03 与 Renderer 一起实现
