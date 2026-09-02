# Vk Pbr Viewer（名称暂定）

> **仅作参考**：本文档为项目构思基线，不跟踪实现现状。若内容与实际代码或其他文档不一致，一律以实际代码为准，并顺手更新本文档消除偏差。

## 项目介绍
Vk Pbr Viewer 是一个使用 **C++** 开发，以 **Vulkan** 作为图形 API 的 PBR 小型渲染器（侧重于预览与交互，因此命名为 Viewer 而非通用的 Renderer）。

用户可以通过 UI 选择内置模型或从外部导入模型，自由调节材质参数（支持直接设置数值或导入纹理贴图），并调整灯光、天空盒等场景参数，以实时查看高精度的 PBR 渲染结果。

## 项目模块

- **UI 模块**  
  负责显示实时运行信息（帧率、帧间隔等），并提供参数设置面板，涵盖天空盒贴图切换、灯光参数调节（颜色、强度、位置）、模型选择、材质参数（BaseColor、Metallic、Roughness 等）调整，以及外部模型/贴图的导入入口。

- **窗口与输入模块**  
  基于 GLFW 创建 Vulkan 窗口，并封装一个与窗口对应的输入处理类，统一管理键盘、鼠标及窗口事件（如 resize），确保交互响应及时。
  
  **相机控制交互方式（核心）**：
  - **旋转（Orbit）**：按住鼠标左键并拖动，实现相机围绕场景中心（LookAt 目标点）的轨道旋转。
  - **缩放（Zoom）**：使用鼠标滚轮控制相机**沿视线方向前后移动（推拉相机位置）**。采用该方式而非修改 FOV，以避免透视畸变，保持模型观感的一致性。
  - **平移（Pan）**：按住鼠标中键（或 Shift + 左键）并拖动，实现视口的上下左右平移。

- **渲染器模块**  
  负责完整的 Vulkan 渲染流程。内部细分为多个子类（如 `RenderContext`、`PipelineManager`、`ResourceManager` 等），大致包含二至三个核心渲染通道（Pass）：
  - **天空盒 Pass**：渲染场景背景。
  - **阴影 Pass**（可选）：生成 Shadow Map 以支持阴影效果。
  - **PBR 主渲染 Pass**：基于 Cook-Torrance BRDF 模型计算物体表面光照。

- **材质与模型数据模块**  
  纯 CPU 端数据结构（结构体/类），用于存储模型顶点、索引以及材质参数（颜色值、纹理路径、金属度/粗糙度标量等）。数据由 UI 修改，渲染器每帧读取并同步至 GPU。

- **App 业务逻辑模块**  
  作为整个应用的中枢，负责串联 UI 响应、用户输入处理、场景数据更新以及渲染画面的联动变化，确保各模块之间的调用关系清晰、解耦。

## 技术栈

- **C++**  
  使用 C++17 标准。充分利用 RAII 机制管理 Vulkan 资源生命周期，使用 `std::optional`、`std::variant` 等特性简化错误处理和参数传递。

- **Vulkan**  
  启用 Vulkan 1.3 版本，并启用 `VK_KHR_dynamic_rendering` 扩展。该扩展允许在不创建传统 RenderPass 和 Framebuffer 对象的情况下直接开始渲染，显著减少样板代码，使多 Pass 管理更加灵活。

