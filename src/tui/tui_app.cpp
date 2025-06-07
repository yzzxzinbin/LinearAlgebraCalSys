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

TuiApp::TuiApp() 
    : historyIndex(0), running(true), 
      inStepDisplayMode(false), currentStep(0), totalSteps(0), isExpansionHistory(false),
      cursorPosition(0) // 初始化光标位置
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
        updateUI();

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
    // 在结果区域显示文本
    Terminal::setCursor(resultRow++, 0);
    Terminal::setForeground(color);
    std::cout << text << std::endl;
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

        // 每次执行命令前先清除结果区域
        clearResultArea();

        // 总是先显示命令输入
        Terminal::setCursor(resultRow, 0);
        Terminal::setForeground(Color::GREEN);
        std::cout << "> " << input << std::endl;
        Terminal::resetColor();

        // 预处理输入，处理特殊情况
        std::string processedInput = input;

        // 处理show命令
        if (processedInput.substr(0, 4) == "show" && processedInput.length() > 5)
        {
            std::string varName = processedInput.substr(5);
            // 去除可能的结尾分号
            if (!varName.empty() && varName.back() == ';')
            {
                varName.pop_back();
            }

            showVariable(varName);
            return;
        }

        // 处理help命令
        if (processedInput == "help" || processedInput == "help;")
        {
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

        // 显示结果（这部分可能需要调整，因为步骤显示模式也会清屏）
        // 如果进入步骤模式，这些输出会被覆盖，但如果没进入，则需要它们
        // 考虑在不进入步骤模式时才显示这些
        
        bool enteredStepMode = false;
        if (interpreter.isShowingSteps()) {
            const auto& opHistory = interpreter.getCurrentOpHistory();
            if (opHistory.size() > 0) {
                LOG_INFO("进入步骤展示模式 (OperationHistory), 步骤数: " + std::to_string(opHistory.size()));
                // 清除结果区域并显示最终结果，然后进入步骤模式
                clearResultArea();
                printToResultView("> " + input, Color::GREEN);
                if (ast->type != AstNodeType::COMMAND) { // 只为非命令显示结果
                    printToResultView("= " + variableToString(result), Color::CYAN);
                }
                enterStepDisplayMode(opHistory);
                enteredStepMode = true;
            }

            if (!enteredStepMode) { // 避免重复进入或检查 ExpansionHistory 如果 OperationHistory 已处理
                const auto& expHistory = interpreter.getCurrentExpHistory();
                if (expHistory.size() > 0) {
                    LOG_INFO("进入步骤展示模式 (ExpansionHistory), 步骤数: " + std::to_string(expHistory.size()));
                    clearResultArea();
                    printToResultView("> " + input, Color::GREEN);
                     if (ast->type != AstNodeType::COMMAND) {
                        printToResultView("= " + variableToString(result), Color::CYAN);
                    }
                    enterStepDisplayMode(expHistory);
                    enteredStepMode = true;
                }
            }
        }

        if (!enteredStepMode) {
            // 如果没有进入步骤模式，正常显示结果
            Terminal::setCursor(resultRow, 0); // 确保光标在正确位置
            Terminal::setForeground(Color::GREEN);
            std::cout << "> " << input << std::endl;
            Terminal::resetColor();

            if (ast->type != AstNodeType::COMMAND) { // 只为非命令显示结果
                Terminal::setForeground(Color::CYAN);
                switch (result.type) {
                    case VariableType::FRACTION:
                        std::cout << "= " << result.fractionValue << std::endl;
                        break;
                    case VariableType::VECTOR:
                        std::cout << "= ";
                        result.vectorValue.print();
                        std::cout << std::endl;
                        break;
                    case VariableType::MATRIX:
                        std::cout << "= " << std::endl;
                        result.matrixValue.print();
                        break;
                }
                Terminal::resetColor();
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
    // 从当前光标位置开始显示，而不是重新设置位置
    // resultRow + 1 是因为我们已经显示了命令行
    Terminal::setCursor(resultRow + 1, 0);
    Terminal::setForeground(Color::CYAN);
    std::cout << "帮助信息：\n";
    std::cout << "  help             - 显示此帮助信息\n";
    std::cout << "  clear            - 清屏\n";
    std::cout << "  vars             - 显示所有变量\n";
    std::cout << "  show <变量名>     - 显示特定变量的值\n";
    std::cout << "  exit             - 退出程序\n";
    std::cout << "  steps            - 切换计算步骤显示\n";
    std::cout << "\n";
    std::cout << "变量定义:\n";
    std::cout << "  m1 = [1,2,3;4,5,6] - 定义矩阵\n";
    std::cout << "  v1 = [1,2,3]       - 定义向量\n";
    std::cout << "  f1 = 3/4           - 定义分数\n";
    std::cout << "\n";
    std::cout << "基本运算:\n";
    std::cout << "  矩阵: m3 = m1 + m2, m3 = m1 - m2, m3 = m1 * m2 (矩阵乘法)\n";
    std::cout << "        m1 = m2 * f1 (数乘), m1 = f1 * m2 (数乘)\n";
    std::cout << "  向量: v3 = v1 + v2, v3 = v1 - v2\n";
    std::cout << "        f_dot = v1 * v2    - 向量点积 (返回分数)\n";
    std::cout << "        v_cross = v1 x v2  - 向量叉积 (返回向量, 仅3D)\n";
    std::cout << "        v1 = v2 * f1 (数乘), v1 = f1 * v2 (数乘)\n";
    std::cout << "  分数: f3 = f1 + f2, f3 = f1 - f2, f3 = f1 * f2, f3 = f1 / f2\n";
    std::cout << "\n";
    std::cout << "矩阵函数:\n";
    std::cout << "  m2 = transpose(m1)        - 矩阵转置\n";
    std::cout << "  m2 = inverse(m1)          - 计算逆矩阵(伴随矩阵法)\n";
    std::cout << "  m2 = inverse_gauss(m1)    - 计算逆矩阵(高斯-若尔当法)\n";
    std::cout << "  f1 = det(m1)              - 计算行列式(默认方法)\n";
    std::cout << "  f1 = det_expansion(m1)    - 按行列展开计算行列式\n";
    std::cout << "  f1 = rank(m1)             - 计算矩阵秩\n";
    std::cout << "  m2 = ref(m1)              - 行阶梯形\n";
    std::cout << "  m2 = rref(m1)             - 最简行阶梯形\n";
    std::cout << "  m2 = cofactor_matrix(m1)  - 计算代数余子式矩阵\n";
    std::cout << "  m2 = adjugate(m1)         - 计算伴随矩阵\n";
    std::cout << "\n";
    std::cout << "向量函数:\n";
    std::cout << "  f1 = dot(v1, v2)          - 计算向量点积 (同 v1 * v2)\n";
    std::cout << "  v3 = cross(v1, v2)        - 计算向量叉积 (同 v1 x v2, 仅3D)\n";
    std::cout << "  f1 = norm(v1)             - 计算向量的模长的平方\n";
    std::cout << "  v1 = normalize(v1)        - 向量归一化(返回 v / (norm(v)) )\n";
    Terminal::resetColor();

    // 更新状态消息
    statusMessage = "已显示帮助信息";
}

void TuiApp::showVariables()
{
    // 从当前光标位置开始显示，而不是重新设置位置
    // resultRow + 1 是因为我们已经显示了命令行
    Terminal::setCursor(resultRow + 1, 0);
    const auto &vars = interpreter.getVariables();

    if (vars.empty())
    {
        Terminal::setForeground(Color::YELLOW);
        std::cout << "没有已定义的变量。\n";
        Terminal::resetColor();
        return;
    }

    Terminal::setForeground(Color::CYAN);
    std::cout << "已定义的变量：\n";

    for (const auto &pair : vars)
    {
        std::cout << "  " << pair.first << " = ";

        switch (pair.second.type)
        {
        case VariableType::FRACTION:
            std::cout << pair.second.fractionValue << "\n";
            break;
        case VariableType::VECTOR:
            pair.second.vectorValue.print();
            std::cout << "\n";
            break;
        case VariableType::MATRIX:
            std::cout << "\n";
            pair.second.matrixValue.print();
            break;
        }
    }

    Terminal::resetColor();

    // 更新状态消息
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

    // resultRow + 1 是因为我们已经显示了命令行
    Terminal::setCursor(resultRow + 1, 0);

    if (it == vars.end())
    {
        Terminal::setForeground(Color::RED);
        std::cout << "错误: 变量 '" << varName << "' 未定义。" << std::endl;
        Terminal::resetColor();

        // 更新状态消息
        statusMessage = "变量未找到: " + varName;
        return;
    }

    Terminal::setForeground(Color::CYAN);
    std::cout << varName << " = ";

    switch (it->second.type)
    {
    case VariableType::FRACTION:
        std::cout << it->second.fractionValue << std::endl;
        break;
    case VariableType::VECTOR:
        std::cout << std::endl;
        it->second.vectorValue.print();
        break;
    case VariableType::MATRIX:
        std::cout << std::endl;
        it->second.matrixValue.print();
        break;
    }

    Terminal::resetColor();

    // 更新状态消息
    statusMessage = "显示变量: " + varName;
}

void TuiApp::drawResultArea()
{
    clearResultArea();
}

// 进入步骤展示模式 - 操作历史版本
void TuiApp::enterStepDisplayMode(const OperationHistory& history) {
    if (history.size() == 0) return;
    
    inStepDisplayMode = true;
    currentStep = 0;
    totalSteps = history.size();
    currentHistory = history;
    isExpansionHistory = false;
    
    // 清屏并显示第一步
    clearResultArea();
    displayCurrentStep();
    drawStepProgressBar();
    
    // 更新状态消息
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
    
    // 清屏并显示第一步
    clearResultArea();
    displayCurrentStep();
    drawStepProgressBar();
    
    // 更新状态消息
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

// 显示当前步骤
void TuiApp::displayCurrentStep() {
    // 清除旧的步骤内容区域 (在 "步骤 X / Y:" 行之下)
    for (int i = RESULT_AREA_TITLE_ROW + 2; i < inputRow; i++) { // 从 "步骤 X/Y:" 下一行开始清除
        Terminal::setCursor(i, 0);
        std::string spaces(terminalCols, ' ');
        std::cout << spaces;
    }
    
    // 在结果区域固定位置显示当前步骤信息 ("步骤 X / Y:")
    Terminal::setCursor(RESULT_AREA_TITLE_ROW + 1, 0); // 例如，行 3
    Terminal::setForeground(Color::YELLOW);
    std::cout << "步骤 " << (currentStep + 1) << " / " << totalSteps << ":" << std::endl;
    Terminal::resetColor();
    
    // 根据历史类型在固定位置显示步骤内容
    // 例如，在 "步骤 X / Y:" 之后空一行开始打印，即行 RESULT_AREA_TITLE_ROW + 3
    Terminal::setCursor(RESULT_AREA_TITLE_ROW + 3, 0); // 例如，行 5
    Terminal::setForeground(Color::CYAN);
    if (isExpansionHistory) {
        currentExpHistory.getStep(currentStep).print(std::cout);
    } else {
        currentHistory.getStep(currentStep).print(std::cout);
    }
    Terminal::resetColor();
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
