#pragma once
#include <string>
#include <vector>
#include "grammar_token.h"
#include "../utils/logger.h"

class Tokenizer {
private:
    std::string input;
    size_t position;

public:
    Tokenizer(const std::string& input);
    
    // 获取下一个词法单元
    Token nextToken();
    
    // 将整个输入字符串转换为词法单元序列
    std::vector<Token> tokenize();
    
private:
    // 辅助方法
    char peek() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespace();
    
    // 解析各种类型的词法单元
    Token identifier();
    Token number();
};
