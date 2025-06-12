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
    }
    return ss.str();
}

std::vector<std::string> TuiApp::getVariableNames() const {
    std::vector<std::string> names;
    const auto& vars = interpreter.getVariables();
    for (const auto& pair : vars) {
        names.push_back(pair.first);
    }
    return names;
}
