#include "Application.h"
#include <iostream>

/**
 * @brief 程序入口点。
 * 思路：保持 main 简洁。它只负责创建、初始化和运行 Application。
 * 所有的工作都在 Application 类中完成。
 */
int main() {
    Application app;

    if (app.init() != 0) {
        std::cerr << "Failed to initialize application." << std::endl;
        return -1;
    }

    //app.run();

    return 0;
}