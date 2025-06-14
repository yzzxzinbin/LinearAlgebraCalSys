#include "enhanced_matrix_editor.h"
#include <algorithm> // For std::min, std::max, std::sort
#include <iostream>  // For std::cout (used by Terminal)
#include "../grammar/grammar_interpreter.h"

EnhancedMatrixEditor::EnhancedMatrixEditor(Variable& var, std::string varName, bool isMat, int termRows, int termCols)
    : workingCopy(var), variableName(std::move(varName)), isMatrix(isMat),
      cursorRow(0), cursorCol(0), cellInputActive(false),
      cursorOnAddRow(false), cursorOnAddCol(false),
      terminalRows(termRows), terminalCols(termCols) {
    
        std::iostream::sync_with_stdio(false); // 禁用同步以提高性能
    // 修改：如果矩阵/向量为空，默认选中添加行按钮
    if ((isMatrix && workingCopy.matrixValue.rowCount() == 0 && workingCopy.matrixValue.colCount() == 0) ||
        (!isMatrix && workingCopy.vectorValue.size() == 0)) {
        cursorOnAddRow = true;
    }
    
    updateStatus("编辑模式：方向键移动，直接输入数字修改，CTRL+回车选择，ESC保存退出");
}

EnhancedMatrixEditor::~EnhancedMatrixEditor() {
    std::iostream::sync_with_stdio(true); // 恢复同步
}

void EnhancedMatrixEditor::updateStatus(const std::string& msg) {
    statusMessage = msg;
}

