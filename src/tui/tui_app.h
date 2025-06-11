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
#include "tui_suggestion_box.h" // 新增：包含候选框头文件

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
    int stepDisplayStartRow; // 新增: 步骤显示开始的行
    
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

    // 移除旧的矩阵编辑模式相关状态
    // bool inMatrixEditMode;
    // std::string editingVariableName;
    // Variable editingVariableCopy; 
    // bool editingIsMatrix;         
    // size_t editCursorRow;         
    // size_t editCursorCol;         
    // std::string currentCellInput;   
    // bool cellInputActive;         

    // 新增：增强型矩阵编辑器实例
    std::unique_ptr<EnhancedMatrixEditor> matrixEditor;
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
    // 移除旧的矩阵编辑模式输入处理函数
    // void handleMatrixEditInput(int key); 
    
    // 命令执行函数
    void showHelp();
    void showVariables();
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

    // 移除旧的矩阵编辑模式相关函数
    // void enterMatrixEditMode(const std::string& varName, bool isNew, bool isMatrix, int rows = 0, int cols = 0);
    // void exitMatrixEditMode(bool saveChanges);
    // void drawMatrixEditor();
    std::string generateNewVariableName(bool isMatrix); // 保持此辅助函数
    
    // 辅助函数
    void printToResultView(const std::string& text, Color color = Color::DEFAULT);
    std::string variableToString(const Variable& var);
    std::vector<std::string> getVariableNames() const; // 新增：获取变量名列表
    std::string getCurrentWordForSuggestion(size_t& wordStartPosInInput) const; // 新增：获取当前输入单词以供建议
                                                                            // wordStartPosInInput will be relative to currentInput string
    
public:
    TuiApp();
    void run();
    void initUI();
    void updateUI();
    void handleInput();
    void executeCommand(const std::string &input);

    // 新增：已知函数和命令列表 (在 .cpp 文件中定义)
    static const std::vector<std::string> KNOWN_FUNCTIONS;
    static const std::vector<std::string> KNOWN_COMMANDS;
};
