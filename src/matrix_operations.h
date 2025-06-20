#pragma once
#include "matrix.h"
#include "operation_step.h"
#include "determinant_expansion.h"

class MatrixOperations {
public:
    // 初等行变换方法 - 返回新矩阵
    static Matrix swapRows(const Matrix& mat, size_t row1, size_t row2);
    static Matrix scaleRow(const Matrix& mat, size_t row, const Fraction& scalar);
    static Matrix addScaledRow(const Matrix& mat, size_t targetRow, size_t sourceRow, const Fraction& scalar);
    
    // 带操作历史记录的初等行变换方法
    static void swapRows(Matrix& mat, size_t row1, size_t row2, OperationHistory& history);
    static void scaleRow(Matrix& mat, size_t row, const Fraction& scalar, OperationHistory& history);
    static void addScaledRow(Matrix& mat, size_t targetRow, size_t sourceRow, const Fraction& scalar, OperationHistory& history);
    
    // 化简为行阶梯形（高斯消元法）
    static Matrix toRowEchelonForm(const Matrix& mat);
    static void toRowEchelonForm(Matrix& mat, OperationHistory& history);
    
    // 化简为最简行阶梯形（高斯-若尔当消元法）
    static Matrix toReducedRowEchelonForm(const Matrix& mat);
    static void toReducedRowEchelonForm(Matrix& mat, OperationHistory& history);
    
    // 计算矩阵的秩
    static int rank(const Matrix& mat);
    
    // 计算方阵的行列式
    static Fraction determinant(const Matrix& mat);
    static Fraction determinant(const Matrix& mat, OperationHistory& history);
    
    // 新增：计算代数余子式矩阵
    static Matrix cofactorMatrix(const Matrix& mat);
    
    // 新增：计算伴随矩阵
    static Matrix adjugate(const Matrix& mat);
    
    // 新增：按行列展开计算行列式
    static Fraction determinantByExpansion(const Matrix& mat);
    static Fraction determinantByExpansion(const Matrix& mat, ExpansionHistory& history);
    
    // 新增：计算逆矩阵 (伴随矩阵法)
    static Matrix inverse(const Matrix& mat);
    static Matrix inverse(const Matrix& mat, OperationHistory& history);
    
    // 新增：计算逆矩阵 (高斯-若尔当消元法)
    static Matrix inverseGaussJordan(const Matrix& mat);
    static Matrix inverseGaussJordan(const Matrix& mat, OperationHistory& history);
};
