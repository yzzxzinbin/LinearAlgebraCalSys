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

// 新增：全局变量保存工作文件路径和TuiApp指针
#include <atomic>
#include <mutex>
std::string g_work_env_file_path;
TuiApp *g_app_ptr = nullptr;
std::mutex g_export_mutex;

// 新增：退出事件处理函数
#ifdef _WIN32
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    if (!g_work_env_file_path.empty() && g_app_ptr)
    {
        std::lock_guard<std::mutex> lock(g_export_mutex);
        // 新增：判断是否需要自动保存
        if (!g_app_ptr->getNoSavingOnExit())
        {
            g_app_ptr->exportVariablesOnExit(g_work_env_file_path);
        }
    }
    return FALSE; // 允许系统继续处理
}
#else
#include <signal.h>
#include <unistd.h>
void signal_handler(int signo)
{
    if (!g_work_env_file_path.empty() && g_app_ptr)
    {
        std::lock_guard<std::mutex> lock(g_export_mutex);
        // 新增：判断是否需要自动保存
        if (!g_app_ptr->getNoSavingOnExit())
        {
            g_app_ptr->exportVariablesOnExit(g_work_env_file_path);
        }
    }
    // 还原默认处理并重新发送信号以终止进程
    signal(signo, SIG_DFL);
    raise(signo);
}
#endif

int main()
{
    // 控制台标题设置
#ifdef _WIN32
    SetConsoleTitleA("LACSv1.3");
#else
    std::cout << "\033]0;LACSv1.3\007"; // 设置Linux终端标题
#endif

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
        if (!Terminal::init())
        {
            // 对于非Windows平台或Windows上虚拟终端处理失败的情况，可以记录日志
            // 但程序仍可尝试继续，只是ANSI颜色和控制序列可能不工作
            LOG_WARNING("主程序: 终端初始化失败，ANSI特性可能无法正常工作。");
        }

        // 运行启动界面
        LOG_INFO("显示启动界面");
        std::string currentDir;
        try
        {
            currentDir = std::filesystem::current_path().string();
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            LOG_ERROR("无法获取当前工作目录: " + std::string(e.what()));
            // 提供一个默认或备用路径，或者直接退出
            // 为简单起见，这里我们可能让它使用一个空字符串或 "."
            currentDir = ".";
        }

        StartupScreen startup("rebel.txt", currentDir);
        std::string selectedFile = startup.run(); // run() 内部会处理 raw mode

        if (!selectedFile.empty())
        {
            LOG_INFO("启动界面选择了文件: " + selectedFile);
            // 准备 import 命令。文件名应为简单标识符。
            std::string importCommand = "import \""; // 在文件名前添加 "import \""
            importCommand += selectedFile;           // 添加文件名
            importCommand += "\"";                   // 在文件名后添加 "
            initialCommandToRun = importCommand;
            g_work_env_file_path = selectedFile; // 记录工作环境文件路径
        }
        else
        {
            LOG_INFO("启动界面未选择文件或已退出 (ESC 或选择了 NULL 选项)");
            g_work_env_file_path.clear();
        }

        // 注册退出监听器(用于导出变量和历史)
#ifdef _WIN32
        SetConsoleCtrlHandler(CtrlHandler, TRUE);
#else
        struct sigaction act;
        act.sa_handler = signal_handler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        sigaction(SIGQUIT, &act, NULL);
        sigaction(SIGHUP, &act, NULL);
#endif

        // 创建并运行TUI应用程序
        LOG_INFO("创建TUI应用程序实例");
        TuiApp app(initialCommandToRun); // 将初始命令传递给TuiApp
        g_app_ptr = &app;                // 记录指针以便退出时访问
        LOG_INFO("开始运行TUI应用程序主循环");
        app.run();

        // 程序正常退出时判断是否需要自动保存
        if (!g_work_env_file_path.empty() && g_app_ptr && !g_app_ptr->getNoSavingOnExit())
        {
            std::lock_guard<std::mutex> lock(g_export_mutex);
            g_app_ptr->exportVariablesOnExit(g_work_env_file_path);
        }

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
