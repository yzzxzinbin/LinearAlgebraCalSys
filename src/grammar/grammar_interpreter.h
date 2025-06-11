#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <vector> // 确保 vector 也被包含
#include <deque>  // 新增：包含 deque 头文件
#include "grammar_parser.h"
#include "../matrix.h"
#include "../vector.h"
#include "../fraction.h"
#include "../result.h"  // 新增：包含Result头文件
#include "../matrix_operations.h"
#include "../operation_step.h"
#include "../determinant_expansion.h"

// 变量类型
enum class VariableType {
    FRACTION,
    VECTOR,
    MATRIX,
    RESULT  // 新增：Result类型
};

// 变量存储
struct Variable {
    VariableType type;
    Fraction fractionValue;
    Vector vectorValue;
    Matrix matrixValue;
    Result resultValue;  // 新增：Result值
    
    // 构造函数
    Variable() : type(VariableType::FRACTION), fractionValue(0), vectorValue(0), matrixValue(0, 0), resultValue() {}
    
    Variable(const Fraction& f) : type(VariableType::FRACTION), fractionValue(f), vectorValue(0), matrixValue(0, 0), resultValue() {}
    
    Variable(const Vector& v) : type(VariableType::VECTOR), vectorValue(v), matrixValue(0, 0), resultValue() {}
    
    Variable(const Matrix& m) : type(VariableType::MATRIX), vectorValue(0), matrixValue(m), resultValue() {}
    
    Variable(const Result& r) : type(VariableType::RESULT), vectorValue(0), matrixValue(0, 0), resultValue(r) {}  // 新增：Result构造函数
};

// 解释器类
class Interpreter {
private:
    std::unordered_map<std::string, Variable> variables;
    bool showSteps;
    OperationHistory currentOpHistory_;
    ExpansionHistory currentExpHistory_;

    // 新增：导出和导入的辅助方法
    std::string serializeVariable(const std::string& name, const Variable& var) const;
    std::pair<std::string, Variable> deserializeLine(const std::string& line) const;
    Fraction parseFractionString(const std::string& s) const;
    std::vector<std::string> splitString(const std::string& s, char delimiter) const;


public:
    static const std::string HISTORY_MARKER; // 新增：历史记录标记

    Interpreter();
    
    // 执行语法树
    Variable execute(const std::unique_ptr<AstNode>& node);
    
    // 执行命令
    void executeCommand(const std::string& command, const std::vector<std::string>& args);
    
    // 获取所有变量 (const 版本)
    const std::unordered_map<std::string, Variable>& getVariables() const;
    
    // 新增：获取所有变量 (non-const 版本，用于修改)
    std::unordered_map<std::string, Variable>& getVariablesNonConst();
    
    // 设置是否显示步骤
    void setShowSteps(bool show);
    
    // 检查是否显示步骤
    bool isShowingSteps() const;
    
    // 新增：获取当前操作历史
    const OperationHistory& getCurrentOpHistory() const;
    
    // 新增：获取当前展开历史
    const ExpansionHistory& getCurrentExpHistory() const;
    
    // 新增：清除当前历史记录
    void clearCurrentHistories();

    // 修改：导出和导入变量的方法签名
    std::string exportVariables(const std::string& filename, const std::deque<std::string>& commandHistory);
    std::pair<std::string, std::vector<std::string>> importVariables(const std::string& filename);

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
