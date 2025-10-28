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

    // 预计算的内在属性
    std::vector<double> edge_lengths_prev; // 边 P[i-1] -> P[i] 的长度
    std::vector<double> edge_lengths_next; // 边 P[i] -> P[i+1] 的长度
    std::vector<double> angles;            // 在 P[i] 处的角度 (单位：度)

    /**
     * @brief 从JSON文件加载顶点。
     * @param filepath 指向JSON文件的路径。
     * @return 成功加载返回 true。
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief 计算并填充 edge_lengths 和 angles 向量。
     * 思路：遍历所有顶点，计算连接前一个、当前和后一个顶点的
     * 两个向量。存储它们的长度，并计算它们之间的夹角。
     */
    void precomputeIntrinsics();
};