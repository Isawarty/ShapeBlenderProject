# Fuzzy ShapeBlender Project

本项目使用 C++ 、Python 和 ImGui 实现了一个二维多边形形状渐变（morphing）算法。算法的核心基于1996年 Zhang 提出的“基于模糊数学的二维多边形形状渐变”方法。

它能够自动计算两个（拓扑上相似的）轮廓之间的顶点对应关系，并使用仿射变换插值来平滑地计算中间帧。

## 核心功能

- **自动顶点对应**：使用 ==基于模糊数学的图论求解方法==  和 $O(m^2n)$ 动态规划来自动寻找两个多边形之间的最佳顶点匹配。
    
- **平滑插值**：使用基于局部仿射变换和矩阵分解的插值方法，以避免线性插值导致的“收缩”和“枯萎”问题。
    
- **轮廓提取**：包含一个 Python 脚本，使用 OpenCV 自动从黑白轮廓图中提取多边形顶点。
    
- **实时交互界面**：使用 ImGui 构建，允许用户：
    - 实时拖动时间滑块（`t`）查看渐变。
    - 实时调整 `sim_t` 和 `smooth_a` 算法的权重，以观察对匹配结果的影响。
    - 动态加载不同的多边形文件。
        

## 🗂️ 文件架构

```
ShapeBlenderProject/
├── assets/                  # 存放输入/输出数据
│   ├── image_*.png          # 示例输入图像 A B
│   ├── poly_*.json          # (由 Python 脚本生成)
│
├── build/                   # (CMake 生成的文件，需要自己构建)
│
├── include/                 # C++ 头文件 (.h)
│   ├── Application.h        # 封装 ImGui 和 GLFW 窗口
│   ├── Polygon.h            # 多边形数据结构
│   └── ShapeBlender.h       # 核心算法类
│
├── lib/                     # 外部依赖库 (作为子模块或源码)
│   ├── eigen/               # Eigen (线性代数)
│   ├── glfw/                # GLFW (窗口和输入) [源代码]
│   ├── imgui/               # ImGui (GUI) [源代码]
│   └── json/                # nlohmann/json.hpp (JSON 解析)
│
├── scripts/                 # 预处理脚本
│   ├── extract_contours.py  # 轮廓提取脚本
│   └── requirements.txt     # (opencv-python, numpy)
│
├── src/                     # C++ 源文件 (.cpp)
│   ├── Application.cpp
│   ├── main.cpp
│   ├── Polygon.cpp
│   └── ShapeBlender.cpp
│
└── CMakeLists.txt           # 主构建脚本
```

## 🛠️ 依赖库

本项目通过 Git 子模块和 CMake 在本地编译所有依赖项，实现了完全可移植。

- **C++ 核心**：
    - [Eigen](https://gitlab.com/libeigen/eigen) (v3.4+): 用于所有线性代数（矩阵、向量）运算。
    - [Dear ImGui](https://github.com/ocornut/imgui) (v1.89+): 用于创建图形用户界面。
    - [nlohmann/json](https://github.com/nlohmann/json) (v3.11+): 用于解析 Python 脚本生成的 `.json` 顶点文件。
    - [GLFW](https://github.com/glfw/glfw) (v3.3+): 作为 `add_subdirectory` 由 CMake 在本地编译，用于创建窗口和处理 OpenGL 上下文。
        
- **Python 脚本** (`scripts/`)：
    - **OpenCV-Python**: 用于从图像中读取和提取轮廓。
    - **NumPy**: 用于辅助 OpenCV 的数据操作。

## 🚀 启动

请按以下步骤运行本项目。

### 1. 运行 Python 脚本 (提取轮廓)

在运行 C++ 程序之前，必须先生成 `.json` 顶点文件。

1. **安装 Python 依赖**：
```Bash
# 安装 requirements.txt 中的包
pip install -r scripts/requirements.txt
```


2. **运行提取脚本**：
```Bash
python scripts/extract_contours.py
```
    
- 脚本会**弹出两个 GUI 窗口**，请依次选择“图像 A”和“图像 B”。
    - 脚本会自动处理顶点密度，并**强制保证** `poly_a.json` 的顶点数 (`m`) 大于等于 `poly_b.json` 的顶点数 (`n`)。
    - 它会在 `assets/` 目录中生成 `poly_a.json` 和 `poly_b.json`。
        

### 2. 编译和运行 C++ 程序

1. **配置与编译项目**：
``` Bash
mkdir build
cd build
cmake ..
make
```
    
2. **运行**：
    - 可执行文件 `ShapeBlender` 会在 `build/` 目录中生成。
    - **重要**： `assets` 文件夹位于 `build/` 目录的**上一级**（`../`）。
    - `Application.cpp` 中默认的 `m_pathABuf` 和 `m_pathBBuf` 路径（`../assets/poly_a.json`）是**正确**的。
```Bash
./ShapeBlender
```
    

### 3. 使用程序

1. 程序启动后，应自动加载 `../assets/poly_a.json` 和 `../assets/poly_b.json`。在程序打开伊始，"Viewport"窗口和"Controls"窗口可能会黏在一起，因此需要调节一下。
    
2. 在 "Viewport" 窗口中（可以拖动靠边形成DockSpace），可以看到红色的为源图像，蓝色的为目标图像，白色的为中间图像。
    
3. 在 "Controls" 窗口中，**将 "Render Scale" 滑块从 `1.0` 向下拖动**到 `0.1` 或 `0.05`，直到您能看到三个多边形。
    
4. 拖动 **"Time (t)"** 滑块来查看渐变。
    
5. 在 "Controls" 窗口中**调节 `sim_t` 和 `smooth_a` 权重**，然后**点击 "Load & Recompute"** 来查看算法匹配结果的变化。
    - `sim_t` 权重会影响 DP 算法的匹配结果。
    - `smooth_a` 权重会影响仿射基的选择。