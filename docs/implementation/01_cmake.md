# 01. CMake 项目骨架

- [ ] 添加 GLFW Submodule：`git submodule add https://github.com/glfw/glfw.git third_party/glfw`
- [ ] 添加 Vulkan-Headers Submodule：`git submodule add https://github.com/KhronosGroup/Vulkan-Headers.git third_party/Vulkan-Headers`
- [ ] 添加 VMA Submodule：`git submodule add https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git third_party/VMA`
- [ ] 执行 `git submodule update --init --recursive`
- [ ] 创建根 `CMakeLists.txt`，配置 C++17 标准
- [ ] 创建可执行文件目标（VkPbrViewer）
- [ ] 创建 `src/`、`assets/`、`third_party/` 目录结构
- [ ] 在 `main()` 入口处检查 `assets/` 目录是否存在，不存在则输出错误提示并 `std::exit(EXIT_FAILURE)`
- [ ] 写一个 Hello World 的 `main.cpp`，验证构建链路能跑通
