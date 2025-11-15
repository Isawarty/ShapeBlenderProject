#include "ShapeBlender.h"
#include "Eigen/Core"
#include "Polygon.h"
#include <iostream>
#include <iterator>
#include <limits>
#include <ostream>
#include <utility>
#include <vector>
#include <algorithm> // for std::sort
#include <functional>
#include "Eigen/LU"

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

    if (m_polyA.signFlag != m_polyB.signFlag) {
        std::cout << "Winding order mismatch detected. Reversing Polygon B." << std::endl;
        //反转 B 的顶点列表
        std::reverse(m_polyB.vertices.begin(), m_polyB.vertices.end());
        //重新计算 B 的所有内在属性
        m_polyB.precomputeIntrinsics();
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
    double term1_num = std::fabs(e1_0 * e2_1 - e1_1 * e2_0);
    double term1_den = e1_0 * e2_1 + e1_1 * e2_0;
    
    double sim_edges = (term1_den < 1e-9) ? 1.0 : (1.0 - term1_num / term1_den);
    double sim_angles = 1.0 - std::fabs(angle_0 - angle_1) / 360.0;

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

    double diff_edges = std::fabs(e0_A - e0_B) + std::fabs(e1_A - e1_B) + std::fabs(e2_A - e2_B);
    double sum_edges = (e0_A + e1_A + e2_A) + (e0_B + e1_B + e2_B);
    double sim_S_edges = (sum_edges < 1e-7) ? 1.0 : (1.0 - diff_edges / sum_edges);

    //b. 角度相似度
    double a0_A = m_polyA.angles_curr[i_A];
    double a1_A = m_polyA.angles_prev[i_A];
    double a2_A = m_polyA.angles_next[i_A];

    double a0_B = m_polyB.angles_curr[i_B];
    double a1_B = m_polyB.angles_prev[i_B];
    double a2_B = m_polyB.angles_next[i_B];

    
    double diff_angles = std::fabs(a0_A- a0_B) + std::fabs(a1_A- a1_B) + std::fabs(a2_A- a2_B);
    double sim_S_angles = 1.0 - diff_angles / 180.0;
    sim_S_angles = std::max(0.0, sim_S_angles);

    double S = 0.5 * sim_S_angles + 0.5 * sim_S_edges;

    //----- 计算R旋转 -----
    //我们选 e2 的 (v_curr -> v_next) 边旋转
    const auto& v_curr_A = m_polyA.vertices[i_A];
    const auto& v_next_A = m_polyA.vertices[m_polyA.get_next_idx(i_A)];
    Eigen::Vector2d vec_A = v_next_A - v_curr_A;

    const auto& v_curr_B = m_polyB.vertices[i_B];
    const auto& v_next_B = m_polyB.vertices[m_polyB.get_next_idx(i_B)];
    Eigen::Vector2d vec_B = v_next_B - v_curr_B;

    double angle_vec_A = std::atan2(vec_A.y(), vec_A.x());
    double angle_vec_B = std::atan2(vec_B.y(), vec_B.x());

    double R_rad = angle_vec_B - angle_vec_A;
    R_rad = std::atan2(std::sin(R_rad), std::cos(R_rad));
    double R_deg = std::fabs(R_rad * 180.0 / M_PI);

    double R = 1.0 - R_deg / 180.0;

    // ----- 计算 A (面积比) -----
    double area_sum_triangles = m_polyA.cornerTriangle_areas[i_A] + m_polyB.cornerTriangle_areas[i_B];
    double area_sum_polygons = m_polyA.totalArea + m_polyB.totalArea;
    double A = (area_sum_polygons < 1e-9) ? 0.0 : area_sum_triangles / area_sum_polygons;

    return m_smooth_a_wS * S + m_smooth_a_wR * R + m_smooth_a_wA * A;
}

