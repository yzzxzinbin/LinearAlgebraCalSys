#pragma once
#include <vector>
#include <iostream>
#include "fraction.h"

// 前向声明
class ExpansionHistory;

class Matrix {
private:
    std::vector<std::vector<Fraction>> data;
    size_t rows, cols;

    // 辅助方法：获取去掉指定行和列的子矩阵
    Matrix getSubMatrix(size_t excludeRow, size_t excludeCol) const;
    
    // 将私有的递归方法重命名，以避免歧义
    Fraction determinantByExpansionRecursive(ExpansionHistory& history, int depth = 0) const;

public:
    Matrix(size_t r, size_t c);
    Matrix(const std::vector<std::vector<Fraction>>& d);

    size_t rowCount() const;
    size_t colCount() const;

    Fraction& at(size_t r, size_t c);
    const Fraction& at(size_t r, size_t c) const;

    void input(std::istream& is = std::cin);
    void print(std::ostream& os = std::cout) const;

    Matrix operator+(const Matrix& rhs) const;
    Matrix operator-(const Matrix& rhs) const;
    Matrix operator*(const Fraction& k) const;
    Matrix operator*(const Matrix& rhs) const; // 添加矩阵乘法运算符
    Matrix transpose() const;

    // 新增方法：计算代数余子式
    Fraction cofactor(size_t row, size_t col) const;
    
    // 新增方法：计算代数余子式矩阵
    Matrix cofactorMatrix() const;
    
    // 新增方法：计算伴随矩阵（代数余子式矩阵的转置）
    Matrix adjugate() const;
    
    // 新增方法：按行列式展开计算行列式（自动选择最优展开行/列）
    Fraction determinantByExpansion() const;
    Fraction determinantByExpansion(ExpansionHistory& history) const;
    
    // 新增辅助方法：查找零元素最多的行或列
    std::pair<bool, size_t> findOptimalExpansionIndex() const;

    // 新增方法：创建增广矩阵 [A|B]
    Matrix augment(const Matrix& B) const;
    
    // 新增方法：创建与当前矩阵同维度的单位矩阵
    static Matrix identity(size_t n);
    
    // 新增辅助方法：从增广矩阵中提取右侧部分
    Matrix extractRightPart(size_t colStart) const;

    // 新增：用于编辑器修改矩阵结构的方法
    void addRow(size_t rowIndex, const std::vector<Fraction>& rowData); // 在指定索引处插入行
    void addRow(size_t rowIndex); // 在指定索引处插入零行, rowIndex可以等于rowCount()表示末尾添加
    void addColumn(size_t colIndex, const std::vector<Fraction>& colData); // 在指定索引处插入列
    void addColumn(size_t colIndex); // 在指定索引处插入零列, colIndex可以等于colCount()表示末尾添加
    void deleteRow(size_t rowIndex);
    void deleteColumn(size_t colIndex);
    void resize(size_t newRows, size_t newCols); // 调整大小，用0填充新元素
};
