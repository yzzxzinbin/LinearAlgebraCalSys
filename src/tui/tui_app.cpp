#include "tui_app.h"
#include "tui_terminal.h" 
#include <iostream>
#include <string>
#include <algorithm>
#include <iomanip> 
#include <cctype>
#include <fstream> // 新增：用于文件操作
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp> // 新增：任意精度十进制浮点数
#include "../utils/logger.h"
#include "../grammar/grammar_tokenizer.h"
#include "../grammar/grammar_parser.h"
#include "enhanced_matrix_editor.h" 
#include "tui_suggestion_box.h" // 已经在 tui_app.h 中包含

// 新增：定义输出区域的布局常量
const int RESULT_AREA_TITLE_ROW = 2; // "输出区域:" 标题所在的行 (0-indexed)
const int RESULT_AREA_CONTENT_START_ROW = RESULT_AREA_TITLE_ROW + 1; // 实际内容开始的第一行
const int MATRIX_EDITOR_CELL_WIDTH = 8; // 矩阵编辑器单元格宽度

// 定义已知函数和命令列表
const std::vector<std::string> TuiApp::KNOWN_FUNCTIONS = {
    "transpose", "inverse", "inverse_gauss", "det", "det_expansion",
    "rank", "ref", "rref", "cofactor_matrix", "adjugate",
    "dot", "cross", "norm", "normalize"
    // 可以根据实际情况添加更多函数
};

const std::vector<std::string> TuiApp::KNOWN_COMMANDS = {
    "help", "clear", "vars", "show", "exit", "steps", "new", "edit", "export", "import",
    "del", "rename", "csv" 
};


TuiApp::TuiApp() 
    : historyIndex(0), running(true), 
      inStepDisplayMode(false), currentStep(0), totalSteps(0), isExpansionHistory(false),
      cursorPosition(0), 
      // 初始化新的编辑器指针为空
      matrixEditor(nullptr),
      suggestionBox(nullptr), // 初始化 suggestionBox
      stepDisplayStartRow(0) // 初始化步骤显示起始行
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
    statusMessage = "欢迎使用线性代数计算系统! 输入 'help' 获取帮助。";
}

void TuiApp::run()
{
    // 初始化UI
    initUI();

    // 进入原始模式，以便直接读取按键
    Terminal::setRawMode(true);

    // 主循环
    while (running)
    {
        // 更新UI
        if (matrixEditor) { // 如果增强型编辑器激活
            matrixEditor->draw(); 
            // 状态栏由编辑器或TuiApp更新
            // drawStatusBar(); // 确保状态栏在编辑器绘制后更新，如果编辑器不自己画的话
        } else {
            updateUI(); // updateUI 会调用 drawInputPrompt, drawInputPrompt 会调用 suggestionBox->draw
        }
        drawStatusBar(); // 总是绘制状态栏，确保它在最下面且最新
        
        // 确保UI更新立即显示
        std::cout.flush();

        // 处理输入
        handleInput();
    }

    // 恢复终端状态
    Terminal::setRawMode(false);
    Terminal::resetColor();
    Terminal::clear();
    Terminal::setCursor(0, 0);
}

void TuiApp::initUI()
{
    Terminal::clear();
    drawHeader();
    // 如果编辑器激活，不绘制标准输入提示和结果区，由编辑器负责
    if (!matrixEditor) {
        drawInputPrompt(); // drawInputPrompt 内部会处理 suggestionBox 的绘制
        drawResultArea(); 
    }
    drawStatusBar();
}

void TuiApp::updateUI()
{
    // 检查终端大小是否变化
    auto [rows, cols] = Terminal::getSize();
    if (rows != terminalRows || cols != terminalCols)
    {
        terminalRows = rows;
        terminalCols = cols;
        inputRow = terminalRows - 2;
        // 如果终端大小改变，可能需要重新创建或更新 suggestionBox 的宽度
        suggestionBox = std::make_unique<SuggestionBox>(terminalCols);
        initUI();
    }

    // 更新输入提示行 (仅当编辑器未激活时)
    if (!matrixEditor) {
        drawInputPrompt(); // drawInputPrompt 内部会处理 suggestionBox 的绘制
    }
    // 状态栏总是更新 (或者由编辑器更新自己的状态消息)
    // drawStatusBar(); // 已移至run循环末尾
}

// 新增：清除候选框可能占用的区域
void TuiApp::clearSuggestionArea() {
    // 确保此函数定义存在且在 TuiApp 类的作用域内
    // 如果 suggestionBox 本身不可见，或者它自己管理清除，这里可能不需要做太多
    // 但为了安全，在它隐藏后，我们确保它占用的区域被重绘为背景
    
    // 假设候选框最多5行，绘制在 inputRow 之上
    // 这个函数主要在 suggestionBox->hide() 之后，确保屏幕干净
    int maxSuggestionLines = 5; // 与 SuggestionBox 的 maxDisplayItems 对应
    int startClearRow = inputRow - maxSuggestionLines;
    if (startClearRow < 0) startClearRow = 0;

    // 只清除到输入行之前，避免清除输入行本身或状态栏
    for (int i = startClearRow; i < inputRow; ++i) {
        if (i >= 0) { // 确保行号有效
            Terminal::setCursor(i, 0);
            std::string spaces(terminalCols, ' ');
            std::cout << spaces;
        }
    }
}


