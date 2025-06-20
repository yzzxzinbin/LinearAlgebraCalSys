#include "grammar_interpreter.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm> // 用于 std::transform 和 std::find
#include <cctype>
#include <fstream> // 用于文件操作
#include "../utils/logger.h" // 用于日志记录
#include "../similar_matrix_operations.h" // 新增包含
#include "../tui/tui_app.h" // 新增包含以访问 TuiApp::KNOWN_COMMANDS
#include "../vectorset_operation.h" // 新增包含

Interpreter::Interpreter() : showSteps(false) {}

Variable Interpreter::execute(const std::unique_ptr<AstNode>& node) {
    if (!node) {
        throw std::runtime_error("空节点无法执行");
    }
    
    clearCurrentHistories(); // 在每次执行主命令前清除历史

    switch (node->type) {
        case AstNodeType::VARIABLE:
            return executeVariable(static_cast<const VariableNode*>(node.get()));
        case AstNodeType::LITERAL:
            return executeLiteral(static_cast<const LiteralNode*>(node.get()));
        case AstNodeType::BINARY_OP:
            return executeBinaryOp(static_cast<const BinaryOpNode*>(node.get()));
        case AstNodeType::FUNCTION_CALL:
            return executeFunctionCall(static_cast<const FunctionCallNode*>(node.get()));
        case AstNodeType::ASSIGNMENT:
            return executeAssignment(static_cast<const AssignmentNode*>(node.get()));
        case AstNodeType::COMMAND: {
            const auto* cmdNode = static_cast<const CommandNode*>(node.get());
            std::string commandName = cmdNode->command;
            const auto& commandArgs = cmdNode->arguments;

            std::string commandNameLower = commandName;
            std::transform(commandNameLower.begin(), commandNameLower.end(), commandNameLower.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            std::string delegationMessageStr = "DELEGATE_COMMAND:" + commandName; // 使用原始大小写的命令名

            if (commandNameLower == "steps") {
                this->showSteps = !this->showSteps; // 切换状态
                delegationMessageStr += (this->showSteps ? " on" : " off");
            } else {
                // 如果不是 "steps"，继续检查命令是否在 TuiApp::KNOWN_COMMANDS 列表中
                auto it = std::find(TuiApp::KNOWN_COMMANDS.begin(), TuiApp::KNOWN_COMMANDS.end(), commandNameLower);
                if (it != TuiApp::KNOWN_COMMANDS.end()) {
                    // 命令是已知的TUI命令，附加参数
                    for (const auto& arg : commandArgs) {
                        delegationMessageStr += " " + arg;
                    }
                } else {
                    throw std::runtime_error("解释器接收到未知或无法处理的命令: " + commandName);
                }
            }
            
            return Variable(Result(delegationMessageStr));
        }
        default:
            throw std::runtime_error("未知的节点类型");
    }
}

const std::unordered_map<std::string, Variable>& Interpreter::getVariables() const {
    return variables;
}

// 新增：实现 getVariablesNonConst
std::unordered_map<std::string, Variable>& Interpreter::getVariablesNonConst() {
    return variables;
}

void Interpreter::setShowSteps(bool show) {
    showSteps = show;
}

bool Interpreter::isShowingSteps() const {
    return showSteps;
}

const OperationHistory& Interpreter::getCurrentOpHistory() const {
    return currentOpHistory_;
}

const ExpansionHistory& Interpreter::getCurrentExpHistory() const {
    return currentExpHistory_;
}

void Interpreter::clearCurrentHistories() {
    currentOpHistory_.clear();
    currentExpHistory_.clear();
}

// 新增：实现 clearVariables 方法
void Interpreter::clearVariables() {
    variables.clear();
    LOG_INFO("所有变量已被清除。");
}

// 新增：实现 deleteVariable 方法
void Interpreter::deleteVariable(const std::string& name) {
    auto it = variables.find(name);
    if (it == variables.end()) {
        throw std::runtime_error("无法删除变量: 变量 '" + name + "' 未定义。");
    }
    variables.erase(it);
    LOG_INFO("变量 '" + name + "' 已被删除。");
}

// 新增：实现 renameVariable 方法
void Interpreter::renameVariable(const std::string& oldName, const std::string& newName) {
    if (oldName == newName) {
        LOG_INFO("旧名称和新名称相同，无需重命名变量 '" + oldName + "'。");
        return; // 名称相同，无需操作
    }

    auto old_it = variables.find(oldName);
    if (old_it == variables.end()) {
        throw std::runtime_error("无法重命名变量: 旧变量名 '" + oldName + "' 未定义。");
    }

    auto new_it = variables.find(newName);
    if (new_it != variables.end()) {
        throw std::runtime_error("无法重命名变量: 新变量名 '" + newName + "' 已存在。");
    }

    // 复制变量到新名称，然后删除旧名称
    variables[newName] = old_it->second;
    variables.erase(old_it);
    LOG_INFO("变量 '" + oldName + "' 已重命名为 '" + newName + "'。");
}

Variable Interpreter::executeVariable(const VariableNode* node) {
    auto it = variables.find(node->name);
    if (it == variables.end()) {
        throw std::runtime_error("未定义的变量: " + node->name);
    }
    return it->second;
}

Variable Interpreter::executeLiteral(const LiteralNode* node) {
    return convertToVariable(node->value);
}

Variable Interpreter::executeBinaryOp(const BinaryOpNode* node) {
    Variable left = execute(node->left);
    Variable right = execute(node->right);
    
    switch (node->op) {
        case TokenType::PLUS:
            return add(left, right);
        case TokenType::MINUS:
            return subtract(left, right);
        case TokenType::MULTIPLY:
            return multiply(left, right);
        case TokenType::DIVIDE:
            return divide(left, right);
        case TokenType::CROSS_PRODUCT: // 新增对叉乘的处理
            if (left.type == VariableType::VECTOR && right.type == VariableType::VECTOR) {
                return Variable(left.vectorValue.cross(right.vectorValue));
            } else {
                throw std::runtime_error("叉乘操作 (x) 仅支持两个向量");
            }
        default:
            throw std::runtime_error("不支持的二元运算符");
    }
}

Variable Interpreter::executeFunctionCall(const FunctionCallNode* node) {
    // 获取参数
    std::vector<Variable> args;
    for (const auto& argNode : node->arguments) {
        args.push_back(execute(argNode)); // 注意：这里递归调用execute，它会调用clearCurrentHistories
                                          // 这可能导致嵌套函数调用的历史记录被清除。
                                          // 顶层的 execute 清除历史，这里的 execute 是为了参数求值，不应清除。
                                          // 为了解决这个问题，execute(AstNode) 应该只在顶层调用时清除历史。
                                          // 或者，参数求值不应调用顶层 execute，而是特定的求值函数。
                                          // 暂时，我们假设参数求值不会触发新的历史记录清除，因为它们不是顶层命令。
                                          // 一个更安全的做法是，execute 的重载版本，一个给TuiApp，一个内部用。
                                          // 或者，execute 接受一个 isTopLevel 调用标志。
                                          // 目前，由于参数通常是变量或字面量，它们不会调用executeFunctionCall，所以暂时安全。
    }
    
    std::string funcNameOriginal = node->name;
    std::string funcNameLower = funcNameOriginal;
    std::transform(funcNameLower.begin(), funcNameLower.end(), funcNameLower.begin(), 
                   [](unsigned char c){ return std::tolower(c); });

    // 如果启用了步骤显示，尝试使用带历史记录的版本
    if (showSteps) {
        if (funcNameLower == "det" && args.size() == 1 && args[0].type == VariableType::MATRIX) {
            return Variable(MatrixOperations::determinant(args[0].matrixValue, currentOpHistory_));
        } else if (funcNameLower == "inverse" && args.size() == 1 && args[0].type == VariableType::MATRIX) {
            return Variable(MatrixOperations::inverse(args[0].matrixValue, currentOpHistory_));
        } else if (funcNameLower == "inverse_gauss" && args.size() == 1 && args[0].type == VariableType::MATRIX) {
            return Variable(MatrixOperations::inverseGaussJordan(args[0].matrixValue, currentOpHistory_));
        } else if (funcNameLower == "ref" && args.size() == 1 && args[0].type == VariableType::MATRIX) {
            Matrix mat = args[0].matrixValue; // 复制矩阵以进行修改
            MatrixOperations::toRowEchelonForm(mat, currentOpHistory_);
            return Variable(mat);
        } else if (funcNameLower == "rref" && args.size() == 1 && args[0].type == VariableType::MATRIX) {
            Matrix mat = args[0].matrixValue; // 复制矩阵以进行修改
            MatrixOperations::toReducedRowEchelonForm(mat, currentOpHistory_);
            return Variable(mat);
        } else if (funcNameLower == "det_expansion" && args.size() == 1 && args[0].type == VariableType::MATRIX) {
            return Variable(MatrixOperations::determinantByExpansion(args[0].matrixValue, currentExpHistory_));
        }
        // 如果 showSteps 为 true 但函数不在此列表中，则会执行下面的常规逻辑
    }
    
    // 常规执行逻辑（showSteps 为 false 或函数不支持步骤显示）
    if (funcNameLower == "transpose") {
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("transpose函数需要一个矩阵参数");
        }
        return Variable(args[0].matrixValue.transpose());
    } else if (funcNameLower == "inverse") { // 无历史记录版本
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("inverse函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::inverse(args[0].matrixValue));
    } else if (funcNameLower == "inverse_gauss") { // 无历史记录版本
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("inverse_gauss函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::inverseGaussJordan(args[0].matrixValue));
    } else if (funcNameLower == "det") { // 无历史记录版本
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("det函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::determinant(args[0].matrixValue));
    } else if (funcNameLower == "det_expansion") { // 无历史记录版本
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("det_expansion函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::determinantByExpansion(args[0].matrixValue));
    } else if (funcNameLower == "rank") {
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("rank函数需要一个矩阵参数");
        }
        return Variable(Fraction(MatrixOperations::rank(args[0].matrixValue)));
    } else if (funcNameLower == "ref") { // 无历史记录版本
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("ref函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::toRowEchelonForm(args[0].matrixValue));
    } else if (funcNameLower == "rref") { // 无历史记录版本
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("rref函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::toReducedRowEchelonForm(args[0].matrixValue));
    } else if (funcNameLower == "cofactor_matrix") {
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("cofactor_matrix函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::cofactorMatrix(args[0].matrixValue));
    } else if (funcNameLower == "adjugate") {
        if (args.size() != 1 || args[0].type != VariableType::MATRIX) {
            throw std::runtime_error("adjugate函数需要一个矩阵参数");
        }
        return Variable(MatrixOperations::adjugate(args[0].matrixValue));
    } else if (funcNameLower == "dot") {
        if (args.size() != 2 || args[0].type != VariableType::VECTOR || args[1].type != VariableType::VECTOR) {
            throw std::runtime_error("dot函数需要两个向量参数");
        }
        return Variable(args[0].vectorValue.dot(args[1].vectorValue));
    } else if (funcNameLower == "cross") {
        if (args.size() != 2 || args[0].type != VariableType::VECTOR || args[1].type != VariableType::VECTOR) {
            throw std::runtime_error("cross函数需要两个向量参数");
        }
        return Variable(args[0].vectorValue.cross(args[1].vectorValue));
    } else if (funcNameLower == "norm") {
        if (args.size() != 1 || args[0].type != VariableType::VECTOR) {
            throw std::runtime_error("norm函数需要一个向量参数");
        }
        return Variable(args[0].vectorValue.norm());
    } else if (funcNameLower == "normalize") {
        if (args.size() != 1 || args[0].type != VariableType::VECTOR) {
            throw std::runtime_error("normalize函数需要一个向量参数");
        }
        return Variable(args[0].vectorValue.normalize());
    } else if (funcNameLower == "diag") {
        if (args.empty()) {
            throw std::runtime_error("diag函数需要至少一个参数 (对角线元素、向量或单列矩阵)");
        }
        std::vector<Fraction> diagElements;

        if (args.size() == 1) { // 单参数模式
            const auto& arg = args[0];
            if (arg.type == VariableType::VECTOR) {
                const Vector& v = arg.vectorValue;
                for (size_t i = 0; i < v.size(); ++i) {
                    diagElements.push_back(v.at(i));
                }
            } else if (arg.type == VariableType::MATRIX) {
                const Matrix& m = arg.matrixValue;
                if (m.colCount() == 1) { // 检查是否为列向量（单列矩阵）
                    for (size_t i = 0; i < m.rowCount(); ++i) {
                        diagElements.push_back(m.at(i, 0));
                    }
                } else {
                    throw std::runtime_error("diag函数如果参数是矩阵，则该矩阵必须为列向量 (只有一列)");
                }
            } else if (arg.type == VariableType::FRACTION) {
                diagElements.push_back(arg.fractionValue); // 例如 diag(f1)
            } else {
                std::ostringstream error_msg;
                error_msg << "diag函数的单个参数必须是向量、单列矩阵或分数。实际收到的参数类型 ID: "
                          << static_cast<int>(arg.type);
                throw std::runtime_error(error_msg.str());
            }
        } else { // 多参数模式: diag(f1, f2, ...)
            for (const auto& arg : args) {
                if (arg.type != VariableType::FRACTION) {
                    std::ostringstream error_msg;
                    error_msg << "diag函数的多参数形式其参数必须都是分数。实际收到的参数类型 ID: "
                              << static_cast<int>(arg.type);
                    throw std::runtime_error(error_msg.str());
                }
                diagElements.push_back(arg.fractionValue);
            }
        }

        if (diagElements.empty()) { 
            throw std::runtime_error("diag函数需要有效的对角线元素");
        }
        return Variable(SimilarMatrixOperations::createDiagonalMatrix(diagElements));
    } else if (funcNameLower == "solveq") {  // 新增：方程组求解函数
        if (args.size() == 1 && args[0].type == VariableType::MATRIX) {
            // 齐次方程组 Ax = 0
            if (showSteps) {
                return Variable(EquationSolver::solveHomogeneous(args[0].matrixValue, currentOpHistory_));
            } else {
                return Variable(EquationSolver::solveHomogeneous(args[0].matrixValue));
            }
        } else if (args.size() == 2 && args[0].type == VariableType::MATRIX) {
            // 非齐次方程组 Ax = b
            if (args[1].type == VariableType::MATRIX) {
                if (showSteps) {
                    return Variable(EquationSolver::solve(args[0].matrixValue, args[1].matrixValue, currentOpHistory_));
                } else {
                    return Variable(EquationSolver::solve(args[0].matrixValue, args[1].matrixValue));
                }
            } else if (args[1].type == VariableType::VECTOR) {
                if (showSteps) {
                    return Variable(EquationSolver::solve(args[0].matrixValue, args[1].vectorValue, currentOpHistory_));
                } else {
                    return Variable(EquationSolver::solve(args[0].matrixValue, args[1].vectorValue));
                }
            } else {
                throw std::runtime_error("solveq函数第二个参数(常数项b)必须是矩阵或向量");
            }
        } else {
            throw std::runtime_error("solveq函数需要一个矩阵参数(齐次Ax=0)或一个矩阵和一个矩阵/向量参数(非齐次Ax=b)");
        }
    } else if (funcNameLower == "rep_vecset") { // 新增：向量组线性表示关系
        if (args.size() != 2)
            throw std::runtime_error("rep_vecset函数需要两个参数（向量或矩阵）");
        // 支持向量（视为单列矩阵）
        Matrix m1 = (args[0].type == VariableType::VECTOR) ? Matrix(args[0].vectorValue.size(), 1) : args[0].matrixValue;
        if (args[0].type == VariableType::VECTOR)
            for (size_t i = 0; i < args[0].vectorValue.size(); ++i) m1.at(i, 0) = args[0].vectorValue.at(i);
        Matrix m2 = (args[1].type == VariableType::VECTOR) ? Matrix(args[1].vectorValue.size(), 1) : args[1].matrixValue;
        if (args[1].type == VariableType::VECTOR)
            for (size_t i = 0; i < args[1].vectorValue.size(); ++i) m2.at(i, 0) = args[1].vectorValue.at(i);
        // 修改：返回字符串类型Result
        return Variable(Result::fromString(rep_vecset(m1, m2).getString()));
    } else {
        throw std::runtime_error("未知函数: " + funcNameOriginal);
    }
}

Variable Interpreter::executeAssignment(const AssignmentNode* node) {
    Variable value = execute(node->expression);
    variables[node->variableName] = value;
    return value;
}

Variable Interpreter::convertToVariable(const ParsedValue& value) {
    switch (value.type) {
        case ParsedValue::Type::FRACTION:
            return Variable(value.fractionValue);
        case ParsedValue::Type::VECTOR: {
            // 创建Vector对象
            Vector vec(value.vectorValue);
            return Variable(vec);
        }
        case ParsedValue::Type::MATRIX: {
            // 创建Matrix对象
            Matrix mat(value.matrixValue);
            return Variable(mat);
        }
        default:
            throw std::runtime_error("无法转换未知类型的值");
    }
}

Variable Interpreter::add(const Variable& left, const Variable& right) {
    if (left.type != right.type) {
        throw std::runtime_error("类型不匹配，无法执行加法操作");
    }
    
    switch (left.type) {
        case VariableType::FRACTION:
            return Variable(left.fractionValue + right.fractionValue);
        case VariableType::VECTOR:
            return Variable(left.vectorValue + right.vectorValue);
        case VariableType::MATRIX:
            return Variable(left.matrixValue + right.matrixValue);
        default:
            throw std::runtime_error("不支持的类型");
    }
}

Variable Interpreter::subtract(const Variable& left, const Variable& right) {
    if (left.type != right.type) {
        throw std::runtime_error("类型不匹配，无法执行减法操作");
    }
    
    switch (left.type) {
        case VariableType::FRACTION:
            return Variable(left.fractionValue - right.fractionValue);
        case VariableType::VECTOR:
            return Variable(left.vectorValue - right.vectorValue);
        case VariableType::MATRIX:
            return Variable(left.matrixValue - right.matrixValue);
        default:
            throw std::runtime_error("不支持的类型");
    }
}

Variable Interpreter::multiply(const Variable& left, const Variable& right) {
    // 分数 * 分数
    if (left.type == VariableType::FRACTION && right.type == VariableType::FRACTION) {
        return Variable(left.fractionValue * right.fractionValue);
    }
    
    // 分数 * 向量
    if (left.type == VariableType::FRACTION && right.type == VariableType::VECTOR) {
        return Variable(right.vectorValue * left.fractionValue);
    }
    
    // 向量 * 分数
    if (left.type == VariableType::VECTOR && right.type == VariableType::FRACTION) {
        return Variable(left.vectorValue * right.fractionValue);
    }
    
    // 分数 * 矩阵
    if (left.type == VariableType::FRACTION && right.type == VariableType::MATRIX) {
        return Variable(right.matrixValue * left.fractionValue);
    }
    
    // 矩阵 * 分数
    if (left.type == VariableType::MATRIX && right.type == VariableType::FRACTION) {
        return Variable(left.matrixValue * right.fractionValue);
    }
    
    // 矩阵 * 矩阵
    if (left.type == VariableType::MATRIX && right.type == VariableType::MATRIX) {
        return Variable(left.matrixValue * right.matrixValue);
    }

    // 向量 * 向量 (点积)
    if (left.type == VariableType::VECTOR && right.type == VariableType::VECTOR) {
        return Variable(left.vectorValue.dot(right.vectorValue)); // 点积返回分数
    }
    
    throw std::runtime_error("不支持的乘法操作或类型组合");
}

Variable Interpreter::divide(const Variable& left, const Variable& right) {
    // 只支持分数除法
    if (left.type == VariableType::FRACTION && right.type == VariableType::FRACTION) {
        return Variable(left.fractionValue / right.fractionValue);
    }
    
    // 向量 / 分数
    if (left.type == VariableType::VECTOR && right.type == VariableType::FRACTION) {
        return Variable(left.vectorValue * (Fraction(1) / right.fractionValue));
    }
    
    // 矩阵 / 分数
    if (left.type == VariableType::MATRIX && right.type == VariableType::FRACTION) {
        return Variable(left.matrixValue * (Fraction(1) / right.fractionValue));
    }
    
    throw std::runtime_error("不支持的除法操作");
}

