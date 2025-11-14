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
target_include_directories(ShapeBlender PUBLIC
    ${INCLUDE_DIR}
    ${LIB_DIR}/eigen
    ${LIB_DIR}/json
    ${LIB_DIR}/imgui
    ${LIB_DIR}/imgui/backends
)


find_package(OpenGL REQUIRED)
find_library(CORE_FOUNDATION CoreFoundation)
find_library(IOKIT IOKit)

target_link_libraries(ShapeBlender PUBLIC
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