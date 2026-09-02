# 构建与运行（Windows）

## 环境

- 平台：Windows x64；需装 Vulkan SDK（提供链接库与 `glslc`，CMake 中 `find_program(glslc REQUIRED)`，要求它在 PATH 上）。
- CMake 生成器：Visual Studio 17 2022（multi-config），构建树在 `build/`。

## 依赖

- submodule：`third_party/{glfw, VMA, Vulkan-Headers}`；首次克隆后执行 `git submodule update --init --recursive`（CMake 配置期会检查存在性，缺失即报错）。
- FetchContent（首次配置需联网）：glm 1.0.3 / tinygltf v2.9.7（**勿升 v3**，API 不兼容）/ stb（commit 锁定）/ imgui v1.92.9b（勿降级，动态渲染需 ≥ 1.91.0）。
- 注：VMA 已作为 submodule 就位，但尚未接入 CMake。

## 常用命令（Git Bash，在仓库根执行）

- 配置：`cmake -S . -B build`（生成 VS 2022 解决方案）
- 构建：`cmake --build build --config Debug`（VS 是多配置生成器，必须带 `--config`）
- 运行：**工作目录必须是仓库根**（shader 按相对路径 `assets/shaders/` 加载）：`./build/Debug/VkPbrViewer.exe`
- 配置报错先检查：submodule 是否初始化、Vulkan SDK 是否在 PATH。

## Shader

- GLSL 源在 `assets/shaders/`，构建时由自定义目标 `shaders` 调 glslc 编译为 `*.spv`（产物写回 `assets/shaders/`，已 .gitignore，不提交）。
- 改 `.glsl`（含 `include/*.glsl`）后重新构建即自动重编；运行期报缺 `.spv` 先确认构建成功。
- 新增 shader 需登记 `CMakeLists.txt` 的 `SHADER_SOURCES` / `SHADER_OUTPUTS`。

## 编译/链接要点（MSVC）

- C++17（已定义 `GLM_FORCE_DEPTH_ZERO_TO_ONE`）；`/utf-8` `/W4` 警告全开。
- Win32 额外链接 `gdi32`、`comdlg32`（文件对话框仅 Windows，其他平台该按钮为 no-op）。
- `src/` 新增源文件时登记到 `CMakeLists.txt` 的 `add_executable` 列表（无 glob）。