EnhancedMatrixEditor::EditorResult EnhancedMatrixEditor::handleInput(int key) {
    size_t numRows = isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size();
    size_t numCols = isMatrix ? workingCopy.matrixValue.colCount() : 1;
    
    // 检测是否为空矩阵/向量
    bool isEmpty = (numRows == 0 || (isMatrix && numCols == 0));

    // 添加调试代码，帮助识别键码
    if (key > 1000) {
        std::ostringstream debug_msg;
        debug_msg << "检测到特殊键码: " << key;
        updateStatus(debug_msg.str());
    }

    // 首先处理导航和通用键，无论是否在输入模式
    switch (key) {
        case KEY_UP:
            if (!sharedInputBuffer.empty() && !cellInputActive) {
                // 如果有未应用的输入且不是批量编辑模式，先应用输入
                applySharedInputBuffer();
                sharedInputBuffer.clear();
            }
            
            // 特殊处理空矩阵情况
            if (isEmpty && isMatrix) {
                // 在空矩阵的情况下，允许在添加行和添加列按钮之间切换
                if (cursorOnAddCol) {
                    cursorOnAddCol = false;
                    cursorOnAddRow = true;
                }
                return EditorResult::CONTINUE;
            }
            
            // 原有逻辑
            if (cursorOnAddRow) {
                cursorOnAddRow = false;
                cursorRow = numRows > 0 ? numRows - 1 : 0;
            } else if (cursorRow > 0) {
                cursorRow--;
            }
            cursorOnAddCol = false;
            return EditorResult::CONTINUE;
            
        case KEY_DOWN:
            if (!sharedInputBuffer.empty() && !cellInputActive) {
                // 如果有未应用的输入且不是批量编辑模式，先应用输入
                applySharedInputBuffer();
                sharedInputBuffer.clear();
            }
            
            // 特殊处理空矩阵情况
            if (isEmpty && isMatrix) {
                // 在空矩阵的情况下，允许在添加行和添加列按钮之间切换
                if (cursorOnAddRow) {
                    cursorOnAddRow = false;
                    cursorOnAddCol = true;
                }
                return EditorResult::CONTINUE;
            }
            
            // 原有逻辑
            if (cursorRow < (numRows > 0 ? numRows - 1 : 0)) {
                cursorRow++;
            } else if (!cursorOnAddRow && numRows > 0) {
                cursorOnAddRow = true;
                cursorCol = 0;
            }
            cursorOnAddCol = false;
            return EditorResult::CONTINUE;
            
        case KEY_LEFT:
            if (!sharedInputBuffer.empty() && !cellInputActive) {
                // 如果有未应用的输入且不是批量编辑模式，先应用输入
                applySharedInputBuffer();
                sharedInputBuffer.clear();
            }
            
            // 特殊处理空矩阵情况
            if (isEmpty && isMatrix) {
                // 在空矩阵的情况下，允许在添加行和添加列按钮之间切换
                if (cursorOnAddCol) {
                    cursorOnAddCol = false;
                    cursorOnAddRow = true;
                }
                return EditorResult::CONTINUE;
            }
            
            // 原有逻辑
            if (cursorOnAddCol) {
                cursorOnAddCol = false;
                cursorCol = numCols > 0 ? numCols - 1 : 0;
            } else if (cursorCol > 0) {
                cursorCol--;
            }
            cursorOnAddRow = false;
            return EditorResult::CONTINUE;
            
        case KEY_RIGHT:
            if (!sharedInputBuffer.empty() && !cellInputActive) {
                // 如果有未应用的输入且不是批量编辑模式，先应用输入
                applySharedInputBuffer();
                sharedInputBuffer.clear();
            }
            
            // 特殊处理空矩阵情况
            if (isEmpty && isMatrix) {
                // 在空矩阵的情况下，允许在添加行和添加列按钮之间切换
                if (cursorOnAddRow) {
                    cursorOnAddRow = false;
                    cursorOnAddCol = true;
                }
                return EditorResult::CONTINUE;
            }
            
            // 原有逻辑
            if (cursorCol < (numCols > 0 ? numCols - 1 : 0)) {
                cursorCol++;
            } else if (isMatrix && !cursorOnAddCol && numCols > 0) {
                cursorOnAddCol = true;
                cursorRow = 0;
            }
            cursorOnAddRow = false;
            return EditorResult::CONTINUE;
    }

    // 处理其他特殊键
    switch (key) {
        case KEY_ESCAPE:
            // 如果有未应用的输入，先应用
            if (!sharedInputBuffer.empty()) {
                applySharedInputBuffer();
            }
            return EditorResult::EXIT_SAVE;

        case KEY_ENTER:
            if (cursorOnAddRow) {
                addRowAction();
                draw(true); // 结构变化，需要完整重绘
                return EditorResult::UPDATE_STATUS;
            } else if (cursorOnAddCol) {
                addColumnAction();
                draw(true); // 结构变化，需要完整重绘
                
                // 添加调试信息，帮助确认状态
                std::ostringstream debug_msg;
                debug_msg << "矩阵大小更新为 " << workingCopy.matrixValue.rowCount() << "x" 
                         << workingCopy.matrixValue.colCount() << ", 光标位置: [" 
                         << cursorRow << "," << cursorCol << "]";
                updateStatus(debug_msg.str());
                
                return EditorResult::UPDATE_STATUS;
            } else if (!selectedCells.empty()) {
                // 如果有批量选择且有输入，应用输入
                if (!sharedInputBuffer.empty() && cellInputActive) {
                    applySharedInputBuffer();
                    // 修改：应用输入后清除所有选择，而不只是关闭输入模式
                    clearSelectionsAndInput(); // 这会同时清除选择和输入缓冲区，并设置cellInputActive=false
                    updateStatus("批量单元格值已设置并清除选择");
                    return EditorResult::UPDATE_STATUS;
                }
                // 如果没有输入，则只清除选择
                clearSelectionsAndInput();
                updateStatus("选择已清除");
                return EditorResult::UPDATE_STATUS;
            } else if (!sharedInputBuffer.empty()) {
                // 在没有选择的情况下，Enter键会应用当前输入
                applySharedInputBuffer();
                sharedInputBuffer.clear();
                updateStatus("单元格值已设置");
                return EditorResult::UPDATE_STATUS;
            }
            return EditorResult::CONTINUE;

        case KEY_CTRL_ENTER:
            if (!cursorOnAddRow && !cursorOnAddCol && numRows > cursorRow && (isMatrix ? numCols > cursorCol : true)) {
                // 如果有未应用的输入且不是批量编辑模式，先应用
                if (!sharedInputBuffer.empty() && !cellInputActive) {
                    applySharedInputBuffer();
                    sharedInputBuffer.clear();
                }
                
                toggleCellSelection(cursorRow, cursorCol);
                // 批量选择模式下才需要设置cellInputActive标志
                if (!selectedCells.empty()) {
                    cellInputActive = true;
                    updateStatus("已选择单元格，输入值将应用到所有选定单元格...");
                } else {
                    cellInputActive = false;
                    updateStatus("选择已清除");
                }
            }
            return EditorResult::UPDATE_STATUS;
        
        case KEY_CTRL_UP:
            if (isMatrix && !cursorOnAddRow && !cursorOnAddCol && numCols > cursorCol) {
                // 先应用任何未提交的输入
                if (!sharedInputBuffer.empty()) {
                    applySharedInputBuffer();
                    sharedInputBuffer.clear();
                }
                
                selectColumn(cursorCol);
                updateStatus("列选择已" + std::string(isFullColumnSelected(cursorCol) ? "启用" : "取消"));
                return EditorResult::UPDATE_STATUS;
            }
            return EditorResult::CONTINUE;
            
        case KEY_CTRL_DOWN:
            if (isMatrix && !cursorOnAddRow && !cursorOnAddCol && numCols > cursorCol) {
                // 先应用任何未提交的输入
                if (!sharedInputBuffer.empty()) {
                    applySharedInputBuffer();
                    sharedInputBuffer.clear();
                }
                
                selectColumn(cursorCol);
                updateStatus("列选择已" + std::string(isFullColumnSelected(cursorCol) ? "启用" : "取消"));
                return EditorResult::UPDATE_STATUS;
            }
            return EditorResult::CONTINUE;
            
        case KEY_CTRL_LEFT:
            if (!cursorOnAddRow && !cursorOnAddCol && numRows > cursorRow) {
                // 先应用任何未提交的输入
                if (!sharedInputBuffer.empty()) {
                    applySharedInputBuffer();
                    sharedInputBuffer.clear();
                }
                
                selectRow(cursorRow);
                updateStatus("行选择已" + std::string(isFullRowSelected(cursorRow) ? "启用" : "取消"));
                return EditorResult::UPDATE_STATUS;
            }
            return EditorResult::CONTINUE;
            
        case KEY_CTRL_RIGHT:
            if (!cursorOnAddRow && !cursorOnAddCol && numRows > cursorRow) {
                // 先应用任何未提交的输入
                if (!sharedInputBuffer.empty()) {
                    applySharedInputBuffer();
                    sharedInputBuffer.clear();
                }
                
                selectRow(cursorRow);
                updateStatus("行选择已" + std::string(isFullRowSelected(cursorRow) ? "启用" : "取消"));
                return EditorResult::CONTINUE;
            }
            return EditorResult::CONTINUE;

        case KEY_DELETE:
            // 如果有未应用的输入，先丢弃
            if (!sharedInputBuffer.empty()) {
                sharedInputBuffer.clear();
            }
            
            deleteSelectedRowsAction();
            deleteSelectedColumnsAction();
            
            // 光标和选择状态可能需要在删除后调整
            if (cursorRow >= (isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size())) {
                cursorRow = (isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size());
                if (cursorRow > 0) cursorRow--; else cursorRow = 0;
            }
            if (cursorCol >= (isMatrix ? workingCopy.matrixValue.colCount() : 0)) {
                cursorCol = (isMatrix ? workingCopy.matrixValue.colCount() : 0);
                if (cursorCol > 0) cursorCol--; else cursorCol = 0;
            }
            clearSelectionsAndInput();
            updateStatus("删除操作已完成");
            draw(true); // 结构可能变化，需要完整重绘
            return EditorResult::UPDATE_STATUS;

        case KEY_BACKSPACE:
            if (!sharedInputBuffer.empty()) {
                sharedInputBuffer.pop_back();
                return EditorResult::CONTINUE;
            }
            return EditorResult::CONTINUE;

        case KEY_CTRL_A:
            // 全选所有单元格
            if (!cursorOnAddRow && !cursorOnAddCol && numRows > cursorRow && (isMatrix ? numCols > cursorCol : true)) {
                selectAllCells();
                return EditorResult::UPDATE_STATUS;
            }
            return EditorResult::CONTINUE;
    }

    // 处理数字和分数输入
    if ((key >= '0' && key <= '9') || key == '/' || key == '.' || key == '-' || key == ' ') {
        if (!cursorOnAddRow && !cursorOnAddCol && numRows > cursorRow && (isMatrix ? numCols > cursorCol : true)) {
            // 如果当前单元格为空且未在批量编辑模式下，先清空buffer
            if (sharedInputBuffer.empty() && !cellInputActive) {
                // 直接开始编辑
                sharedInputBuffer.clear();
            }
            
            if (sharedInputBuffer.length() < EDITOR_CELL_WIDTH * 2) {
                sharedInputBuffer += static_cast<char>(key);
            }
            return EditorResult::CONTINUE;
        }
    }

    return EditorResult::CONTINUE;
}

