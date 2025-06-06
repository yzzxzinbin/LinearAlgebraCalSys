#include <iostream>
#include <string>
#include "tui/tui_app.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
    try {
        // 设置控制台编码为UTF-8
        #ifdef _WIN32
        SetConsoleCP(65001);       // 设置控制台输入为UTF-8编码
        SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
        #endif
        
        std::cout << "启动线性代数计算系统..." << std::endl;
        
        // 创建并运行TUI应用程序
        TuiApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "程序异常终止: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "程序发生未知异常而终止" << std::endl;
        return 2;
    }
    
    return 0;
}
