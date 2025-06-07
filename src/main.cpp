#include <iostream>
#include <string>
#include "tui/tui_app.h"
#include "utils/logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
    try
    {
        // 初始化日志系统
        Logger::getInstance()->setLogLevel(LogLevel::DEBUG); // 开发阶段设为DEBUG
        LOG_INFO("应用程序启动");
#ifdef _WIN32
        // 设置控制台编码为UTF-8
        SetConsoleCP(65001);       // 设置控制台输入为UTF-8编码
        SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
        LOG_INFO("控制台编码设置为UTF-8");
#endif

        // 创建并运行TUI应用程序
        LOG_INFO("创建TUI应用程序");
        TuiApp app;
        LOG_INFO("开始运行TUI应用程序");
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