- **CMake**  
  作为核心构建工具。为了在保证工程复现性的同时最大化 AI 辅助编程工具的上下文理解能力，采用 **“核心底层库 Submodule + 高频工具库 FetchContent”** 的混合依赖管理模式：
  - **Git Submodule（存放于 `third_party/`）**：将 **GLFW** 及 **VulkanMemoryAllocator (VMA)** 作为 Git 子模块引入。这些库直接与操作系统和 Vulkan 驱动交互，接口稳定性要求极高。通过 Submodule 将源码锁定在特定 Commit，避免上游更新导致的构建断裂；同时这些源码物理存在于项目目录中，AI Agent 可直接索引其头文件与宏定义，在生成 Vulkan 对象创建或内存分配代码时能准确把握参数结构，显著降低 AI 幻觉。
  - **FetchContent（CMake 配置时自动拉取）**：将 **glm**、**stb_image**、**tinyobjloader** 以及 **Dear ImGui** 通过 `FetchContent` 在构建配置阶段自动下载。这些库接口高度标准化且极少变动，采用自动下载可大幅减轻仓库体积，并简化团队协作时的环境配置成本。
  
  工程根目录的 `CMakeLists.txt` 中将包含子模块存在性检查逻辑，若开发者未执行 `git submodule update --init`，构建时会直接输出友好错误提示，该提示可直接反馈给 AI Agent，辅助其自动修复环境问题。

- **UI 库（Dear ImGui）**  
  采用即时模式 GUI 库 Dear ImGui，并集成其官方 Vulkan 后端与 GLFW 后端。用于构建调试面板及参数控制界面，支持滑块、颜色选择器、文件对话框等交互组件。

- **模型加载（tinyobjloader）**  
  用于解析 `.obj` 格式的模型文件，提取顶点位置、法线、纹理坐标及索引数据。若后续需要支持 glTF 等更复杂的格式，可扩展或替换为 `cgltf` / `Assimp`。

- **图像加载（stb_image）**  
  用于加载常见格式（如 PNG、JPG、BMP、HDR）的纹理贴图与天空盒图片，支持将数据直接上传至 Vulkan 图像对象。

- **数学库（GLM）**  
  提供向量（vec）、矩阵（mat）及常用几何变换运算。需配置为使用 Vulkan 的 NDC 坐标系（`GLM_FORCE_DEPTH_ZERO_TO_ONE` 和 `GLM_FORCE_LEFT_HANDED` 视具体设置而定，通常配合 Y 轴翻转处理）。

- **内存管理（VulkanMemoryAllocator / VMA）**  
  用于高效管理 Vulkan 设备内存（Device Memory）和绑定（Binding），自动处理内存类型选择、分配与释放，避免手动管理带来的碎片化和繁琐的 API 调用。该库采用单头文件形式，通过 Submodule 引入。

- **着色器编译（glslangValidator）**  
  使用 Vulkan SDK 自带的 `glslangValidator` 工具，在 CMake 构建时将 GLSL 源码编译为 SPIR-V 二进制文件，运行时直接加载。

## 运行环境与约束

- **工作目录约束（强制）**  
  本项目的所有资源文件（模型、贴图、天空盒等）均存放于项目根目录下的 `assets/` 文件夹中，代码中的资源加载路径均采用 **相对于项目根目录** 的相对路径（例如 `assets/models/...`、`assets/textures/...`）。因此，**可执行文件必须在项目根目录下启动**（即 `CMAKE_SOURCE_DIR`），否则将因找不到资源文件而导致加载失败。  
  开发者在 IDE（如 Visual Studio、VS Code、CLion）中调试时，**必须将工作目录（Working Directory）设置为项目根目录**，而非默认的 `build/` 或 `bin/` 输出目录。

- **启动时资源完整性检查（强制）**  
  应用程序的入口点（`main` 函数或 `App` 类的初始化阶段）**必须**在执行任何渲染逻辑之前，主动检查项目根目录下是否存在 `assets/` 文件夹。  
  若 `assets/` 目录不存在，程序应立即输出明确的错误提示（例如 `[Fatal] assets/ directory not found. Please run the executable from the project root folder.`）并调用 `std::exit(EXIT_FAILURE)` 终止进程。  
  该检查旨在将“路径配置错误”的问题暴露在最早阶段，避免在后续的 Vulkan 初始化或渲染循环中因加载不到贴图而导致难以追踪的空指针访问或 GPU 崩溃。