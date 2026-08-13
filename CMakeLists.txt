cmake_minimum_required(VERSION 3.21)
project(VkPbrViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ===== FetchContent =====
include(FetchContent)

# glm — 数学库，header-only，零依赖
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)

# tinyobjloader — 轻量 OBJ 解析库
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        v2.0.0rc13
)
FetchContent_MakeAvailable(tinyobjloader)

# stb — 单头文件图像库，无版本号，用 commit hash 锁定
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

# ===== Submodule 存在性检查 =====
# 这些库将在后续阶段通过 add_subdirectory 或 target_link_libraries 集成，
# 此处先检查目录是否存在，未初始化时给出明确提示。
set(SUBMODULE_DIRS
    third_party/glfw
    third_party/VMA
    third_party/Vulkan-Headers
)
foreach(dir ${SUBMODULE_DIRS})
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${dir}/.git" AND
       NOT EXISTS "${CMAKE_SOURCE_DIR}/${dir}/CMakeLists.txt" AND
       NOT EXISTS "${CMAKE_SOURCE_DIR}/${dir}/include")
        message(FATAL_ERROR
            "[CMake] Missing dependency: ${dir}\n"
            "  Run: git submodule update --init --recursive\n"
            "  If the submodule hasn't been added yet, see docs/implementation/01_cmake.md")
    endif()
endforeach()

# ===== GLFW =====
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

# ===== Vulkan-Headers（来自 submodule，提供编译期头文件）=====
add_subdirectory(third_party/Vulkan-Headers EXCLUDE_FROM_ALL)

# ===== Vulkan Loader（来自系统 Vulkan SDK，提供链接库）=====
find_package(Vulkan REQUIRED)

# ===== Shader Compiler (glslc) =====
find_program(GLSLC_EXECUTABLE glslc REQUIRED)
message(STATUS "Shader compiler: glslc (${GLSLC_EXECUTABLE})")

# ===== 可执行文件 =====
add_executable(VkPbrViewer
    src/main.cpp

    # core
    src/core/log.cpp
    src/core/window.cpp

    # render
    src/render/frame_sync.cpp
    src/render/render_context.cpp
    src/render/render_pass.cpp
    src/render/render_resources.cpp
    src/render/render_scene.cpp
    src/render/render_utils.cpp
    src/render/renderer.cpp
    src/render/swap_chain.cpp

    # scene
    src/scene/camera.cpp
    src/scene/light.cpp
    src/scene/material.cpp
    src/scene/mesh.cpp
    src/scene/texture.cpp
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
    tinyobjloader
    stb
)

if(WIN32)
    target_link_libraries(VkPbrViewer PRIVATE gdi32)
endif()

# ===== Shader 编译（GLSL → SPIR-V）=====
# *.spv 文件已在 .gitignore 中排除，编译产物直接输出到源码目录 assets/shaders/
# renderer.cpp 从工作目录下的 assets/shaders/ 加载这些文件
set(SHADER_SOURCES
    assets/shaders/lit.vert
    assets/shaders/lit.frag
)
set(SHADER_OUTPUTS
    assets/shaders/lit_vert.spv
    assets/shaders/lit_frag.spv
)

set(SHADER_INCLUDES
    assets/shaders/common/per_frame_data.glsl
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