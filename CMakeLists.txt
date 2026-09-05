cmake_minimum_required(VERSION 3.21)
project(VkPbrViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ======================================================================
# 依赖管理：统一 FetchContent，版本以 GIT_TAG（tag 或 commit）锁定
#
# 两种接入模式，按"上游是否提供 CMake 目标"划分：
#   A. 提供 CMake 目标     → FetchContent_MakeAvailable 直接引入
#   B. 无/不用上游 CMake   → FetchContent_Populate + 手动定义 target
# ======================================================================
include(FetchContent)

# ----- A1. glm — 数学库，header-only，零依赖 -----
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)

# ----- A2. glfw — 窗口与输入 -----
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.5.1
)
FetchContent_MakeAvailable(glfw)

# ----- A3. Vulkan-Headers — 编译期头文件，提供 Vulkan::Headers target -----
FetchContent_Declare(
    vulkan-headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG        v1.4.362
)
FetchContent_MakeAvailable(vulkan-headers)

# ----- Vulkan Loader — 来自系统 Vulkan SDK，提供链接库（Vulkan::Vulkan）-----
find_package(Vulkan REQUIRED)

# ----- B1. tinygltf — glTF 2.0 解析库（自带 json.hpp，single-header 模式）-----
# 锁定 2.x 系列：v3 重写分支改名 tiny_gltf_v3.h 且 API 不兼容，勿升级
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.7
)
FetchContent_GetProperties(tinygltf)
if(NOT tinygltf_POPULATED)
    FetchContent_Populate(tinygltf)
endif()
add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE ${tinygltf_SOURCE_DIR})

# ----- B2. stb — 单头文件图像库（stb_image 供纹理 / HDR 解码）-----
# 上游无版本 tag，用 commit hash 锁定
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        2c980bb59875b0d32144a71867fbdebb2f77cd20
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# ----- B3. Dear ImGui — 即时模式 GUI（Vulkan + GLFW 后端，静态编译）-----
# 锁定 v1.92.9b：动态渲染支持自 v1.91.0 起提供，本项目无经典 RenderPass，必须走该路径
# 其 target 依赖 A 组的 glfw / Vulkan::Headers / Vulkan::Vulkan，故置于 A 组之后
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9b
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Headers Vulkan::Vulkan)

# ===== 着色器编译器（glslc）=====
find_program(GLSLC_EXECUTABLE glslc REQUIRED)
message(STATUS "Shader compiler: glslc (${GLSLC_EXECUTABLE})")

# ===== 可执行文件 =====
add_executable(VkPbrViewer
    src/main.cpp

    # core
    src/core/input.cpp
    src/core/log.cpp
    src/core/window.cpp

    # rhi
    src/rhi/compute_pipeline.cpp
    src/rhi/context.cpp
    src/rhi/frame_sync.cpp
    src/rhi/graphics_pipeline.cpp
    src/rhi/one_shot_command.cpp
    src/rhi/rendering_scope.cpp
    src/rhi/shader_module.cpp
    src/rhi/swap_chain.cpp
    src/rhi/utils.cpp

    # resource
    src/resource/asset_manager.cpp
    src/resource/asset_utils.cpp
    src/resource/compute_conversion.cpp
    src/resource/descriptor_manager.cpp
    src/resource/descriptor_writer.cpp
    src/resource/environment_baker.cpp
    src/resource/graveyard.cpp
    src/resource/resource_manager.cpp
    src/resource/resource_utils.cpp

    # render
    src/render/render_pipeline.cpp
    src/render/render_scene.cpp
    src/render/renderer.cpp
    src/render/scene_proxy.cpp

    # render passes
    src/render/passes/lit_pass.cpp
    src/render/passes/post_process_pass.cpp
    src/render/passes/shadow_pass.cpp
    src/render/passes/skybox_pass.cpp
    src/render/passes/ui_pass.cpp

    # scene
    src/scene/camera.cpp
    src/scene/light.cpp
    src/scene/material.cpp
    src/scene/object.cpp
    src/scene/post_process.cpp
    src/scene/scene.cpp
    src/scene/skybox.cpp
    src/scene/utils.cpp

    # application
    src/application/app.cpp
    src/application/fps_camera_controller.cpp
    src/application/orbit_camera_controller.cpp
    src/application/ui.cpp
    src/application/ui_utils.cpp
)

