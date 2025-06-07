#include "tui_app.h"
#include "tui_terminal.h" // KEY_... 常量现在从此头文件获得
#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>
#include "../utils/logger.h"
#include "../grammar/grammar_tokenizer.h"
#include "../grammar/grammar_parser.h"

// 新增：定义输出区域的布局常量
const int RESULT_AREA_TITLE_ROW = 2; // "输出区域:" 标题所在的行 (0-indexed)
const int RESULT_AREA_CONTENT_START_ROW = RESULT_AREA_TITLE_ROW + 1; // 实际内容开始的第一行
const int MATRIX_EDITOR_CELL_WIDTH = 8; // 矩阵编辑器单元格宽度

TuiApp::TuiApp() 
    : historyIndex(0), running(true), 
      inStepDisplayMode(false), currentStep(0), totalSteps(0), isExpansionHistory(false),
      cursorPosition(0), // 初始化光标位置
      inMatrixEditMode(false), editCursorRow(0), editCursorCol(0), cellInputActive(false), // 初始化矩阵编辑模式状态
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
        // 如果在矩阵编辑模式，则调用 drawMatrixEditor，否则调用 updateUI
        if (inMatrixEditMode) {
            drawMatrixEditor(); // 绘制编辑器界面
            drawStatusBar();    // 状态栏仍然需要更新
        } else {
            updateUI();
        }

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
    drawInputPrompt();
    drawStatusBar();
    drawResultArea(); // 这会调用 clearResultArea，进而设置 this->resultRow
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
        initUI();
    }

    // 更新输入提示行
    drawInputPrompt();

    // 更新状态栏
    drawStatusBar();
}

void TuiApp::handleInput()
{
    // 读取一个字符
    int key = Terminal::readChar();

    if (inMatrixEditMode) {
        handleMatrixEditInput(key);
        return;
    }

    // 如果处于步骤显示模式，处理导航键
    if (inStepDisplayMode)
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
            if (currentInput.empty())
            {
                running = false; // 如果输入为空，退出程序
            }
            else
            {
                currentInput.clear(); // 否则清空当前输入
                cursorPosition = 0;
                drawInputPrompt();
            }
        }
    }
    else if (key == KEY_ENTER)
    {
        // 如果输入不为空，执行命令
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
            drawInputPrompt();
        }
    }
    else if (key == KEY_BACKSPACE)
    {
        // 删除光标前一个字符
        if (cursorPosition > 0 && !currentInput.empty())
        {
            currentInput.erase(cursorPosition - 1, 1);
            cursorPosition--;
            drawInputPrompt();
        }
    }
    // 注意: KEY_DELETE 的处理需要 tui_terminal.cpp 中的 readChar 正确返回一个可识别的 KEY_DELETE 值
    // 假设 KEY_DELETE 在 tui_terminal.h 中定义，并且 readChar 能返回它
    // else if (key == KEY_DELETE) { // 处理 Delete 键
    //     if (cursorPosition < currentInput.length()) {
    //         currentInput.erase(cursorPosition, 1);
    //         drawInputPrompt();
    //     }
    // }
    else if (key >= 32 && key <= 126) // 可打印字符
    {
        currentInput.insert(cursorPosition, 1, static_cast<char>(key));
        cursorPosition++;
        drawInputPrompt();
        // 如果用户在历史导航时输入，则将当前输入视为新命令
        if (historyIndex != 0) {
            historyIndex = 0; // 不再处于历史导航状态
            // tempInputBuffer 将在下次按 UP 时重新填充，或在按 ENTER 时清除
        }
    }
    else // 处理其他特殊键，如箭头键，这些可能由 readChar 直接返回定义好的常量
    {
        handleSpecialKey(key); // 将其他键传递给 handleSpecialKey
    }
}