void ShapeBlender::computeCorrespondence(int manual_k){
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
    Eigen::MatrixXd costGraph(m, n); 
    
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            double sim = compute_sim_t(i, j);
            costGraph(i, j) = 1.0 - sim;
        }
    }


    double min_total_cost = std::numeric_limits<double>::max();
    int best_k = 0; // 最佳的 A 的起始顶点
    
    //----- 将DP逻辑抽象为一个辅助函数 -----
    auto run_single_dp_pass = [&](int k) -> std::pair<double, Eigen::MatrixXi> {
        
        Eigen::MatrixXd dpCost(m, n);
        Eigen::MatrixXi dpPath(m, n); // 0=SE, 1=S, 2=Start

        auto get_cost = [&](int i, int j) {
            return costGraph((i + k) % m, j); // (i+k)%m 是 A 中的真实索引
        };

        // 初始化
        dpCost(0, 0) = get_cost(0, 0);
        dpPath(0, 0) = 2; // Start
        
        for(int i = 1; i < m; ++i){
            dpCost(i, 0) = dpCost(i - 1, 0) + get_cost(i, 0); 
            dpPath(i, 0) = 1; // 1 = South
        }
        for(int j = 1; j < n; ++j){
            // (i=0, j>0) 的路径是不可能的，因为这需要 'East' 移动
            dpCost(0, j) = std::numeric_limits<double>::infinity();
            dpPath(0, j) = -1; 
        }

        // 填充DP表
        for(int i = 1; i < m; ++i){
            for(int j = 1; j < n; ++j){
                
                // i 必须大于等于 j
                if (j > i) {
                    dpCost(i, j) = std::numeric_limits<double>::infinity();
                    dpPath(i, j) = -1;
                    continue;
                }

                double costS = dpCost(i - 1, j);    // South
                double costSE = dpCost(i - 1, j - 1); // Southeast

                //我们只允许 S 和 SE
                if (costSE <= costS) { 
                    dpCost(i, j) = costSE + get_cost(i, j);
                    dpPath(i, j) = 0; // 0 = SE
                } else {
                    dpCost(i, j) = costS + get_cost(i, j);
                    dpPath(i, j) = 1; // 1 = S
                }
            }
        }
        
        return { dpCost(m - 1, n - 1), dpPath };
    };


    if (manual_k == -1) {
        // --- 自动模式 ---
        // (遍历 A 的 m 个起始点)
        std::cout << "Running Auto-Search for best k (O(m^2*n))..." << std::endl;
        for (int k = 0; k < m; ++k) {
            double current_total_cost = run_single_dp_pass(k).first; // 只获取代价
            if (current_total_cost < min_total_cost) {
                 min_total_cost = current_total_cost;
                 m_bestK = k; 
            }
        }
        std::cout << "  - (Auto-Search) Best start vertex (k) = " << m_bestK << std::endl;

    } else {
        // --- 手动模式 ---
        // (只运行一次，使用用户指定的 k)
        std::cout << "Running Manual-Search for k = " << manual_k << " (O(m*n))..." << std::endl;
        m_bestK = manual_k;
    }

    // -----------------------------------------------------------------
    // 重走 'best_k'
    std::pair<double, Eigen::MatrixXi> result = run_single_dp_pass(m_bestK);
    if(manual_k != -1) min_total_cost = result.first;
    Eigen::MatrixXi dpPath = result.second; 

    m_correspondence.clear();
    int i = m-1;
    int j = n-1;
    
    while (i >= 0 && j >= 0) {
        // 将 "窗口" 索引 (i, j) 转换回 "真实" 索引
        m_correspondence[(i + best_k) % m] = j;
        
        int path = dpPath(i,j);
        
        if (path == 2) { // Start
            break; 
        }
        
        if(path == 0){ // SE
            i--;
            j--;
        } else { // S
            i--;
        }
    }
    std::cout << "  - i = " << i << "; j = " << j << std::endl;
    std::cout << "  - Best path start index (A_start) = " << best_k << " (maps to B[ 0 ])" << std::endl;
    std::cout << "  - Min total cost = " << min_total_cost << std::endl;
    std::cout << "  - Correspondence map size: " << m_correspondence.size() << " (should be " << m << ")" << std::endl;
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
    return T.colPivHouseholderQr().solve(rhs);    
} 

Eigen::Vector2d ShapeBlender::getWorldCoords(const Eigen::Vector2d& uv, 
                                   const Eigen::Vector2d& a, 
                                   const Eigen::Vector2d& b, 
                                   const Eigen::Vector2d& c) const{

    return b + uv[0] * (a - b) + uv[1] * (c - b);
}

