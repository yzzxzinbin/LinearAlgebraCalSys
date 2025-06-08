#pragma once
#include "../matrix.h"
#include "../vector.h" 
#include "../fraction.h"
#include "../grammar/grammar_interpreter.h" // 包含解释器头文件以获取 Variable 定义
#include "tui_terminal.h"
#include <string>
#include <vector>
#include <set>
#include <utility> // For std::pair
#include <sstream> // For std::ostringstream

class EnhancedMatrixEditor {
public:
    enum class EditorResult {
        CONTINUE,
        EXIT_SAVE,
        EXIT_DISCARD,
        UPDATE_STATUS 
    };

private:
    Variable workingCopy;      
    std::string variableName;
    bool isMatrix;

    size_t cursorRow;
    size_t cursorCol;

    std::set<std::pair<size_t, size_t>> selectedCells;
    std::string sharedInputBuffer; // 当前输入缓冲区，用于直接编辑和批量编辑

    bool cellInputActive;  // 仅用于批量选择模式

    bool cursorOnAddRow;    
    bool cursorOnAddCol;    

    int terminalRows;
    int terminalCols;
    std::string statusMessage;
    
    static const int EDITOR_CELL_WIDTH = 8;

    // 绘制相关方法
    void drawGrid();
    void drawAddControls();
    
    // 输入处理和动作方法
    void applySharedInputBuffer();
    void clearSelectionsAndInput();
    void toggleCellSelection(size_t r, size_t c);
    void selectRow(size_t r);
    void selectColumn(size_t c);
    void selectAllCells(); // 新增：全选所有单元格

    void addRowAction();
    void addColumnAction();
    void deleteSelectedRowsAction();
    void deleteSelectedColumnsAction();

    bool isFullRowSelected(size_t r) const;
    bool isFullColumnSelected(size_t c) const;
    void updateStatus(const std::string& msg);
    void clearEditArea(); // 添加一个辅助方法来清除整个编辑区域

public:
    EnhancedMatrixEditor(Variable& var, std::string varName, bool isMat, int termRows, int termCols);
    ~EnhancedMatrixEditor();

    EditorResult handleInput(int key);
    void draw(bool fullRedraw = false);
    
    Variable getEditedVariableCopy() const; // 返回编辑后的变量副本
    const std::string& getVariableName() const;
    std::string getStatusMessage() const;
};
