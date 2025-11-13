#pragma once
#include "ShapeBlender.h"
#include <imgui.h>

//前向声明 GLFW 窗口
struct GLFWwindow;


/**
 * @brief 封装 ImGui/GLFW 窗口、主循环和渲染。
 * 思路：这是将算法与窗口管理和渲染分离的“胶水”类。
 * 1. init(): 初始化 GLFW, ImGui。
 * 2. loadData(): 调用 m_blender 的加载和预计算函数。
 * 3. run(): 运行主循环。
 * 4. mainLoop(): 轮询事件, 开始新帧, 调用 drawUI(), 渲染。
 * 5. drawUI(): 绘制 ImGui 控件 (滑块, 按钮) 和视口。
 * 6. drawPolygon(): 一个辅助函数，用于将 Eigen::Vector2d 绘制到 ImDrawList。
 */
class Application{

public:
    Application();
    ~Application();

    /**
     * @brief 初始化 GLFW 窗口, OpenGL 上下文, 和 ImGui。
     */
    int init();

    /**
     * @brief 运行主事件循环。
     */
    void run();

private:
    /**
     * @brief 主循环的单次迭代。
     */
    void mainLoop();

    /**
     * @brief 清理资源。
     */
    void shutdown();

    /**
     * @brief 绘制所有 ImGui 界面元素。
     */
    void drawUI();

    /**
     * @brief 辅助函数，将一个多边形绘制到 ImGui 绘图列表。
     */
    void drawPolygon(ImDrawList* drawList, const Polygon& poly, ImU32 color, const ImVec2& offset, float scale) const;

    /**
     * @brief 封装了加载和预计算的逻辑。
     */
    void loadData();

    GLFWwindow* m_window = nullptr;
    ShapeBlender m_blender;
    float m_interpTime = 0.0f;
    float m_renderScale = 1.0f;

    // 用于 ImGui 文本输入的缓冲区
    char m_pathABuf[128];
    char m_pathBBuf[128];
};