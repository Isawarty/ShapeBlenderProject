#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>

/**
 * @brief 存储多边形顶点及其预计算的内在属性。
 * 它负责从Python脚本生成的JSON文件中加载顶点数据。
 * 加载后，它立即调用 precomputeIntrinsics() 来计算每个顶点的
 * 相邻边长和角度，这些是"模糊相似图"所必需的。
 */
struct Polygon {
    std::vector<Eigen::Vector2d> vertices;
    int n = 0; // 顶点数
    double totalArea = 0.0;//多边形面积
    bool signFlag;

    // --- "角三角形" (v_prev, v_curr, v_next) 的属性 ---

    // 1. 边长
    std::vector<double> edge_e1_lengths; // 边 v_prev -> v_curr 
    std::vector<double> edge_e2_lengths; // 边 v_curr -> v_next
    std::vector<double> edge_e0_lengths; // 边 v_prev -> v_next
    
    // 2. 角度 
    std::vector<double> angles_curr;       // 在 v_curr 处的角
    std::vector<double> angles_prev;       // 在 v_prev 处的角
    std::vector<double> angles_next;       // 在 v_next 处的角

    // 3. 面积
    std::vector<double> cornerTriangle_areas; // 角三角形的面积


    // --- 辅助函数 ---
    inline int get_prev_idx(int i) const {
        return (i == 0) ? n - 1 : i - 1;
    }
    inline int get_next_idx(int i) const {
        return (i == n - 1) ? 0 : i + 1;
    }


    /**
     * @brief 从JSON文件加载顶点。
     * @param filepath 指向JSON文件的路径。
     * @return 成功加载返回 true。
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief 计算并填充 edge_lengths 和 angles 向量。
     * 遍历所有顶点，计算连接前一个、当前和后一个顶点的
     * 两个向量。存储它们的长度，并计算它们之间的夹角。
     */
    void precomputeIntrinsics();

private:
    /**
     * @brief 计算并返回一个三角形(p1, p2, p3)的三个角（单位：度）。
     * @param p1 顶点1
     * @param p2 顶点2
     * @param p3 顶点3
     * @return 一个包含三个角 {angle_at_p1, angle_at_p2, angle_at_p3} 的 std::array。
     */
    std::array<double, 3> computeTriangleAngles(
        const Eigen::Vector2d& p1, 
        const Eigen::Vector2d& p2, 
        const Eigen::Vector2d& p3) const;
};