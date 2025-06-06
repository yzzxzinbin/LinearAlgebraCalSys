#pragma once
#include <string>
#include <vector>
#include <deque>
#include "../grammar/grammar_tokenizer.h"
#include "../grammar/grammar_parser.h"
#include "../grammar/grammar_interpreter.h"

class TuiApp {
private:
    Interpreter interpreter;
    std::deque<std::string> history;
    size_t historyIndex;
    std::string currentInput;
    std::string statusMessage;
    bool running;
    
    // 终端大小
    int terminalRows;
    int terminalCols;
    
    // UI状态
    int inputRow;  // 输入行的位置
    int resultRow; // 结果开始的行
    
    // 最大历史记录数量
    static const size_t MAX_HISTORY = 100;

public:
    TuiApp();
    
    // 运行应用程序
    void run();
    
private:
    // 初始化UI
    void initUI();
    
    // 更新UI
    void updateUI();
    
    // 处理输入
    void handleInput();
    
    // 执行命令
    void executeCommand(const std::string& input);
    
    // 显示帮助
    void showHelp();
    
    // 显示变量
    void showVariables();
    
    // 处理特殊键
    void handleSpecialKey(int key);
    
    // 历史记录导航
    void navigateHistory(bool up);
    
    // 绘制UI元素
    void drawHeader();
    void drawInputPrompt();
    void drawStatusBar();
    void drawResultArea();
};
