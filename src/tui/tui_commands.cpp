#include "tui_app.h"
#include "tui_terminal.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip> // For std::setw, std::setprecision, std::fixed, std::scientific
#include <algorithm> // For std::string operations
#include <fstream>   // For export/import/csv
#include <stdexcept> // For std::invalid_argument, std::runtime_error
#include <cmath>     // For std::pow, std::log10, std::abs, std::round in formatValue/formatValueDecimal
#include <limits>    // For std::numeric_limits in formatValue/formatValueDecimal
// #include <boost/lexical_cast.hpp> // Not strictly needed if using std::stoi, etc.
#include <boost/multiprecision/cpp_dec_float.hpp> // For HighPrecisionFloat
#include "../utils/logger.h"
#include "../grammar/grammar_tokenizer.h"
#include "../grammar/grammar_parser.h"
#include "enhanced_matrix_editor.h"
#include "tui_suggestion_box.h"


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

                std::string csvData;
                VariableType varType = it->second.type;

                if (varType == VariableType::MATRIX) {
                    const Matrix& matrixToExport = it->second.matrixValue;
                    std::ostringstream oss;
                    for (size_t r = 0; r < matrixToExport.rowCount(); ++r) {
                        for (size_t c = 0; c < matrixToExport.colCount(); ++c) {
                            oss << matrixToExport.at(r, c); // 直接输出分数，不加引号
                            if (c < matrixToExport.colCount() - 1) {
                                oss << ",";
                            }
                        }
                        if (r < matrixToExport.rowCount() - 1) {
                            oss << "\n";
                        }
                    }
                    csvData = oss.str();
                } else if (varType == VariableType::VECTOR) {
                    const Vector& vectorToExport = it->second.vectorValue;
                    std::ostringstream oss;
                    for (size_t i = 0; i < vectorToExport.size(); ++i) {
                        oss << vectorToExport.at(i); // 直接输出分数，不加引号
                        if (i < vectorToExport.size() - 1) {
                            oss << ",";
                        }
                    }
                    csvData = oss.str();
                } else if (varType == VariableType::RESULT) {
                    const Result& resultToExport = it->second.resultValue;
                    csvData = resultToExport.toCsvString(); // Result::toCsvString 已修改为不加引号
                } else {
                    throw std::runtime_error("变量 '" + varName + "' 不是 Matrix, Vector, 或 Result 类型，无法导出为CSV。");
                }
                
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
                throw std::runtime_error("csv 命令需要一个参数 (Matrix, Vector 或 Result 类型的变量名)。用法: csv <变量名>");
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
        if (commandStr == "export") {
            // Extract the argument part directly from processedInput
            std::string argument_part;
            size_t cmd_len = commandStr.length();
            if (processedInput.length() > cmd_len) {
                argument_part = processedInput.substr(cmd_len);
                // Trim leading whitespace from the argument part
                size_t first_char_idx = argument_part.find_first_not_of(" \t");
                if (first_char_idx != std::string::npos) {
                    argument_part = argument_part.substr(first_char_idx);
                } else {
                    // Only whitespace after command, treat as no argument
                    argument_part.clear();
                }
            }

            // Remove trailing semicolon if present on the argument itself
            if (!argument_part.empty() && argument_part.back() == ';') {
                argument_part.pop_back();
            }
            
            if (argument_part.empty()) {
                 throw std::runtime_error("export 命令需要一个文件名参数。用法: export <\"文件名\"> 或 export <文件名>");
            }

            std::string filename = argument_part; // This is the full filename argument
            
            // If the user provides quotes for export, interpret the path inside the quotes.
            if (filename.length() >= 2 && filename.front() == '"' && filename.back() == '"') {
                filename = filename.substr(1, filename.length() - 2);
            }
            
            std::string export_message = interpreter.exportVariables(filename, history); // 导出数据
            printToResultView(export_message, Color::YELLOW);
            statusMessage = export_message;
            return;
        }

        // 处理import命令
        if (commandStr == "import") {
            // Extract the argument part directly from processedInput
            std::string argument_part;
            size_t cmd_len = commandStr.length();
            if (processedInput.length() > cmd_len) {
                argument_part = processedInput.substr(cmd_len);
                // Trim leading whitespace from the argument part
                size_t first_char_idx = argument_part.find_first_not_of(" \t");
                if (first_char_idx != std::string::npos) {
                    argument_part = argument_part.substr(first_char_idx);
                } else {
                    // Only whitespace after command, treat as no argument
                    argument_part.clear();
                }
            }

            // Remove trailing semicolon if present on the argument itself
            if (!argument_part.empty() && argument_part.back() == ';') {
                argument_part.pop_back();
            }
            
            if (argument_part.empty()) {
                 throw std::runtime_error("import 命令需要一个文件名参数。用法: import <\"文件名\"> 或 import <文件名>");
            }

            std::string filename_arg = argument_part; // This is the full filename argument
            std::string filename_to_import;

            // 检查文件名参数是否被双引号包围
            if (filename_arg.length() >= 2 && filename_arg.front() == '"' && filename_arg.back() == '"') {
                // 如果是，则去除双引号
                filename_to_import = filename_arg.substr(1, filename_arg.length() - 2);
                LOG_INFO("Importing from (quoted): " + filename_to_import);
            } else {
                // 否则，按原样处理
                filename_to_import = filename_arg;
                LOG_INFO("Importing from: " + filename_to_import);
            }

            auto import_result = interpreter.importVariables(filename_to_import);
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
            // 启动增强型变量预览器
            if (!matrixEditor && !variableViewer) { // 确保没有其他编辑器在运行
                variableViewer = std::make_unique<EnhancedVariableViewer>(interpreter, terminalRows, terminalCols);
                statusMessage = variableViewer->getStatusMessage();
                initUI(); // 重绘UI以适应预览器
            }
            return;
        }
        // 添加对vars -l命令的处理
        else if (processedInput == "vars -l" || processedInput == "vars -l;")
        {
            showVariables(true); // 传入true表示只列出变量名和类型
            // 新增：在vars -l末尾显示变量总数
            const auto &vars = interpreter.getVariables();
            Terminal::setCursor(resultRow, 0);
            Terminal::setForeground(Color::CYAN);
            std::cout << "\n总计: " << vars.size() << " 个变量" << std::endl;
            Terminal::resetColor();
            resultRow += 2; // 为换行和总计行更新 resultRow
            return;
        }

        // 处理exit命令（支持 --no-saving 参数）
        if (commandStr == "exit") {
            // 检查是否有 --no-saving 参数
            bool foundNoSaving = false;
            for (const auto& arg : commandArgs) {
                if (arg == "--no-saving") {
                    foundNoSaving = true;
                    break;
                }
            }
            if (foundNoSaving) {
                noSavingOnExit = true;
                printToResultView("本次退出将不会自动保存变量和历史。", Color::YELLOW);
                statusMessage = "已设置为退出时不自动保存";
            }
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
    std::cout << "  csv <变量名>                   - 将 Matrix, Vector 或 Result 类型变量导出为CSV文件\n";
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
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  m_diag = diag(v1)         - 使用向量v1创建对角矩阵\n";
    resultRow++;
    Terminal::setCursor(resultRow, 0);
    std::cout << "  sov = solveq(m1, v1)      - 求解方程组 Ax = b (v1可选)\n";
    // resultRow++; // 最后一行不需要再递增，除非后面还有输出
    std::cout << "\n"; // 确保最后有换行
    Terminal::resetColor();

    // 更新状态消息
    statusMessage = "已显示帮助信息";
}

