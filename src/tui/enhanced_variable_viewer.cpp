#include "enhanced_variable_viewer.h"
#include "../utils/tui_utils.h"
#include "./tui_app.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

EnhancedVariableViewer::EnhancedVariableViewer(const Interpreter& interp, int termRows, int termCols)
    : currentSelection(0), scrollOffset(0), terminalRows(termRows), terminalCols(termCols), 
      interpreter(interp), filterInput("") {
    std::iostream::sync_with_stdio(false);
    refreshVariableList();
    updateLayout();
    updateStatus("变量预览器: ↑↓选择变量, 输入过滤, ESC退出");
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
    updateFilter(); // 新增：刷新过滤后的列表
}

void EnhancedVariableViewer::updateFilter() {
    filteredVariableList.clear();
    if (filterInput.empty()) {
        filteredVariableList = variableList;
    } else if (filterInput.size() > 3 && filterInput[0] == '<' && filterInput[1] == '!' && filterInput.back() == '>') {
        // 类型过滤：如 <!MATRIX>
        std::string typeName = filterInput.substr(2, filterInput.size() - 3);
        // 转大写
        std::transform(typeName.begin(), typeName.end(), typeName.begin(), ::toupper);
        for (const auto& item : variableList) {
            std::string itemType;
            itemType = variableTypeString(item.type);
            if (itemType == typeName) {
                filteredVariableList.push_back(item);
            }
        }
    } else {
        std::string filterLower = filterInput;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
        for (const auto& item : variableList) {
            std::string nameLower = item.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find(filterLower) != std::string::npos) {
                filteredVariableList.push_back(item);
            }
        }
    }
    // 修正 currentSelection 和 scrollOffset
    if (currentSelection >= filteredVariableList.size() && !filteredVariableList.empty()) {
        currentSelection = filteredVariableList.size() - 1;
    }
    if (filteredVariableList.empty()) {
        currentSelection = 0;
        scrollOffset = 0;
    } else {
        updateScrolling();
    }
}

