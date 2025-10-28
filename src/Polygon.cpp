#include "Polygon.h"
#include <fstream>
#include <iostream>
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

void Polygon::precomputeIntrinsics(){
    edge_lengths_prev.clear();
    edge_lengths_next.clear();
    angles.clear();

    edge_lengths_prev.resize(n);
    edge_lengths_next.resize(n);
    angles.resize(n);


    for(int i = 0; i < n; ++i){
        int prev_idx = (i == 0) ? n - 1 : i - 1;
        int next_idx = (i == n - 1) ? 0 : i + 1;

        const auto& v_prev = vertices[prev_idx];
        const auto& v_next = vertices[next_idx];
        const auto& v_curr = vertices[i];

        //计算相邻向量
        Eigen::Vector2d vec_prev = v_curr - v_prev;
        Eigen::Vector2d vec_next = v_next - v_curr;

        //存储边长
        edge_lengths_next[i] = vec_next.norm();
        edge_lengths_prev[i] = vec_prev.norm();

        //计算角度
        if(edge_lengths_next[i] < 1e-6 || edge_lengths_prev[i] < 1e-6){
            angles[i] = 0.0;
        }else{
            double dot = vec_prev.normalized().dot(vec_next.normalized());
            // 钳制 acos 的输入范围以避免NaN
            dot = std::clamp(dot, -1.0, 1.0);
            double angle_rad = std::acos(dot);
            angles[i] = angle_rad * 180.0 / M_PI; // 转换为度
        }

    }
}