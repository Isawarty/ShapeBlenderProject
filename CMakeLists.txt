cmake_minimum_required(VERSION 3.15)
project(ShapeBlenderProject LANGUAGES CXX)

# ------------------------------------------------------------
# 项目设置
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DGL_SILENCE_DEPRECATION)

# ------------------------------------------------------------
# 路径变量
# ------------------------------------------------------------
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")

# ------------------------------------------------------------
# (新) 构建依赖项 (GLFW)
# ------------------------------------------------------------
# 这一步会告诉 CMake 进入 lib/glfw 目录, 运行它的 CMakeLists.txt,
# 并创建一个名为 "glfw" 的内部目标。
# (注意：这必须在 add_executable 之前)
add_subdirectory(${LIB_DIR}/glfw)

# ------------------------------------------------------------
# 源文件
# ------------------------------------------------------------
file(GLOB SRC_FILES
    "${SRC_DIR}/*.cpp"
)

file(GLOB IMGUI_SOURCES
    "${LIB_DIR}/imgui/*.cpp"
    "${LIB_DIR}/imgui/backends/imgui_impl_glfw.cpp"
    "${LIB_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)

# ------------------------------------------------------------
# 可执行文件
# ------------------------------------------------------------
add_executable(ShapeBlender
    ${SRC_FILES}
    ${IMGUI_SOURCES}
)

# ------------------------------------------------------------
# 包含目录
# ------------------------------------------------------------
# (注意：我们不再需要手动添加 ${LIB_DIR}/glfw/include，
#  因为 target_link_libraries(ShapeBlender PUBLIC glfw) 会自动处理)
target_include_directories(ShapeBlender PUBLIC
    ${INCLUDE_DIR}
    ${LIB_DIR}/eigen
    ${LIB_DIR}/json
    ${LIB_DIR}/imgui
    ${LIB_DIR}/imgui/backends
)

# ------------------------------------------------------------
# (修改) 链接库
# ------------------------------------------------------------
# 1. (删除) 我们不再 "find_package" GLFW
# find_package(glfw3 REQUIRED) <--- 删除这一行

# 2. (保留) 我们仍然需要系统提供 OpenGL 和 Apple 框架
find_package(OpenGL REQUIRED)
find_library(CORE_FOUNDATION CoreFoundation)
find_library(IOKIT IOKit)

# 3. (修改) 链接
target_link_libraries(ShapeBlender PUBLIC
    # 关键： "glfw" 现在指向我们用 add_subdirectory 
    # 在本地编译的那个目标，而不是系统库。
    glfw                
    OpenGL::GL          
    ${CORE_FOUNDATION}  
    ${IOKIT}            
)

# ------------------------------------------------------------
# 信息打印
# ------------------------------------------------------------
message(STATUS "✅ Project: ShapeBlenderProject (Portable)")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source Dir: ${SRC_DIR}")
message(STATUS "Include Dir: ${INCLUDE_DIR}")
message(STATUS "Lib Dir: ${LIB_DIR}")