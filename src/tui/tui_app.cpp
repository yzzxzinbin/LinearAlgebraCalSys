#include "tui_app.h"
#include "tui_terminal.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>
#include "../utils/logger.h"

// 定义特殊键码
const int KEY_ENTER = 13;
const int KEY_ESCAPE = 27;
const int KEY_BACKSPACE = 8;
const int KEY_DELETE = 127;
const int KEY_UP = 256;
const int KEY_DOWN = 257;
const int KEY_LEFT = 258;
const int KEY_RIGHT = 259;

TuiApp::TuiApp() : historyIndex(0), running(true)
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
    resultRow = 3;

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
    drawResultArea();
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
                case 'A':
                    handleSpecialKey(KEY_UP);
                    break; // 上箭头
                case 'B':
                    handleSpecialKey(KEY_DOWN);
                    break; // 下箭头
                case 'C':
                    handleSpecialKey(KEY_RIGHT);
                    break; // 右箭头
                case 'D':
                    handleSpecialKey(KEY_LEFT);
                    break; // 左箭头
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

            historyIndex = 0;
            currentInput.clear();
            drawInputPrompt();
        }
    }
    else if (key == KEY_BACKSPACE)
    {
        // 删除最后一个字符
        if (!currentInput.empty())
        {
            currentInput.pop_back();
            drawInputPrompt();
        }
    }
    else if (key >= 32 && key <= 126)
    {
        // 可打印字符
        currentInput.push_back(static_cast<char>(key));
        drawInputPrompt();
    }
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

        // 执行语法树
        LOG_DEBUG("开始执行语法树");
        Variable result = interpreter.execute(ast);
        LOG_INFO("命令执行完成，结果类型: " + std::to_string(static_cast<int>(result.type)));

        // 显示结果
        Terminal::setCursor(resultRow, 0);
        Terminal::setForeground(Color::GREEN);
        std::cout << "> " << input << std::endl;
        Terminal::resetColor();

        // 仅在有意义的结果时显示
        if (ast->type != AstNodeType::COMMAND)
        {
            Terminal::setForeground(Color::CYAN);
            switch (result.type)
            {
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

        // 更新状态消息
        statusMessage = "命令执行成功";
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
    std::cout << "  steps            - 显示计算步骤\n";
    std::cout << "\n";
    std::cout << "变量定义:\n";
    std::cout << "  m1 = [1,2,3;4,5,6] - 定义矩阵\n";
    std::cout << "  v1 = [1,2,3]       - 定义向量\n";
    std::cout << "  f1 = 3/4           - 定义分数\n";
    std::cout << "\n";
    std::cout << "基本运算:\n";
    std::cout << "  m3 = m1 + m2       - 矩阵加法\n";
    std::cout << "  m3 = m1 * m2       - 矩阵乘法\n";
    std::cout << "  v3 = v1 + v2       - 向量加法\n";
    std::cout << "  f3 = f1 * f2       - 分数乘法\n";
    std::cout << "\n";
    std::cout << "矩阵函数:\n";
    std::cout << "  m2 = transpose(m1)        - 矩阵转置\n";
    std::cout << "  m2 = inverse(m1)          - 计算逆矩阵(伴随矩阵法)\n";
    std::cout << "  m2 = inverse_gauss(m1)    - 计算逆矩阵(高斯-若尔当法)\n";
    std::cout << "  f1 = det(m1)              - 计算行列式\n";
    std::cout << "  f1 = det_expansion(m1)    - 按行列展开计算行列式\n";
    std::cout << "  f1 = rank(m1)             - 计算矩阵秩\n";
    std::cout << "  m2 = ref(m1)              - 行阶梯形\n";
    std::cout << "  m2 = rref(m1)             - 最简行阶梯形\n";
    std::cout << "  m2 = cofactor_matrix(m1)  - 计算代数余子式矩阵\n";
    std::cout << "  m2 = adjugate(m1)         - 计算伴随矩阵\n";
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
        // 可以添加更多特殊键处理
    }
}

void TuiApp::navigateHistory(bool up)
{
    if (history.empty())
    {
        return;
    }

    if (up)
    {
        // 向上浏览历史
        if (historyIndex < history.size())
        {
            // 保存当前输入
            if (historyIndex == 0)
            {
                // 这是第一次按上箭头
                history.push_front(currentInput);
                if (history.size() > MAX_HISTORY + 1)
                {
                    history.pop_back();
                }
            }

            historyIndex++;
            currentInput = history[historyIndex - 1];
            drawInputPrompt();
        }
    }
    else
    {
        // 向下浏览历史
        if (historyIndex > 1)
        {
            historyIndex--;
            currentInput = history[historyIndex - 1];
            drawInputPrompt();
        }
        else if (historyIndex == 1)
        {
            // 回到最新的输入
            historyIndex = 0;
            currentInput = history[0];
            // 删除临时保存的输入
            history.pop_front();
            drawInputPrompt();
        }
    }
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

    // 清除行
    std::string spaces(terminalCols - 2, ' ');
    std::cout << spaces;

    // 显示当前输入
    Terminal::setCursor(inputRow, 2);
    std::cout << currentInput;

    // 将光标定位到输入位置
    Terminal::setCursor(inputRow, 2 + currentInput.length());
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
    // 清空结果区域
    for (int i = resultRow; i < inputRow; i++)
    {
        Terminal::setCursor(i, 0);
        std::string spaces(terminalCols, ' ');
        std::cout << spaces;
    }

    // 重新绘制结果区域标题
    Terminal::setCursor(2, 0);
    Terminal::setForeground(Color::YELLOW);
    std::cout << "输出区域:" << std::endl;
    Terminal::resetColor();
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