void TuiApp::handleInput()
{
    // 读取一个字符
    int key = Terminal::readChar();
    
    // 将 suggestionBoxWasVisible 的声明移到函数开头
    bool suggestionBoxWasVisible = suggestionBox->isVisible();

    // 如果增强型编辑器激活，将输入传递给它
    if (matrixEditor) {
        EnhancedMatrixEditor::EditorResult result = matrixEditor->handleInput(key);
        statusMessage = matrixEditor->getStatusMessage(); // 获取编辑器的状态消息

        if (result == EnhancedMatrixEditor::EditorResult::EXIT_SAVE) {
            interpreter.getVariablesNonConst()[matrixEditor->getVariableName()] = matrixEditor->getEditedVariableCopy();
            statusMessage = "数据更改已生效于 " + matrixEditor->getVariableName() ;
            matrixEditor.reset(); 
            initUI(); // 重绘标准UI
        } else if (result == EnhancedMatrixEditor::EditorResult::EXIT_DISCARD) {
            // 当前 EXIT_DISCARD 未实现，ESC 默认为 EXIT_SAVE
            statusMessage = "已退出 " + matrixEditor->getVariableName() + " 的编辑器";
            matrixEditor.reset();
            initUI();
        }
        // 如果是 CONTINUE 或 UPDATE_STATUS，状态栏会在 run() 循环中重绘
        return;
    }
    
    // 优先处理候选框输入
    if (suggestionBox->isVisible()) {
        SuggestionAction action = suggestionBox->handleKey(key);
        if (action == SuggestionAction::APPLY_SUGGESTION) {
            std::string selectedText = suggestionBox->getSelectedSuggestion().text;
            std::string prefixToReplace = suggestionBox->getCurrentInputPrefix();
            size_t wordStartPos = currentInput.rfind(prefixToReplace, cursorPosition - prefixToReplace.length());
            
            if (wordStartPos != std::string::npos && wordStartPos + prefixToReplace.length() == cursorPosition) {
                 currentInput.replace(wordStartPos, prefixToReplace.length(), selectedText);
                 cursorPosition = wordStartPos + selectedText.length();
            } else { 
                size_t originalCursor = cursorPosition;
                currentInput.insert(cursorPosition, selectedText);
                cursorPosition += selectedText.length();
                if (originalCursor >= prefixToReplace.length() && 
                    currentInput.substr(originalCursor - prefixToReplace.length(), prefixToReplace.length()) == prefixToReplace) {
                    currentInput.erase(originalCursor - prefixToReplace.length(), prefixToReplace.length());
                    cursorPosition -= prefixToReplace.length();
                }
            }
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
            drawInputPrompt();     
            return; // 修改：移除对key的检查，Tab键和Enter键不再冲突
        } else if (action == SuggestionAction::CLOSE_BOX) {
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
            drawInputPrompt();
            return;
        } else if (action == SuggestionAction::NAVIGATION) {
            drawInputPrompt(); 
            return;
        }
    }
    
    // 检查是否是退格键(包括Linux的127) - 仅当编辑器未激活
    if (key == KEY_BACKSPACE) {
        // 删除光标前一个字符
        if (cursorPosition > 0 && !currentInput.empty())
        {
            currentInput.erase(cursorPosition - 1, 1);
            cursorPosition--;
            // drawInputPrompt(); // Defer drawing until after suggestion update
        } else { // Backspace at start of empty line
             return;
        }
    } else if (inStepDisplayMode)
    {
        if (key == KEY_ESCAPE)
        {
            // ESC键退出步骤导航模式
            exitStepDisplayMode();
            return;
        }
        else if (key == KEY_LEFT || (key == 27 && Terminal::hasInput() && Terminal::readChar() == '[' && Terminal::readChar() == 'D'))
        {
            // 左箭头，显示上一步
            if (currentStep > 0)
            {
                currentStep--;
                displayCurrentStep();
                drawStepProgressBar();
            }
            return;
        }
        else if (key == KEY_RIGHT || (key == 27 && Terminal::hasInput() && Terminal::readChar() == '[' && Terminal::readChar() == 'C'))
        {
            // 右箭头，显示下一步
            if (currentStep < totalSteps - 1)
            {
                currentStep++;
                displayCurrentStep();
                drawStepProgressBar();
            }
            return;
        }
        // 在步骤显示模式下忽略其他键
        return;
    }

    // 处理特殊键
    if (key == KEY_ESCAPE)
    {
        // 检查是否是转义序列
        if (Terminal::hasInput())
        {
            int next = Terminal::readChar();
            if (next == '[')
            {
                // 这是一个ANSI转义序列
                int code = Terminal::readChar();
                switch (code)
                {
                case 'A': // 上箭头
                    handleSpecialKey(KEY_UP);
                    break; 
                case 'B': // 下箭头
                    handleSpecialKey(KEY_DOWN);
                    break; 
                case 'C': // 右箭头
                    handleSpecialKey(KEY_RIGHT);
                    break; 
                case 'D': // 左箭头
                    handleSpecialKey(KEY_LEFT);
                    break; 
                // 根据 tui_terminal.cpp 的 readChar 实现，Delete 键可能不会通过这里
                // 它可能直接返回一个特定的 KEY_DELETE 值（如果定义了）
                // 或者需要更复杂的序列检测
                }
            }
        }
        else
        {
            // ESC键
            if (suggestionBox->isVisible()) { // 如果候选框可见，ESC优先关闭候选框
                suggestionBox->hide();
                clearSuggestionArea(); // 调用 clearSuggestionArea
                // drawInputPrompt(); // Defer drawing
            } else if (currentInput.empty())
            {
                running = false; // 如果输入为空，退出程序
            }
            else
            {
                currentInput.clear(); // 否则清空当前输入
                cursorPosition = 0;
                // drawInputPrompt(); // Defer
            }
        }
    } else if (key == KEY_ENTER) {
        // Enter键现在只用于执行命令，不再用于选择候选词
        // 如果候选框可见，先隐藏它
        if (suggestionBox->isVisible()){ 
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
        }
        if (!currentInput.empty())
        {
            executeCommand(currentInput);

            // 将命令添加到历史记录
            if (history.empty() || history.front() != currentInput)
            {
                history.push_front(currentInput);
                if (history.size() > MAX_HISTORY)
                {
                    history.pop_back();
                }
            }
            
            if (!tempInputBuffer.empty()) { // 清除历史导航时的临时缓冲
                tempInputBuffer.clear();
            }

            historyIndex = 0;
            currentInput.clear();
            cursorPosition = 0;
            // drawInputPrompt(); // Defer
        }
    } else if (key >= 32 && key <= 126) { // 可打印字符
        if (key == '(') {
            // 括号自动补全：当输入(时，自动添加一对括号并将光标放在中间
            currentInput.insert(cursorPosition, "()");
            cursorPosition++; // 只移动一位，使光标位于()之间
        } else {
            // 正常处理其他可打印字符
            currentInput.insert(cursorPosition, 1, static_cast<char>(key));
            cursorPosition++;
        }
        
        if (historyIndex != 0) {
            historyIndex = 0; 
        }
        // drawInputPrompt(); // Defer
    } else { // 处理其他特殊键，如箭头键
        // 如果候选框不可见，才允许历史导航等
        if (!suggestionBoxWasVisible) { // Use suggestionBoxWasVisible to avoid conflict if ESC closed the box
             handleSpecialKey(key); // handleSpecialKey calls drawInputPrompt
        } else {
            // If suggestion box was visible but didn't handle the key (e.g. left/right arrow)
            // and it wasn't a char key, it might be a command for TuiApp itself.
            // For now, if suggestion box was visible, assume it or the char handler takes precedence.
            // This 'else' branch for special keys might need refinement if specific non-char,
            // non-suggestion-box keys need to work while box is visible.
            // However, up/down/enter/esc are handled by suggestion box.
            // Left/right should affect cursorPosition, then suggestions update.
            if (key == KEY_LEFT || key == KEY_RIGHT) {
                 handleSpecialKey(key); // This will move cursor and call drawInputPrompt
            }
        }
    }

    // 更新候选词 (在输入或删除后)
    // 但不在Enter后，因为命令已执行，输入已清空
    if (key != KEY_ENTER) {
        size_t currentWordStartPos = 0; // Relative to currentInput string
        std::string word_prefix = getCurrentWordForSuggestion(currentWordStartPos);
        if (!word_prefix.empty()) {
            // 先清除旧的候选框区域，然后再更新显示新的候选词
            clearSuggestionArea(); 
            suggestionBox->updateSuggestions(word_prefix, getVariableNames(), KNOWN_FUNCTIONS, KNOWN_COMMANDS);
        } else {
            if (suggestionBox->isVisible()) { // If no prefix but box was visible, hide it
                suggestionBox->hide();
                clearSuggestionArea(); // 调用 clearSuggestionArea
            }
        }
    } else { // After Enter, ensure box is hidden
        if (suggestionBox->isVisible()) {
            suggestionBox->hide();
            clearSuggestionArea(); // 调用 clearSuggestionArea
        }
    }
    
    // 统一在末尾绘制输入提示，确保所有状态更新后UI正确显示
    drawInputPrompt();
    std::cout.flush();
}

void TuiApp::printToResultView(const std::string& text, Color color) {
    if (matrixEditor) return; // 不在编辑器模式下打印到主结果视图
    Terminal::setForeground(color);
    std::istringstream iss(text);
    std::string line;
    bool firstLine = true;
    while (std::getline(iss, line)) {
        Terminal::setCursor(resultRow, 0); // 设置光标到当前 resultRow
        // 如果text以换行符开头（例如矩阵的variableToString），第一行可能是空的
        // 或者如果text不以换行符开头，则直接打印
        // 为了处理 "= \nmatrix"，我们希望 "=" 和 "matrix" 在不同行且 resultRow 正确增加
        
        // 如果是 "= \nmatrix_content" 这样的情况，第一行 getline 会得到 "= "
        // 第二行 getline 会得到 "matrix_content"
        // std::cout << line << std::endl; 会在每行后加换行
        std::cout << line;
        // 如果 iss 还有内容或者这不是原始text的最后一行（通过查看iss状态或line是否为空判断不完美）
        // 简单起见，每调用getline得到的line都输出后换行并增加resultRow
        std::cout << std::endl; 
        resultRow++;
    }
    Terminal::resetColor();
}

std::string TuiApp::variableToString(const Variable& var) {
    std::stringstream ss;
    switch (var.type) {
        case VariableType::FRACTION:
            ss << var.fractionValue;
            break;
        case VariableType::VECTOR:
            var.vectorValue.print(ss);
            break;
        case VariableType::MATRIX:
            ss << std::endl;
            var.matrixValue.print(ss);
            break;
    }
    return ss.str();
}

