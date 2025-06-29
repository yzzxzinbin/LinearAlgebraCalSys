#pragma once

#include <string>
#include <deque>
#include <memory>
#include <sstream>
#include <vector> // Required for KNOWN_FUNCTIONS/COMMANDS
#include "../grammar/grammar_interpreter.h"
#include "../grammar/grammar_tokenizer.h"
#include "../grammar/grammar_parser.h"
#include "../operation_step.h"
#include "../determinant_expansion.h"
#include "tui_terminal.h"
#include "enhanced_matrix_editor.h"
#include "enhanced_variable_viewer.h"
#include "enhanced_help_viewer.h"  // 新增：帮助查看器头文件
#include "tui_suggestion_box.h" // 新增：包含候选框头文件
#include "../utils/logger.h" // 新增：包含日志记录头文件
#include "../utils/tui_utils.h" // 新增：包含TUI工具函数头文件

// 最大历史记录数量
const int MAX_HISTORY = 50;

// 输出区域的布局常量
const int RESULT_AREA_TITLE_ROW = 2; // "输出区域:" 标题所在的行 (0-indexed)
const int RESULT_AREA_CONTENT_START_ROW = RESULT_AREA_TITLE_ROW + 1; // 实际内容开始的第一行
const int MATRIX_EDITOR_CELL_WIDTH = 8; // 矩阵编辑器单元格宽度

class TuiApp {
private:
    // 窗口和UI状态
    int terminalRows;
    int terminalCols;
    int inputRow;
    int resultRow; // 表示结果区域下一可用行
    size_t cursorPosition; // 新增：跟踪光标在currentInput中的位置
    int stepDisplayStartRow; // 新增: 步骤显示开始的行
    std::string initialCommandToExecute; // 新增：存储启动时要执行的命令
    
    // 命令和历史记录
    std::string currentInput;
    std::string tempInputBuffer; // 新增：用于在历史导航时暂存当前输入
    std::deque<std::string> history;
    size_t historyIndex;
    
    // 程序状态
    bool running;
    std::string statusMessage;

    // 新增：是否退出时不自动保存
    bool noSavingOnExit = false;
    
    // 解释器
    Interpreter interpreter;
    
    // 步骤显示模式相关
    bool inStepDisplayMode;
    size_t currentStep;
    size_t totalSteps;
    OperationHistory currentHistory;
    ExpansionHistory currentExpHistory;
    bool isExpansionHistory;

    // 新增：增强型矩阵编辑器实例
    std::unique_ptr<EnhancedMatrixEditor> matrixEditor;
    // 新增：增强型变量预览器实例  
    std::unique_ptr<EnhancedVariableViewer> variableViewer;
    // 新增：帮助查看器实例
    std::unique_ptr<EnhancedHelpViewer> helpViewer; 
    // 新增：命令候选框实例
    std::unique_ptr<SuggestionBox> suggestionBox;
    
    // 绘制UI元素
    void drawHeader();
    void drawInputPrompt();
    void drawStatusBar();
    void drawResultArea();
    void clearResultArea(); // 保持不变
    void clearSuggestionArea(); // 新增：清除候选框区域

    // 处理命令和输入
    void handleSpecialKey(int key);
    void navigateHistory(bool up);
    
    // 命令执行函数
    void showVariables(bool listOnly = false); // 添加参数，当为true时只显示变量名和类型
    void showVariable(const std::string& varName);
    void showVariableWithFormat(const std::string& varName, int precision, bool saveResult = false, const std::string& resultVarName = ""); // -f 选项：有效数字
    void showVariableWithDecimalFormat(const std::string& varName, int decimalPlaces, bool saveResult = false, const std::string& resultVarName = ""); // -p 选项：小数位数
    void showVariableWithHighPrecision(const std::string& varName, int precision, int floatPrecision);
    
    template<typename HighPrecFloat>
    void displayWithPrecision(const Variable& var, int precision);
    
    // 步骤显示模式相关函数
    void enterStepDisplayMode(const OperationHistory& history);
    void enterStepDisplayMode(const ExpansionHistory& history);
    void exitStepDisplayMode();
    void displayCurrentStep();
    void drawStepProgressBar();

    std::string generateNewVariableName(bool isMatrix); // 保持此辅助函数
    
    // 辅助函数
    void printToResultView(const std::string& text, Color color = Color::DEFAULT);
    static std::string variableToString(const Variable& var);
    static std::string formatStringWithBracketHighlight(const std::string& text, size_t cursorPos); // 新增：带括号高亮的格式化函数
    std::vector<std::string> getVariableNames() const; // 新增：获取变量名列表
    std::string getCurrentWordForSuggestion(size_t& wordStartPosInInput) const; // 新增：获取当前输入单词以供建议

public:
    TuiApp(const std::string& initialCommand = ""); // 修改构造函数以接受初始命令
    void run();
    void initUI();
    void updateUI();
    void handleInput();
    void executeCommand(const std::string &input);

    // 新增：已知函数和命令列表 (在 .cpp 文件中定义)
    static const std::vector<std::string> KNOWN_FUNCTIONS;
    static const std::vector<std::string> KNOWN_COMMANDS;

    // 新增：退出时导出变量和历史
    void exportVariablesOnExit(const std::string& filename);

    // 新增：获取 noSavingOnExit 状态
    bool getNoSavingOnExit() const { return noSavingOnExit; }
};
