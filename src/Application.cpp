#include "Application.h"
#include <cstring>
#include <iostream>
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
    strncpy(m_pathABuf, "../assets/poly_a.json", 128);
    strncpy(m_pathBBuf, "../assets/poly_b.json", 128);
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
    
    m_window = glfwCreateWindow(1280, 720, "Fuzzy Shape Blending", NULL, NULL);
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
        m_blender.computeCorrespondence();
        std::cout << "Compute Correspondence done." << std::endl;
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

    // 1. 设置一个全屏、无装饰的窗口作为DockSpace的“背景”
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos); // 使用 WorkPos 避开菜单栏（如果有）
    ImGui::SetNextWindowSize(viewport->WorkSize); // 使用 WorkSize 避开菜单栏
    ImGui::SetNextWindowViewport(viewport->ID);

    // 2. 设置窗口样式
    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    host_window_flags |= ImGuiWindowFlags_NoBackground; // 可选：如果您希望dockspace透明

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    // 3. 开始这个背景窗口
    ImGui::Begin("DockSpaceHost", nullptr, host_window_flags);

    ImGui::PopStyleVar(3);

    // 4. 创建 DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode; // 这使得中心节点透明，可以绘制3D场景
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    // 5. 结束背景窗口
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
    // 控制面板
    ImGui::Begin("Controls");
    ImGui::InputText("Polygon A Path", m_pathABuf, 128);
    ImGui::InputText("Polygon B Path", m_pathBBuf, 128);
    if (ImGui::Button("Load & Recompute")) {
        loadData();
    }
    
    ImGui::Separator();
    
    ImGui::SliderFloat("Time (t)", &m_interpTime, 0.0f, 1.0f);
    ImGui::DragFloat("Render Scale", &m_renderScale, 0.01f, 0.1f, 10.0f);

    ImGui::End();

    // 渲染视口
    ImGui::Begin("Viewport");
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    
    // 绘制多边形
    const auto& polyA = m_blender.getPolyA();
    const auto& polyB = m_blender.getPolyB();
    Polygon interpPoly = m_blender.getInterpolatedPolygon(m_interpTime);

    // 计算偏移量，使B在A的右侧
    ImVec2 offsetA = canvasPos;
    ImVec2 offsetB = ImVec2(canvasPos.x + 400 * m_renderScale, canvasPos.y);
    ImVec2 offsetInterp = ImVec2(canvasPos.x + 200 * m_renderScale, canvasPos.y + 400 * m_renderScale);

    // 红色: 源
    drawPolygon(drawList, polyA, IM_COL32(255, 0, 0, 255), offsetA, m_renderScale);
    // 蓝色: 目标
    drawPolygon(drawList, polyB, IM_COL32(0, 0, 255, 255), offsetB, m_renderScale);
    // 白色: 插值
    drawPolygon(drawList, interpPoly, IM_COL32(255, 255, 255, 255), offsetInterp, m_renderScale);

    ImGui::End();
}

void Application::drawPolygon(ImDrawList* drawList, const Polygon& poly, ImU32 color, const ImVec2& offset, float scale) const {
    if (poly.n == 0) return;

    for (int i = 0; i < poly.n; ++i) {
        const auto& v1_eig = poly.vertices[i];
        const auto& v2_eig = poly.vertices[(i + 1) % poly.n];

        ImVec2 p1 = ImVec2(offset.x + v1_eig.x() * scale, offset.y + v1_eig.y() * scale);
        ImVec2 p2 = ImVec2(offset.x + v2_eig.x() * scale, offset.y + v2_eig.y() * scale);

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