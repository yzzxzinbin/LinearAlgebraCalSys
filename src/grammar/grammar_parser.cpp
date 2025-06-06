#include "grammar_parser.h"
#include <stdexcept>
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

Token Parser::peek() const {
    if (isAtEnd()) return Token(TokenType::END_OF_INPUT, "");
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::END_OF_INPUT;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    std::stringstream error;
    error << message << ": 期望 '" << static_cast<int>(type) << "', 但得到 '" 
          << static_cast<int>(peek().type) << "'";
    throw std::runtime_error(error.str());
}

std::unique_ptr<AstNode> Parser::parse() {
    try {
        return statement();
    } catch (const std::exception& e) {
        std::cerr << "解析错误: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<AstNode> Parser::statement() {
    // 处理关键字命令
    if (match(TokenType::KEYWORD)) {
        std::string command = previous().value;
        auto node = std::make_unique<CommandNode>(command);
        
        // 安全收集命令参数，直到遇到终止符或者输入结束
        while (!isAtEnd() && peek().type != TokenType::END_OF_INPUT && 
               peek().type != TokenType::SEMICOLON) {
            try {
                // 确保我们只添加有效的参数
                if (peek().type == TokenType::IDENTIFIER || 
                    peek().type == TokenType::INTEGER ||
                    peek().type == TokenType::FRACTION) {
                    node->arguments.push_back(advance().value);
                } else {
                    // 如果不是有效参数类型，跳过
                    advance();
                }
            } catch (const std::exception& e) {
                // 如果处理参数时出错，停止收集参数
                std::cerr << "参数处理错误: " << e.what() << std::endl;
                break;
            }
        }
        
        // 如果有分号结束符，消费它
        match(TokenType::SEMICOLON);
        
        return node;
    }
    
    // 处理赋值语句
    if (match(TokenType::IDENTIFIER) && check(TokenType::ASSIGN)) {
        return assignment();
    }
    
    // 默认处理为表达式
    return expression();
}

std::unique_ptr<AstNode> Parser::assignment() {
    std::string name = previous().value;
    consume(TokenType::ASSIGN, "赋值语句缺少'='");
    
    auto expr = expression();
    return std::make_unique<AssignmentNode>(name, std::move(expr));
}

std::unique_ptr<AstNode> Parser::expression() {
    return term();
}

std::unique_ptr<AstNode> Parser::term() {
    auto expr = factor();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        TokenType op = previous().type;
        auto right = factor();
        expr = std::make_unique<BinaryOpNode>(op, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<AstNode> Parser::factor() {
    auto expr = primary();
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE)) {
        TokenType op = previous().type;
        auto right = primary();
        expr = std::make_unique<BinaryOpNode>(op, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<AstNode> Parser::primary() {
    // 处理标识符（变量或函数调用）
    if (match(TokenType::IDENTIFIER)) {
        std::string name = previous().value;
        
        // 函数调用
        if (match(TokenType::LEFT_PAREN)) {
            return functionCall(name);
        }
        
        // 变量引用
        return std::make_unique<VariableNode>(name);
    }
    
    // 处理整数和分数
    if (match(TokenType::INTEGER) || match(TokenType::FRACTION)) {
        std::string value = previous().value;
        
        if (previous().type == TokenType::INTEGER) {
            return std::make_unique<LiteralNode>(ParsedValue(Fraction(std::stoll(value))));
        } else { // TokenType::FRACTION
            size_t pos = value.find('/');
            if (pos != std::string::npos) {
                long long num = std::stoll(value.substr(0, pos));
                long long den = std::stoll(value.substr(pos + 1));
                return std::make_unique<LiteralNode>(ParsedValue(Fraction(num, den)));
            } else {
                return std::make_unique<LiteralNode>(ParsedValue(Fraction(std::stoll(value))));
            }
        }
    }
    
    // 处理矩阵字面量
    if (match(TokenType::LEFT_BRACKET)) {
        // 尝试解析矩阵或向量
        if (peek().type == TokenType::INTEGER || peek().type == TokenType::FRACTION) {
            if (tokens[current + 1].type == TokenType::COMMA && 
                tokens[current + 2].type != TokenType::LEFT_BRACKET) {
                // 看起来是向量
                return std::make_unique<LiteralNode>(parseVector());
            } else {
                // 看起来是矩阵
                return std::make_unique<LiteralNode>(parseMatrix());
            }
        }
    }
    
    // 处理括号表达式
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = expression();
        consume(TokenType::RIGHT_PAREN, "表达式缺少右括号')'");
        return expr;
    }
    
    throw std::runtime_error("无法解析表达式");
}

std::unique_ptr<AstNode> Parser::functionCall(const std::string& name) {
    auto node = std::make_unique<FunctionCallNode>(name);
    
    // 如果没有参数，直接返回
    if (match(TokenType::RIGHT_PAREN)) {
        return node;
    }
    
    // 解析参数列表
    node->arguments = arguments();
    consume(TokenType::RIGHT_PAREN, "函数调用缺少右括号')'");
    
    return node;
}

std::vector<std::unique_ptr<AstNode>> Parser::arguments() {
    std::vector<std::unique_ptr<AstNode>> args;
    
    // 添加第一个参数
    args.push_back(expression());
    
    // 添加剩余的参数
    while (match(TokenType::COMMA)) {
        args.push_back(expression());
    }
    
    return args;
}

// 解析矩阵字面量
ParsedValue Parser::parseMatrix() {
    std::vector<std::vector<Fraction>> matrix;
    std::vector<Fraction> currentRow;
    
    // 跳过左括号，因为它已经在调用函数中被消费了
    
    try {
        while (!isAtEnd()) {
            if (match(TokenType::RIGHT_BRACKET)) {
                // 如果当前行不为空，添加到矩阵中
                if (!currentRow.empty()) {
                    matrix.push_back(currentRow);
                }
                break;
            } else if (match(TokenType::SEMICOLON)) {
                // 行分隔符，保存当前行并开始新行
                matrix.push_back(currentRow);
                currentRow.clear();
            } else if (match(TokenType::INTEGER) || match(TokenType::FRACTION)) {
                // 解析元素
                std::string value = previous().value;
                Fraction frac;
                
                try {
                    if (previous().type == TokenType::INTEGER) {
                        frac = Fraction(std::stoll(value));
                    } else { // TokenType::FRACTION
                        size_t pos = value.find('/');
                        if (pos != std::string::npos) {
                            // 安全地解析分子和分母
                            try {
                                long long num = std::stoll(value.substr(0, pos));
                                long long den = std::stoll(value.substr(pos + 1));
                                frac = Fraction(num, den);
                            } catch (const std::exception& e) {
                                std::cerr << "解析分数时出错: " << e.what() << std::endl;
                                frac = Fraction(0); // 设置默认值
                            }
                        } else {
                            frac = Fraction(std::stoll(value));
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "解析数字时出错: " << e.what() << std::endl;
                    frac = Fraction(0); // 设置默认值
                }
                
                currentRow.push_back(frac);
                
                // 如果后面是逗号，跳过
                match(TokenType::COMMA);
            } else {
                // 无效的标记
                throw std::runtime_error("解析矩阵时遇到无效的标记");
            }
        }
    } catch (const std::exception& e) {
        // 添加错误处理
        std::cerr << "解析矩阵时出错: " << e.what() << std::endl;
        // 返回一个空矩阵
        return ParsedValue(std::vector<std::vector<Fraction>>());
    }
    
    return ParsedValue(matrix);
}

// 解析向量字面量
ParsedValue Parser::parseVector() {
    std::vector<Fraction> vector;
    
    // 跳过左括号，因为它已经在调用函数中被消费了
    
    try {
        while (!isAtEnd()) {
            if (match(TokenType::RIGHT_BRACKET)) {
                break;
            } else if (match(TokenType::INTEGER) || match(TokenType::FRACTION)) {
                // 解析元素
                std::string value = previous().value;
                Fraction frac;
                
                try {
                    if (previous().type == TokenType::INTEGER) {
                        frac = Fraction(std::stoll(value));
                    } else { // TokenType::FRACTION
                        size_t pos = value.find('/');
                        if (pos != std::string::npos) {
                            // 安全地解析分子和分母
                            try {
                                long long num = std::stoll(value.substr(0, pos));
                                long long den = std::stoll(value.substr(pos + 1));
                                frac = Fraction(num, den);
                            } catch (const std::exception& e) {
                                std::cerr << "解析分数时出错: " << e.what() << std::endl;
                                frac = Fraction(0); // 设置默认值
                            }
                        } else {
                            frac = Fraction(std::stoll(value));
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "解析数字时出错: " << e.what() << std::endl;
                    frac = Fraction(0); // 设置默认值
                }
                
                vector.push_back(frac);
                
                // 如果后面是逗号，跳过
                match(TokenType::COMMA);
            } else {
                // 无效的标记
                throw std::runtime_error("解析向量时遇到无效的标记");
            }
        }
    } catch (const std::exception& e) {
        // 添加错误处理
        std::cerr << "解析向量时出错: " << e.what() << std::endl;
        // 返回一个空向量
        return ParsedValue(std::vector<Fraction>());
    }
    
    return ParsedValue(vector);
}

// 解析分数字面量
ParsedValue Parser::parseFraction() {
    // 这个方法已经在 primary() 中以内联的方式实现了
    // 这里只是为了保持接口的完整性
    throw std::runtime_error("该方法不应该被直接调用");
}
