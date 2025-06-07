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
    int resultRow; // 表示结果区域下一可用行
    size_t cursorPosition; // 新增：跟踪光标在currentInput中的位置
    
    // 命令和历史记录
    std::string currentInput;
    std::string tempInputBuffer; // 新增：用于在历史导航时暂存当前输入
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

    // 新增：矩阵编辑模式相关状态
    bool inMatrixEditMode;
    std::string editingVariableName;
    Variable editingVariableCopy; // 编辑对象的副本
    bool editingIsMatrix;         // 标记正在编辑的是矩阵还是向量
    size_t editCursorRow;         // 编辑器内光标行
    size_t editCursorCol;         // 编辑器内光标列
    std::string currentCellInput;   // 当前单元格的输入字符串
    bool cellInputActive;         // 单元格是否处于输入激活状态
    
    // 绘制UI元素
    void drawHeader();
    void drawInputPrompt();
    void drawStatusBar();
    void drawResultArea();
    void clearResultArea(); // 保持不变
    
    // 处理命令和输入
    void handleSpecialKey(int key);
    void navigateHistory(bool up);
    void handleMatrixEditInput(int key); // 新增：处理矩阵编辑模式下的输入
    
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

    // 新增：矩阵编辑模式相关函数
    void enterMatrixEditMode(const std::string& varName, bool isNew, bool isMatrix, int rows = 0, int cols = 0);
    void exitMatrixEditMode(bool saveChanges);
    void drawMatrixEditor();
    std::string generateNewVariableName(bool isMatrix);
    
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
