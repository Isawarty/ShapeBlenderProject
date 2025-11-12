#include "ShapeBlender.h"
#include "Eigen/Core"
#include "Polygon.h"
#include <iostream>
#include <limits>
#include <ostream>
#include <vector>
#include <algorithm> // for std::sort
#include <functional>

// 用于M_PI
#define _USE_MATH_DEFINES
#include <cmath>


bool ShapeBlender::loadPolygons(const std::string& pathA, const std::string& pathB){
    if (!m_polyA.loadFromFile(pathA)) {
        std::cerr << "Failed to load Polygon A" << std::endl;
        return false;
    }
    if (!m_polyB.loadFromFile(pathB)) {
        std::cerr << "Failed to load Polygon B" << std::endl;
        return false;
    }

    std::cout << "Loading Polygons : A (" << m_polyA.n <<  "verts ) and B (" << m_polyB.n << " verts)." << std::endl;
    return true;
}


double ShapeBlender::compute_sim_t(int i_A, int i_B) const{
    double e1_0 = m_polyA.edge_e1_lengths[i_A];
    double e2_0 = m_polyA.edge_e2_lengths[i_A];
    double angle_0 = m_polyA.angles_curr[i_A];


    double e1_1 = m_polyB.edge_e1_lengths[i_B];
    double e2_1 = m_polyB.edge_e2_lengths[i_B];
    double angle_1 = m_polyB.angles_curr[i_B];

    // 计算 sim_t
    double term1_num = std::abs(e1_0 * e2_1 - e1_1 * e2_0);
    double term1_den = e1_0 * e2_1 + e1_1 * e2_0;
    
    double sim_edges = (term1_den < 1e-9) ? 1.0 : (1.0 - term1_num / term1_den);
    double sim_angles = 1.0 - std::abs(angle_0 - angle_1) / 360;

    return m_w1 * sim_edges + m_w2 * sim_angles;
}

double ShapeBlender::compute_smooth_a(int i_A, int i_B) const{
    double angle_A_curr = m_polyA.angles_curr[i_A];
    double angle_B_curr = m_polyB.angles_curr[i_B];
    if(angle_A_curr <= 1e-3 || angle_A_curr >= 179.9 || 
        angle_B_curr <= 1e-3 || angle_B_curr >= 179.9) return 0.0; //排除极端情况
    
    //----- 计算S相似度 -----
    //a. 边长相似度
    double e0_A = m_polyA.edge_e0_lengths[i_A];
    double e1_A = m_polyA.edge_e1_lengths[i_A];
    double e2_A = m_polyA.edge_e2_lengths[i_A];

    double e0_B = m_polyB.edge_e0_lengths[i_B];
    double e1_B = m_polyB.edge_e1_lengths[i_B];
    double e2_B = m_polyB.edge_e2_lengths[i_B];

    double diff_edges = std::abs(e0_A - e0_B) + std::abs(e1_A - e1_B) + std::abs(e2_A - e2_B);
    double sum_edges = (e0_A + e1_A + e2_A) + (e0_B + e1_B + e2_B);
    double sim_S_edges = (sum_edges < 1e-7) ? 1.0 : (1.0 - diff_edges / sum_edges);

    //b. 角度相似度
    double a0_A = m_polyA.angles_curr[i_A];
    double a1_A = m_polyA.angles_prev[i_A];
    double a2_A = m_polyA.angles_next[i_A];

    double a0_B = m_polyB.angles_curr[i_B];
    double a1_B = m_polyB.angles_prev[i_B];
    double a2_B = m_polyB.angles_next[i_B];

    
    double diff_angles = std::abs(a0_A- a0_B) + std::abs(a1_A- a1_B) + std::abs(a2_A- a2_B);
    double sim_S_angles = 1.0 - diff_angles / 180.0;
    sim_S_angles = std::max(0.0, sim_S_angles);

    double S = 0.5 * sim_S_angles + 0.5 * sim_S_edges;

    //----- 计算R旋转 -----
    //我们选 e2 的 (v_curr -> v_next) 边旋转
    const auto& v_curr_A = m_polyA.vertices[i_A];
    const auto& v_next_A = m_polyA.vertices[m_polyA.get_next_idx(i_A)];
    Eigen::Vector2d vec_A = v_next_A - v_curr_A;

    const auto& v_curr_B = m_polyB.vertices[i_B];
    const auto& v_next_B = m_polyB.vertices[m_polyA.get_next_idx(i_B)];
    Eigen::Vector2d vec_B = v_next_B - v_curr_B;

    double angle_vec_A = std::atan2(vec_A.y(), vec_A.x());
    double angle_vec_B = std::atan2(vec_B.y(), vec_B.x());

    double R_rad = angle_vec_B - angle_vec_A;
    R_rad = std::atan2(std::sin(R_rad), std::cos(R_rad));
    double R_deg = std::abs(R_rad * 180.0 / M_PI);

    double R = 1.0 - R_deg / 180.0;

    // 计算 A (面积比)
    double area_sum_triangles = m_polyA.cornerTriangle_areas[i_A] + m_polyB.cornerTriangle_areas[i_B];
    double area_sum_polygons = m_polyA.totalArea + m_polyB.totalArea;
    double A = (area_sum_polygons < 1e-9) ? 0.0 : area_sum_triangles / area_sum_polygons;

    return m_smooth_a_wS * S + m_smooth_a_wR * R + m_smooth_a_wA * A;
}

