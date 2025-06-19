#include "enhanced_variable_viewer.h"
#include "../utils/tui_utils.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

EnhancedVariableViewer::EnhancedVariableViewer(const Interpreter& interp, int termRows, int termCols)
    : currentSelection(0), scrollOffset(0), terminalRows(termRows), terminalCols(termCols), 
      interpreter(interp) {
    
    std::iostream::sync_with_stdio(false);
    refreshVariableList();
    updateLayout();
    updateStatus("变量预览器：上下键选择变量，ESC退出");
}

EnhancedVariableViewer::~EnhancedVariableViewer() {
    std::iostream::sync_with_stdio(true);
}

void EnhancedVariableViewer::refreshVariableList() {
    variableList.clear();
    const auto& vars = interpreter.getVariables();
    
    for (const auto& pair : vars) {
        VariableItem item;
        item.name = pair.first;
        item.type = pair.second.type;
        item.typeString = getTypeString(pair.second.type);
        item.sizeInfo = getSizeInfo(pair.second);
        variableList.push_back(item);
    }
    
    // 确保选择索引有效
    if (currentSelection >= variableList.size() && !variableList.empty()) {
        currentSelection = variableList.size() - 1;
    }
    if (variableList.empty()) {
        currentSelection = 0;
    }
}

void EnhancedVariableViewer::updateLayout() {
    // 列表占左侧约1/3，预览窗口占右侧约2/3
    listStartRow = 2;
    listStartCol = 1;
    listHeight = terminalRows - 4; // 留出标题和状态栏空间
    listWidth = std::max(20, terminalCols / 3);
    
    previewStartRow = 2;
    previewStartCol = listStartCol + listWidth + 2; // 留2列间距
    previewHeight = terminalRows - 4;
    previewWidth = terminalCols - previewStartCol - 1;
    
    // 确保预览窗口有最小尺寸
    if (previewWidth < 10) {
        listWidth = std::max(15, terminalCols - 15);
        previewStartCol = listStartCol + listWidth + 2;
        previewWidth = terminalCols - previewStartCol - 1;
    }
}

std::string EnhancedVariableViewer::getTypeString(VariableType type) const {
    switch (type) {
        case VariableType::FRACTION: return "分数";
        case VariableType::VECTOR: return "向量";
        case VariableType::MATRIX: return "矩阵";
        case VariableType::RESULT: return "结果";
        case VariableType::EQUATION_SOLUTION: return "方程组解";
        default: return "未知";
    }
}

std::string EnhancedVariableViewer::getSizeInfo(const Variable& var) const {
    switch (var.type) {
        case VariableType::VECTOR:
            return std::to_string(var.vectorValue.size()) + "维";
        case VariableType::MATRIX: {
            size_t rows = var.matrixValue.rowCount();
            size_t cols = var.matrixValue.colCount();
            return std::to_string(rows) + "×" + std::to_string(cols);
        }
        case VariableType::FRACTION:
        case VariableType::RESULT:
        case VariableType::EQUATION_SOLUTION:
        default:
            return "";
    }
}

void EnhancedVariableViewer::updateStatus(const std::string& msg) {
    statusMessage = msg;
}

void EnhancedVariableViewer::clearScreen() {
    for (int i = 1; i < terminalRows - 1; i++) {
        Terminal::setCursor(i, 0);
        std::cout << std::string(terminalCols, ' ');
    }
}

EnhancedVariableViewer::ViewerResult EnhancedVariableViewer::handleInput(int key) {
    switch (key) {
        case KEY_UP:
            if (currentSelection > 0) {
                currentSelection--;
                updateScrolling();
            }
            return ViewerResult::CONTINUE;
            
        case KEY_DOWN:
            if (currentSelection < variableList.size() - 1) {
                currentSelection++;
                updateScrolling();
            }
            return ViewerResult::CONTINUE;
            
        case KEY_ESCAPE:
            return ViewerResult::EXIT;
            
        default:
            return ViewerResult::CONTINUE;
    }
}

void EnhancedVariableViewer::updateScrolling() {
    if (variableList.empty()) return;
    
    if (currentSelection < scrollOffset) {
        scrollOffset = currentSelection;
    } else if (currentSelection >= scrollOffset + listHeight) {
        scrollOffset = currentSelection - listHeight + 1;
    }
    scrollOffset = std::max(size_t(0), scrollOffset);
}

void EnhancedVariableViewer::draw() {
    clearScreen();
    
    // 绘制标题
    Terminal::setCursor(0, 0);
    Terminal::setForeground(Color::CYAN);
    Terminal::setBackground(Color::BLUE);
    std::string title = " 变量预览器 ";
    int padding = (terminalCols - title.length()) / 2;
    std::string header(terminalCols, ' ');
    for (size_t i = 0; i < title.length(); i++) {
        header[padding + i] = title[i];
    }
    std::cout << header;
    Terminal::resetColor();
    
    drawLayout();
}

