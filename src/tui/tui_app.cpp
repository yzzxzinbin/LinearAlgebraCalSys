#include "tui_app.h"
#include "tui_terminal.h" 
#include <iostream>
#include <string>
#include <algorithm>
#include "../utils/logger.h"
#include "../grammar/grammar_tokenizer.h"
#include "../grammar/grammar_parser.h"
#include "enhanced_matrix_editor.h" // 已经在 tui_app.h 中包含

// 新增：定义输出区域的布局常量
const int RESULT_AREA_TITLE_ROW = 2; // "输出区域:" 标题所在的行 (0-indexed)
const int RESULT_AREA_CONTENT_START_ROW = RESULT_AREA_TITLE_ROW + 1; // 实际内容开始的第一行
const int MATRIX_EDITOR_CELL_WIDTH = 8; // 矩阵编辑器单元格宽度

TuiApp::TuiApp() 
    : historyIndex(0), running(true), 
      inStepDisplayMode(false), currentStep(0), totalSteps(0), isExpansionHistory(false),
      cursorPosition(0), 
      // 初始化新的编辑器指针为空
      matrixEditor(nullptr),
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
        if (matrixEditor) { // 如果增强型编辑器激活
            matrixEditor->draw(); 
            // 状态栏由编辑器或TuiApp更新
            // drawStatusBar(); // 确保状态栏在编辑器绘制后更新，如果编辑器不自己画的话
        } else {
            updateUI();
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
        drawInputPrompt();
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
        initUI();
    }

    // 更新输入提示行 (仅当编辑器未激活时)
    if (!matrixEditor) {
        drawInputPrompt();
    }
    // 状态栏总是更新 (或者由编辑器更新自己的状态消息)
    // drawStatusBar(); // 已移至run循环末尾
}

void TuiApp::handleInput()
{
    // 读取一个字符
    int key = Terminal::readChar();
    
    // 如果增强型编辑器激活，将输入传递给它
    if (matrixEditor) {
        EnhancedMatrixEditor::EditorResult result = matrixEditor->handleInput(key);
        statusMessage = matrixEditor->getStatusMessage(); // 获取编辑器的状态消息

        if (result == EnhancedMatrixEditor::EditorResult::EXIT_SAVE) {
            interpreter.getVariablesNonConst()[matrixEditor->getVariableName()] = matrixEditor->getEditedVariableCopy();
            statusMessage = "Changes saved to " + matrixEditor->getVariableName() + ". " + statusMessage;
            matrixEditor.reset(); 
            initUI(); // 重绘标准UI
        } else if (result == EnhancedMatrixEditor::EditorResult::EXIT_DISCARD) {
            // 当前 EXIT_DISCARD 未实现，ESC 默认为 EXIT_SAVE
            statusMessage = "Exited editor for " + matrixEditor->getVariableName() + ". " + statusMessage;
            matrixEditor.reset();
            initUI();
        }
        // 如果是 CONTINUE 或 UPDATE_STATUS，状态栏会在 run() 循环中重绘
        return;
    }
    
    // 检查是否是退格键(包括Linux的127) - 仅当编辑器未激活
    if (key == KEY_BACKSPACE) {
        // 删除光标前一个字符
        if (cursorPosition > 0 && !currentInput.empty())
        {
            currentInput.erase(cursorPosition - 1, 1);
            cursorPosition--;
            drawInputPrompt();
            std::cout.flush(); // 确保更新立即显示
        }
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
    if (matrixEditor) return; // 不在编辑器模式下显示帮助
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
    std::cout << "  export <文件名>   - 导出所有变量到文件\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  import <文件名>   - 从文件导入变量\n";
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
    if (matrixEditor) return; // 编辑器激活时不绘制主输入提示
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
    }
    Terminal::resetColor();
    statusMessage = "显示变量: " + varName;
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