void TuiApp::executeCommand(const std::string &input)
{
    if (suggestionBox->isVisible()) { // Ensure box is hidden before execution
        suggestionBox->hide();
        clearSuggestionArea(); // 调用 clearSuggestionArea
        drawInputPrompt(); 
        std::cout.flush();
    }
    try
    {
        // 确保输入非空
        if (input.empty())
        {
            return;
        }

        LOG_INFO("执行命令: " + input);

        // 1. 每次执行命令前先清除结果区域 (如果编辑器未激活)
        if (!matrixEditor) {
            clearResultArea(); 
        } else {
            // 如果编辑器激活时尝试执行命令（理论上不应发生，因为输入被编辑器捕获）
            // 可能需要先退出编辑器
            LOG_WARNING("Attempted to execute command while matrix editor is active.");
            return;
        }


        // 2. 总是先显示命令输入 (有且仅有这一次)
        Terminal::setCursor(resultRow, 0); 
        Terminal::setForeground(Color::GREEN);
        std::cout << "> " << input << std::endl;
        Terminal::resetColor();
        resultRow++; // 更新下一行位置，为结果或步骤显示做准备

        // 预处理输入，处理特殊情况
        std::string processedInput = input;
        std::string commandStr;
        std::vector<std::string> commandArgs;

        // 简单解析命令和参数 (主要用于 new 和 edit)
        std::stringstream ss_input(processedInput);
        ss_input >> commandStr;
        std::string arg_token;
        while(ss_input >> arg_token) {
            if (arg_token.back() == ';') arg_token.pop_back();
            if (!arg_token.empty()) commandArgs.push_back(arg_token);
        }
        // 移除命令本身末尾的分号
        if (!commandStr.empty() && commandStr.back() == ';') {
            commandStr.pop_back();
        }

        // 处理clear命令
        if (commandStr == "clear") {
            if (commandArgs.empty()) {
                // clear (无参数): 清屏
                initUI();
                statusMessage = "屏幕已清除";
            } else if (commandArgs.size() == 1) {
                if (commandArgs[0] == "-v") {
                    interpreter.clearVariables();
                    printToResultView("所有变量已清除。", Color::YELLOW);
                    statusMessage = "所有变量已清除";
                } else if (commandArgs[0] == "-h") {
                    history.clear();
                    historyIndex = 0;
                    tempInputBuffer.clear();
                    printToResultView("命令历史已清除。", Color::YELLOW);
                    statusMessage = "命令历史已清除";
                } else if (commandArgs[0] == "-v") {
                    // clear -v: 清除所有变量
                    // 注意: Interpreter 类需要有 clearVariables 方法
                    interpreter.clearVariables(); 
                    printToResultView("所有变量已清除。", Color::YELLOW);
                    statusMessage = "所有变量已清除";
                } else if (commandArgs[0] == "-a") {
                    // clear -a: 清屏、清除历史、清除变量
                    initUI(); // 清屏
                    history.clear();
                    historyIndex = 0;
                    tempInputBuffer.clear();
                    interpreter.clearVariables(); // 清除变量
                    // printToResultView 在 initUI 中已被调用来清空区域，这里可以不再打印特定消息到结果区
                    // 或者可以打印一个总结性消息
                    printToResultView("已恢复初始状态 (屏幕、历史、变量已清除)。", Color::YELLOW);
                    statusMessage = "已恢复初始状态";
                } else {
                    throw std::invalid_argument("无效的 clear 命令参数。用法: clear [-h | -v | -a]");
                }
            } else {
                throw std::invalid_argument("无效的 clear 命令参数。用法: clear [-h | -v | -a]");
            }
            return; // clear 命令处理完毕
        }

        // 新增：处理del命令
        if (commandStr == "del") {
            if (commandArgs.size() == 1) {
                const std::string& varNameToDelete = commandArgs[0];
                interpreter.deleteVariable(varNameToDelete);
                printToResultView("变量 '" + varNameToDelete + "' 已删除。", Color::YELLOW);
                statusMessage = "变量 '" + varNameToDelete + "' 已删除。";
            } else {
                throw std::runtime_error("del 命令需要一个参数 (变量名)。用法: del <变量名>");
            }
            return;
        }

        // 新增：处理rename命令
        if (commandStr == "rename") {
            if (commandArgs.size() == 2) {
                const std::string& oldName = commandArgs[0];
                const std::string& newName = commandArgs[1];
                interpreter.renameVariable(oldName, newName);
                printToResultView("变量 '" + oldName + "' 已重命名为 '" + newName + "'。", Color::YELLOW);
                statusMessage = "变量 '" + oldName + "' 已重命名为 '" + newName + "'。";
            } else {
                throw std::runtime_error("rename 命令需要两个参数 (旧变量名和新变量名)。用法: rename <旧变量名> <新变量名>");
            }
            return;
        }

        // 新增：处理csv命令
        if (commandStr == "csv") {
            if (commandArgs.size() == 1) {
                const std::string& varName = commandArgs[0];
                const auto& vars = interpreter.getVariables();
                auto it = vars.find(varName);

                if (it == vars.end()) {
                    throw std::runtime_error("变量 '" + varName + "' 未定义。");
                }
                if (it->second.type != VariableType::RESULT) {
                    throw std::runtime_error("变量 '" + varName + "' 不是一个Result类型，无法导出为CSV。");
                }

                const Result& resultToExport = it->second.resultValue;
                std::string csvData = resultToExport.toCsvString();
                
                std::string filename = varName + ".csv";
                std::ofstream outFile(filename);
                if (!outFile.is_open()) {
                    throw std::runtime_error("无法打开文件 '" + filename + "' 进行写入。");
                }
                outFile << csvData;
                outFile.close();

                printToResultView("变量 '" + varName + "' 已成功导出到 " + filename, Color::YELLOW);
                statusMessage = "变量 '" + varName + "' 已导出到 " + filename;

            } else {
                throw std::runtime_error("csv 命令需要一个参数 (Result类型的变量名)。用法: csv <变量名>");
            }
            return;
        }


        // 处理new命令
        if (commandStr == "new") {
            if (commandArgs.size() == 1) { // new N (vector)
                int dim = std::stoi(commandArgs[0]);
                if (dim <= 0) throw std::invalid_argument("向量维度必须为正。");
                std::string newName = generateNewVariableName(false);
                interpreter.getVariablesNonConst()[newName] = Variable(Vector(dim)); 
                // 进入增强型编辑模式
                Variable& varToEdit = interpreter.getVariablesNonConst().at(newName);
                matrixEditor = std::make_unique<EnhancedMatrixEditor>(varToEdit, newName, false, terminalRows, terminalCols);
                statusMessage = matrixEditor->getStatusMessage();
                initUI(); // 重绘UI以适应编辑器
            } else if (commandArgs.size() == 2) { // new R C (matrix)
                int r = std::stoi(commandArgs[0]);
                int c = std::stoi(commandArgs[1]);
                if (r <= 0 || c <= 0) throw std::invalid_argument("矩阵行列数必须为正。");
                std::string newName = generateNewVariableName(true);
                interpreter.getVariablesNonConst()[newName] = Variable(Matrix(r, c)); 
                // 进入增强型编辑模式
                Variable& varToEdit = interpreter.getVariablesNonConst().at(newName);
                matrixEditor = std::make_unique<EnhancedMatrixEditor>(varToEdit, newName, true, terminalRows, terminalCols);
                statusMessage = matrixEditor->getStatusMessage();
                initUI(); // 重绘UI以适应编辑器
            } else {
                throw std::invalid_argument("new 命令参数错误。用法: new <维度> (用于向量) 或 new <行数> <列数> (用于矩阵)");
            }
            return;
        }

        // 处理edit命令
        if (commandStr == "edit" && commandArgs.size() == 1) {
            const std::string& varName = commandArgs[0];
            if (interpreter.getVariables().find(varName) == interpreter.getVariables().end()) {
                throw std::runtime_error("变量 '" + varName + "' 未定义。");
            }
            Variable& varToEdit = interpreter.getVariablesNonConst().at(varName); // 需要非const引用传递给编辑器
            if (varToEdit.type == VariableType::MATRIX) {
                matrixEditor = std::make_unique<EnhancedMatrixEditor>(varToEdit, varName, true, terminalRows, terminalCols);
                statusMessage = matrixEditor->getStatusMessage();
                initUI(); 
            } else if (varToEdit.type == VariableType::VECTOR) {
                matrixEditor = std::make_unique<EnhancedMatrixEditor>(varToEdit, varName, false, terminalRows, terminalCols);
                statusMessage = matrixEditor->getStatusMessage();
                initUI();
            } else {
                throw std::runtime_error("变量 '" + varName + "' 不是矩阵或向量，无法编辑。");
            }
            return;
        }

        // 处理export命令
        if (commandStr == "export" && commandArgs.size() == 1) {
            std::string filename = commandArgs[0];
            std::string export_message = interpreter.exportVariables(filename, history); // 传递历史记录
            printToResultView(export_message, Color::YELLOW);
            statusMessage = export_message;
            return;
        }

        // 处理import命令
        if (commandStr == "import" && commandArgs.size() == 1) {
            std::string filename = commandArgs[0];
            auto import_result = interpreter.importVariables(filename);
            std::string import_message = import_result.first;
            const auto& imported_cmds_from_file = import_result.second; // 文件中的顺序 (最新在前)

            printToResultView(import_message, Color::YELLOW);
            statusMessage = import_message;

            // 集成导入的历史记录
            // imported_cmds_from_file 是从文件中读取的顺序 (最新命令在前)
            // 我们希望將它们按此顺序添加到历史记录的前面
            // 因此，我们逆序遍历导入的命令 (从最旧的导入命令开始)
            // 并将它们 push_front 到 TuiApp 的 history 队列
            for (auto it = imported_cmds_from_file.rbegin(); it != imported_cmds_from_file.rend(); ++it) {
                const std::string& cmd_to_add = *it;
                // 避免添加重复项
                bool exists = false;
                for (const auto& existing_cmd : history) {
                    if (existing_cmd == cmd_to_add) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    history.push_front(cmd_to_add);
                }
            }
            // 修剪历史记录，如果超出最大大小
            while (history.size() > MAX_HISTORY) {
                history.pop_back(); // 移除最旧的命令
            }
            historyIndex = 0; // 重置历史导航索引

            return;
        }


        // 处理show命令
        if (processedInput.substr(0, 4) == "show" && processedInput.length() > 5)
        {
            std::string params = processedInput.substr(5);
            // 去除可能的结尾分号
            if (!params.empty() && params.back() == ';')
            {
                params.pop_back();
            }
            
            // 提取变量名和选项
            std::istringstream iss(params);
            std::string varName, option;
            iss >> varName; // 第一个词是变量名
            
            bool useFloat = false;
            bool useDecimal = false;
            bool saveResult = false;
            std::string resultVarName;
            int precision = 2; // 默认精度
            
            // 检查是否有更多参数
            while (iss >> option) {
                // 检查是否是 -f 选项 (有效数字)
                if (option.substr(0, 2) == "-f") {
                    useFloat = true;
                    // 检查是否指定了精度
                    if (option.length() > 2) {
                        try {
                            precision = std::stoi(option.substr(2));
                        } catch (const std::exception&) {
                            // 解析精度失败，使用默认值
                        }
                    }
                }
                // 检查是否是 -p 选项 (小数点后位数)
                else if (option.substr(0, 2) == "-p") {
                    useDecimal = true;
                    // 检查是否指定了精度
                    if (option.length() > 2) {
                        try {
                            precision = std::stoi(option.substr(2));
                        } catch (const std::exception&) {
                            // 解析精度失败，使用默认值为0
                            precision = 0;
                        }
                    } else {
                        precision = 0; // 默认为整数显示
                    }
                }
                // 检查是否是 -r 选项 (保存结果)
                else if (option.substr(0, 2) == "-r") {
                    saveResult = true;
                    if (option.length() > 2) {
                        resultVarName = option.substr(2);
                    } else {
                        // 如果-r后没有变量名，尝试读取下一个参数
                        if (iss >> option) {
                            resultVarName = option;
                        } else {
                            throw std::invalid_argument("-r 选项需要指定结果变量名");
                        }
                    }
                }
            }
            
            // 根据选项显示变量
            if (useFloat) {
                showVariableWithFormat(varName, precision, saveResult, resultVarName);
            } else if (useDecimal) {
                showVariableWithDecimalFormat(varName, precision, saveResult, resultVarName);
            } else {
                showVariable(varName);
            }
            return;
        }

        // 处理help命令
        if (processedInput == "help" || processedInput == "help;")
        {
            // showHelp 内部使用 resultRow
            showHelp();
            return;
        }

        // 旧的 clear 命令处理逻辑已移到前面并扩展

        // 处理vars命令
        if (processedInput == "vars" || processedInput == "vars;")
        {
            // showVariables 内部使用 resultRow
            showVariables();
            return;
        }

        // 处理exit命令
        if (processedInput == "exit" || processedInput == "exit;")
        {
            running = false;
            return;
        }

        // 如果是纯命令如help、clear等，确保以分号结尾
        if (processedInput.find('=') == std::string::npos &&
            processedInput.find('(') == std::string::npos &&
            processedInput.back() != ';')
        {
            processedInput += ';'; // 添加结束符
            LOG_DEBUG("为命令添加分号: " + processedInput);
        }

        // 对于矩阵和向量赋值，确保分号分隔和结束符存在
        if ((processedInput.find('[') != std::string::npos) &&
            processedInput.back() != ';')
        {
            processedInput += ';';
            LOG_DEBUG("为矩阵/向量赋值添加分号: " + processedInput);
        }

        // 标记化输入
        LOG_DEBUG("开始标记化输入");
        Tokenizer tokenizer(processedInput);
        std::vector<Token> tokens = tokenizer.tokenize();

        LOG_DEBUG("标记数量: " + std::to_string(tokens.size()));
        for (size_t i = 0; i < tokens.size(); ++i)
        {
            LOG_DEBUG("标记 " + std::to_string(i) + ": 类型=" +
                      std::to_string(static_cast<int>(tokens[i].type)) +
                      ", 值=\"" + tokens[i].value + "\"");
        }

        // 确保至少有一个有效标记
        if (tokens.empty() || (tokens.size() == 1 && tokens[0].type == TokenType::END_OF_INPUT))
        {
            LOG_WARNING("输入没有有效标记");
            return; // 空输入，不做任何处理
        }

        // 解析标记
        LOG_DEBUG("开始解析标记");
        Parser parser(tokens);
        std::unique_ptr<AstNode> ast = parser.parse();

        if (!ast)
        {
            LOG_ERROR("解析失败，无法创建语法树");
            throw std::runtime_error("解析失败，无法创建语法树");
        }

        LOG_DEBUG("语法树创建成功，类型: " + std::to_string(static_cast<int>(ast->type)));

        // 从 Interpreter 执行 AST
        Variable result = interpreter.execute(ast);
        LOG_INFO("命令执行完成，结果类型: " + std::to_string(static_cast<int>(result.type)));
        
        bool enteredStepMode = false;
        if (interpreter.isShowingSteps()) {
            const auto& opHistory = interpreter.getCurrentOpHistory();
            if (opHistory.size() > 0) {
                LOG_INFO("进入步骤展示模式 (OperationHistory), 步骤数: " + std::to_string(opHistory.size()));
                // 命令已打印。如果是非命令节点，打印其最终结果。
                if (ast->type != AstNodeType::COMMAND) { 
                    printToResultView("= " + variableToString(result), Color::CYAN);
                }
                enterStepDisplayMode(opHistory); // enterStepDisplayMode 会使用当前的 resultRow
                enteredStepMode = true;
            }

            if (!enteredStepMode) { 
                const auto& expHistory = interpreter.getCurrentExpHistory();
                if (expHistory.size() > 0) {
                    LOG_INFO("进入步骤展示模式 (ExpansionHistory), 步骤数: " + std::to_string(expHistory.size()));
                    if (ast->type != AstNodeType::COMMAND) {
                       printToResultView("= " + variableToString(result), Color::CYAN);
                    }
                    enterStepDisplayMode(expHistory); // enterStepDisplayMode 会使用当前的 resultRow
                    enteredStepMode = true;
                }
            }
        }

        if (!enteredStepMode) {
            // 如果没有进入步骤模式，正常显示结果
            // 命令已在开头打印。现在只需打印结果。
            if (ast->type != AstNodeType::COMMAND) { // 只为非命令显示结果
                printToResultView("= " + variableToString(result), Color::CYAN);
            }
        }
        
        // 更新状态消息
        if (!enteredStepMode) { // 只有在未进入步骤模式时才更新常规状态
            if (ast->type == AstNodeType::COMMAND) {
                const CommandNode* cmdNode = static_cast<const CommandNode*>(ast.get());
                if (cmdNode->command == "steps") {
                    if (interpreter.isShowingSteps()) {
                        statusMessage = "计算步骤显示已开启";
                    } else {
                        statusMessage = "计算步骤显示已关闭";
                    }
                } else if (cmdNode->command == "clear") {
                     statusMessage = "屏幕已清除"; 
                } else {
                     statusMessage = "命令执行成功";
                }
            } else {
                statusMessage = "命令执行成功";
            }
        }
    }
    catch (const std::out_of_range &e)
    {
        // 特别处理字符串越界错误
        Terminal::setCursor(resultRow, 0);
        Terminal::setForeground(Color::RED);
        std::cout << "错误: 解析输入时出现字符串索引越界。可能是语法错误。" << std::endl;
        std::cout << "详细信息: " << e.what() << std::endl;
        Terminal::resetColor();

        // 更新状态消息
        statusMessage = "命令解析失败: 语法错误";
    }
    catch (const std::invalid_argument &e)
    {
        // 处理格式错误
        Terminal::setCursor(resultRow, 0);
        Terminal::setForeground(Color::RED);
        std::cout << "错误: 输入格式不正确。" << std::endl;
        std::cout << "详细信息: " << e.what() << std::endl;
        Terminal::resetColor();

        // 更新状态消息
        statusMessage = "格式错误: " + std::string(e.what());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("命令执行失败: " + std::string(e.what()));

        // 显示用户友好的错误消息
        Terminal::setCursor(resultRow, 0);
        Terminal::setForeground(Color::RED);
        std::cout << "错误: " << e.what() << std::endl;
        std::cout << "请检查日志文件以获取详细信息。" << std::endl;
        Terminal::resetColor();

        // 更新状态消息
        statusMessage = "命令执行失败: 请查看日志文件";
    }
    catch (...)
    {
        // 捕获所有其他未知异常
        Terminal::setCursor(resultRow, 0);
        Terminal::setForeground(Color::RED);
        std::cout << "发生未知错误" << std::endl;
        Terminal::resetColor();

        statusMessage = "发生未知错误";
    }
}