void EnhancedMatrixEditor::draw(bool fullRedraw) {
    int editorContentStartRow = 2;
    
    // 仅在需要完整重绘时清除整个编辑区域
    if (fullRedraw) {
        for (int i = editorContentStartRow - 1; i < terminalRows - 2; i++) {
            Terminal::setCursor(i, 0);
            std::cout << std::string(terminalCols, ' '); // 用空格填充整行
        }
    }

    // 绘制标题
    Terminal::setCursor(editorContentStartRow - 1, 0);
    Terminal::setForeground(Color::YELLOW);
    std::string title = std::string("正在编辑") + (isMatrix ? "矩阵 " : "向量 ") + variableName;
    std::cout << title << std::string(terminalCols > title.length() ? terminalCols - title.length() : 0, ' ') << std::endl;
    Terminal::resetColor();

    drawGrid();
    drawAddControls();
}

void EnhancedMatrixEditor::drawGrid() {
    int displayStartRow = 3; 
    size_t numRows = isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size();
    size_t numCols = isMatrix ? workingCopy.matrixValue.colCount() : 1;

    // 特殊处理：如果是矩阵且有行但没有列，仍然绘制行边框
    if (isMatrix && numRows > 0 && numCols == 0) {
        for (size_t r = 0; r < numRows; ++r) {
            if (displayStartRow + (int)r >= terminalRows - 2) break;
            Terminal::setCursor(displayStartRow + r, 1);
            Terminal::setForeground(Color::CYAN);
            std::cout << "| |"; // 显示空行
            Terminal::resetColor();
        }
        return;
    }

    for (size_t r = 0; r < numRows; ++r) {
        if (displayStartRow + (int)r >= terminalRows - 2) break; 
        Terminal::setCursor(displayStartRow + r, 1);
        if (isMatrix) std::cout << "| ";

        for (size_t c = 0; c < numCols; ++c) {
            bool isCursorCell = (r == cursorRow && c == cursorCol && !cursorOnAddRow && !cursorOnAddCol);
            bool isSelected = selectedCells.count({r, c});
            
            std::string cellDisplayString;
            if (isCursorCell && !sharedInputBuffer.empty() && !cellInputActive) {
                // 当前单元格有输入但未提交
                cellDisplayString = sharedInputBuffer + "_";
            } else if (isSelected && cellInputActive && !sharedInputBuffer.empty()) {
                // 选中的单元格正在批量编辑
                cellDisplayString = sharedInputBuffer + "_";
            } else {
                // 显示单元格的实际值
                Fraction val = isMatrix ? workingCopy.matrixValue.at(r, c) : workingCopy.vectorValue.at(r);
                std::ostringstream oss; oss << val; cellDisplayString = oss.str();
            }

            if (isCursorCell && !sharedInputBuffer.empty() && !cellInputActive) {
                Terminal::setBackground(Color::GREEN); 
                Terminal::setForeground(Color::BLACK);
            } else if (isSelected && cellInputActive && !sharedInputBuffer.empty()) {
                Terminal::setBackground(Color::GREEN); 
                Terminal::setForeground(Color::BLACK);
            } else if (isSelected) {
                Terminal::setBackground(Color::MAGENTA); 
                Terminal::setForeground(Color::WHITE);
            } else if (isCursorCell) {
                Terminal::setBackground(Color::WHITE); 
                Terminal::setForeground(Color::BLACK);
            } else {
                Terminal::setForeground(Color::CYAN);
            }

            if (cellDisplayString.length() > EDITOR_CELL_WIDTH) 
                std::cout << cellDisplayString.substr(0, EDITOR_CELL_WIDTH);
            else 
                std::cout << cellDisplayString << std::string(EDITOR_CELL_WIDTH - cellDisplayString.length(), ' ');
            
            Terminal::resetColor();
            if (isMatrix && c < numCols - 1) std::cout << " ";
        }
        if (isMatrix) { Terminal::setForeground(Color::CYAN); std::cout << " |"; Terminal::resetColor(); }
    }
}

