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
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) || match(TokenType::CROSS_PRODUCT)) { // 添加对 CROSS_PRODUCT 的匹配
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
        LOG_DEBUG("开始解析矩阵或向量字面量");
        
        // 尝试先看是矩阵还是向量
        // 保存当前位置，以便回溯
        size_t startPos = current;
        bool isVector = true;
        bool hasSemicolon = false;
        
        // 向前查看直到找到右括号、分号或结束
        int bracketLevel = 1; // 已经消费了一个左括号
        while (current < tokens.size() && bracketLevel > 0) {
            if (tokens[current].type == TokenType::LEFT_BRACKET) {
                bracketLevel++;
            } else if (tokens[current].type == TokenType::RIGHT_BRACKET) {
                bracketLevel--;
            } else if (tokens[current].type == TokenType::SEMICOLON) {
                hasSemicolon = true;
                isVector = false;
                break;
            }
            current++;
        }
        
        // 回到原来的位置
        current = startPos;
        
        // 根据是否有分号决定是矩阵还是向量
        if (hasSemicolon || !isVector) {
            LOG_DEBUG("检测到矩阵格式，调用parseMatrix()");
            return std::make_unique<LiteralNode>(parseMatrix());
        } else {
            LOG_DEBUG("检测到向量格式，调用parseVector()");
            return std::make_unique<LiteralNode>(parseVector());
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
    LOG_DEBUG("开始解析矩阵");
    
    try {
        while (!isAtEnd()) {
            if (match(TokenType::RIGHT_BRACKET)) {
                // 如果当前行不为空，添加到矩阵中
                if (!currentRow.empty()) {
                    matrix.push_back(currentRow);
                    LOG_DEBUG("矩阵最后一行添加完成，行元素数: " + std::to_string(currentRow.size()));
                }
                LOG_DEBUG("矩阵解析完成，维度: " + std::to_string(matrix.size()) + "x" + 
                          (matrix.empty() ? "0" : std::to_string(matrix[0].size())));
                break;
            } else if (match(TokenType::SEMICOLON)) {
                // 行分隔符，保存当前行并开始新行
                if (!currentRow.empty()) {
                    matrix.push_back(currentRow);
                    LOG_DEBUG("矩阵新行添加完成，行元素数: " + std::to_string(currentRow.size()));
                    currentRow.clear();
                } else {
                    LOG_WARNING("遇到空行，忽略");
                }
            } else if (match(TokenType::INTEGER) || match(TokenType::FRACTION)) {
                // 解析元素
                std::string value = previous().value;
                LOG_DEBUG("解析矩阵元素: " + value);
                
                Fraction frac;
                try {
                    // 确保值不包含非法字符
                    size_t valueStartPos = 0;
                    // 跳过非数字字符
                    while (valueStartPos < value.length() && !std::isdigit(value[valueStartPos]) && value[valueStartPos] != '-') {
                        valueStartPos++;
                    }
                    
                    // 提取纯数字部分
                    std::string cleanValue = value.substr(valueStartPos);
                    LOG_DEBUG("清理后的元素值: " + cleanValue);
                    
                    if (previous().type == TokenType::INTEGER) {
                        frac = Fraction(std::stoll(cleanValue));
                    } else { // TokenType::FRACTION
                        size_t pos = cleanValue.find('/');
                        if (pos != std::string::npos) {
                            long long num = std::stoll(cleanValue.substr(0, pos));
                            long long den = std::stoll(cleanValue.substr(pos + 1));
                            frac = Fraction(num, den);
                        } else {
                            frac = Fraction(std::stoll(cleanValue));
                        }
                    }
                    currentRow.push_back(frac);
                    LOG_DEBUG("元素解析成功: " + cleanValue);
                } catch (const std::exception& e) {
                    LOG_ERROR("解析数字时出错: " + std::string(e.what()) + ", 值: " + value);
                    frac = Fraction(0); // 设置默认值
                    currentRow.push_back(frac);
                }
                
                // 如果后面是逗号，跳过
                match(TokenType::COMMA);
            } else {
                // 无效的标记
                LOG_ERROR("解析矩阵时遇到无效的标记: " + std::to_string(static_cast<int>(peek().type)));
                throw std::runtime_error("解析矩阵时遇到无效的标记");
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("解析矩阵时出错: " + std::string(e.what()));
        
        // 如果已经有一些有效的行，尝试返回已解析的部分
        if (!matrix.empty()) {
            LOG_INFO("返回部分解析的矩阵，行数: " + std::to_string(matrix.size()));
            return ParsedValue(matrix);
        }
        
        // 否则返回一个空矩阵
        LOG_WARNING("返回空矩阵");
        return ParsedValue(std::vector<std::vector<Fraction>>());
    }
    
    // 确保矩阵每行的列数相同
    if (!matrix.empty()) {
        size_t cols = matrix[0].size();
        for (size_t i = 1; i < matrix.size(); ++i) {
            if (matrix[i].size() != cols) {
                LOG_ERROR("矩阵行的列数不一致");
                throw std::runtime_error("矩阵格式错误：每行的元素数量必须相同");
            }
        }
    }
    
    return ParsedValue(matrix);
}

// 解析向量字面量 (类似的修改)
ParsedValue Parser::parseVector() {
    std::vector<Fraction> vector;
    
    // 跳过左括号，因为它已经在调用函数中被消费了
    LOG_DEBUG("开始解析向量");
    
    try {
        while (!isAtEnd()) {
            if (match(TokenType::RIGHT_BRACKET)) {
                LOG_DEBUG("向量解析完成，长度: " + std::to_string(vector.size()));
                break;
            } else if (match(TokenType::INTEGER) || match(TokenType::FRACTION)) {
                // 解析元素
                std::string value = previous().value;
                LOG_DEBUG("解析向量元素: " + value);
                
                Fraction frac;
                try {
                    // 清理非数字字符
                    size_t valueStartPos = 0;
                    while (valueStartPos < value.length() && !std::isdigit(value[valueStartPos]) && value[valueStartPos] != '-') {
                        valueStartPos++;
                    }
                    
                    std::string cleanValue = value.substr(valueStartPos);
                    LOG_DEBUG("清理后的元素值: " + cleanValue);
                    
                    if (previous().type == TokenType::INTEGER) {
                        frac = Fraction(std::stoll(cleanValue));
                    } else { // TokenType::FRACTION
                        size_t pos = cleanValue.find('/');
                        if (pos != std::string::npos) {
                            long long num = std::stoll(cleanValue.substr(0, pos));
                            long long den = std::stoll(cleanValue.substr(pos + 1));
                            frac = Fraction(num, den);
                        } else {
                            frac = Fraction(std::stoll(cleanValue));
                        }
                    }
                    vector.push_back(frac);
                    LOG_DEBUG("元素解析成功: " + cleanValue);
                } catch (const std::exception& e) {
                    LOG_ERROR("解析数字时出错: " + std::string(e.what()) + ", 值: " + value);
                    frac = Fraction(0); // 设置默认值
                    vector.push_back(frac);
                }
                
                // 如果后面是逗号，跳过
                match(TokenType::COMMA);
            } else {
                // 无效的标记
                LOG_ERROR("解析向量时遇到无效的标记: " + std::to_string(static_cast<int>(peek().type)));
                throw std::runtime_error("解析向量时遇到无效的标记");
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("解析向量时出错: " + std::string(e.what()));
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
