#pragma once
#include <string>
#include <memory>
#include "matrix.h"

enum class OperationType {
    SWAP_ROWS,           // 交换两行
    SCALE_ROW,           // 行乘以常数
    ADD_SCALED_ROW,      // 一行加上另一行的倍数
    INITIAL_STATE,       // 初始状态
    RESULT_STATE         // 最终结果状态
};

class OperationStep {
private:
    OperationType type;
    std::string description;
    Matrix matrixState;  // 操作后的矩阵状态
    int row1, row2;      // 操作涉及的行索引
    Fraction scalar;     // 操作涉及的常数

public:
    // 构造函数
    OperationStep(OperationType type, const std::string& desc, const Matrix& matrix, 
                 int r1 = -1, int r2 = -1, const Fraction& scalar = Fraction(1));

    // 获取操作信息
    OperationType getType() const;
    std::string getDescription() const;
    const Matrix& getMatrixState() const;
    int getRow1() const;
    int getRow2() const;
    Fraction getScalar() const;

    // 打印操作步骤和矩阵状态
    void print(std::ostream& os = std::cout) const;
};

// 操作历史管理类
class OperationHistory {
private:
    std::vector<OperationStep> steps;

public:
    // 添加操作步骤
    void addStep(const OperationStep& step);
    
    // 获取步骤数量
    size_t size() const;
    
    // 获取指定索引的步骤
    const OperationStep& getStep(size_t index) const;
    
    // 打印所有步骤
    void printAll(std::ostream& os = std::cout) const;
    
    // 打印指定索引的步骤
    void printStep(size_t index, std::ostream& os = std::cout) const;
    
    // 清空历史
    void clear();
};
