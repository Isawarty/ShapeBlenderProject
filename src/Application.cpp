#include "Application.h"
#include <cstring>
#include <iostream>
#include <limits>
#include <ostream>
#include <string.h> // for strncpy

// ImGui 和后端
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// GLFW (必须在 imgui 后端之前包含)
#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

Application::Application(){
    strncpy(m_pathABuf, "../assets/bunny_contour.json", 128);
    strncpy(m_pathBBuf, "../assets/gorilla_contour.json", 128);
}

Application::~Application(){
    shutdown();
}

int Application::init() {
    // 1. 初始化 GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 2. 创建窗口
    const char* glsl_version = "#version 330 core";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    m_window = glfwCreateWindow(1600, 900, "Fuzzy Shape Blending", NULL, NULL);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // 启用垂直同步

    // 3. 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; //启用窗口停靠
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;//启用多窗口支持

    //设置样式
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // 4. 初始化 ImGui 后端
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 5. 加载初始数据
    loadData();

    return 0;
}

void Application::loadData(){
    std::cout << "Loading Data..." << std::endl;
    if (!m_blender.loadPolygons(m_pathABuf, m_pathBBuf)) {
        std::cerr << "Error Loading polygons. Check paths." << std::endl;
    } else {
        int k_to_run = m_autoFindK ? -1 : m_manualK;
        m_blender.computeCorrespondence(k_to_run);
        std::cout << "Compute Correspondence done." << std::endl;

        m_manualK = m_blender.getBestK();
        m_blender.findOptimalBasis();
        std::cout << "Find Optimal Basis done." << std::endl;
    }
}

void Application::run(){
    while (!glfwWindowShouldClose(m_window)) {
        mainLoop();
    }
}

void Application::mainLoop() {
    glfwPollEvents();

    // 开始 ImGui 帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 设置一个全屏、无装饰的窗口作为DockSpace的“背景”
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos); // 使用 WorkPos 避开菜单栏（如果有）
    ImGui::SetNextWindowSize(viewport->WorkSize); // 使用 WorkSize 避开菜单栏
    ImGui::SetNextWindowViewport(viewport->ID);

    // 设置窗口样式
    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    host_window_flags |= ImGuiWindowFlags_NoBackground;  

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    // 开始这个背景窗口
    ImGui::Begin("DockSpaceHost", nullptr, host_window_flags);

    ImGui::PopStyleVar(3);

    // 创建 DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode; 
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    // 结束背景窗口
    ImGui::End(); 

    // 绘制 UI
    drawUI();

    // 渲染
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 更新和渲染其他视口 (如果启用了)
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(m_window);
}