void TuiApp::showHelp()
{
    if (matrixEditor) return; // 不在编辑器模式下显示帮助

    Terminal::setCursor(resultRow, 0); 
    Terminal::setForeground(Color::CYAN);
    std::cout << "帮助信息：\n"; // 示例：这会占用 resultRow
    resultRow++;                 // 更新 resultRow
    Terminal::setCursor(resultRow, 0);
    std::cout << "  help                           - 显示此帮助信息\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  clear [-h|-v|-a]               - 清屏/清除历史/清除变量/全部清除\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  vars                           - 显示所有变量\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  show <变量名> [-f<精度>|-p<精度>] [-r <结果变量名>] - 显示变量(可选格式化)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  exit                           - 退出程序\n";   
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  steps                          - 切换计算步骤显示\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  new <行数> <列数>              - 创建一个新的矩阵变量 (例如: new 2 3)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  edit <变量名>                  - 编辑已存在的矩阵或向量变量\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  export <文件名>                - 导出所有变量和历史到文件\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  import <文件名>                - 从文件导入变量和历史\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  del <变量名>                   - 删除指定的变量\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  rename <旧变量名> <新变量名>   - 重命名指定的变量\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  csv <变量名>                   - 将Result类型变量导出为CSV文件\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "变量定义:\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m1 = [1,2,3;4,5,6] - 定义矩阵\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  v1 = [1,2,3]       - 定义向量\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  f1 = 3/4           - 定义分数\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "基本运算:\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  矩阵: m3 = m1 + m2, m3 = m1 - m2, m3 = m1 * m2 (矩阵乘法)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  向量: v3 = v1 + v2, v3 = v1 - v2\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "        f_dot = v1 * v2    - 向量点积 (返回分数)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "        v_cross = v1 x v2  - 向量叉积 (返回向量, 仅3D)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "矩阵函数:\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m2 = transpose(m1)        - 矩阵转置\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m2 = inverse(m1)          - 计算逆矩阵(伴随矩阵法)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m2 = inverse_gauss(m1)    - 计算逆矩阵(高斯-若尔当法)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  f1 = det(m1)              - 计算行列式(默认方法)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  f1 = det_expansion(m1)    - 按行列展开计算行列式\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  f1 = rank(m1)             - 计算矩阵秩\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m2 = ref(m1)              - 行阶梯形\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m2 = rref(m1)             - 最简行阶梯形\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m2 = cofactor_matrix(m1)  - 计算代数余子式矩阵\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m2 = adjugate(m1)         - 计算伴随矩阵\n";
    // resultRow++; // 最后一行不需要再递增，除非后面还有输出
    std::cout << "\n"; // 确保最后有换行
    Terminal::resetColor();

    // 更新状态消息
    statusMessage = "已显示帮助信息";
}

