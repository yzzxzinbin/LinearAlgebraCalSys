#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "grammar_token.h"

// 抽象语法树节点类型
enum class AstNodeType {
    VARIABLE,       // 变量引用
    LITERAL,        // 字面量（数字、矩阵、向量）
    BINARY_OP,      // 二元运算
    UNARY_OP,       // 一元运算
    FUNCTION_CALL,  // 函数调用
    ASSIGNMENT,     // 赋值语句
    COMMAND         // 系统命令
};

// 抽象语法树节点
class AstNode {
public:
    AstNodeType type;
    
    AstNode(AstNodeType type) : type(type) {}
    virtual ~AstNode() = default;
};

// 变量引用节点
class VariableNode : public AstNode {
public:
    std::string name;
    
    VariableNode(const std::string& name) : AstNode(AstNodeType::VARIABLE), name(name) {}
};

// 字面量节点
class LiteralNode : public AstNode {
public:
    ParsedValue value;
    
    LiteralNode(const ParsedValue& value) : AstNode(AstNodeType::LITERAL), value(value) {}
};

// 二元运算节点
class BinaryOpNode : public AstNode {
public:
    TokenType op;
    std::unique_ptr<AstNode> left;
    std::unique_ptr<AstNode> right;
    
    BinaryOpNode(TokenType op, std::unique_ptr<AstNode> left, std::unique_ptr<AstNode> right)
        : AstNode(AstNodeType::BINARY_OP), op(op), left(std::move(left)), right(std::move(right)) {}
};

// 函数调用节点
class FunctionCallNode : public AstNode {
public:
    std::string name;
    std::vector<std::unique_ptr<AstNode>> arguments;
    
    FunctionCallNode(const std::string& name) : AstNode(AstNodeType::FUNCTION_CALL), name(name) {}
};

// 赋值语句节点
class AssignmentNode : public AstNode {
public:
    std::string variableName;
    std::unique_ptr<AstNode> expression;
    
    AssignmentNode(const std::string& variableName, std::unique_ptr<AstNode> expression)
        : AstNode(AstNodeType::ASSIGNMENT), variableName(variableName), expression(std::move(expression)) {}
};

// 命令节点
class CommandNode : public AstNode {
public:
    std::string command;
    std::vector<std::string> arguments;
    
    CommandNode(const std::string& command) : AstNode(AstNodeType::COMMAND), command(command) {}
};

// 语法解析器
class Parser {
private:
    std::vector<Token> tokens;
    size_t current;

public:
    Parser(const std::vector<Token>& tokens);
    
    // 解析输入语句
    std::unique_ptr<AstNode> parse();
    
private:
    // 辅助方法
    Token peek() const;
    Token previous() const;
    Token advance();
    bool isAtEnd() const;
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token consume(TokenType type, const std::string& message);
    
    // 递归下降解析方法
    std::unique_ptr<AstNode> statement();
    std::unique_ptr<AstNode> assignment();
    std::unique_ptr<AstNode> expression();
    std::unique_ptr<AstNode> term();
    std::unique_ptr<AstNode> factor();
    std::unique_ptr<AstNode> primary();
    std::unique_ptr<AstNode> functionCall(const std::string& name);
    std::vector<std::unique_ptr<AstNode>> arguments();
    
    // 解析字面量
    ParsedValue parseMatrix();
    ParsedValue parseVector();
    ParsedValue parseFraction();
};