void EnhancedVariableViewer::drawLayout() {
    drawVariableList();
    drawPreviewWindow();
    
    // 绘制状态栏
    Terminal::setCursor(terminalRows - 1, 0);
    Terminal::setForeground(Color::BLACK);
    Terminal::setBackground(Color::WHITE);
    std::string status = " " + statusMessage;
    status.resize(terminalCols, ' ');
    std::cout << status;
    Terminal::resetColor();
}

void EnhancedVariableViewer::drawVariableList() {
    if (variableList.empty()) {
        Terminal::setCursor(listStartRow + 1, listStartCol);
        Terminal::setForeground(Color::YELLOW);
        std::cout << "没有已定义的变量";
        Terminal::resetColor();
        return;
    }
    
    for (int i = 0; i < listHeight && (scrollOffset + i) < variableList.size(); i++) {
        size_t itemIndex = scrollOffset + i;
        const auto& item = variableList[itemIndex];
        
        Terminal::setCursor(listStartRow + i, listStartCol);
        
        bool isSelected = (itemIndex == currentSelection);
        if (isSelected) {
            Terminal::setBackground(Color::CYAN);
            Terminal::setForeground(Color::BLACK);
        } else {
            Terminal::setForeground(Color::WHITE);
        }
        
        std::string line = item.name + " (" + item.typeString;
        if (!item.sizeInfo.empty()) {
            line += " " + item.sizeInfo;
        }
        line += ")";
        
        if (line.length() > listWidth - 2) {
            line = line.substr(0, listWidth - 5) + "...";
        }
        line.resize(listWidth - 2, ' ');
        
        std::cout << " " << line << " ";
        Terminal::resetColor();
    }
}

void EnhancedVariableViewer::drawPreviewWindow() {
    // 绘制预览窗口边框
    TuiUtils::drawBox(previewStartRow - 1, previewStartCol - 1, 
                      previewHeight + 2, previewWidth + 2, 
                      " 预览 ", Color::WHITE, Color::DEFAULT);
    
    if (variableList.empty() || currentSelection >= variableList.size()) {
        Terminal::setCursor(previewStartRow + 1, previewStartCol + 1);
        Terminal::setForeground(Color::YELLOW);
        std::cout << "无变量可预览";
        Terminal::resetColor();
        return;
    }
    
    const auto& selectedItem = variableList[currentSelection];
    const auto& vars = interpreter.getVariables();
    auto it = vars.find(selectedItem.name);
    
    if (it != vars.end()) {
        drawPreviewContent(it->second);
    }
}

void EnhancedVariableViewer::drawPreviewContent(const Variable& var) {
    switch (var.type) {
        case VariableType::FRACTION:
            drawFractionPreview(var.fractionValue);
            break;
        case VariableType::VECTOR:
            drawVectorPreview(var.vectorValue);
            break;
        case VariableType::MATRIX:
            drawMatrixPreview(var.matrixValue);
            break;
        case VariableType::RESULT:
            drawResultPreview(var.resultValue);
            break;
        case VariableType::EQUATION_SOLUTION:
            drawEquationSolutionPreview(var.equationSolutionValue);
            break;
    }
}

void EnhancedVariableViewer::drawFractionPreview(const Fraction& fraction) {
    Terminal::setCursor(previewStartRow, previewStartCol);
    Terminal::setForeground(Color::GREEN);
    std::cout << "值: ";
    Terminal::setForeground(Color::WHITE);
    std::cout << fraction;
    Terminal::resetColor();
}

void EnhancedVariableViewer::drawVectorPreview(const Vector& vector) {
    Terminal::setCursor(previewStartRow, previewStartCol);
    Terminal::setForeground(Color::GREEN);
    std::cout << "维数: " << vector.size();
    Terminal::resetColor();
    
    Terminal::setCursor(previewStartRow + 1, previewStartCol);
    Terminal::setForeground(Color::GREEN);
    std::cout << "内容: ";
    Terminal::setForeground(Color::WHITE);
    
    int currentRow = previewStartRow + 1;
    int currentCol = previewStartCol + 6; // "内容: " 的长度
    
    std::cout << "[";
    currentCol++;
    
    for (size_t i = 0; i < vector.size() && currentRow < previewStartRow + previewHeight - 1; i++) {
        std::ostringstream oss;
        oss << vector.at(i);
        std::string valueStr = oss.str();
        
        if (i > 0) {
            std::cout << ", ";
            currentCol += 2;
        }
        
        // 检查是否需要换行
        if (currentCol + valueStr.length() >= previewStartCol + previewWidth - 2) {
            currentRow++;
            Terminal::setCursor(currentRow, previewStartCol + 1);
            currentCol = previewStartCol + 1;
        }
        
        std::cout << valueStr;
        currentCol += valueStr.length();
    }
    
    std::cout << "]";
    Terminal::resetColor();
}

