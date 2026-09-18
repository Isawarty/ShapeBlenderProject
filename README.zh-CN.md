# 模糊数学二维形状渐变

**简体中文** | [English](README.md)

这是一个使用 C++17 实现的交互式二维多边形渐变项目。程序先利用局部边长比例、角度和模糊相似度建立有序顶点对应，再在稳定的仿射坐标系中计算中间轮廓。

![兔子到猩猩的渐变示例](docs/demo-bunny-gorilla.png)

## 项目特点

- **自动循环对应**：比较局部边长比例与角度，保持轮廓顺序，并以 `O(m²n)` 复杂度搜索源轮廓的所有循环起点。
- **稳定仿射基**：综合局部相似度与基三角形覆盖面积选择三对对应点，避免基点接近共线造成插值塌缩。
- **结构保持插值**：分别插值仿射变换和顶点局部坐标，而不是直接对世界坐标做线性混合。
- **交互式参数观察**：通过 Dear ImGui 调节时间、匹配权重和基点选择权重。
- **视口自动适配**：源轮廓、中间轮廓和目标轮廓分别居中并自动缩放，启动后即可看清默认示例。

默认示例把 274 个顶点的兔子轮廓渐变为 248 个顶点的猩猩轮廓，当前自动搜索得到的循环起点为 `k = 46`。

## 算法流程

1. 从两张剪影图中提取有序多边形轮廓。
2. 用相邻两条边的长度和局部角度描述每个顶点。
3. 把局部描述差异转换为 `[0,1]` 范围内的模糊相似度。
4. 在匹配代价矩阵上运行保持顺序的动态规划。
5. 遍历源轮廓的所有循环起点，选择总代价最低的对应路径。
6. 从高质量对应点中选择覆盖范围足够大的三点仿射基。
7. 插值仿射变换与各顶点的局部坐标，生成中间轮廓。

当前实现要求多边形 A 的顶点数不少于多边形 B。默认示例满足这一要求。

## 编译与运行

当前版本已在 macOS、Apple Clang、CMake 和 OpenGL 3.3 环境下验证。

```bash
git clone --recurse-submodules https://github.com/Isawarty/ShapeBlenderProject.git
cd ShapeBlenderProject
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build
./ShapeBlender
```

请从 `build/` 目录启动程序，因为默认资源路径以该目录为基准。

## 界面操作

- **Time (t)**：查看从源轮廓 `0` 到目标轮廓 `1` 的渐变。
- **Auto-Find Best k**：自动搜索闭合轮廓的最佳循环起点。
- **Manual k**：关闭自动模式后，手动检查指定起点。
- **sim_t 权重**：调节边长比例相似度和角度相似度的占比。
- **smooth_a 权重**：调节基点选择中的形状、旋转和面积因素。
- **Recompute 按钮**：修改参数后重新计算，无需重启程序。

## 从其他图片生成轮廓

可选的预处理工具依赖 Python、OpenCV 和 NumPy：

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r scripts/requirements.txt
cd scripts
python extract_contours.py
```

脚本会生成 `assets/poly_a.json` 和 `assets/poly_b.json`。随后可在控制面板中加载这两个路径。多边形 A 的顶点数必须不少于多边形 B。

## 目录结构

```text
ShapeBlenderProject/
├── assets/
│   ├── image_*.png              # 示例剪影图片
│   ├── bunny_contour.json       # 默认源轮廓：兔子
│   └── gorilla_contour.json     # 默认目标轮廓：猩猩
├── docs/
│   └── demo-bunny-gorilla.png   # README 使用的演示图
├── include/
│   ├── Application.h            # 窗口、界面与视口声明
│   ├── Polygon.h                # 多边形数据和局部特征描述
│   └── ShapeBlender.h           # 顶点对应与插值接口
├── lib/
│   ├── eigen/                   # 线性代数
│   ├── glfw/                    # 窗口与 OpenGL 上下文
│   ├── imgui/                   # 交互式用户界面
│   └── json/                    # JSON 解析
├── scripts/
│   ├── extract_contours.py      # 可选的图片轮廓提取工具
│   └── requirements.txt         # OpenCV 与 NumPy 依赖
├── src/
│   ├── Application.cpp          # ImGui 控件与自适应视口绘制
│   ├── Polygon.cpp              # 轮廓读取与局部特征计算
│   ├── ShapeBlender.cpp         # 动态规划对应与仿射插值
│   └── main.cpp                 # 程序入口
├── CMakeLists.txt               # CMake 构建配置
├── README.md                    # 英文文档
└── README.zh-CN.md              # 简体中文文档
```

## 实现说明

- “模糊”指连续的相似程度，与图像模糊滤波无关。
- 动态规划路径只允许向下或向右下移动，因此保持轮廓顺序；当 `m > n` 时，允许多个源顶点对应同一个目标顶点。
- 自动起点搜索把最佳偏移保存在 `m_bestK` 中，回溯阶段使用同一个偏移恢复真实顶点编号。
- 仿射基选择会排除归一化三角形覆盖率低于 `0.02` 的候选组合，降低插值过程中的数值不稳定。

## 致谢

本课程项目采用课程资料中归于 Zhang（1996）的模糊数学二维多边形渐变思路。Eigen、GLFW、Dear ImGui、nlohmann/json、OpenCV 和 NumPy 分别提供线性代数、窗口与界面、数据读取及轮廓预处理支持。