void TuiApp::printToResultView(const std::string& text, Color color) {
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
    try
    {
        // 确保输入非空
        if (input.empty())
        {
            return;
        }

        LOG_INFO("执行命令: " + input);

        // 1. 每次执行命令前先清除结果区域
        clearResultArea(); // 这会将 resultRow 设置为 RESULT_AREA_CONTENT_START_ROW

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


        // 处理new命令
        if (commandStr == "new") {
            if (commandArgs.size() == 1) { // new N (vector)
                int dim = std::stoi(commandArgs[0]);
                if (dim <= 0) throw std::invalid_argument("向量维度必须为正。");
                std::string newName = generateNewVariableName(false);
                interpreter.getVariablesNonConst()[newName] = Variable(Vector(dim)); // 使用 non-const 版本
                enterMatrixEditMode(newName, true, false, dim, 1);
            } else if (commandArgs.size() == 2) { // new R C (matrix)
                int r = std::stoi(commandArgs[0]);
                int c = std::stoi(commandArgs[1]);
                if (r <= 0 || c <= 0) throw std::invalid_argument("矩阵行列数必须为正。");
                std::string newName = generateNewVariableName(true);
                interpreter.getVariablesNonConst()[newName] = Variable(Matrix(r, c)); // 使用 non-const 版本
                enterMatrixEditMode(newName, true, true, r, c);
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
            const Variable& varToEdit = interpreter.getVariables().at(varName);
            if (varToEdit.type == VariableType::MATRIX) {
                enterMatrixEditMode(varName, false, true, varToEdit.matrixValue.rowCount(), varToEdit.matrixValue.colCount());
            } else if (varToEdit.type == VariableType::VECTOR) {
                enterMatrixEditMode(varName, false, false, varToEdit.vectorValue.size(), 1);
            } else {
                throw std::runtime_error("变量 '" + varName + "' 不是矩阵或向量，无法编辑。");
            }
            return;
        }


        // 处理show命令
        if (processedInput.substr(0, 4) == "show" && processedInput.length() > 5)
        {
            std::string varName = processedInput.substr(5);
            // 去除可能的结尾分号
            if (!varName.empty() && varName.back() == ';')
            {
                varName.pop_back();
            }
            // showVariable 内部使用 resultRow，它现在是命令之后的那一行
            showVariable(varName);
            return;
        }

        // 处理help命令
        if (processedInput == "help" || processedInput == "help;")
        {
            // showHelp 内部使用 resultRow
            showHelp();
            return;
        }

        // 处理clear命令
        if (processedInput == "clear" || processedInput == "clear;")
        {
            // 清除整个屏幕并重绘UI
            initUI();
            statusMessage = "屏幕已清除";
            return;
        }

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
    // resultRow 当前指向命令行的下一行
    Terminal::setCursor(resultRow, 0); // 从 resultRow 开始打印帮助信息
    Terminal::setForeground(Color::CYAN);
    std::cout << "帮助信息：\n"; // 示例：这会占用 resultRow
    resultRow++;                 // 更新 resultRow
    Terminal::setCursor(resultRow, 0);
    std::cout << "  help             - 显示此帮助信息\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  clear            - 清屏\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  vars             - 显示所有变量\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  show <变量名>     - 显示特定变量的值\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  exit             - 退出程序\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  steps            - 切换计算步骤显示\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  new <行数> <列数> - 创建一个新的矩阵变量 (例如: new 2 3)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  edit <变量名>    - 编辑已存在的矩阵或向量变量\n";
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
    std::cout << "        m1 = m2 * f1 (数乘), m1 = f1 * m2 (数乘)\n";
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
    std::cout << "        v1 = v2 * f1 (数乘), v1 = f1 * v2 (数乘)\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  分数: f3 = f1 + f2, f3 = f1 - f2, f3 = f1 * f2, f3 = f1 / f2\n";
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
        std::cout << std::endl; // 为 "m = " 和矩阵内容之间提供一行间隔
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
    }
    Terminal::resetColor();
    statusMessage = "显示变量: " + varName;
}

void TuiApp::drawResultArea()
{
    clearResultArea(); // 这会重置 resultRow
}
// 进入步骤展示模式 - 操作历史版本
void TuiApp::enterStepDisplayMode(const OperationHistory& history) {
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

// 新增：矩阵编辑模式相关函数实现
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

void TuiApp::enterMatrixEditMode(const std::string& varName, bool isNew, bool isMatrix, int rows, int cols) {
    inMatrixEditMode = true;
    editingVariableName = varName;
    editingIsMatrix = isMatrix;

    if (isNew) {
        // 变量已在 executeCommand 中创建并加入 interpreter.variables
        // editingVariableCopy 将从 interpreter 中获取最新的（刚创建的）
    }
    // 从解释器获取变量的副本进行编辑
    editingVariableCopy = interpreter.getVariables().at(varName); // 读取时仍可用 const 版本


    editCursorRow = 0;
    editCursorCol = 0;
    cellInputActive = false;
    currentCellInput.clear();

    clearResultArea(); // 清除主输出区域为编辑器腾出空间
    // drawMatrixEditor(); // 将由主循环的 updateUI 部分调用
    statusMessage = "矩阵编辑模式: 方向键移动, Enter编辑, ESC保存并退出";
    // drawStatusBar(); // 将由主循环的 updateUI 部分调用
}

void TuiApp::exitMatrixEditMode(bool saveChanges) {
    if (saveChanges) {
        // 如果 cellInputActive，则先尝试保存当前单元格的输入
        if (cellInputActive) {
            try {
                Fraction f_val;
                std::string temp_input = currentCellInput;
                if (temp_input.find('/') != std::string::npos) {
                    size_t slash_pos = temp_input.find('/');
                    long long num = std::stoll(temp_input.substr(0, slash_pos));
                    long long den = std::stoll(temp_input.substr(slash_pos + 1));
                    f_val = Fraction(num, den);
                } else {
                    f_val = Fraction(std::stoll(temp_input));
                }

                if (editingIsMatrix) {
                    editingVariableCopy.matrixValue.at(editCursorRow, editCursorCol) = f_val;
                } else { // Vector
                    editingVariableCopy.vectorValue.at(editCursorRow) = f_val;
                }
            } catch (const std::exception& e) {
                // 解析失败，忽略当前单元格的更改
                LOG_WARNING("退出编辑模式时，当前单元格输入解析失败: " + currentCellInput + " - " + e.what());
            }
        }
        // 将编辑后的副本保存回解释器
        interpreter.getVariablesNonConst()[editingVariableName] = editingVariableCopy; // 使用 non-const 版本
        statusMessage = "更改已保存到 " + editingVariableName;
    } else {
        statusMessage = "已退出编辑模式，更改未保存";
    }

    inMatrixEditMode = false;
    cellInputActive = false;
    currentCellInput.clear();
    
    initUI(); // 重绘标准UI
}

void TuiApp::drawMatrixEditor() {
    // 清除编辑器区域 (标题行以下，输入行以上)
    // RESULT_AREA_TITLE_ROW (e.g. 2) is "输出区域:"
    // We can reuse this area.
    for (int i = RESULT_AREA_CONTENT_START_ROW; i < inputRow; ++i) {
        Terminal::setCursor(i, 0);
        std::cout << std::string(terminalCols, ' ');
    }

    Terminal::setCursor(RESULT_AREA_TITLE_ROW, 0);
    Terminal::setForeground(Color::YELLOW);
    std::cout << "编辑 " << (editingIsMatrix ? "矩阵 " : "向量 ") << editingVariableName << ":" << std::endl;
    Terminal::resetColor();

    int displayStartRow = RESULT_AREA_CONTENT_START_ROW; // 内容从 "输出区域:" 下一行开始

    size_t numRows = editingIsMatrix ? editingVariableCopy.matrixValue.rowCount() : editingVariableCopy.vectorValue.size();
    size_t numCols = editingIsMatrix ? editingVariableCopy.matrixValue.colCount() : 1;

    for (size_t r = 0; r < numRows; ++r) {
        if (displayStartRow + (int)r >= inputRow) break; // 防止超出屏幕
        Terminal::setCursor(displayStartRow + r, 1); // 留出左边距

        if (editingIsMatrix) std::cout << "| ";

        for (size_t c = 0; c < numCols; ++c) {
            bool isSelectedCell = (r == editCursorRow && c == editCursorCol);
            
            if (isSelectedCell) {
                Terminal::setBackground(Color::WHITE);
                Terminal::setForeground(Color::BLACK);
            } else {
                Terminal::setForeground(Color::CYAN);
            }

            std::string cellStr;
            if (isSelectedCell && cellInputActive) {
                cellStr = currentCellInput;
                // 添加一个闪烁光标的模拟，或仅显示输入
                if (cellStr.length() < MATRIX_EDITOR_CELL_WIDTH) {
                     cellStr += "_"; // 简单光标
                }
            } else {
                Fraction val = editingIsMatrix ? editingVariableCopy.matrixValue.at(r, c) : editingVariableCopy.vectorValue.at(r);
                std::ostringstream oss;
                oss << val;
                cellStr = oss.str();
            }

            // 格式化输出以适应单元格宽度
            if (cellStr.length() > MATRIX_EDITOR_CELL_WIDTH) {
                std::cout << cellStr.substr(0, MATRIX_EDITOR_CELL_WIDTH);
            } else {
                std::cout << cellStr;
                for (size_t pad = cellStr.length(); pad < MATRIX_EDITOR_CELL_WIDTH; ++pad) {
                    std::cout << " ";
                }
            }
            
            if (isSelectedCell) {
                Terminal::resetColor(); // 重置高亮单元格的颜色
            }
             Terminal::setForeground(Color::CYAN); // 确保后续分隔符颜色正确
            if (editingIsMatrix && c < numCols -1) std::cout << " "; // 列间距
        }
        if (editingIsMatrix) std::cout << " |";
        Terminal::resetColor(); // 重置行末颜色
    }
}


void TuiApp::handleMatrixEditInput(int key) {
    size_t maxRows = editingIsMatrix ? editingVariableCopy.matrixValue.rowCount() : editingVariableCopy.vectorValue.size();
    size_t maxCols = editingIsMatrix ? editingVariableCopy.matrixValue.colCount() : 1;

    if (cellInputActive) {
        switch (key) {
            case KEY_ENTER: {
                try {
                    Fraction f_val;
                    if (currentCellInput.empty()) { // 如果为空，则视为0
                        f_val = Fraction(0);
                    } else if (currentCellInput.find('/') != std::string::npos) {
                        size_t slash_pos = currentCellInput.find('/');
                        long long num = std::stoll(currentCellInput.substr(0, slash_pos));
                        long long den = std::stoll(currentCellInput.substr(slash_pos + 1));
                        if (den == 0) throw std::invalid_argument("分母不能为零");
                        f_val = Fraction(num, den);
                    } else {
                        f_val = Fraction(std::stoll(currentCellInput));
                    }

                    if (editingIsMatrix) {
                        editingVariableCopy.matrixValue.at(editCursorRow, editCursorCol) = f_val;
                    } else { // Vector
                        editingVariableCopy.vectorValue.at(editCursorRow) = f_val;
                    }
                } catch (const std::exception& e) {
                    statusMessage = "错误: 无效的分数格式 - " + std::string(e.what());
                    return; 
                }
                cellInputActive = false;
                currentCellInput.clear();
                statusMessage = "矩阵编辑模式: 方向键移动, Enter编辑, ESC保存并退出";
                break;
            }
            case KEY_ESCAPE:
                cellInputActive = false;
                currentCellInput.clear(); // 取消编辑，清除当前输入
                statusMessage = "矩阵编辑模式: 方向键移动, Enter编辑, ESC保存并退出";
                break;
            case KEY_BACKSPACE:
                if (!currentCellInput.empty()) {
                    currentCellInput.pop_back();
                }
                break;
            case KEY_UP:
            case KEY_DOWN:
            case KEY_LEFT:
            case KEY_RIGHT: {
                bool errorOccurred = false;
                if (!currentCellInput.empty()) {
                    try {
                        Fraction f_val;
                        if (currentCellInput.find('/') != std::string::npos) {
                            size_t slash_pos = currentCellInput.find('/');
                            long long num = std::stoll(currentCellInput.substr(0, slash_pos));
                            long long den = std::stoll(currentCellInput.substr(slash_pos + 1));
                            if (den == 0) throw std::invalid_argument("分母不能为零");
                            f_val = Fraction(num, den);
                        } else {
                            f_val = Fraction(std::stoll(currentCellInput));
                        }

                        if (editingIsMatrix) {
                            editingVariableCopy.matrixValue.at(editCursorRow, editCursorCol) = f_val;
                        } else { // Vector
                            editingVariableCopy.vectorValue.at(editCursorRow) = f_val;
                        }
                    } catch (const std::exception& e) {
                        statusMessage = "错误: 无效输入 '" + currentCellInput + "', 未保存。";
                        errorOccurred = true;
                    }
                }
                // If currentCellInput was empty, or parsing failed, editingVariableCopy.at(row,col) is not changed by this attempt.

                cellInputActive = false;
                currentCellInput.clear();
                
                if (!errorOccurred) {
                    statusMessage = "矩阵编辑模式: 方向键移动, Enter编辑, ESC保存并退出";
                }

                // Perform navigation
                if (key == KEY_UP) {
                    if (editCursorRow > 0) editCursorRow--;
                } else if (key == KEY_DOWN) {
                    if (editCursorRow < maxRows - 1) editCursorRow++;
                } else if (key == KEY_LEFT) {
                    if (editCursorCol > 0) editCursorCol--;
                } else if (key == KEY_RIGHT) {
                    if (editCursorCol < maxCols - 1) editCursorCol++;
                }
                return; // Key handled
            }
            default:
                if (key >= 32 && key <= 126) { 
                    if (currentCellInput.length() < MATRIX_EDITOR_CELL_WIDTH -1 ) { 
                        if (std::isdigit(key) || key == '/' || key == '-' || 
                            (key == '.' && currentCellInput.find('.') == std::string::npos) ) { 
                            currentCellInput += static_cast<char>(key);
                        }
                    }
                }
                break;
        }
    } else { // Navigation mode (cellInputActive is false)
        switch (key) {
            case KEY_UP:
                if (editCursorRow > 0) editCursorRow--;
                break;
            case KEY_DOWN:
                if (editCursorRow < maxRows - 1) editCursorRow++;
                break;
            case KEY_LEFT:
                if (editCursorCol > 0) editCursorCol--;
                break;
            case KEY_RIGHT:
                if (editCursorCol < maxCols - 1) editCursorCol++;
                break;
            case KEY_ENTER: {
                cellInputActive = true;
                Fraction val = editingIsMatrix ? editingVariableCopy.matrixValue.at(editCursorRow, editCursorCol) : editingVariableCopy.vectorValue.at(editCursorRow);
                std::ostringstream oss;
                oss << val;
                currentCellInput = oss.str(); // Populate with current cell value for editing
                statusMessage = "编辑单元格 (" + std::to_string(editCursorRow+1) + "," + std::to_string(editCursorCol+1) + "): Enter确认, ESC取消, 方向键保存并移动";
                break;
            }
            case KEY_ESCAPE:
                exitMatrixEditMode(true); 
                return; 
            default:
                if (std::isdigit(key) || key == '-') { // Start editing directly if a number or minus is pressed
                    cellInputActive = true;
                    currentCellInput = static_cast<char>(key);
                    statusMessage = "编辑单元格 (" + std::to_string(editCursorRow+1) + "," + std::to_string(editCursorCol+1) + "): Enter确认, ESC取消, 方向键保存并移动";
                }
                break;
        }
    }
}

// 显示当前步骤
void TuiApp::displayCurrentStep() {
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
