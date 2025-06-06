#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "grammar_parser.h"
#include "../matrix.h"
#include "../vector.h"
#include "../fraction.h"
#include "../matrix_operations.h"
#include "../operation_step.h"
#include "../determinant_expansion.h"

// 变量类型
enum class VariableType {
    FRACTION,
    VECTOR,
    MATRIX
};

// 变量存储
struct Variable {
    VariableType type;
    Fraction fractionValue;
    Vector vectorValue;
    Matrix matrixValue;
    
    // 构造函数
    Variable() : type(VariableType::FRACTION), fractionValue(0), vectorValue(0), matrixValue(0, 0) {}
    
    Variable(const Fraction& f) : type(VariableType::FRACTION), fractionValue(f), vectorValue(0), matrixValue(0, 0) {}
    
    Variable(const Vector& v) : type(VariableType::VECTOR), vectorValue(v), matrixValue(0, 0) {}
    
    Variable(const Matrix& m) : type(VariableType::MATRIX), vectorValue(0), matrixValue(m) {}
};

// 解释器类
class Interpreter {
private:
    std::unordered_map<std::string, Variable> variables;
    bool showSteps;

public:
    Interpreter();
    
    // 执行语法树
    Variable execute(const std::unique_ptr<AstNode>& node);
    
    // 执行命令
    void executeCommand(const std::string& command, const std::vector<std::string>& args);
    
    // 获取所有变量
    const std::unordered_map<std::string, Variable>& getVariables() const;
    
    // 设置是否显示步骤
    void setShowSteps(bool show);
    
private:
    // 执行各种节点类型
    Variable executeVariable(const VariableNode* node);
    Variable executeLiteral(const LiteralNode* node);
    Variable executeBinaryOp(const BinaryOpNode* node);
    Variable executeFunctionCall(const FunctionCallNode* node);
    Variable executeAssignment(const AssignmentNode* node);
    
    // 转换ParsedValue到Variable
    Variable convertToVariable(const ParsedValue& value);
    
    // 二元运算实现
    Variable add(const Variable& left, const Variable& right);
    Variable subtract(const Variable& left, const Variable& right);
    Variable multiply(const Variable& left, const Variable& right);
    Variable divide(const Variable& left, const Variable& right);
    
    // 显示步骤
    void displaySteps(const OperationHistory& history);
    void displaySteps(const ExpansionHistory& history);
};
