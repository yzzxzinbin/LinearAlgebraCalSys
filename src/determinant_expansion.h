#pragma once
#include <string>
#include <vector>
#include <memory>
#include "matrix.h"

// 行列式展开的步骤类型
enum class ExpansionType {
    ROW_EXPANSION,        // 按行展开
    COLUMN_EXPANSION,     // 按列展开
    SUBMATRIX_CALCULATION,// 计算子矩阵行列式
    INITIAL_STATE,        // 初始状态
    RESULT_STATE          // 最终结果状态
};

// 行列式展开的单个计算步骤
class ExpansionStep {
private:
    ExpansionType type;
    std::string description;
    Matrix matrixState;          // 当前处理的矩阵
    int expansionIndex;          // 展开的行或列索引
    int elementIndex;            // 当前处理的元素索引
    Fraction element;            // 当前元素值
    Fraction cofactor;           // 当前元素的代数余子式
    Fraction termValue;          // 当前项的值 (element * cofactor)
    Fraction accumulatedValue;   // 累积和

public:
    // 构造函数
    ExpansionStep(
        ExpansionType type,
        const std::string& desc,
        const Matrix& matrix,
        int expIndex = -1,
        int elemIndex = -1,
        const Fraction& elem = Fraction(0),
        const Fraction& cof = Fraction(0),
        const Fraction& termVal = Fraction(0),
        const Fraction& accVal = Fraction(0)
    );

    // 获取步骤信息
    ExpansionType getType() const;
    std::string getDescription() const;
    const Matrix& getMatrixState() const;
    int getExpansionIndex() const;
    int getElementIndex() const;
    Fraction getElement() const;
    Fraction getCofactor() const;
    Fraction getTermValue() const;
    Fraction getAccumulatedValue() const;

    // 打印步骤信息
    void print(std::ostream& os = std::cout) const;
};

// 行列式展开的历史记录
class ExpansionHistory {
private:
    std::vector<ExpansionStep> steps;

public:
    // 添加步骤
    void addStep(const ExpansionStep& step);
    
    // 获取步骤数量
    size_t size() const;
    
    // 获取指定索引的步骤
    const ExpansionStep& getStep(size_t index) const;
    
    // 打印所有步骤
    void printAll(std::ostream& os = std::cout) const;
    
    // 打印指定索引的步骤
    void printStep(size_t index, std::ostream& os = std::cout) const;
    
    // 清空历史
    void clear();
};
