cmake_minimum_required(VERSION 3.21)
project(VkPbrViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
            "[CMake] 缺少依赖库: ${dir}\n"
            "  请先执行: git submodule update --init --recursive\n"
            "  若尚未添加 submodule，请参考 docs/implementation/01_cmake.md")
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

# ===== 可执行文件 =====
add_executable(VkPbrViewer
    src/main.cpp

    # core
    src/core/window.cpp

    # render
    src/render/render_context.cpp
    src/render/render_utils.cpp
)

target_include_directories(VkPbrViewer PRIVATE ${CMAKE_SOURCE_DIR}/src)

# ===== 编译选项 =====
if(WIN32)
    target_compile_options(VkPbrViewer PRIVATE /utf-8)
endif()

# ===== 链接 =====
target_link_libraries(VkPbrViewer PRIVATE
    glfw
    Vulkan::Headers
    Vulkan::Vulkan
)

if(WIN32)
    target_link_libraries(VkPbrViewer PRIVATE gdi32)
endif()