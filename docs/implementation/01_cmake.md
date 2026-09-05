# 01. CMake 项目骨架

> **阶段目标**：项目能编译、能运行，`main()` 中 assets 检查生效。

- [x] 添加 GLFW Submodule：`git submodule add https://github.com/glfw/glfw.git third_party/glfw`
- [x] 添加 VMA Submodule：`git submodule add https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git third_party/VMA`
- [x] 执行 `git submodule update --init --recursive`
- [x] 创建根 `CMakeLists.txt`，配置 C++17 标准
- [x] 创建可执行文件目标（VkPbrViewer）
- [x] 创建 `src/`、`assets/`、`third_party/` 目录结构
- [x] 在 `main()` 入口处检查 `assets/` 目录是否存在，不存在则输出错误提示并 `std::exit(EXIT_FAILURE)`
- [x] 写一个 Hello World 的 `main.cpp`，验证构建链路能跑通

> **更新**：依赖管理已统一迁移到 FetchContent——GLFW、Vulkan-Headers 一并迁入（GIT_TAG 锁定版本），未使用的 VMA submodule 已移除，`third_party/` 目录与 submodule 存在性检查已删除。上方 submodule 步骤为历史记录。