void EnhancedMatrixEditor::drawAddControls() {
    int displayStartRow = 3;
    size_t numRows = isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size();
    size_t numCols = isMatrix ? workingCopy.matrixValue.colCount() : 1;
    
    // 更精确地判断空矩阵状态
    bool isReallyEmpty = (numRows == 0 && numCols == 0) || 
                        (isMatrix && (numRows == 0 || numCols == 0));

    // 绘制添加行按钮
    int addRowColPos = isReallyEmpty ? 4 : 
               (1 + (isMatrix ? 2 : 0) + (numCols * (EDITOR_CELL_WIDTH + (isMatrix ? 1 : 0)) - 1) / 2);
    
    if (displayStartRow + numRows < terminalRows - 3) {
        Terminal::setCursor(displayStartRow + numRows, addRowColPos);
        if (cursorOnAddRow) { 
            Terminal::setBackground(Color::GREEN); 
            Terminal::setForeground(Color::BLACK); 
        } else { 
            Terminal::setForeground(Color::YELLOW); 
        }
        std::cout << "+";
        Terminal::resetColor();
        
        // 在按钮旁边添加提示文本
        if (isReallyEmpty) {
            std::cout << " 添加行";
        }
    }

    // 对于矩阵，绘制添加列按钮
    if (isMatrix) {
        int addColRowPos = isReallyEmpty ? displayStartRow : 
                         (displayStartRow + (numRows > 0 ? numRows / 2 : 0));
        int addColColPos = isReallyEmpty ? 15 : 
                         (1 + (isMatrix ? 2 : 0) + numCols * (EDITOR_CELL_WIDTH + 1) + (isMatrix ? 2 : 0));

        if (addColColPos < terminalCols - 1 && addColRowPos < terminalRows - 3) {
            Terminal::setCursor(addColRowPos, addColColPos);
            if (cursorOnAddCol) {
                Terminal::setBackground(Color::GREEN);
                Terminal::setForeground(Color::BLACK);
            } else {
                Terminal::setForeground(Color::YELLOW);
            }
            std::cout << "+";
            Terminal::resetColor();
            
            // 在按钮旁边添加提示文本
            if (isReallyEmpty) {
                std::cout << " 添加列";
            }
        }
    }
}