void EnhancedVariableViewer::updateLayout() {
    // 列表占左侧约1/4，预览窗口占右侧约3/4
    listStartRow = 2;
    listStartCol = 1;
    listHeight = terminalRows - 4; // 留出标题和状态栏空间
    listWidth = std::max(20, terminalCols / 4);
    
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
    // 处理输入框相关按键
    if (key == KEY_BACKSPACE || key == KEY_DELETE) {
        if (!filterInput.empty()) {
            filterInput.pop_back();
            updateFilter();
            currentSelection = 0;
            scrollOffset = 0;
        }
        return ViewerResult::CONTINUE;
    }
    // 仅允许输入可打印字符
    if (key >= 32 && key <= 126) {
        filterInput += static_cast<char>(key);
        updateFilter();
        currentSelection = 0;
        scrollOffset = 0;
        return ViewerResult::CONTINUE;
    }
    // 方向键和ESC
    switch (key) {
        case KEY_UP:
            if (currentSelection > 0) {
                currentSelection--;
                updateScrolling();
            }
            return ViewerResult::CONTINUE;
        case KEY_DOWN:
            if (currentSelection + 1 < filteredVariableList.size()) {
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
    if (filteredVariableList.empty()) return;
    // 列表高度减去输入框那一行
    int visibleListHeight = listHeight - 1;
    if (currentSelection < scrollOffset) {
        scrollOffset = currentSelection;
    } else if (currentSelection >= scrollOffset + visibleListHeight) {
        scrollOffset = currentSelection - visibleListHeight + 1;
    }
    scrollOffset = std::max(size_t(0), scrollOffset);
}

void EnhancedVariableViewer::draw() {
    clearScreen();

    // 绘制标题
    Terminal::setCursor(0, 0);
    Terminal::setForeground(Color::CYAN);
    Terminal::setBackground(Color::BLUE);
    std::string title = "  变量预览器";
    int padding = (terminalCols - TuiUtils::calculateUtf8VisualWidth(title)) / 2;
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
    // 输入框
    Terminal::setCursor(listStartRow, listStartCol);
    Terminal::setForeground(Color::WHITE);
    Terminal::setBackgroundRGB(45,63,118); // 深蓝色背景
    std::string prompt = "筛选: ";
    std::string inputText = prompt + filterInput;
    size_t maxInputVisualWidth = listWidth - 1;
    std::string displayInput = inputText;
    size_t inputVisualWidth = TuiUtils::calculateUtf8VisualWidth(inputText);

    if (inputVisualWidth > maxInputVisualWidth) {
        displayInput = TuiUtils::trimToUtf8VisualWidth(inputText, maxInputVisualWidth);
        inputVisualWidth = TuiUtils::calculateUtf8VisualWidth(displayInput);
    }

    std::cout << displayInput;
    // 光标模拟
    Terminal::setBackground(Color::WHITE);
    std::cout << " "; // 光标块
    Terminal::resetColor();
    // 补齐空格到列表宽度
    if (inputVisualWidth < maxInputVisualWidth) {
        Terminal::setBackgroundRGB(45,63,118); // 深蓝色背景
        std::cout << std::string(maxInputVisualWidth - inputVisualWidth, ' ');
    }
    Terminal::resetColor();

    // 变量列表
    int visibleListHeight = listHeight - 1;
    if (filteredVariableList.empty()) {
        Terminal::setCursor(listStartRow + 1, listStartCol);
        Terminal::setForeground(Color::YELLOW);
        std::cout << "没有匹配的变量";
        Terminal::resetColor();
        // 清空剩余行
        for (int i = 2; i < visibleListHeight; ++i) {
            Terminal::setCursor(listStartRow + i, listStartCol);
            std::cout << std::string(listWidth, ' ');
        }
        return;
    }

    for (int i = 0; i < visibleListHeight; i++) {
        size_t itemIndex = scrollOffset + i;
        Terminal::setCursor(listStartRow + 1 + i, listStartCol);
        if (itemIndex >= filteredVariableList.size()) {
            std::cout << std::string(listWidth, ' ');
            continue;
        }
        const auto& item = filteredVariableList[itemIndex];

        bool isSelected = (itemIndex == currentSelection);
        if (isSelected) {
            Terminal::setBackground(Color::CYAN);
            Terminal::setForeground(Color::BLACK);
        } else {
            Terminal::setForeground(Color::WHITE);
        }

        // 构建变量名部分
        std::string namePart = item.name;

        // 构建类型部分（参数在前，类型在后，如 "4维 向量" 或 "4x4 矩阵"）
        std::string typePart;
        if (!item.sizeInfo.empty()) {
            typePart = item.sizeInfo + " " + item.typeString;
        } else {
            typePart = item.typeString;
        }

        // 使用视觉宽度计算
        size_t nameWidth = TuiUtils::calculateUtf8VisualWidth(namePart);
        size_t typeWidth = TuiUtils::calculateUtf8VisualWidth(typePart);
        int totalWidth = listWidth - 2; // 两侧各留1空格

        int remainingWidth = static_cast<int>(totalWidth) - static_cast<int>(nameWidth) - static_cast<int>(typeWidth);
        if (remainingWidth < 2) remainingWidth = 2; // 至少留2个空格分隔

        // 组合最终行 (变量名 + 空格 + 右对齐类型)
        std::string line = namePart + std::string(remainingWidth, ' ') + typePart;

        // 超长处理: 优先截断变量名
        while (TuiUtils::calculateUtf8VisualWidth(line) > static_cast<size_t>(totalWidth)) {
            if (namePart.length() > 1) {
                namePart = namePart.substr(0, namePart.length() - 1);
                if (namePart.length() > 3)
                    namePart.replace(namePart.length() - 3, 3, "...");
            } else {
                break;
            }
            nameWidth = TuiUtils::calculateUtf8VisualWidth(namePart);
            remainingWidth = static_cast<int>(totalWidth) - static_cast<int>(nameWidth) - static_cast<int>(typeWidth);
            if (remainingWidth < 2) remainingWidth = 2;
            line = namePart + std::string(remainingWidth, ' ') + typePart;
        }

        // 补齐到总宽度
        size_t lineVisualWidth = TuiUtils::calculateUtf8VisualWidth(line);
        if (lineVisualWidth < static_cast<size_t>(totalWidth)) {
            line += std::string(totalWidth - lineVisualWidth, ' ');
        } else if (lineVisualWidth > static_cast<size_t>(totalWidth)) {
            // 再次截断，防止极端情况
            line = TuiUtils::trimToUtf8VisualWidth(line, totalWidth);
        }

        std::cout << " " << line << " ";
        Terminal::resetColor();
    }
}

void EnhancedVariableViewer::drawPreviewWindow() {
    // 绘制预览窗口边框
    TuiUtils::drawBox(previewStartRow - 1, previewStartCol - 1, 
                      previewHeight + 2, previewWidth + 2, 
                      " 预览 ", Color::WHITE, Color::DEFAULT);
    
    if (filteredVariableList.empty() || currentSelection >= filteredVariableList.size()) {
        Terminal::setCursor(previewStartRow + 1, previewStartCol + 1);
        Terminal::setForeground(Color::YELLOW);
        std::cout << "无变量可预览";
        Terminal::resetColor();
        return;
    }
    const auto& selectedItem = filteredVariableList[currentSelection];
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
        // 使用视觉宽度截断
        if (TuiUtils::calculateUtf8VisualWidth(result_line) > static_cast<size_t>(previewWidth - 2)) {
            result_line = TuiUtils::trimToUtf8VisualWidth(result_line, previewWidth - 5) + "...";
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
        // 使用视觉宽度截断
        if (TuiUtils::calculateUtf8VisualWidth(sol_line) > static_cast<size_t>(previewWidth - 2)) {
            sol_line = TuiUtils::trimToUtf8VisualWidth(sol_line, previewWidth - 5) + "...";
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