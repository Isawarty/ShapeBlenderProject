cmake_minimum_required(VERSION 3.15)
project(ShapeBlenderProject LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")

include_directories(
    ${INCLUDE_DIR}
    ${LIB_DIR}/eigen
    ${LIB_DIR}/json
    ${LIB_DIR}/imgui
)
# ------------------------------------------------------------
# 源文件
# ------------------------------------------------------------
file(GLOB SRC_FILES
    "${SRC_DIR}/*.cpp"
)

# 将 imgui 源码直接编译进项目
file(GLOB IMGUI_SOURCES
    "${LIB_DIR}/imgui/*.cpp"
)

# ------------------------------------------------------------
# 可执行文件
# ------------------------------------------------------------
add_executable(ShapeBlender
    ${SRC_FILES}
    ${IMGUI_SOURCES}
)


# ------------------------------------------------------------
# 信息打印
# ------------------------------------------------------------
message(STATUS "✅ Project: ShapeBlenderProject")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source Dir: ${SRC_DIR}")
message(STATUS "Include Dir: ${INCLUDE_DIR}")
message(STATUS "Lib Dir: ${LIB_DIR}")
message(STATUS "Output Dir: ${ASSETS_DIR}/build")