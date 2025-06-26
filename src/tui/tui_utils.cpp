#include "tui_app.h"
#include "tui_terminal.h"
#include <iostream>
#include <string>
#include <sstream> // For std::istringstream, std::stringstream
#include <vector>  // For getVariableNames

void TuiApp::printToResultView(const std::string& text, Color color) {
    if (matrixEditor) return; // 不在编辑器模式下打印到主结果视图
    if (resultRow < 0) return; // Ensure resultRow is valid

    Terminal::setForeground(color);
    std::istringstream iss(text);
    std::string line;
    // bool firstLine = true; // Not used
    while (std::getline(iss, line)) {
        if (resultRow >= inputRow -1) break; // Stop if about to overwrite input/status bar
        Terminal::setCursor(resultRow, 0); 
        std::cout << line;
        std::cout << std::endl; 
        resultRow++;
    }
    Terminal::resetColor();
}

std::string TuiApp::variableToString(const Variable& var) {
    std::stringstream ss;
    switch (var.type) {
        case VariableType::FRACTION:
            ss << var.fractionValue;
            break;
        case VariableType::VECTOR:
            var.vectorValue.print(ss);
            break;
        case VariableType::MATRIX:
            ss << std::endl; // Matrix print usually starts with a newline for alignment
            var.matrixValue.print(ss);
            break;
        case VariableType::RESULT: // Assuming Result has an operator<< or a toString method
            ss << var.resultValue;
            break;
        case VariableType::EQUATION_SOLUTION:
            ss << var.equationSolutionValue.getDetailedDescription(); 
    }
    return ss.str();
}

std::string TuiApp::formatStringWithBracketHighlight(const std::string& text, size_t cursorPos) {
    if (!TuiUtils::areBracketsBalanced(text)) {
        return text; // 括号不平衡，不高亮
    }
    
    if (!TuiUtils::isCursorInBrackets(text, cursorPos)) {
        return text; // 光标不在括号内，不高亮
    }
    
    TuiUtils::BracketPair pair = TuiUtils::findInnermostBracketPair(text, cursorPos);
    if (pair.openPos == std::string::npos || pair.closePos == std::string::npos) {
        return text;
    }
    
    std::string result;
    
    // 开括号之前的部分
    result += text.substr(0, pair.openPos);
    
    // 开括号
    result += text[pair.openPos];
    
    // 括号内容（加下划线）
    result += "\033[4m"; // ANSI下划线开始
    result += text.substr(pair.openPos + 1, pair.closePos - pair.openPos - 1);
    result += "\033[24m"; // ANSI下划线结束
    
    // 闭括号
    result += text[pair.closePos];
    
    // 闭括号之后的部分
    if (pair.closePos + 1 < text.length()) {
        result += text.substr(pair.closePos + 1);
    }
    
    return result;
}

std::string variableTypeString(const VariableType& type) {
    switch (type) {
        case VariableType::FRACTION: return "FRACTION";
        case VariableType::VECTOR: return "VECTOR";
        case VariableType::MATRIX: return "MATRIX";
        case VariableType::RESULT: return "RESULT";
        case VariableType::EQUATION_SOLUTION: return "EQUATION_SOLUTION";
        default: return "UNKNOWN";
    }
}

std::vector<std::string> TuiApp::getVariableNames() const {
    std::vector<std::string> names;
    const auto& vars = interpreter.getVariables();
    for (const auto& pair : vars) {
        names.push_back(pair.first);
    }
    return names;
}