void ShapeBlender::computeCorrespondence(){
    int m = m_polyA.n;
    int n = m_polyB.n;

    //前置检查
    if (m == 0 || n == 0) return;
    if (m < n) {
        std::cerr << "Error in computeCorrespondence: Polygon A (" << m 
                  << ") must have >= vertices than Polygon B (" << n << ").\n"
                  << "Please reload polygons with the larger one as 'Polygon A'." << std::endl;
        return; // DP逻辑基于 m >= n
    }

    // 构建代价图(m x n)，
    //m_similarityGraph.resize(m, n);
    Eigen::MatrixXd costGraph(m, n); 
    
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            double sim = compute_sim_t(i, j);
            //m_similarityGraph(i, j) = sim;
            costGraph(i, j) = 1.0 - sim;
        }
    }

    //----- 初始化DP所需的结构 -----
    int n_double = 2 * n;
    Eigen::MatrixXd dpCost(m,n_double);
    Eigen::MatrixXd dpPath(m,n_double);// 0 = SE ,1 = S, 2 = Start

    //辅助函数
    auto get_cost = [&](int i, int j){
        return costGraph(i, j % n);
    };

    // ----- 初始化DP表 -----
    for(int j = 0; j < n_double; ++j){
        dpCost(0,j) = get_cost(0,j); 
        dpPath(0,j) = 2; // 起点 
    }

    for(int i = 1; i < m; ++i){
        dpCost(i,0) = dpCost(i - 1, 0) + get_cost(i,0); 
        dpPath(i,0) = 1;//1 = South 
    }

    // ----- 填充DP表 -----
    for(int i = 1; i < m; ++i){
        for(int j = 1; j < n_double; ++j){
            double costS = dpCost(i - 1,j); //南下
            double costSE = dpCost(i - 1, j - 1); //东南下


            if(costSE < costS){
                dpCost(i,j) = costSE + get_cost(i,j);
                dpPath(i,j) = 0;
            }else{
                dpCost(i,j) = costS + get_cost(i,j);
                dpPath(i,j) = 1;
            }
        }
    }

    // ----- 寻找最佳终点 -----
    double min_total_cost = std::numeric_limits<double>::max();
    int best_j_end = n - 1;

    for(int j = n - 1; j < n_double; ++j){
        if(dpCost(m - 1,j) < min_total_cost){
            min_total_cost = dpCost(m - 1, j);
            best_j_end = j;
        }
    }

    // ----- 回溯路径 -----
    m_correspondence.clear();
    int i = m-1;
    int j = best_j_end;
    
    while (i >= 0 && j >=0) {
        m_correspondence[i] = j % n;
        int path = dpPath(i,j);
        if(path == 0){
            i--;
            j--;
        }else if (path == 1) {
            i--;
        }else if(path == 2){
            break;
        }    
    }

    std::cout << "  - Best path end index (j_end) = " << best_j_end << " (maps to B[" << (best_j_end % n) << "])" << std::endl;
    std::cout << "  - Min total cost = " << min_total_cost << std::endl;
}