void TuiApp::showVariables()
{
    if (matrixEditor) return; // 不在编辑器模式下显示变量
    // resultRow 当前指向命令行的下一行
    Terminal::setCursor(resultRow, 0);
    const auto &vars = interpreter.getVariables();

    if (vars.empty())
    {
        Terminal::setForeground(Color::YELLOW);
        std::cout << "没有已定义的变量。\n";
        Terminal::resetColor();
        resultRow++;
        return;
    }

    Terminal::setForeground(Color::CYAN);
    std::cout << "已定义的变量：\n";
    resultRow++;

    for (const auto &pair : vars)
    {
        Terminal::setCursor(resultRow, 0);
        std::cout << "  " << pair.first << " = ";

        switch (pair.second.type)
        {
        case VariableType::FRACTION:
            std::cout << pair.second.fractionValue << "\n";
            resultRow++;
            break;
        case VariableType::VECTOR:
            pair.second.vectorValue.print();
            std::cout << "\n";
            resultRow++;
            break;
        case VariableType::MATRIX:
            std::cout << "\n"; // 为 "m = " 和矩阵内容之间提供一行间隔
            resultRow++; 
            Terminal::setCursor(resultRow, 0); // 确保矩阵从新行开始
            // 假设 matrixValue.print() 会自行处理多行打印，并返回打印的行数或我们手动计算
            // 为简化，我们让 matrixValue.print() 直接打印，并手动增加 resultRow
            // 或者，更好的方式是 matrixValue.print() 接受一个起始行参数
            {
                std::stringstream matrix_ss;
                pair.second.matrixValue.print(matrix_ss);
                std::string matrix_str = matrix_ss.str();
                std::istringstream matrix_iss(matrix_str);
                std::string matrix_line;
                while(std::getline(matrix_iss, matrix_line)) {
                    Terminal::setCursor(resultRow, 0);
                    std::cout << "  " << matrix_line << "\n"; // 添加缩进
                    resultRow++;
                }
            }
            break;
        }
    }
    Terminal::resetColor();
    statusMessage = "已显示变量列表";
}

void TuiApp::handleSpecialKey(int key)
{
    if (matrixEditor) return; // 编辑器处理自己的特殊键
    switch (key)
    {
    case KEY_UP:
        navigateHistory(true);
        break;
    case KEY_DOWN:
        navigateHistory(false);
        break;
    case KEY_LEFT:
        if (cursorPosition > 0) {
            cursorPosition--;
            drawInputPrompt();
        }
        break;
    case KEY_RIGHT:
        if (cursorPosition < currentInput.length()) {
            cursorPosition++;
            drawInputPrompt();
        }
        break;
    // case KEY_DELETE: // 如果 readChar 返回 KEY_DELETE
    //     if (cursorPosition < currentInput.length()) {
    //         currentInput.erase(cursorPosition, 1);
    //         drawInputPrompt();
    //     }
    //     break;
    }
}

void TuiApp::navigateHistory(bool up)
{
    if (matrixEditor) return; // 不在编辑器模式下导航历史
    if (up) { // 向上浏览历史
        if (history.empty()) {
            return;
        }
        if (historyIndex == 0) { // 第一次按上箭头，或从最新输入向上
            tempInputBuffer = currentInput; // 保存当前输入行
        }
        if (historyIndex < history.size()) {
            historyIndex++;
            currentInput = history[historyIndex - 1];
            cursorPosition = currentInput.length(); // 光标移到末尾
        }
    } else { // 向下浏览历史
        if (historyIndex > 1) {
            historyIndex--;
            currentInput = history[historyIndex - 1];
            cursorPosition = currentInput.length(); // 光标移到末尾
        } else if (historyIndex == 1) { // 到达历史记录的“底部”，恢复之前暂存的输入
            historyIndex = 0;
            currentInput = tempInputBuffer;
            // tempInputBuffer.clear(); // 不立即清除，以便再次向上时能恢复
            cursorPosition = currentInput.length(); // 光标移到末尾
        }
    }
    drawInputPrompt();
}