void TuiApp::showVariables(bool listOnly)
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
    if (listOnly) {
        std::cout << "变量列表（名称和类型）：\n";
    } else {
        std::cout << "已定义的变量：\n";
    }
    resultRow++;

    for (const auto &pair : vars)
    {
        // 在非列表模式下跳过方程组解类型
        if (!listOnly && pair.second.type == VariableType::EQUATION_SOLUTION) {
            continue;
        }

        Terminal::setCursor(resultRow, 0);
        
        // 获取变量类型的字符串表示
        std::string typeStr;
        switch (pair.second.type)
        {
        case VariableType::FRACTION:
            typeStr = "分数";
            break;
        case VariableType::VECTOR:
            if (listOnly) {
                // 为向量添加维度信息
                size_t dim = pair.second.vectorValue.size();
                typeStr = "向量 (" + std::to_string(dim) + "维)";
            } else {
                typeStr = "向量";
            }
            break;
        case VariableType::MATRIX:
            if (listOnly) {
                // 为矩阵添加行列信息
                size_t rows = pair.second.matrixValue.rowCount();
                size_t cols = pair.second.matrixValue.colCount();
                typeStr = "矩阵 (" + std::to_string(rows) + "×" + std::to_string(cols) + ")";
            } else {
                typeStr = "矩阵";
            }
            break;
        case VariableType::RESULT:
            typeStr = "结果";
            break;
        case VariableType::EQUATION_SOLUTION:  // 新增：处理方程组解类型
            if (listOnly) {
                typeStr = "方程组解";
            } else {
                typeStr = "方程组解";
            }
            break;
        default:
            typeStr = "未知类型";
        }
        
        if (listOnly) {
            // 只显示变量名和类型
            std::cout << "  " << pair.first << " : " << typeStr << "\n";
            resultRow++;
        } else {
            // 显示完整内容（原有行为）
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
            case VariableType::RESULT:  // 新增：处理Result类型
                 std::cout << "\n"; 
                 resultRow++; // 为 "name = " 和 Result 内容之间的空行增加 resultRow
                 // 将 Result 对象的打印输出到 stringstream 以处理多行
                 {
                     std::stringstream result_ss;
                     result_ss << pair.second.resultValue; // 使用 operator<<
                     std::string result_str = result_ss.str();
                     std::istringstream result_iss(result_str);
                     std::string result_line;
                     while(std::getline(result_iss, result_line)) {
                         Terminal::setCursor(resultRow, 0);
                         std::cout << "  " << result_line << "\n"; // 添加缩进
                         resultRow++;
                     }
                 }
                 break;
            case VariableType::EQUATION_SOLUTION:  // 新增：处理方程组解类型
                 std::cout << "\n"; 
                 resultRow++;
                 {
                     std::stringstream sol_ss;
                     pair.second.equationSolutionValue.print(sol_ss);
                     std::string sol_str = sol_ss.str();
                     std::istringstream sol_iss(sol_str);
                     std::string sol_line;
                     while(std::getline(sol_iss, sol_line)) {
                         Terminal::setCursor(resultRow, 0);
                         std::cout << "  " << sol_line << "\n"; // 添加缩进
                         resultRow++;
                     }
                 }
                 break;
            }
        }
    }
    Terminal::resetColor();
    
    if (listOnly) {
        statusMessage = "已显示变量列表（仅名称和类型）";
    } else {
        statusMessage = "已显示变量列表";
    }
}

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
        resultRow++; // 为 "name = " 和 Result 内容之间的空行增加 resultRow
        // 将 Result 对象的打印输出到 stringstream 以处理多行
        {
            std::stringstream result_ss;
            result_ss << it->second.resultValue; // 使用 operator<<
            std::string result_str = result_ss.str();
            std::istringstream result_iss(result_str);
            std::string result_line;
            while(std::getline(result_iss, result_line)) {
                Terminal::setCursor(resultRow, 0);
                std::cout << result_line << std::endl;
                resultRow++;
            }
        }
        break;
    case VariableType::EQUATION_SOLUTION:  // 新增：处理方程组解类型
        std::cout << "\n"; // 为 "solution = " 和解内容之间提供一行间隔
        resultRow++;
        {
            std::stringstream sol_ss;
            it->second.equationSolutionValue.print(sol_ss);
            std::string sol_str = sol_ss.str();
            std::istringstream sol_iss(sol_str);
            std::string sol_line;
            while(std::getline(sol_iss, sol_line)) {
                Terminal::setCursor(resultRow, 0);
                std::cout << sol_line << std::endl;
                resultRow++;
            }
        }
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
            HighPrecisionFloat numerator_hp(frac.getNumerator().str());
            HighPrecisionFloat denominator_hp(frac.getDenominator().str());
            if (denominator_hp == 0) return "NaN"; // Or throw, or handle as error
            HighPrecisionFloat result_hp = numerator_hp / denominator_hp;
            
            // 检查溢出情况 (比较 cpp_dec_float_100 和 double 的范围可能不直接)
            // 通常 cpp_dec_float_100 范围远大于 double
            // 转换为 double 时可能发生溢出或精度损失
            if (boost::multiprecision::isinf(result_hp)) {
                 return result_hp > 0 ? "INF" : "-INF";
            }
            if (boost::multiprecision::isnan(result_hp)) {
                 return "NaN";
            }

            double fval;
            try {
                fval = result_hp.convert_to<double>();
                 if (std::isinf(fval)) {
                    return fval > 0 ? "INF" : "-INF";
                }
                if (std::isnan(fval)) {
                    return "NaN";
                }
            } catch (const std::overflow_error& e) {
                // Handle cases where conversion to double overflows but cpp_dec_float_100 is fine
                // This might indicate a very large or very small number not representable by double
                // For now, we can return INF or -INF based on sign, or use scientific notation from cpp_dec_float
                std::stringstream ss_hp;
                ss_hp << std::scientific << std::setprecision(precision -1) << result_hp;
                return ss_hp.str();
            }
            
            // 检查是否为零
            if (fval == 0.0 && result_hp == 0) { // Check both to be sure
                return "0";
            }
            
            // 检查是否为整数（分母为1的情况）
            if (frac.getDenominator() == 1) {
                std::string intStr = frac.getNumerator().str();
                // For very large integers, double conversion might lose precision or become scientific
                // If precision is high enough to show all digits, show them. Otherwise, scientific.
                if (intStr.length() > static_cast<size_t>(precision + 2) && (intStr[0] == '-' ? intStr.length()-1 : intStr.length()) > precision ) { // Heuristic
                    std::stringstream ss;
                    ss << std::scientific << std::setprecision(precision - 1) << fval; // fval might already be in sci notation
                    return ss.str();
                } else {
                    return intStr;
                }
            }
            
            // 非整数情况：使用有效数字格式
            double absVal = std::abs(fval);
            
            // 判断使用定点还是科学计数法
            if (absVal >= 0.1 && absVal < std::pow(10.0, precision)) {
                int magnitude = (absVal == 0) ? 0 : static_cast<int>(std::floor(std::log10(absVal)));
                int decimalPlaces = precision - magnitude - 1;
                
                if (decimalPlaces < 0) decimalPlaces = 0;
                // Max decimal places for double is around 15-17 for full precision
                if (decimalPlaces > 15) decimalPlaces = 15; 
                
                std::stringstream ss;
                ss << std::fixed << std::setprecision(decimalPlaces) << fval;
                std::string str = ss.str();
                
                if (str.find('.') != std::string::npos) {
                    str = str.substr(0, str.find_last_not_of('0') + 1);
                    if (str.back() == '.') {
                        str.pop_back();
                    }
                }
                return str;
            } else {
                std::stringstream ss;
                ss << std::scientific << std::setprecision(precision - 1) << fval;
                return ss.str();
            }
        } catch (const std::exception& e) {
            // Log e.what() for debugging
            return "ERR";
        }
    };

    Result result_obj; // Renamed from 'result' to avoid conflict with variable 'result' in some contexts
    
    switch (it->second.type) {
    case VariableType::FRACTION:
        {
            std::string formattedValue = formatValue(it->second.fractionValue);
            std::cout << formattedValue << std::endl;
            resultRow++;
            if (saveResult) {
                result_obj = Result(formattedValue);
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
                result_obj = Result(formattedValues);
            }
        }
        break;
    case VariableType::MATRIX:
        {
            std::cout << "\n"; 
            resultRow++;
            
            std::vector<std::vector<std::string>> formattedMatrix;
            for (size_t r = 0; r < it->second.matrixValue.rowCount(); ++r) {
                Terminal::setCursor(resultRow, 0);
                std::cout << "| ";
                std::vector<std::string> row_vec; // Renamed from 'row'
                for (size_t c = 0; c < it->second.matrixValue.colCount(); ++c) {
                    std::string formattedValue = formatValue(it->second.matrixValue.at(r, c));
                    std::cout << std::setw(12) << formattedValue << " ";
                    row_vec.push_back(formattedValue);
                }
                std::cout << "|" << std::endl;
                resultRow++;
                formattedMatrix.push_back(row_vec);
            }
            if (saveResult) {
                result_obj = Result(formattedMatrix);
            }
        }
        break;
    case VariableType::RESULT:
        // Result 类型已经是字符串化表示，其 print 方法能处理多行
        // 但 showVariableWithFormat/DecimalFormat 的目的是格式化 Fraction/Vector/Matrix
        // Result 类型通常是这些格式化操作的目标，而不是源
        // 如果确实要显示一个 Result 类型的变量，并且它可能是多行的，
        // 这里的处理也需要像 showVariable 行为一致，通过 stringstream 逐行打印并更新 resultRow。
        // 然而，当前 formatValue 和 formatValueDecimal 并不直接处理 Result 类型。
        // 假设这里的 Result 是标量或单行向量，则当前逻辑没问题。
        // 如果 Result 内部是 Matrix，则需要修改。
        std::cout << "\n"; // 为 "name = " 和结果内容之间提供一行间隔
        resultRow++;
        {
            std::stringstream result_ss;
            result_ss << it->second.resultValue;
            std::string result_str = result_ss.str();
            std::istringstream result_iss(result_str);
            std::string result_line;
            while(std::getline(result_iss, result_line)) {
                Terminal::setCursor(resultRow, 0);
                std::cout << result_line << std::endl;
                resultRow++;
            }
        }
        // Optionally, allow re-saving a result if -r is used, though it's already a Result
        if (saveResult) {
            result_obj = it->second.resultValue; // Copy existing result
        }
        break;
    }

    if (saveResult && !resultVarName.empty()) {
        interpreter.getVariablesNonConst()[resultVarName] = Variable(result_obj);
        statusMessage = "以 " + std::to_string(precision) + " 位有效数字显示变量: " + varName + "，结果已保存到: " + resultVarName;
    } else {
        statusMessage = "以 " + std::to_string(precision) + " 位有效数字显示变量: " + varName;
    }

    Terminal::resetColor();
}

