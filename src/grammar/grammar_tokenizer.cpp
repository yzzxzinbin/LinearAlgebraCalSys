#include "grammar_tokenizer.h"
#include <cctype>
#include <unordered_map>

// 关键字映射
static const std::unordered_map<std::string, bool> keywords = {
    {"help", true},
    {"clear", true},
    {"vars", true},
    {"exit", true},
    {"steps", true},
    {"show", true},
    {"new", true},
    {"edit", true},
    {"export", true}, // 新增关键字
    {"import", true},  // 新增关键字
    {"csv",true}
};

Tokenizer::Tokenizer(const std::string& input) : input(input), position(0) {}

char Tokenizer::peek() const {
    if (isAtEnd()) return '\0';
    return input[position];
}

char Tokenizer::advance() {
    if (isAtEnd()) return '\0';
    return input[position++];
}

bool Tokenizer::isAtEnd() const {
    return position >= input.length();
}

void Tokenizer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(peek())) {
        advance();
    }
}

Token Tokenizer::identifier() {
    try {
        if (position >= input.length()) {
            return Token(TokenType::UNKNOWN, "");
        }
        
        size_t start = position;
        while (position < input.length() && (std::isalnum(input[position]) || input[position] == '_')) {
            position++;
        }
        
        // 安全检查
        if (start >= input.length() || position <= start) {
            return Token(TokenType::UNKNOWN, "");
        }
        
        std::string text = input.substr(start, position - start);
        
        // 检查是否是关键字
        if (keywords.find(text) != keywords.end()) {
            return Token(TokenType::KEYWORD, text);
        }
        
        return Token(TokenType::IDENTIFIER, text);
    } catch (const std::exception& e) {
        // 如果出现异常，返回未知标记
        std::cerr << "标识符处理错误: " << e.what() << std::endl;
        return Token(TokenType::UNKNOWN, "ERROR");
    }
}

Token Tokenizer::number() {
    try {
        size_t start = position;
        
        // 处理整数部分
        while (!isAtEnd() && std::isdigit(peek())) {
            advance();
        }
        
        // 检查是否有分数部分
        if (peek() == '/') {
            advance(); // 消费 '/'
            
            // 确保分母是有效的数字
            if (!std::isdigit(peek())) {
                return Token(TokenType::UNKNOWN, input.substr(start, position - start));
            }
            
            // 处理分母
            while (!isAtEnd() && std::isdigit(peek())) {
                advance();
            }
            
            return Token(TokenType::FRACTION, input.substr(start, position - start));
        }
        
        return Token(TokenType::INTEGER, input.substr(start, position - start));
    } catch (const std::exception& e) {
        std::cerr << "处理数字时出错: " << e.what() << std::endl;
        return Token(TokenType::UNKNOWN, "ERROR");
    }
}

Token Tokenizer::nextToken() {
    try {
        skipWhitespace();
        
        if (isAtEnd()) {
            return Token(TokenType::END_OF_INPUT, "");
        }
        
        char c = advance(); // 消耗字符
        
        // 处理标识符
        if (std::isalpha(c) || c == '_') {
            position--; // 回退，以便identifier()可以从开头开始处理
            return identifier();
        }
        
        // 处理数字
        if (std::isdigit(c)) {
            position--; // 回退，以便number()可以从开头开始处理
            return number();
        }
        
        // 处理运算符和分隔符
        switch (c) {
            case '+': return Token(TokenType::PLUS, "+");
            case '-': return Token(TokenType::MINUS, "-");
            case '*': return Token(TokenType::MULTIPLY, "*");
            case '/': return Token(TokenType::DIVIDE, "/");
            case '=': return Token(TokenType::ASSIGN, "=");
            case '[': return Token(TokenType::LEFT_BRACKET, "[");
            case ']': return Token(TokenType::RIGHT_BRACKET, "]");
            case '(': return Token(TokenType::LEFT_PAREN, "(");
            case ')': return Token(TokenType::RIGHT_PAREN, ")");
            case ',': return Token(TokenType::COMMA, ",");
            case ';': return Token(TokenType::SEMICOLON, ";");
            case '^': return Token(TokenType::POWER, "^");
            default: return Token(TokenType::UNKNOWN, std::string(1, c));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("标记化错误: " + std::string(e.what()));
        return Token(TokenType::UNKNOWN, "ERROR");
    }
}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    
    while (true) {
        Token token = nextToken();
        tokens.push_back(token);
        
        if (token.type == TokenType::END_OF_INPUT) {
            break;
        }
    }
    
    return tokens;
}
