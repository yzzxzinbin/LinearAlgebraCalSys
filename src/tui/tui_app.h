#pragma once

#include <string>
#include <deque>
#include <memory>
#include <sstream>
#include "../grammar/grammar_interpreter.h"
#include "../grammar/grammar_tokenizer.h"
#include "../grammar/grammar_parser.h"
#include "../operation_step.h"
#include "../determinant_expansion.h"
#include "tui_terminal.h"

// 最大历史记录数量
const int MAX_HISTORY = 50;

class TuiApp {
private:
    // 窗口和UI状态
    int terminalRows;
    int terminalCols;
    int inputRow;
    int resultRow;
    
    // 命令和历史记录
    std::string currentInput;
    std::deque<std::string> history;
    size_t historyIndex;
    
    // 程序状态
    bool running;
    std::string statusMessage;
    
    // 解释器
    Interpreter interpreter;
    
    // 步骤显示模式相关
    bool inStepDisplayMode;
    size_t currentStep;
    size_t totalSteps;
    OperationHistory currentHistory;
    ExpansionHistory currentExpHistory;
    bool isExpansionHistory;
    
    // 绘制UI元素
    void drawHeader();
    void drawInputPrompt();
    void drawStatusBar();
    void drawResultArea();
    void clearResultArea();
    
    // 处理命令和输入
    void handleSpecialKey(int key);
    void navigateHistory(bool up);
    
    // 命令执行函数
    void showHelp();
    void showVariables();
    void showVariable(const std::string &varName);
    
    // 步骤显示模式相关函数
    void enterStepDisplayMode(const OperationHistory& history);
    void enterStepDisplayMode(const ExpansionHistory& history);
    void exitStepDisplayMode();
    void displayCurrentStep();
    void drawStepProgressBar();
    
    // 辅助函数
    void printToResultView(const std::string& text, Color color = Color::DEFAULT);
    std::string variableToString(const Variable& var);
    
public:
    TuiApp();
    void run();
    void initUI();
    void updateUI();
    void handleInput();
    void executeCommand(const std::string &input);
};