void EnhancedMatrixEditor::applySharedInputBuffer() {
    if (sharedInputBuffer.empty()) return; // 没有输入时不执行任何操作

    Fraction f_val;
    try {
        if (sharedInputBuffer.find('/') != std::string::npos) {
            size_t slash_pos = sharedInputBuffer.find('/');
            long long num = std::stoll(sharedInputBuffer.substr(0, slash_pos));
            long long den = std::stoll(sharedInputBuffer.substr(slash_pos + 1));
            if (den == 0) throw std::invalid_argument("分母不能为零");
            f_val = Fraction(num, den);
        } else {
            f_val = Fraction(std::stoll(sharedInputBuffer));
        }
    } catch (const std::exception& e) {
        updateStatus("错误：无效的输入格式 - " + std::string(e.what()));
        return; 
    }

    if (!selectedCells.empty()) {
        // 批量应用到所有选中单元格
        for (const auto& cell_pos : selectedCells) {
            if (isMatrix) workingCopy.matrixValue.at(cell_pos.first, cell_pos.second) = f_val;
            else if (cell_pos.second == 0) workingCopy.vectorValue.at(cell_pos.first) = f_val;
        }
    } else {
        // 应用到当前光标所在单元格
        size_t numRows = isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size();
        size_t numCols = isMatrix ? workingCopy.matrixValue.colCount() : 1;
        if (cursorRow < numRows && cursorCol < numCols) {
            if (isMatrix) workingCopy.matrixValue.at(cursorRow, cursorCol) = f_val;
            else workingCopy.vectorValue.at(cursorRow) = f_val;
        }
    }
}

void EnhancedMatrixEditor::clearSelectionsAndInput() {
    selectedCells.clear();
    sharedInputBuffer.clear();
    cellInputActive = false;
}

void EnhancedMatrixEditor::toggleCellSelection(size_t r, size_t c) {
    std::pair<size_t, size_t> cell = {r, c};
    if (selectedCells.count(cell)) selectedCells.erase(cell);
    else selectedCells.insert(cell);
    updateStatus(selectedCells.count(cell) ? "单元格已选中" : "单元格已取消选中");
}