void TuiApp::drawHeader()
{
    Terminal::setCursor(0, 0);
    Terminal::setForeground(Color::CYAN);
    Terminal::setBackground(Color::BLUE);

    std::string title = " 线性代数计算系统 v1.0 ";
    int padding = (terminalCols - title.length()) / 2;

    std::string header(terminalCols, ' ');
    for (size_t i = 0; i < title.length(); i++)
    {
        header[padding + i] = title[i];
    }

    std::cout << header;
    Terminal::resetColor();
    std::cout << std::endl;
}

void TuiApp::drawInputPrompt()
{
    if (matrixEditor) return; 

    // 先绘制候选框 (如果可见)
    // 候选框的绘制位置需要基于当前光标正在编辑的词的起始位置
    if (suggestionBox->isVisible()) {
        size_t currentWordStartPosInString = 0;
        getCurrentWordForSuggestion(currentWordStartPosInString); // Get the start index of the word in currentInput
        // The column for suggestionBox->draw should be where this word starts on screen.
        // currentWordStartPosInInput is the offset from "> "
        suggestionBox->draw(inputRow, 2, currentWordStartPosInString);
    }


    Terminal::setCursor(inputRow, 0);
    Terminal::setForeground(Color::GREEN);
    std::cout << "> ";

    // 清除旧的输入行内容
    std::string spaces(terminalCols - 2, ' '); // -2 for "> "
    std::cout << spaces;

    // 重新定位光标以打印输入和模拟光标
    Terminal::setCursor(inputRow, 2);

    // 打印光标前的部分
    std::cout << currentInput.substr(0, cursorPosition);

    // 模拟光标：反转颜色打印光标下的字符，或者打印特殊字符
    // 保存当前颜色状态
    // (如果 Terminal 类支持获取当前颜色会更好，这里假设默认是绿前景白背景)
    
    Terminal::setBackground(Color::WHITE); // 设置光标背景色
    Terminal::setForeground(Color::BLACK); // 设置光标前景色

    if (cursorPosition < currentInput.length()) {
        std::cout << currentInput[cursorPosition];
                } else {
        std::cout << " "; // 如果光标在末尾，显示一个空格作为光标块
    }
    
    Terminal::resetColor(); // 重置颜色到终端默认
    Terminal::setForeground(Color::GREEN); // 重新应用绿色前景以打印光标后的文本

    // 打印光标后的部分
    if (cursorPosition < currentInput.length()) {
        std::cout << currentInput.substr(cursorPosition + 1);
    }

    // 将真实的终端光标定位到我们模拟光标的逻辑位置之后
    // 这样，如果终端本身也显示光标，它不会与我们的模拟光标重叠或错位
    Terminal::setCursor(inputRow, 2 + cursorPosition +1);
}

void TuiApp::drawStatusBar()
{
    Terminal::setCursor(terminalRows - 1, 0);
    Terminal::setForeground(Color::BLACK);
    Terminal::setBackground(Color::WHITE);

    // 状态栏信息
    std::string status = " " + statusMessage;
    status.resize(terminalCols, ' ');

    std::cout << status;
    Terminal::resetColor();
}

// 添加新方法以清除结果区域
void TuiApp::clearResultArea()
{
    if (matrixEditor) return; // 编辑器激活时，主结果区可能不需要清除或由编辑器管理
    // 清空结果区域的实际内容部分
    for (int i = RESULT_AREA_CONTENT_START_ROW; i < inputRow; i++)
    {
        Terminal::setCursor(i, 0);
        std::string spaces(terminalCols, ' ');
        std::cout << spaces;
    }

    // 重新绘制结果区域标题
    Terminal::setCursor(RESULT_AREA_TITLE_ROW, 0);
    Terminal::setForeground(Color::YELLOW);
    std::cout << "输出区域:" << std::endl;
    Terminal::resetColor();

    // 重置 TuiApp::resultRow 成员变量，以便下一次打印从内容区域的顶部开始
    this->resultRow = RESULT_AREA_CONTENT_START_ROW;
}

// 添加新方法以显示特定变量的值
void TuiApp::showVariable(const std::string &varName)
{
    if (matrixEditor) return; // 不在编辑器模式下显示变量
    const auto &vars = interpreter.getVariables();
    auto it = vars.find(varName);

    // resultRow 当前指向命令行的下一行
    Terminal::setCursor(resultRow, 0);

    if (it == vars.end())
    {
        Terminal::setForeground(Color::RED);
        std::cout << "错误: 变量 '" << varName << "' 未定义。" << std::endl;
        Terminal::resetColor();
        resultRow++;
        statusMessage = "变量未找到: " + varName;
        return;
    }

    Terminal::setForeground(Color::CYAN);
    std::cout << varName << " = ";

    switch (it->second.type)
    {
    case VariableType::FRACTION:
        std::cout << it->second.fractionValue << std::endl;
        resultRow++;
        break;
    case VariableType::VECTOR:
        // std::cout << std::endl; // Vector.print() 通常不带前导换行
        // resultRow++;
        it->second.vectorValue.print(); // 假设 print() 打印在一行
        std::cout << std::endl; // 确保换行
        resultRow++;
        break;
    case VariableType::MATRIX:
        std::cout << "\n"; // 为 "m = " 和矩阵内容之间提供一行间隔
        resultRow++;
        // 与 showVariables 类似地处理矩阵打印
        {
            std::stringstream matrix_ss;
            it->second.matrixValue.print(matrix_ss);
            std::string matrix_str = matrix_ss.str();
            std::istringstream matrix_iss(matrix_str);
            std::string matrix_line;
            while(std::getline(matrix_iss, matrix_line)) {
                Terminal::setCursor(resultRow, 0);
                std::cout << matrix_line << std::endl;
                resultRow++;
            }
        }
        break;
    case VariableType::RESULT:  // 新增：处理Result类型
        std::cout << "\n"; // 为 "result = " 和结果内容之间提供一行间隔
        std::cout << it->second.resultValue << std::endl;
        resultRow++;
        break;
    }
    Terminal::resetColor();
    statusMessage = "显示变量: " + varName;
}

