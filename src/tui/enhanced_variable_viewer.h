#pragma once
#include "../grammar/grammar_interpreter.h"
#include "tui_terminal.h"
#include <string>
#include <vector>
#include <utility> // For std::pair

// 获得变量类型字符串
extern std::string variableTypeString(const VariableType& type);

class EnhancedVariableViewer {
public:
    enum class ViewerResult {
        CONTINUE,
        EXIT
    };

    struct VariableItem {
        std::string name;
        VariableType type;
        std::string typeString;
        std::string sizeInfo; // 如 "3×2" 对于矩阵，"5维" 对于向量
    };

private:
    std::vector<VariableItem> variableList;
    std::vector<VariableItem> filteredVariableList; // 新增：过滤后的变量列表
    size_t currentSelection;
    size_t scrollOffset;

    std::string filterInput; // 新增：过滤输入框内容
    
    int terminalRows;
    int terminalCols;
    int listStartRow;
    int listStartCol;
    int listHeight;
    int listWidth;
    
    int previewStartRow;
    int previewStartCol;
    int previewHeight;
    int previewWidth;
    
    std::string statusMessage;
    const Interpreter& interpreter;

    // 绘制相关方法
    void drawLayout();
    void drawVariableList();
    void drawPreviewWindow();
    void drawPreviewContent(const Variable& var);
    void drawMatrixPreview(const Matrix& matrix);
    void drawVectorPreview(const Vector& vector);
    void drawFractionPreview(const Fraction& fraction);
    void drawResultPreview(const Result& result);
    void drawEquationSolutionPreview(const EquationSolution& solution);
    
    // 辅助方法
    void updateLayout();
    void updateScrolling();
    std::string getTypeString(VariableType type) const;
    std::string getSizeInfo(const Variable& var) const;
    void updateStatus(const std::string& msg);
    void clearScreen();

    void updateFilter(); // 新增：根据 filterInput 刷新 filteredVariableList

public:
    EnhancedVariableViewer(const Interpreter& interp, int termRows, int termCols);
    ~EnhancedVariableViewer();

    ViewerResult handleInput(int key);
    void draw();
    std::string getStatusMessage() const;
    void refreshVariableList(); // 刷新变量列表
    void updateDimensions(int termRows, int termCols);
};
