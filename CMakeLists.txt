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
    third_party/Vulkan-Headers
    third_party/VMA
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

# ===== 可执行文件 =====
add_executable(VkPbrViewer 
    src/main.cpp
)