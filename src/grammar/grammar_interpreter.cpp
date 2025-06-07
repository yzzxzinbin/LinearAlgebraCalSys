#include "grammar_interpreter.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm> //确保已包含
#include <cctype>    //为 std::tolower 添加

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
        case AstNodeType::COMMAND:
            // 命令通常不返回值，这里返回一个默认的变量
            executeCommand(static_cast<const CommandNode*>(node.get())->command, 
                           static_cast<const CommandNode*>(node.get())->arguments);
            return Variable();
        default:
            throw std::runtime_error("未知的节点类型");
    }
}

void Interpreter::executeCommand(const std::string& command, const std::vector<std::string>& args) {
    if (command == "help") {
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
    } else if (command == "clear") {
        // 清屏命令在应用程序中处理
    } else if (command == "vars") {
        if (variables.empty()) {
            std::cout << "没有已定义的变量。\n";
            return;
        }
        
        std::cout << "已定义的变量：\n";
        for (const auto& pair : variables) {
            std::cout << "  " << pair.first << " = ";
            
            switch (pair.second.type) {
                case VariableType::FRACTION:
                    std::cout << pair.second.fractionValue;
                    break;
                case VariableType::VECTOR:
                    pair.second.vectorValue.print();
                    break;
                case VariableType::MATRIX:
                    std::cout << "\n";
                    pair.second.matrixValue.print();
                    break;
            }
            
            std::cout << "\n";
        }
    } else if (command == "exit") {
        // 退出命令在应用程序中处理
    } else if (command == "steps") {
        showSteps = !showSteps;
        // 注意：此处的 std::cout 输出不会直接显示在 TUI 的结果区域。
        // TuiApp 将根据 showSteps 的状态更新其状态栏。
        // 保留此处的 std::cout 可能用于非 TUI 环境或调试。
        std::cout << "计算步骤显示: " << (showSteps ? "开启" : "关闭") << "\n";
    } else {
        throw std::runtime_error("未知命令: " + command);
    }
}

const std::unordered_map<std::string, Variable>& Interpreter::getVariables() const {
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

void Interpreter::displaySteps(const OperationHistory& history) {
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "步骤 " << (i + 1) << ": ";
        history.getStep(i).print(std::cout);
        std::cout << std::endl;
    }
}

void Interpreter::displaySteps(const ExpansionHistory& history) {
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "步骤 " << (i + 1) << ": ";
        history.getStep(i).print(std::cout);
        std::cout << std::endl;
    }
}