target_include_directories(VkPbrViewer PRIVATE ${CMAKE_SOURCE_DIR}/src)

# ===== 编译定义 =====
# GLM_FORCE_DEPTH_ZERO_TO_ONE: glm::perspective 输出 Vulkan NDC 深度 [0,1]
target_compile_definitions(VkPbrViewer PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)

# ===== 编译选项 =====
if(MSVC)
    # /utf-8: 源码按 UTF-8 解析；/W4: 最高警告级别
    target_compile_options(VkPbrViewer PRIVATE /utf-8 /W4)
else()
    target_compile_options(VkPbrViewer PRIVATE -Wall -Wextra)
endif()

# ===== 链接 =====
target_link_libraries(VkPbrViewer PRIVATE
    glfw
    Vulkan::Headers
    Vulkan::Vulkan
    glm::glm
    tinygltf
    stb
    imgui
)

if(WIN32)
    # gdi32: Vulkan/GLFW 可能需要的系统库；comdlg32: OpenFileDialog(GetOpenFileNameA)
    target_link_libraries(VkPbrViewer PRIVATE gdi32 comdlg32)
else()
    # 当前文件浏览器使用 Win32 API；非 Windows 平台配置期给出警告，对话框按钮为 no-op
    message(WARNING "[CMake] OpenFileDialog currently only supports Windows; it will be a no-op on this platform.")
endif()

# ===== 着色器编译（GLSL → SPIR-V）=====
# *.spv 文件已在 .gitignore 中排除，编译产物直接输出到源码目录 assets/shaders/
# renderer.cpp 从工作目录下的 assets/shaders/ 加载这些文件
set(SHADER_SOURCES
    assets/shaders/lit.vert
    assets/shaders/lit.frag
    assets/shaders/skybox.vert
    assets/shaders/skybox.frag
    assets/shaders/post_process.vert
    assets/shaders/post_process.frag
    assets/shaders/equirect_to_cubemap.comp
    assets/shaders/irradiance_convolution.comp
    assets/shaders/prefilter.comp
    assets/shaders/brdf_integration.comp
    assets/shaders/shadow.vert
    assets/shaders/shadow.frag
)

set(SHADER_OUTPUTS
    assets/shaders/lit_vert.spv
    assets/shaders/lit_frag.spv
    assets/shaders/skybox_vert.spv
    assets/shaders/skybox_frag.spv
    assets/shaders/post_process_vert.spv
    assets/shaders/post_process_frag.spv
    assets/shaders/equirect_to_cubemap_comp.spv
    assets/shaders/irradiance_convolution_comp.spv
    assets/shaders/prefilter_comp.spv
    assets/shaders/brdf_integration_comp.spv
    assets/shaders/shadow_vert.spv
    assets/shaders/shadow_frag.spv
)

set(SHADER_INCLUDES
    assets/shaders/include/constants.glsl
    assets/shaders/include/cubemap_utils.glsl
    assets/shaders/include/ggx.glsl
    assets/shaders/include/hammersley.glsl
    assets/shaders/include/per_frame.glsl
    assets/shaders/include/per_material.glsl
    assets/shaders/include/per_object.glsl
    assets/shaders/include/shader_sets.glsl
)

set(COMPILED_SHADERS)
foreach(src out IN ZIP_LISTS SHADER_SOURCES SHADER_OUTPUTS)
    set(SRC_ABS ${CMAKE_SOURCE_DIR}/${src})
    set(OUT_ABS ${CMAKE_SOURCE_DIR}/${out})

    add_custom_command(
        OUTPUT ${OUT_ABS}
        COMMAND ${GLSLC_EXECUTABLE} ${SRC_ABS} -o ${OUT_ABS} -I ${CMAKE_SOURCE_DIR}/assets/shaders
        DEPENDS ${SRC_ABS} ${SHADER_INCLUDES}
        COMMENT "Compiling shader: ${src} -> ${out}"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${OUT_ABS})
endforeach()

add_custom_target(shaders ALL DEPENDS ${COMPILED_SHADERS}
    COMMENT "All shaders compiled")
add_dependencies(VkPbrViewer shaders)