void EnhancedVariableViewer::drawMatrixPreview(const Matrix& matrix) {
    Terminal::setCursor(previewStartRow, previewStartCol);
    Terminal::setForeground(Color::GREEN);
    std::cout << "大小: " << matrix.rowCount() << "×" << matrix.colCount();
    Terminal::resetColor();
    
    if (matrix.rowCount() == 0 || matrix.colCount() == 0) {
        Terminal::setCursor(previewStartRow + 1, previewStartCol);
        Terminal::setForeground(Color::YELLOW);
        std::cout << "空矩阵";
        Terminal::resetColor();
        return;
    }
    
    int startRow = previewStartRow + 2;
    int maxDisplayRows = std::min((int)matrix.rowCount(), previewHeight - 3);
    int maxDisplayCols = std::min((int)matrix.colCount(), (previewWidth - 4) / 8); // 每个元素大约8个字符
    
    for (int r = 0; r < maxDisplayRows; r++) {
        Terminal::setCursor(startRow + r, previewStartCol);
        Terminal::setForeground(Color::CYAN);
        std::cout << "| ";
        Terminal::setForeground(Color::WHITE);
        
        for (int c = 0; c < maxDisplayCols; c++) {
            std::ostringstream oss;
            oss << matrix.at(r, c);
            std::string valueStr = oss.str();
            
            if (valueStr.length() > 7) {
                valueStr = valueStr.substr(0, 4) + "...";
            }
            std::cout << std::setw(7) << valueStr << " ";
        }
        
        if (maxDisplayCols < (int)matrix.colCount()) {
            std::cout << "...";
        }
        
        Terminal::setForeground(Color::CYAN);
        std::cout << " |";
        Terminal::resetColor();
    }
    
    if (maxDisplayRows < (int)matrix.rowCount()) {
        Terminal::setCursor(startRow + maxDisplayRows, previewStartCol);
        Terminal::setForeground(Color::YELLOW);
        std::cout << "... (显示部分内容)";
        Terminal::resetColor();
    }
}

void EnhancedVariableViewer::drawResultPreview(const Result& result) {
    Terminal::setCursor(previewStartRow, previewStartCol);
    Terminal::setForeground(Color::GREEN);
    std::cout << "结果类型变量";
    Terminal::resetColor();
    
    // 将结果内容输出到字符串流以处理多行
    std::stringstream result_ss;
    result_ss << result;
    std::string result_str = result_ss.str();
    std::istringstream result_iss(result_str);
    std::string result_line;
    
    int currentRow = previewStartRow + 1;
    while (std::getline(result_iss, result_line) && currentRow < previewStartRow + previewHeight - 1) {
        Terminal::setCursor(currentRow, previewStartCol);
        Terminal::setForeground(Color::WHITE);
        
        if (result_line.length() > previewWidth - 2) {
            result_line = result_line.substr(0, previewWidth - 5) + "...";
        }
        std::cout << result_line;
        currentRow++;
    }
    Terminal::resetColor();
}

void EnhancedVariableViewer::drawEquationSolutionPreview(const EquationSolution& solution) {
    Terminal::setCursor(previewStartRow, previewStartCol);
    Terminal::setForeground(Color::GREEN);
    std::cout << "方程组解变量";
    Terminal::resetColor();
    
    // 将解内容输出到字符串流以处理多行
    std::stringstream sol_ss;
    solution.print(sol_ss);
    std::string sol_str = sol_ss.str();
    std::istringstream sol_iss(sol_str);
    std::string sol_line;
    
    int currentRow = previewStartRow + 1;
    while (std::getline(sol_iss, sol_line) && currentRow < previewStartRow + previewHeight - 1) {
        Terminal::setCursor(currentRow, previewStartCol);
        Terminal::setForeground(Color::WHITE);
        
        if (sol_line.length() > previewWidth - 2) {
            sol_line = sol_line.substr(0, previewWidth - 5) + "...";
        }
        std::cout << sol_line;
        currentRow++;
    }
    Terminal::resetColor();
}

std::string EnhancedVariableViewer::getStatusMessage() const {
    return statusMessage;
}

void EnhancedVariableViewer::updateDimensions(int termRows, int termCols) {
    this->terminalRows = termRows;
    this->terminalCols = termCols;
    updateLayout();
}