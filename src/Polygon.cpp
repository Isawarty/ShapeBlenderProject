#include "Polygon.h"
#include <fstream>
#include <iostream>
#include <numeric>
#include "../lib/json/nlohmann/json.hpp"
#include <algorithm>

//for Pi
#define _USE_MATH_DEFINES
#include <math.h>

bool Polygon::loadFromFile(const std::string& filepath){
    vertices.clear();
    
    std::ifstream f(filepath);
    if (!f.is_open()) {
        std::cerr << "Error: Failed to open polygon file: " << filepath << std::endl;
        return false;
    }

    try{
        nlohmann::json data = nlohmann::json::parse(f);
        for(const auto& item : data){
            vertices.push_back(Eigen::Vector2d(item[0].get<double>(), item[1].get<double>()));
        }
    } catch(nlohmann::json::parse_error& e){
        std::cerr << "Error: Failed to parse JSON file: " << filepath << "\n" << e.what() << std::endl;
        return false;
    }

    n = vertices.size();
    if(n < 3){
        std::cerr << "Error: Polygon must have at least 3 vertices." << std::endl;
        return false;
    }

    precomputeIntrinsics();
    return true;

}

std::array<double, 3> Polygon::computeTriangleAngles(
    const Eigen::Vector2d& p1, 
    const Eigen::Vector2d& p2, 
    const Eigen::Vector2d& p3) const 
{
    Eigen::Vector2d v12 = p2 - p1; double e12 = v12.norm();
    Eigen::Vector2d v13 = p3 - p1; double e13 = v13.norm();
    Eigen::Vector2d v23 = p3 - p2; double e23 = v23.norm();

    // 使用 acos 和点积计算角度
    double angle1_rad = 0.0, angle2_rad = 0.0, angle3_rad = 0.0;

    if (e12 > 1e-6 && e13 > 1e-6) {
        double dot = v12.normalized().dot(v13.normalized());
        angle1_rad = std::acos(std::clamp(dot, -1.0, 1.0));
    }
    
    if (e12 > 1e-6 && e23 > 1e-6) {
        double dot = (-v12).normalized().dot(v23.normalized());
        angle2_rad = std::acos(std::clamp(dot, -1.0, 1.0));
    }

    angle3_rad = M_PI - angle1_rad - angle2_rad;
    
    // 转换为度
    return {
        angle1_rad * 180.0 / M_PI, 
        angle2_rad * 180.0 / M_PI, 
        angle3_rad * 180.0 / M_PI
    };
}


void Polygon::precomputeIntrinsics(){
    // 调整所有向量的大小
    edge_e0_lengths.resize(n);
    edge_e1_lengths.resize(n);
    edge_e2_lengths.resize(n);
    angles_curr.resize(n);
    angles_prev.resize(n);
    angles_next.resize(n);
    cornerTriangle_areas.resize(n);

    totalArea = 0.0;

   for (int i = 0; i < n; ++i) {
        //获取顶点
        const auto& v_prev = vertices[get_prev_idx(i)];
        const auto& v_curr = vertices[i];
        const auto& v_next = vertices[get_next_idx(i)];

        edge_e1_lengths[i] = (v_curr - v_prev).norm(); // e1 (prev -> curr)
        edge_e2_lengths[i] = (v_next - v_curr).norm(); // e2 (curr -> next)
        edge_e0_lengths[i] = (v_next - v_prev).norm(); // e0 (prev -> next)

        //计算 "角三角形" 的三个角
        auto tri_angles = computeTriangleAngles(v_prev, v_curr, v_next);
        angles_prev[i] = tri_angles[0]; // 角 at v_prev
        angles_curr[i] = tri_angles[1]; // 角 at v_curr
        angles_next[i] = tri_angles[2]; // 角 at v_next

        //计算 "角三角形" 的面积
        //使用 0.5 * |v1.x*v2.y - v1.y*v2.x|叉积的模
        Eigen::Vector2d v1 = v_curr - v_prev;
        Eigen::Vector2d v2 = v_next - v_prev;
        cornerTriangle_areas[i] = 0.5 * std::abs(v1.x() * v2.y() - v1.y() * v2.x());
    }

    //用鞋带公式，计算多边形的总面积
     for (int i = 0; i < n; ++i)
     {
        const auto& p1 = vertices[i];
        const auto& p2 = vertices[get_next_idx(i)];
        totalArea+= (p1.x() * p2.y() - p1.y() * p2.x());
     }

     signFlag = (totalArea < 0) ? -1 : 1;
     totalArea = 0.5 * std::abs(totalArea);
     
}