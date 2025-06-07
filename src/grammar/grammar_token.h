#pragma once
#include <string>
#include <vector>
#include "../fraction.h"

// 词法单元类型
enum class TokenType {
    // 标识符和文字
    IDENTIFIER,     // 变量名、函数名
    INTEGER,        // 整数
    FRACTION,       // 分数
    
    // 运算符
    PLUS,           // +
    MINUS,          // -
    MULTIPLY,       // *
    DIVIDE,         // /
    CROSS_PRODUCT,  // x (用于向量叉乘)
    ASSIGN,         // =
    
    // 分隔符
    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    COMMA,          // ,
    SEMICOLON,      // ;
    
    // 关键字
    KEYWORD,        // help, clear, vars, exit, steps 等
    
    // 其他
    END_OF_INPUT,   // 输入结束
    UNKNOWN         // 未知标记
};

// 词法单元结构
struct Token {
    TokenType type;
    std::string value;
    
    Token(TokenType t, const std::string& v) : type(t), value(v) {}
};

// 用于表示解析后的矩阵、向量和分数
struct ParsedValue {
    enum class Type {
        NONE,
        FRACTION,
        VECTOR,
        MATRIX
    };
    
    Type type;
    Fraction fractionValue;
    std::vector<Fraction> vectorValue;
    std::vector<std::vector<Fraction>> matrixValue;
    
    ParsedValue() : type(Type::NONE) {}
    
    explicit ParsedValue(const Fraction& f) : type(Type::FRACTION), fractionValue(f) {}
    
    explicit ParsedValue(const std::vector<Fraction>& v) : type(Type::VECTOR), vectorValue(v) {}
    
    explicit ParsedValue(const std::vector<std::vector<Fraction>>& m) : type(Type::MATRIX), matrixValue(m) {}
};
