#include "tui_app.h"
#include "tui_terminal.h" 
#include <iostream> // Still needed for cout in constructor (indirectly via LOG) or if any small utility remains
#include <string>   // For std::string in constants
#include <vector>   // For std::vector in constants
#include <deque>    // For std::deque in TuiApp member (history)
#include <memory>   // For std::make_unique in constructor
#include "../utils/logger.h" // For LOG_WARNING in constructor
#include "startup_screen.h" // 新增：包含启动屏幕头文件 (如果TuiApp负责调用它)

// 定义已知函数和命令列表
const std::vector<std::string> TuiApp::KNOWN_FUNCTIONS = {
    "transpose", "inverse", "inverse_gauss", "det", "det_expansion",
    "rank", "ref", "rref", "cofactor_matrix", "adjugate",
    "dot", "cross", "norm", "normalize", "diag", "solveq" // 添加 solveq
    // 可以根据实际情况添加更多函数
};

const std::vector<std::string> TuiApp::KNOWN_COMMANDS = {
    "help", "clear", "vars", "show", "exit", "steps", "new", "edit", "export", "import",
    "del", "rename", "csv" 
};


TuiApp::TuiApp(const std::string& initialCommand)  // 修改构造函数定义
    : historyIndex(0), running(true), 
      inStepDisplayMode(false), currentStep(0), totalSteps(0), isExpansionHistory(false),
      cursorPosition(0), 
      // 初始化新的编辑器指针为空
      matrixEditor(nullptr),
      suggestionBox(nullptr), // 初始化 suggestionBox
      stepDisplayStartRow(0), // 初始化步骤显示起始行
      initialCommandToExecute(initialCommand) // 保存初始命令
{
    // 初始化终端以支持ANSI转义序列
    if (!Terminal::init())
    {
        LOG_WARNING("终端初始化失败，部分功能可能无法正常工作");
    }

    auto [rows, cols] = Terminal::getSize();
    terminalRows = rows;
    terminalCols = cols;

    // 初始化 suggestionBox
    suggestionBox = std::make_unique<SuggestionBox>(terminalCols);

    // 计算UI布局
    inputRow = terminalRows - 2;
    // resultRow 成员变量现在表示内容区的下一行，由 clearResultArea 初始化/重置
    // this->resultRow = RESULT_AREA_CONTENT_START_ROW; // 将在 initUI -> drawResultArea -> clearResultArea 中设置

    // 初始状态消息
    statusMessage = "欢迎使用线性代数辅助计算系统! 输入 'help' 获取帮助。";
}