void TuiApp::showVariableWithDecimalFormat(const std::string &varName, int decimalPlaces, bool saveResult, const std::string& resultVarName) {
    if (matrixEditor) return; 
    const auto &vars = interpreter.getVariables();
    auto it = vars.find(varName);

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

    using HighPrecisionFloat = boost::multiprecision::cpp_dec_float_100;

    auto formatValueDecimal = [&decimalPlaces](const Fraction& frac) -> std::string {
        try {
            HighPrecisionFloat numerator_hp(frac.getNumerator().str());
            HighPrecisionFloat denominator_hp(frac.getDenominator().str());
            if (denominator_hp == 0) return "NaN";
            HighPrecisionFloat result_hp = numerator_hp / denominator_hp;

            if (boost::multiprecision::isinf(result_hp)) {
                 return result_hp > 0 ? "INF" : "-INF";
            }
            if (boost::multiprecision::isnan(result_hp)) {
                 return "NaN";
            }
            
            // For decimal places, we can directly format the high-precision number
            std::stringstream ss;
            ss << std::fixed << std::setprecision(decimalPlaces) << result_hp;
            return ss.str();

        } catch (const std::exception& e) {
            return "ERR";
        }
    };

    Result result_obj;

    switch (it->second.type) {
    case VariableType::FRACTION:
        {
            std::string formattedValue = formatValueDecimal(it->second.fractionValue);
            std::cout << formattedValue << std::endl;
            resultRow++;
            if (saveResult) {
                result_obj = Result(formattedValue);
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
                result_obj = Result(formattedValues);
            }
        }
        break;
    case VariableType::MATRIX:
        {
            std::cout << "\n"; 
            resultRow++;
            
            std::vector<std::vector<std::string>> formattedMatrix;
            for (size_t r = 0; r < it->second.matrixValue.rowCount(); ++r) {
                Terminal::setCursor(resultRow, 0);
                std::cout << "| ";
                std::vector<std::string> row_vec;
                for (size_t c = 0; c < it->second.matrixValue.colCount(); ++c) {
                    std::string formattedValue = formatValueDecimal(it->second.matrixValue.at(r, c));
                    std::cout << std::setw(10) << formattedValue << " "; // Adjust width as needed
                    row_vec.push_back(formattedValue);
                }
                std::cout << "|" << std::endl;
                resultRow++;
                formattedMatrix.push_back(row_vec);
            }
            if (saveResult) {
                result_obj = Result(formattedMatrix);
            }
        }
        break;
    case VariableType::RESULT:
        // 与 showVariableWithFormat 中的 Result 处理类似
        std::cout << "\n"; // 为 "name = " 和结果内容之间提供一行间隔
        resultRow++;
        {
            std::stringstream result_ss;
            result_ss << it->second.resultValue;
            std::string result_str = result_ss.str();
            std::istringstream result_iss(result_str);
            std::string result_line;
            while(std::getline(result_iss, result_line)) {
                Terminal::setCursor(resultRow, 0);
                std::cout << result_line << std::endl;
                resultRow++;
            }
        }
        if (saveResult) {
            result_obj = it->second.resultValue;
        }
        break;
    }

    if (saveResult && !resultVarName.empty()) {
        interpreter.getVariablesNonConst()[resultVarName] = Variable(result_obj);
        std::string formatDesc = (decimalPlaces == 0 && it->second.type == VariableType::FRACTION && it->second.fractionValue.getDenominator() == 1) ? "整数格式" : (std::to_string(decimalPlaces) + " 位小数");
        statusMessage = "以 " + formatDesc + " 显示变量: " + varName + "，结果已保存到: " + resultVarName;
    } else {
        std::string formatDesc = (decimalPlaces == 0 && it->second.type == VariableType::FRACTION && it->second.fractionValue.getDenominator() == 1) ? "整数格式" : (std::to_string(decimalPlaces) + " 位小数");
        statusMessage = "以 " + formatDesc + " 显示变量: " + varName;
    }
    Terminal::resetColor();
}

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
