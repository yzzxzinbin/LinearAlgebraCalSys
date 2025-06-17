#include <iostream>
#include <string>
#include "tui/tui_app.h"
#include "utils/logger.h"
#include "tui/startup_screen.h" // 新增：包含启动界面头文件
#include "tui/tui_terminal.h"   // 新增：包含Terminal以便在main中初始化
#include <filesystem>           // 新增：包含filesystem以获取当前路径

#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
    std::string initialCommandToRun = "";
    try
    {
        // 初始化日志系统
        Logger::getInstance()->setLogLevel(LogLevel::DEBUG); 
        LOG_INFO("应用程序启动前置初始化");
#ifdef _WIN32
        // 设置控制台编码为UTF-8
        SetConsoleCP(65001);       
        SetConsoleOutputCP(65001); 
        LOG_INFO("Windows控制台编码设置为UTF-8");
#endif
        // 初始化终端以支持ANSI转义序列 (TuiApp也会做，但StartupScreen需要先用)
        if (!Terminal::init()) {
            // 对于非Windows平台或Windows上虚拟终端处理失败的情况，可以记录日志
            // 但程序仍可尝试继续，只是ANSI颜色和控制序列可能不工作
            LOG_WARNING("主程序: 终端初始化失败，ANSI特性可能无法正常工作。");
        }

        // 运行启动界面
        LOG_INFO("显示启动界面");
        std::string currentDir;
        try {
            currentDir = std::filesystem::current_path().string();
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR("无法获取当前工作目录: " + std::string(e.what()));
            // 提供一个默认或备用路径，或者直接退出
            // 为简单起见，这里我们可能让它使用一个空字符串或 "."
            currentDir = "."; 
        }
        
        StartupScreen startup("rebel.txt", currentDir); 
        std::string selectedFile = startup.run(); // run() 内部会处理 raw mode

        if (!selectedFile.empty()) {
            LOG_INFO("启动界面选择了文件: " + selectedFile);
            // 准备 import 命令。文件名应为简单标识符。
            std::string importCommand = "import \""; // 在文件名前添加 "import \""
            importCommand += selectedFile;      // 添加文件名
            importCommand += "\"";               // 在文件名后添加 "
            initialCommandToRun = importCommand;
        } else {
            LOG_INFO("启动界面未选择文件或已退出 (ESC 或选择了 NULL 选项)");
        }

        // 创建并运行TUI应用程序
        LOG_INFO("创建TUI应用程序实例");
        TuiApp app(initialCommandToRun); // 将初始命令传递给TuiApp
        LOG_INFO("开始运行TUI应用程序主循环");
        app.run();

        LOG_INFO("应用程序正常退出");
    }
    catch (const std::exception &e)
    {
        if (Logger::getInstance())
        {
            LOG_FATAL("程序异常终止: " + std::string(e.what()));
        }
        else
        {
            std::cerr << "程序异常终止: " << e.what() << std::endl;
        }
        return 1;
    }
    catch (...)
    {
        if (Logger::getInstance())
        {
            LOG_FATAL("程序发生未知异常而终止");
        }
        else
        {
            std::cerr << "程序发生未知异常而终止" << std::endl;
        }
        return 2;
    }

    return 0;
}
