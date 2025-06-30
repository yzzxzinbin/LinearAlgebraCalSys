#ifndef ALGEBRA_POLYNOMIAL_MATRIX_H
#define ALGEBRA_POLYNOMIAL_MATRIX_H

#include "polynomial.h"
#include "../matrix.h" // 用于转换
#include <vector>
#include <string>

namespace Algebra {

class PolynomialMatrix {
private:
    std::vector<std::vector<Polynomial>> data;
    size_t rows, cols;

public:
    PolynomialMatrix(size_t r, size_t c);
    // 从一个分数矩阵创建特征矩阵 (A - var*I)
    PolynomialMatrix(const Matrix& m, const std::string& var_name = "lambda");

    size_t rowCount() const;
    size_t colCount() const;

    Polynomial& at(size_t r, size_t c);
    const Polynomial& at(size_t r, size_t c) const;

    std::string toString() const;

    // 计算行列式，返回一个多项式
    Polynomial determinant() const;

private:
    // 行列式计算的辅助函数
    PolynomialMatrix getSubMatrix(size_t excludeRow, size_t excludeCol) const;
};

// 新增：计算特征值的函数
std::string calculate_eigenvalues(const Matrix& m);

} // namespace Algebra

#endif // ALGEBRA_POLYNOMIAL_MATRIX_H