void TuiApp::showVariableWithFormat(const std::string &varName, int precision, bool saveResult, const std::string& resultVarName) {
    if (matrixEditor) return; // 不在编辑器模式下显示变量
    const auto &vars = interpreter.getVariables();
    auto it = vars.find(varName);

    // resultRow 当前指向命令行的下一行
    Terminal::setCursor(resultRow, 0);

    if (it == vars.end()) {
        Terminal::setForeground(Color::RED);
        std::cout << "错误: 变量 '" << varName << "' 未定义。" << std::endl;
        Terminal::resetColor();
        resultRow++;
        statusMessage = "变量未找到: " + varName;
        return;
    }

    Terminal::setForeground(Color::CYAN);
    std::cout << varName << " = ";

    // 使用 Boost 的任意精度浮点数类型
    using HighPrecisionFloat = boost::multiprecision::cpp_dec_float_100; // 100位精度

    // 辅助函数：格式化单个数值为有效数字显示
    auto formatValue = [&precision](const Fraction& frac) -> std::string {
        try {
            // 使用任意精度浮点数进行除法运算
            HighPrecisionFloat numerator(frac.getNumerator().str());
            HighPrecisionFloat denominator(frac.getDenominator().str());
            HighPrecisionFloat result = numerator / denominator;
            
            // 检查溢出情况
            if (result > (std::numeric_limits<double>::max)()) {
                return "INF";
            } else if (result < (std::numeric_limits<double>::lowest)()) {
                return "-INF";
            }
            
            double fval = result.convert_to<double>();
            
            // 检查是否为零
            if (fval == 0.0) {
                return "0";
            }
            
            // 检查是否为整数（分母为1的情况）
            if (frac.getDenominator() == 1) {
                // 整数情况：检查是否需要科学计数法
                std::string intStr = frac.getNumerator().str();
                if (intStr.length() > static_cast<size_t>(precision + 1)) {
                    // 使用科学计数法显示大整数
                    std::stringstream ss;
                    ss << std::scientific << std::setprecision(precision - 1) << fval;
                    return ss.str();
            } else {
                    // 直接显示整数
                    return intStr;
                }
            }
            
            // 非整数情况：使用有效数字格式
            double absVal = std::abs(fval);
            
            // 判断使用定点还是科学计数法
            // 当数值在 [0.1, 10^precision) 范围内时使用定点表示法
            if (absVal >= 0.1 && absVal < std::pow(10.0, precision)) {
                // 计算需要的小数位数以达到指定有效数字
                int magnitude = static_cast<int>(std::floor(std::log10(absVal)));
                int decimalPlaces = precision - magnitude - 1;
                
                if (decimalPlaces < 0) decimalPlaces = 0;
                if (decimalPlaces > 15) decimalPlaces = 15; // 限制最大小数位数
                
                std::stringstream ss;
                ss << std::fixed << std::setprecision(decimalPlaces) << fval;
                std::string str = ss.str();
                
                // 移除末尾的零（但保留小数点前的零）
                if (str.find('.') != std::string::npos) {
                    str = str.substr(0, str.find_last_not_of('0') + 1);
                    if (str.back() == '.') {
                        str.pop_back();
                    }
                }
                return str;
            } else {
                // 使用科学计数法
                std::stringstream ss;
                ss << std::scientific << std::setprecision(precision - 1) << fval;
                return ss.str();
            }
        } catch (const std::exception& e) {
            return "ERR";
        }
    };

    Result result;
    
    switch (it->second.type) {
    case VariableType::FRACTION:
        {
            std::string formattedValue = formatValue(it->second.fractionValue);
            std::cout << formattedValue << std::endl;
            resultRow++;
            if (saveResult) {
                result = Result(formattedValue);
            }
        }
        break;
    case VariableType::VECTOR:
        {
            std::cout << "[";
            std::vector<std::string> formattedValues;
            for (size_t i = 0; i < it->second.vectorValue.size(); ++i) {
                std::string formattedValue = formatValue(it->second.vectorValue.at(i));
                std::cout << formattedValue;
                formattedValues.push_back(formattedValue);
                
                if (i < it->second.vectorValue.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "]" << std::endl;
            resultRow++;
            if (saveResult) {
                result = Result(formattedValues);
            }
        }
        break;
    case VariableType::MATRIX:
        {
            std::cout << "\n"; // 为 "m = " 和矩阵内容之间提供一行间隔
            resultRow++;
            
            std::vector<std::vector<std::string>> formattedMatrix;
            for (size_t r = 0; r < it->second.matrixValue.rowCount(); ++r) {
                Terminal::setCursor(resultRow, 0);
                std::cout << "| ";
                std::vector<std::string> row;
                for (size_t c = 0; c < it->second.matrixValue.colCount(); ++c) {
                    std::string formattedValue = formatValue(it->second.matrixValue.at(r, c));
                    std::cout << std::setw(12) << formattedValue << " ";
                    row.push_back(formattedValue);
                }
                std::cout << "|" << std::endl;
                resultRow++;
                formattedMatrix.push_back(row);
            }
            if (saveResult) {
                result = Result(formattedMatrix);
            }
        }
        break;
    case VariableType::RESULT:
        std::cout << it->second.resultValue << std::endl;
        resultRow++;
        break;
    }

    // 如果需要保存结果，将其存储到指定变量
    if (saveResult && !resultVarName.empty()) {
        interpreter.getVariablesNonConst()[resultVarName] = Variable(result);
        statusMessage = "以 " + std::to_string(precision) + " 位有效数字显示变量: " + varName + "，结果已保存到: " + resultVarName;
    } else {
        statusMessage = "以 " + std::to_string(precision) + " 位有效数字显示变量: " + varName;
    }

    Terminal::resetColor();
}

void TuiApp::showVariableWithDecimalFormat(const std::string &varName, int decimalPlaces, bool saveResult, const std::string& resultVarName) {
    if (matrixEditor) return; // 不在编辑器模式下显示变量
    const auto &vars = interpreter.getVariables();
    auto it = vars.find(varName);

    // resultRow 当前指向命令行的下一行
    Terminal::setCursor(resultRow, 0);

    if (it == vars.end()) {
        Terminal::setForeground(Color::RED);
        std::cout << "错误: 变量 '" << varName << "' 未定义。" << std::endl;
        Terminal::resetColor();
        resultRow++;
        statusMessage = "变量未找到: " + varName;
        return;
    }

    Terminal::setForeground(Color::CYAN);
    std::cout << varName << " = ";

    // 使用 Boost 的任意精度浮点数类型
    using HighPrecisionFloat = boost::multiprecision::cpp_dec_float_100; // 100位精度

    // 辅助函数：格式化单个数值为小数点后指定位数显示
    auto formatValueDecimal = [&decimalPlaces](const Fraction& frac) -> std::string {
        try {
            // 使用任意精度浮点数进行除法运算
            HighPrecisionFloat numerator(frac.getNumerator().str());
            HighPrecisionFloat denominator(frac.getDenominator().str());
            HighPrecisionFloat result = numerator / denominator;
            
            // 检查溢出情况
            if (result > (std::numeric_limits<double>::max)()) {
                return "INF";
            } else if (result < (std::numeric_limits<double>::lowest)()) {
                return "-INF";
            }
            
            double fval = result.convert_to<double>();
            
            // 如果小数位数为0，显示为整数
            if (decimalPlaces == 0) {
                // 检查是否为整数（分母为1的情况）
                if (frac.getDenominator() == 1) {
                    return frac.getNumerator().str();
                } else {
                    // 四舍五入到最近的整数
                    long long rounded = static_cast<long long>(std::round(fval));
                    return std::to_string(rounded);
                }
            } else {
                // 设置固定小数位数格式
                std::stringstream ss;
                ss << std::fixed << std::setprecision(decimalPlaces) << fval;
                return ss.str();
            }
        } catch (const std::exception& e) {
            return "ERR";
        }
    };

    Result result;

    switch (it->second.type) {
    case VariableType::FRACTION:
        {
            std::string formattedValue = formatValueDecimal(it->second.fractionValue);
            std::cout << formattedValue << std::endl;
            resultRow++;
            if (saveResult) {
                result = Result(formattedValue);
            }
        }
        break;
    case VariableType::VECTOR:
        {
            std::cout << "[";
            std::vector<std::string> formattedValues;
            for (size_t i = 0; i < it->second.vectorValue.size(); ++i) {
                std::string formattedValue = formatValueDecimal(it->second.vectorValue.at(i));
                std::cout << formattedValue;
                formattedValues.push_back(formattedValue);
                
                if (i < it->second.vectorValue.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "]" << std::endl;
            resultRow++;
            if (saveResult) {
                result = Result(formattedValues);
            }
        }
        break;
    case VariableType::MATRIX:
        {
            std::cout << "\n"; // 为 "m = " 和矩阵内容之间提供一行间隔
            resultRow++;
            
            std::vector<std::vector<std::string>> formattedMatrix;
            for (size_t r = 0; r < it->second.matrixValue.rowCount(); ++r) {
                Terminal::setCursor(resultRow, 0);
                std::cout << "| ";
                std::vector<std::string> row;
                for (size_t c = 0; c < it->second.matrixValue.colCount(); ++c) {
                    std::string formattedValue = formatValueDecimal(it->second.matrixValue.at(r, c));
                    std::cout << std::setw(10) << formattedValue << " ";
                    row.push_back(formattedValue);
                }
                std::cout << "|" << std::endl;
                resultRow++;
                formattedMatrix.push_back(row);
            }
            if (saveResult) {
                result = Result(formattedMatrix);
            }
        }
        break;
    case VariableType::RESULT:
        std::cout << it->second.resultValue << std::endl;
        resultRow++;
        break;
    }

    // 如果需要保存结果，将其存储到指定变量
    if (saveResult && !resultVarName.empty()) {
        interpreter.getVariablesNonConst()[resultVarName] = Variable(result);
        std::string formatDesc = (decimalPlaces == 0) ? "整数格式" : (std::to_string(decimalPlaces) + " 位小数");
        statusMessage = "以 " + formatDesc + " 显示变量: " + varName + "，结果已保存到: " + resultVarName;
    } else {
        std::string formatDesc = (decimalPlaces == 0) ? "整数格式" : (std::to_string(decimalPlaces) + " 位小数");
        statusMessage = "以 " + formatDesc + " 显示变量: " + varName;
    }
    Terminal::resetColor();
}

void TuiApp::drawResultArea()
{
    if (matrixEditor) return; // 编辑器激活时不绘制主结果区
    clearResultArea(); // 这会重置 resultRow
}
// 进入步骤展示模式 - 操作历史版本
void TuiApp::enterStepDisplayMode(const OperationHistory& history) {
    if (matrixEditor) {
        LOG_WARNING("Attempted to enter step display mode while editor is active. Exiting editor first.");
        // Potentially save/discard editor changes here or prompt user
        matrixEditor.reset(); // Forcibly exit editor
        initUI();
    }
    if (history.size() == 0) return;
    
    inStepDisplayMode = true;
    currentStep = 0;
    totalSteps = history.size();
    currentHistory = history;
    isExpansionHistory = false;
    
    // resultRow 此时指向命令和最终结果（如果有）之后的下一可用行
    this->stepDisplayStartRow = this->resultRow;
    
    // displayCurrentStep 会从 stepDisplayStartRow 开始绘制
    displayCurrentStep();
    drawStepProgressBar();
    
    statusMessage = "步骤导航模式: 使用←→箭头浏览步骤, ESC退出";
    drawStatusBar();
}

// 进入步骤展示模式 - 行列式展开版本
void TuiApp::enterStepDisplayMode(const ExpansionHistory& history) {
     if (matrixEditor) {
        LOG_WARNING("Attempted to enter step display mode while editor is active. Exiting editor first.");
        matrixEditor.reset(); 
        initUI();
    }
    if (history.size() == 0) return;
    
    inStepDisplayMode = true;
    currentStep = 0;
    totalSteps = history.size();
    currentExpHistory = history;
    isExpansionHistory = true;

    this->stepDisplayStartRow = this->resultRow;
        
    displayCurrentStep();
    drawStepProgressBar();
    
    statusMessage = "步骤导航模式: 使用←→箭头浏览步骤, ESC退出";
    drawStatusBar();
}

// 退出步骤展示模式
void TuiApp::exitStepDisplayMode() {
    inStepDisplayMode = false;
    
    // 清除结果区域
    clearResultArea();
    
    // 更新状态消息
    statusMessage = "已退出步骤导航模式";
    drawStatusBar();
}

// 新增：矩阵编辑模式相关函数实现 - 这些将被移除或替换
std::string TuiApp::generateNewVariableName(bool isMatrix) {
    const auto& vars = interpreter.getVariables();
    int i = 1;
    std::string baseName = isMatrix ? "m" : "v";
    std::string newName;
    while (true) {
        newName = baseName + std::to_string(i);
        if (vars.find(newName) == vars.end()) {
            return newName;
        }
        i++;
    }
}

// 移除旧的 enterMatrixEditMode, exitMatrixEditMode, drawMatrixEditor, handleMatrixEditInput
// void TuiApp::enterMatrixEditMode(const std::string& varName, bool isNew, bool isMatrix, int rows, int cols) { ... }
// void TuiApp::exitMatrixEditMode(bool saveChanges) { ... }
// void TuiApp::drawMatrixEditor() { ... }
// void TuiApp::handleMatrixEditInput(int key) { ... }


// 显示当前步骤
void TuiApp::displayCurrentStep() {
    if (matrixEditor) return; // 不在编辑器模式下显示步骤
    // 清除旧的步骤内容区域 (从 stepDisplayStartRow 开始，直到进度条之前)
    // 进度条在 inputRow - 2, 进度条上的数字在 inputRow - 3
    // 所以步骤内容区域最多到 inputRow - 4
    int endClearRow = inputRow - 1; // 默认清除到输入行前一行
    if (totalSteps > 0) { // 如果有步骤，则有进度条
        endClearRow = inputRow - 3; // 清除到进度条数字的上一行
    }
    if (endClearRow < stepDisplayStartRow) endClearRow = stepDisplayStartRow; // 防止负范围

    for (int i = stepDisplayStartRow; i < endClearRow; i++) {
        Terminal::setCursor(i, 0);
        std::string spaces(terminalCols, ' ');
        std::cout << spaces;
    }
    
    // 在 stepDisplayStartRow 显示当前步骤信息 ("步骤 X / Y:")
    Terminal::setCursor(stepDisplayStartRow, 0);
    Terminal::setForeground(Color::YELLOW);
    std::cout << "步骤 " << (currentStep + 1) << " / " << totalSteps << ":" << std::endl;
    Terminal::resetColor();
    
    // 在下一行 (stepDisplayStartRow + 1) 开始显示步骤内容
    // 确保有足够的空间显示步骤内容
    if (stepDisplayStartRow + 1 < endClearRow) {
        Terminal::setCursor(stepDisplayStartRow + 1, 0); 
        Terminal::setForeground(Color::CYAN);
        // 捕获步骤打印的输出，以便正确更新光标和处理多行
        std::stringstream step_ss;
        if (isExpansionHistory) {
            currentExpHistory.getStep(currentStep).print(step_ss);
        } else {
            currentHistory.getStep(currentStep).print(step_ss);
        }
        
        std::string step_line;
        int current_print_row = stepDisplayStartRow + 1;
        std::istringstream step_iss(step_ss.str());
        while(std::getline(step_iss, step_line) && current_print_row < endClearRow) {
            Terminal::setCursor(current_print_row++, 0);
            std::cout << step_line << std::endl;
        }
        Terminal::resetColor();
    }
}

// 绘制步骤进度条
void TuiApp::drawStepProgressBar() {
    if (matrixEditor) return; // 不在编辑器模式下绘制进度条
    // 计算进度条位置
    int barRow = inputRow - 2;
    int barWidth = terminalCols - 10; // 留出左右边距
    int barStart = 5;
    
    // 清除进度条行
    Terminal::setCursor(barRow, 0);
    std::string spaces(terminalCols, ' ');
    std::cout << spaces;
    
    // 绘制进度条轨道
    Terminal::setCursor(barRow, barStart);
    Terminal::setForeground(Color::WHITE);
    std::cout << "[";
    for (int i = 0; i < barWidth; i++) {
        std::cout << "-";
    }
    std::cout << "]";

    // 计算当前步骤指示器位置
    int indicatorPos = barStart + 1;
    if (totalSteps > 1) {
        indicatorPos += static_cast<int>((static_cast<double>(currentStep) / (totalSteps - 1)) * barWidth);
    }
    
    // 绘制当前步骤指示器
    Terminal::setCursor(barRow, indicatorPos);
    Terminal::setForeground(Color::GREEN);
    std::cout << "◆";
    
    // 在进度条上方显示步骤号
    Terminal::setCursor(barRow - 1, indicatorPos - 1);
    std::cout << currentStep + 1;
    
    Terminal::resetColor();
}

// 新增辅助函数实现
std::vector<std::string> TuiApp::getVariableNames() const {
    std::vector<std::string> names;
    for (const auto& pair : interpreter.getVariables()) {
        names.push_back(pair.first);
    }
    return names;
}

// 修改：获取当前输入单词以供建议，并返回其在 currentInput 中的起始位置
std::string TuiApp::getCurrentWordForSuggestion(size_t& wordStartPosInInput) const {
    wordStartPosInInput = 0; // Default to 0
    if (currentInput.empty() || cursorPosition == 0) {
        return "";
    }

    size_t endPos = cursorPosition;
        // 如果光标前的字符是空格，则不认为正在输入一个可建议的单词的中间或末尾
        // (除非这是为了开始一个新词，但那时 prefix 为空，由 updateSuggestions 处理)
        // 如果光标前的字符是空格或左括号，则不认为正在输入一个可建议的单词的中间或末尾
    if (std::isspace(currentInput[endPos - 1]) || currentInput[endPos - 1] == '(') { 
        return "";
    }

    size_t startPos = endPos;
    while (startPos > 0 && !std::isspace(currentInput[startPos - 1]) && currentInput[startPos - 1] != '(') {
        startPos--;
    }
    
    // If startPos is still == endPos here, it means cursor was at pos 0 or after a space.
    // But we checked for currentInput[endPos-1] not being a space.
    // So, startPos should be < endPos.
    
    wordStartPosInInput = startPos; // Store the start position of the word in the input string
    return currentInput.substr(startPos, endPos - startPos);
}