void ShapeBlender::findOptimalBasis(){
    if(m_correspondence.size() < 3){
        std::cerr << "Error: Correspondence map has < 3 pairs. Cannot find basis." << std::endl;
        // 设置一个默认的、可能不好的基
        m_basis.polyA_indices = {0, m_polyA.n / 3, 2 * m_polyA.n / 3};
        m_basis.polyB_indices = {0, m_polyB.n / 3, 2 * m_polyB.n / 3};
        return;
    }

    //创建一个向量存储所有的 smooth_a 的值
    std::vector<SmoothPair> smooth_pairs;
    smooth_pairs.reserve(m_correspondence.size());

    //遍历一次计算所有smooth_a的值
    for(auto const& [i_A, i_B] : m_correspondence){
        double s_a = compute_smooth_a(i_A, i_B);
        if(s_a > 1e-7) smooth_pairs.push_back({s_a, i_A, i_B});
    }

    if (smooth_pairs.size() < 3) {
         std::cerr << "Error: Not enough valid pairs (<3) after computing smooth_a." << std::endl;
        // 设置一个默认的、可能不好的基
        m_basis.polyA_indices = {0, m_polyA.n / 3, 2 * m_polyA.n / 3};
        m_basis.polyB_indices = {0, m_polyB.n / 3, 2 * m_polyB.n / 3};
        return;
    }

    // 我们使用 std::greater<> 来进行降序排序
    std::sort(smooth_pairs.begin(), smooth_pairs.end(), std::greater<SmoothPair>());

    //取排序后的前 3 个
    const auto& best_1 = smooth_pairs[0];
    const auto& best_2 = smooth_pairs[1];
    const auto& best_3 = smooth_pairs[2];

    //存储最佳基
    m_basis.polyA_indices[0] = best_1.i_A;
    m_basis.polyB_indices[0] = best_1.i_B;
    
    m_basis.polyA_indices[1] = best_2.i_A;
    m_basis.polyB_indices[1] = best_2.i_B;

    m_basis.polyA_indices[2] = best_3.i_A;
    m_basis.polyB_indices[2] = best_3.i_B;

    double max_smooth_t = best_1.smooth_a_value * best_2.smooth_a_value * best_3.smooth_a_value;

    std::cout << "Found optimal basis with smooth_t = " << max_smooth_t << std::endl;
} 

Eigen::Vector2d ShapeBlender::getLocalCoords(const Eigen::Vector2d& p, 
                                             const Eigen::Vector2d& a, 
                                             const Eigen::Vector2d& b, 
                                             const Eigen::Vector2d& c) const{
    Eigen::Matrix2d T;
    T.col(0) = a - b;
    T.col(1) = c - b;
    Eigen::Vector2d rhs = p - b;   

    // 使用 .solve() 更稳定
    //  我们的 smooth_a 检查旨在防止这种情况）
    return T.colPivHouseholderQr().solve(rhs);    
} 

Eigen::Vector2d ShapeBlender::getWorldCoords(const Eigen::Vector2d& uv, 
                                   const Eigen::Vector2d& a, 
                                   const Eigen::Vector2d& b, 
                                   const Eigen::Vector2d& c) const{

    return b + uv[0] * (a - b) + uv[1] * (c - b);
}

Polygon ShapeBlender::getInterpolatedPolygon(float t) const {
    
}