Polygon ShapeBlender::getInterpolatedPolygon(float t) const {
    Polygon resultPoly;
    if(m_polyA.n == 0 || m_polyB.n == 0) return resultPoly;

    //获取基顶点
    Eigen::Vector2d A1 = m_polyA.vertices[m_basis.polyA_indices[0]];
    Eigen::Vector2d B1 = m_polyA.vertices[m_basis.polyA_indices[1]];
    Eigen::Vector2d C1 = m_polyA.vertices[m_basis.polyA_indices[2]];
   
    Eigen::Vector2d A2 = m_polyB.vertices[m_basis.polyB_indices[0]];
    Eigen::Vector2d B2 = m_polyB.vertices[m_basis.polyB_indices[1]];
    Eigen::Vector2d C2 = m_polyB.vertices[m_basis.polyB_indices[2]];
    
    //获得A和T
    Eigen::Matrix<double, 6, 6> M_solve;
    Eigen::Matrix<double, 6, 1> R_solve;

    M_solve <<  A1.x(), A1.y(), 0, 0, 1, 0,
                0, 0, A1.x(), A1.y(), 0, 1,
                B1.x(), B1.y(), 0, 0, 1, 0,
                0, 0, B1.x(), B1.y(), 0, 1,
                C1.x(), C1.y(), 0, 0, 1, 0,
                0, 0, C1.x(), C1.y(), 0, 1;

    R_solve << A2.x(), A2.y(), B2.x(), B2.y(), C2.x(), C2.y(); 
    Eigen::Matrix<double, 6, 1> X_solve = M_solve.colPivHouseholderQr().solve(R_solve);

    Eigen::Matrix2d A_mat;
    A_mat << X_solve(0), X_solve(1), X_solve(2), X_solve(3);

    Eigen::RowVector2d T_vec;
    T_vec << X_solve(4), X_solve(5);

    // 下面开始分解 A 矩阵
    Eigen::Matrix2d B_mat;
    Eigen::Matrix2d C_mat;
    double theta = 0.0;

    double det_A = A_mat.determinant();
    double sign_detA = (det_A < 0) ? -1.0 : 1.0;

    Eigen::Matrix2d cofactor_matrix;
    cofactor_matrix << A_mat(1,1), -A_mat(1,0),  
                       -A_mat(0,1), A_mat(0,0); 
    B_mat = A_mat + sign_detA * cofactor_matrix;

    //归一化 B
    Eigen::Vector2d b1 = B_mat.col(0);
    b1.normalize();
    B_mat << b1.x(), -b1.y(), 
             b1.y(),  b1.x();
    
    theta = std::atan2(b1.y(), b1.x());
    C_mat = B_mat.inverse() * A_mat;


    // 插值并且重新组合
    double t_f = static_cast<double>(t);
    Eigen::Matrix2d A_t;
    Eigen::RowVector2d T_t;

    //插值旋转 B
    double theta_t = t_f * theta;
    Eigen::Matrix2d B_t;
    B_t << std::cos(theta_t), -std::sin(theta_t),
           std::sin(theta_t),  std::cos(theta_t);

    //插值缩放 C
    Eigen::Matrix2d C_t = t_f * C_mat;

    //插值平移 T
    T_t = t_f * T_vec;
    
    //重新组合 A
    Eigen::Matrix2d I = Eigen::Matrix2d::Identity();
    A_t = (1.0-t_f) * I + B_t * C_t;

    //----- 应用变化 ----------
    Eigen::Vector2d A_t_final = A_t * A1 + T_t.transpose();
    Eigen::Vector2d B_t_final = A_t * B1 + T_t.transpose();
    Eigen::Vector2d C_t_final = A_t * C1 + T_t.transpose();

    resultPoly.vertices.resize(m_polyA.n);
    resultPoly.n = m_polyA.n;

    for(int i_A = 0; i_A < m_polyA.n; ++i_A){
        const Eigen::Vector2d& X1 = m_polyA.vertices[i_A];

        if(m_correspondence.count(i_A)){
            int i_B = m_correspondence.at(i_A);
            const Eigen::Vector2d& X2 = m_polyB.vertices[i_B];

            Eigen::Vector2d uv1 = getLocalCoords(X1, A1, B1, C1);
            Eigen::Vector2d uv2 = getLocalCoords(X2, A2, B2, C2);
            Eigen::Vector2d uv_t = (1.0-t_f) * uv1 + t_f * uv2; 

            resultPoly.vertices[i_A] = getWorldCoords(uv_t, A_t_final, B_t_final, C_t_final);
        }else {
            std::cerr << "m_correspondense[" << i_A << "] Cannot Find i_B"  << std::endl; 
        }

    }
    return resultPoly;
}