void EnhancedMatrixEditor::selectRow(size_t r) {
    size_t numCols = isMatrix ? workingCopy.matrixValue.colCount() : 1;
    if (numCols == 0 && isMatrix) return; // 无法选择0列矩阵的行

    bool currently_selected = isFullRowSelected(r);
    for (size_t c = 0; c < numCols; ++c) {
        if (currently_selected) selectedCells.erase({r, c});
        else selectedCells.insert({r, c});
    }

    // 修改：如果选择了行，自动激活输入模式；如果取消选择，只有在没有其他选中单元格时才关闭输入模式
    if (!currently_selected) {
        cellInputActive = true;
        sharedInputBuffer.clear();
        updateStatus("已选择行，可直接输入值应用到所有选定单元格");
    } else if (selectedCells.empty()) { // 只有在没有选中单元格时才关闭输入模式
        cellInputActive = false;
        sharedInputBuffer.clear();
        updateStatus("所有选择已取消");
    } else {
        updateStatus("行选择已取消，仍有" + std::to_string(selectedCells.size()) + "个单元格被选中");
    }
}

void EnhancedMatrixEditor::selectColumn(size_t c) {
    if (!isMatrix) return;
    size_t numRows = workingCopy.matrixValue.rowCount();
    if (numRows == 0) return; // 无法选择0行矩阵的列

    bool currently_selected = isFullColumnSelected(c);
    for (size_t r = 0; r < numRows; ++r) {
        if (currently_selected) selectedCells.erase({r, c});
        else selectedCells.insert({r, c});
    }

    // 修改：如果选择了列，自动激活输入模式；如果取消选择，只有在没有其他选中单元格时才关闭输入模式
    if (!currently_selected) {
        cellInputActive = true;
        sharedInputBuffer.clear();
        updateStatus("已选择列，可直接输入值应用到所有选定单元格");
    } else if (selectedCells.empty()) { // 只有在没有选中单元格时才关闭输入模式
        cellInputActive = false;
        sharedInputBuffer.clear();
        updateStatus("所有选择已取消");
    } else {
        updateStatus("列选择已取消，仍有" + std::to_string(selectedCells.size()) + "个单元格被选中");
    }
}

void EnhancedMatrixEditor::selectAllCells() {
    size_t numRows = isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size();
    size_t numCols = isMatrix ? workingCopy.matrixValue.colCount() : 1;
    
    if (numRows == 0 || (isMatrix && numCols == 0)) {
        updateStatus("矩阵/向量为空，无法选择");
        return; // 空矩阵或向量，无法选择
    }
    
    // 先检查是否已全选，如果是则取消全选
    bool allSelected = (selectedCells.size() == numRows * numCols);
    
    if (allSelected) {
        clearSelectionsAndInput();
        updateStatus("已取消全选");
    } else {
        // 选择所有单元格
        selectedCells.clear();
        for (size_t r = 0; r < numRows; ++r) {
            for (size_t c = 0; c < numCols; ++c) {
                selectedCells.insert({r, c});
            }
        }
        cellInputActive = true;
        updateStatus("已全选单元格，输入值将应用到所有单元格");
    }
}

void EnhancedMatrixEditor::addRowAction() {
    if (isMatrix) {
        // 如果是空矩阵（0行0列），添加行的同时也添加一列
        // 否则无法编辑添加的行
        if (workingCopy.matrixValue.rowCount() == 0 && workingCopy.matrixValue.colCount() == 0) {
            workingCopy.matrixValue.addRow(0);
            workingCopy.matrixValue.addColumn(0);
        } else {
            workingCopy.matrixValue.addRow(workingCopy.matrixValue.rowCount());
        }
    } else { 
        workingCopy.vectorValue.resize(workingCopy.vectorValue.size() + 1); 
    }
    cursorOnAddRow = false; 
    cursorRow = (isMatrix ? workingCopy.matrixValue.rowCount() : workingCopy.vectorValue.size()) - 1;
    cursorCol = 0;
    updateStatus("已添加新行");
    // 不在这里调用draw，由handleInput负责调用
}

