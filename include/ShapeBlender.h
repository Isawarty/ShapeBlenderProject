#pragma once

#include "Polygon.h"
#include <map>
#include <array>

/**
 * @brief 存储用于仿射插值的最佳基（三对顶点）。
 */
struct AffineBasis {
    std::array<int, 3> polyA_indices;
    std::array<int, 3> polyB_indices;
};


/**
 * @brief 封装模糊形状渐变算法的核心逻辑。
 * 协调整个渐变过程。
 * 1. loadPolygons()：加载两个多边形。
 * 2. computeCorrespondence()：构建相似图并运行DP，填充 m_correspondence。
 * 3. findOptimalBasis()：遍历 m_correspondence，找到“最好”的三对顶点 。
 * 4. getInterpolatedPolygon()：使用 m_basis 和 m_correspondence 计算中间帧 。
 */
class ShapeBlender {
    public:
        ShapeBlender() = default;

        /** 
        * @brief 加载源多边形和目标多边形
        */
        bool loadPolygons(const std::string& pathA, const std::string& pathB);


        /**
        * @brief 计算顶点对应关系。
        * 思路：构建一个 (m x n) 的“模糊相似图” G 。
        * G[i][j] = sim_t(A[i], B[j])。
        * 然后在代价图 (1 - G) 上运行动态规划，找到最短路径。
        * 这条路径就是顶点对应关系。
        */
        void computeCorrespondence();


        /**
        * @brief 寻找最佳仿射基。
        * 思路：遍历已建立的对应关系 m_correspondence，
        * 找到三对顶点，它们的"smooth_t"函数值最大。
        * (简化：我们使用 sim_t 作为 smooth_a 的代理)。
        */
        void findOptimalBasis();


        /**
        * @brief 计算并返回给定t值的插值多边形。
        * 思路：按照 [cite: 421-426] 中的插值方法。
        * 1. 线性插值三个基顶点 A, B, C 得到 A(t), B(t), C(t)。
        * 2. 遍历所有对应点 (X1, X2)。
        * 3. 计算 X1 相对于 (A1, B1, C1) 的局部坐标 (u1, v1) [cite: 394]。
        * 4. 计算 X2 相对于 (A2, B2, C2) 的局部坐标 (u2, v2) [cite: 395]。
        * 5. 插值局部坐标 (u(t), v(t)) [cite: 423, 425]。
        * 6. 将 (u(t), v(t)) 转换回世界坐标，使用 (A(t), B(t), C(t)) [cite: 426]。
        */
        Polygon getInterpolatedPolygon(float t) const;

        // 访问器，以便Application可以绘制它们
        const Polygon& getPolyA() const { return m_polyA; }
        const Polygon& getPolyB() const { return m_polyB; }

    private:
    Polygon m_polyA; // 源
    Polygon m_polyB; // 目标
    Eigen::MatrixXd m_similarityGraph; // 映射 m_polyA 的索引 -> m_polyB 的索引

    std::map<int, int> m_correspondence;
    AffineBasis m_basis;

    // sim_t 的权重
    double m_w1 = 0.5;
    double m_w2 = 0.5;

    /**
     * @brief 计算两个“多边形角”之间的三角形相似度 (sim_t)。
     */
    double compute_sim_t(int i_A, int i_B) const;
    
    /**
     * @brief 计算点 P 在由 (A, B, C) 定义的局部坐标系中的 (u, v) 坐标。
     * 求解 P = B + u(A-B) + v(C-B)。
     */
    Eigen::Vector2d getLocalCoords(const Eigen::Vector2d& p, 
                                   const Eigen::Vector2d& a, 
                                   const Eigen::Vector2d& b, 
                                   const Eigen::Vector2d& c) const;

    /**
     * @brief 将局部坐标 (u, v) 转换回世界坐标。
     * X = B + u(A-B) + v(C-B)。
     */
    Eigen::Vector2d getWorldCoords(const Eigen::Vector2d& uv, 
                                   const Eigen::Vector2d& a, 
                                   const Eigen::Vector2d& b, 
                                   const Eigen::Vector2d& c) const;
};