void Application::drawUI() {
    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    const float controlsWidth = 360.0f;

    // 控制面板
    ImGui::SetNextWindowPos(
        ImVec2(mainViewport->WorkPos.x + 12.0f, mainViewport->WorkPos.y + 12.0f),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2(controlsWidth, mainViewport->WorkSize.y - 24.0f),
        ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls");
    ImGui::InputText("Polygon A Path", m_pathABuf, 128);
    ImGui::InputText("Polygon B Path", m_pathBBuf, 128);
    if (ImGui::Button("Load")) {
        loadData();
    }
    
    ImGui::Separator();
    
    ImGui::SliderFloat("Time (t)", &m_interpTime, 0.0f, 1.0f);
    ImGui::TextDisabled("Viewport uses automatic fit-to-panel scaling.");

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Correspondence (Start Vertex k)");
    ImGui::Checkbox("Auto-Find Best k", &m_autoFindK);
    
    // 如果是自动模式，就禁用滑块
    if (m_autoFindK) {
        ImGui::Text("Best k (auto-found): %d", m_blender.getBestK());
    } else {
        // 假设 polyA 已经加载
        int max_k = m_blender.getPolyA().n - 1;
        if (max_k < 0) max_k = 0;
        
        // 手动滑块
        ImGui::SliderInt("Manual k", &m_manualK, 0, max_k);
        ImGui::SameLine();
        if (ImGui::Button("Run with this k")) {
            // 只运行 correspondence，不加载文件，也不 re-find basis
             m_blender.computeCorrespondence(m_manualK);
             std::cout << "Recompute Correspondence done (Manual k)." << std::endl;
        }
    }
    
    ImGui::Separator();
    ImGui::Spacing();

    // --- sim_t 权重 (w1, w2) ---
    ImGui::Text("sim_t Weights (for Correspondence)");
    bool sim_changed = false;
    if (ImGui::SliderFloat("w1 (Edge)", &m_blender.m_w1, 0.0f, 1.0f)) {
        m_blender.m_w2 = 1.0f - m_blender.m_w1; // 自动更新 w2
        sim_changed = true;
    }
    ImGui::Text("w2 (Angle): %.2f", m_blender.m_w2); // 显示自动计算的 w2

    ImGui::SameLine(); // 将下一个控件放在同一行
    if (ImGui::Button("Reset##SimWeights")) { // "##SimWeights" 是一个唯一的ID
        m_blender.m_w1 = 0.5f; // 恢复 ShapeBlender.h 中的默认值
        m_blender.m_w2 = 0.5f;
        sim_changed = true;
    }

    if (ImGui::Button("Recompute Correspondence")) {
         m_blender.computeCorrespondence();
         std::cout << "Recompute Correspondence done." << std::endl;
    }

    ImGui::Separator(); 
    ImGui::Spacing();

    // --- smooth_a 权重 (wS, wR, wA) ---
    ImGui::Text("smooth_a Weights (for Basis Finding)");
    bool smooth_changed = false;

    bool changed_s = ImGui::SliderFloat("wS (Shape)", &m_blender.m_smooth_a_wS, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    bool changed_r = ImGui::SliderFloat("wR (Rotation)", &m_blender.m_smooth_a_wR, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

    if (changed_s) {
        if (m_blender.m_smooth_a_wS + m_blender.m_smooth_a_wR > 1.0f) {
            m_blender.m_smooth_a_wR = 1.0f - m_blender.m_smooth_a_wS; // 当 wS 增加时，挤压 wR
        }
        smooth_changed = true;
    } else if (changed_r) {
        if (m_blender.m_smooth_a_wS + m_blender.m_smooth_a_wR > 1.0f) {
            m_blender.m_smooth_a_wS = 1.0f - m_blender.m_smooth_a_wR; // 当 wR 增加时，挤压 wS
        }
        smooth_changed = true;
    }
    //自动计算wA
    m_blender.m_smooth_a_wA = 1.0f - m_blender.m_smooth_a_wS - m_blender.m_smooth_a_wR;

    ImGui::Text("wA (Area): %.3f", m_blender.m_smooth_a_wA); // 显示自动计算的 wA
    
    ImGui::SameLine();
    if (ImGui::Button("Reset##SmoothWeights")) {
        m_blender.m_smooth_a_wS = 0.333f; // 恢复 ShapeBlender.h 中的默认值
        m_blender.m_smooth_a_wR = 0.333f;
        // m_blender.m_smooth_a_wA 会在下一帧自动更新为 0.334f
        smooth_changed = true;
    }

    if (ImGui::Button("Recompute Optimal Basis")) {
        m_blender.findOptimalBasis();
        std::cout << "Recompute Optimal Basis done." << std::endl;
    }
    ImGui::Separator();


    ImGui::End();

    // 渲染视口
    ImGui::SetNextWindowPos(
        ImVec2(mainViewport->WorkPos.x + controlsWidth + 24.0f,
               mainViewport->WorkPos.y + 12.0f),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2(mainViewport->WorkSize.x - controlsWidth - 36.0f,
               mainViewport->WorkSize.y - 24.0f),
        ImGuiCond_FirstUseEver);
    ImGui::Begin("Viewport");
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 300.0f);
    canvasSize.y = std::max(canvasSize.y, 240.0f);
    
    // 绘制多边形
    const auto& polyA = m_blender.getPolyA();
    const auto& polyB = m_blender.getPolyB();
    Polygon interpPoly = m_blender.getInterpolatedPolygon(m_interpTime);

    const Polygon* polygons[] = {&polyA, &interpPoly, &polyB};
    const char* labels[] = {"Source · Bunny", "Interpolated", "Target · Gorilla"};
    const ImU32 colors[] = {
        IM_COL32(238, 92, 92, 255),
        IM_COL32(245, 245, 245, 255),
        IM_COL32(72, 137, 235, 255),
    };

    const float gap = 12.0f;
    const float cellWidth = (canvasSize.x - gap * 2.0f) / 3.0f;
    for (int panel = 0; panel < 3; ++panel) {
        ImVec2 panelMin(canvasPos.x + panel * (cellWidth + gap), canvasPos.y);
        ImVec2 panelMax(panelMin.x + cellWidth, canvasPos.y + canvasSize.y);
        drawList->AddRectFilled(panelMin, panelMax, IM_COL32(24, 29, 38, 255), 8.0f);
        drawList->AddRect(panelMin, panelMax, IM_COL32(70, 80, 96, 255), 8.0f);
        drawList->AddText(ImVec2(panelMin.x + 12.0f, panelMin.y + 10.0f),
                          colors[panel], labels[panel]);
        drawPolygonFitted(drawList, *polygons[panel], colors[panel],
                          ImVec2(panelMin.x + 14.0f, panelMin.y + 38.0f),
                          ImVec2(panelMax.x - 14.0f, panelMax.y - 14.0f));
    }

    ImGui::Dummy(canvasSize);

    ImGui::End();
}

void Application::drawPolygonFitted(ImDrawList* drawList, const Polygon& poly,
                                    ImU32 color, const ImVec2& panelMin,
                                    const ImVec2& panelMax) const {
    if (poly.n == 0) return;

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    for (const auto& vertex : poly.vertices) {
        minX = std::min(minX, vertex.x());
        minY = std::min(minY, vertex.y());
        maxX = std::max(maxX, vertex.x());
        maxY = std::max(maxY, vertex.y());
    }

    const double width = std::max(maxX - minX, 1e-9);
    const double height = std::max(maxY - minY, 1e-9);
    const float availableWidth = std::max(panelMax.x - panelMin.x, 1.0f);
    const float availableHeight = std::max(panelMax.y - panelMin.y, 1.0f);
    const float scale = 0.88f * std::min(
        availableWidth / static_cast<float>(width),
        availableHeight / static_cast<float>(height));

    const double centerX = (minX + maxX) * 0.5;
    const double centerY = (minY + maxY) * 0.5;
    const ImVec2 panelCenter((panelMin.x + panelMax.x) * 0.5f,
                             (panelMin.y + panelMax.y) * 0.5f);
    const ImVec2 offset(
        panelCenter.x - static_cast<float>(centerX) * scale,
        panelCenter.y - static_cast<float>(centerY) * scale);
    drawPolygon(drawList, poly, color, offset, scale);
}

void Application::drawPolygon(ImDrawList* drawList, const Polygon& poly, ImU32 color, const ImVec2& offset, float scale) const {
    if (poly.n == 0) return;

    for (int i = 0; i < poly.n; ++i) {
        const auto& v1_eig = poly.vertices[i];
        const auto& v2_eig = poly.vertices[(i + 1) % poly.n];

        ImVec2 p1 = ImVec2(offset.x + v1_eig.x() * scale, offset.y + v1_eig.y() * scale);
        ImVec2 p2 = ImVec2(offset.x + v2_eig.x() * scale, offset.y + v2_eig.y() * scale);

        if (std::isnan(p1.x) || std::isnan(p1.y) || std::isnan(p2.x) || std::isnan(p2.y)) {
             static bool nan_error_printed = false;
            if (!nan_error_printed) {
                std::cerr << "!!! FATAL RENDER ERROR: NaN detected in polygon vertices." << std::endl;
                nan_error_printed = true; 
            }
            continue; 
        }

        drawList->AddLine(p1, p2, color, 2.0f);
    }
}

void Application::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}