void EnhancedMatrixEditor::addColumnAction() {
    if (!isMatrix) return;
    
    // 如果是空矩阵（0行0列），添加列的同时也添加一行
    // 否则没有行就无法显示添加的列
    if (workingCopy.matrixValue.rowCount() == 0 && workingCopy.matrixValue.colCount() == 0) {
        workingCopy.matrixValue.addRow(0);
        workingCopy.matrixValue.addColumn(0);
    } else {
        workingCopy.matrixValue.addColumn(workingCopy.matrixValue.colCount());
    }
    
    cursorOnAddCol = false;
    cursorOnAddRow = false;
    cursorCol = workingCopy.matrixValue.colCount() - 1;
    cursorRow = 0;
    
    // 在添加列后强制刷新UI状态
    updateStatus("已添加新列");
}

void EnhancedMatrixEditor::deleteSelectedRowsAction() {
    if (!isMatrix) return; 
    std::vector<size_t> rows_to_delete;
    for (size_t r = 0; r < workingCopy.matrixValue.rowCount(); ++r) {
        if (isFullRowSelected(r)) rows_to_delete.push_back(r);
    }
    std::sort(rows_to_delete.rbegin(), rows_to_delete.rend());
    for (size_t r_idx : rows_to_delete) {
        if (workingCopy.matrixValue.rowCount() > 0) { // Allow deleting to 0 rows
             workingCopy.matrixValue.deleteRow(r_idx);
        }
    }
    
    // 删除后，如果矩阵变为空，设置光标到添加行按钮
    if (workingCopy.matrixValue.rowCount() == 0) {
        cursorOnAddRow = true;
        cursorOnAddCol = false;
        cursorRow = 0;
        cursorCol = 0;
    }
    
    if (!rows_to_delete.empty()) {
        clearSelectionsAndInput();
        updateStatus(std::to_string(rows_to_delete.size()) + " 行已删除");
    }
}

void EnhancedMatrixEditor::deleteSelectedColumnsAction() {
    if (!isMatrix) return;
    std::vector<size_t> cols_to_delete;
    for (size_t c = 0; c < workingCopy.matrixValue.colCount(); ++c) {
        if (isFullColumnSelected(c)) cols_to_delete.push_back(c);
    }
    std::sort(cols_to_delete.rbegin(), cols_to_delete.rend());
    for (size_t c_idx : cols_to_delete) {
        if (workingCopy.matrixValue.colCount() > 0) { // Allow deleting to 0 columns
            workingCopy.matrixValue.deleteColumn(c_idx);
        }
    }
    
    // 删除后，如果矩阵变为空，设置光标到添加行按钮
    if (workingCopy.matrixValue.colCount() == 0) {
        cursorOnAddRow = true;
        cursorOnAddCol = false;
        cursorRow = 0;
        cursorCol = 0;
    }
    
    if (!cols_to_delete.empty()) {
        clearSelectionsAndInput();
        updateStatus(std::to_string(cols_to_delete.size()) + " 列已删除");
    }
}

bool EnhancedMatrixEditor::isFullRowSelected(size_t r) const {
    size_t numCols = isMatrix ? workingCopy.matrixValue.colCount() : 1;
    if (numCols == 0 && isMatrix) return false; // A 0-column row cannot be "fully selected" in a meaningful way for deletion
    if (numCols == 0 && !isMatrix) return selectedCells.count({r,0}); // Vector case

    for (size_t c = 0; c < numCols; ++c) {
        if (!selectedCells.count({r, c})) return false;
    }
    return numCols > 0 || (!isMatrix && selectedCells.count({r,0})); // True if all cols selected, or if vector element selected
}

bool EnhancedMatrixEditor::isFullColumnSelected(size_t c) const {
    if (!isMatrix) return false; 
    size_t numRows = workingCopy.matrixValue.rowCount();
    if (numRows == 0) return false; // A 0-row col cannot be "fully selected"

    for (size_t r = 0; r < numRows; ++r) {
        if (!selectedCells.count({r, c})) return false;
    }
    return numRows > 0;
}

Variable EnhancedMatrixEditor::getEditedVariableCopy() const {
    return workingCopy; // Return a copy
}
const std::string& EnhancedMatrixEditor::getVariableName() const {
    return variableName;
}
std::string EnhancedMatrixEditor::getStatusMessage() const {
    return statusMessage;
}
