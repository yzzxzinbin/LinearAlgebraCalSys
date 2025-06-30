#include "polynomial_matrix.h"
#include "equation.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace Algebra {

PolynomialMatrix::PolynomialMatrix(size_t r, size_t c) : rows(r), cols(c) {
    if (r == 0 || c == 0) {
        data.clear();
        rows = 0;
        cols = 0;
        return;
    }
    data.resize(r, std::vector<Polynomial>(c));
}

// 从一个分数矩阵创建特征矩阵 (A - var*I)
PolynomialMatrix::PolynomialMatrix(const Matrix& m, const std::string& var_name) {
    if (m.rowCount() != m.colCount()) {
        throw std::invalid_argument("Matrix must be square to create characteristic matrix.");
    }
    rows = m.rowCount();
    cols = m.colCount();
    data.resize(rows, std::vector<Polynomial>(cols));

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            if (i == j) {
                // 对角线元素: A_ii - var
                Polynomial a_ii(Monomial(m.at(i, j)));
                Polynomial x_term(Monomial(Fraction(-1), var_name, Fraction(1)));
                data[i][j] = a_ii + x_term;
            } else {
                // 非对角线元素: A_ij
                data[i][j] = Polynomial(Monomial(m.at(i, j)));
            }
        }
    }
}

size_t PolynomialMatrix::rowCount() const { return rows; }
size_t PolynomialMatrix::colCount() const { return cols; }

Polynomial& PolynomialMatrix::at(size_t r, size_t c) {
    return data.at(r).at(c);
}

const Polynomial& PolynomialMatrix::at(size_t r, size_t c) const {
    return data.at(r).at(c);
}

std::string PolynomialMatrix::toString() const {
    std::stringstream ss;
    for (size_t i = 0; i < rows; ++i) {
        ss << "[ ";
        for (size_t j = 0; j < cols; ++j) {
            ss << "(" << data[i][j].toString() << ")";
            if (j < cols - 1) {
                ss << ", ";
            }
        }
        ss << " ]\n";
    }
    return ss.str();
}

PolynomialMatrix PolynomialMatrix::getSubMatrix(size_t excludeRow, size_t excludeCol) const {
    if (rows <= 1 || cols <= 1) {
        return PolynomialMatrix(0,0);
    }
    PolynomialMatrix sub(rows - 1, cols - 1);
    size_t r_sub = 0;
    for (size_t r = 0; r < rows; ++r) {
        if (r == excludeRow) continue;
        size_t c_sub = 0;
        for (size_t c = 0; c < cols; ++c) {
            if (c == excludeCol) continue;
            sub.at(r_sub, c_sub) = this->at(r, c);
            c_sub++;
        }
        r_sub++;
    }
    return sub;
}

// 使用代数余子式展开计算行列式
Polynomial PolynomialMatrix::determinant() const {
    if (rows != cols) {
        throw std::runtime_error("Determinant can only be calculated for square matrices.");
    }
    if (rows == 0) {
        return Polynomial("1"); // 约定
    }
    if (rows == 1) {
        return data[0][0];
    }
    if (rows == 2) {
        return (data[0][0] * data[1][1]) - (data[0][1] * data[1][0]);
    }

    Polynomial det_poly; // 初始化为 0
    Polynomial sign_poly(Monomial(Fraction(1))); // 符号从 +1 开始

    // 沿第一行展开
    for (size_t j = 0; j < cols; ++j) {
        Polynomial sub_det = getSubMatrix(0, j).determinant();
        Polynomial term = data[0][j] * sub_det;
        det_poly = det_poly + (sign_poly * term);
        sign_poly = sign_poly * Polynomial(Monomial(Fraction(-1))); // 翻转符号
    }

    return det_poly;
}

// 计算特征值的函数
std::string calculate_eigenvalues(const Matrix& m) {
    if (m.rowCount() != m.colCount()) {
        return "Error: Eigenvalues can only be calculated for square matrices.";
    }
    if (m.rowCount() == 0) {
        return "Eigenvalues: (none for empty matrix)";
    }

    // 1. 创建特征矩阵 A - x*I
    std::string var_name = "x";
    PolynomialMatrix char_matrix(m, var_name);

    // 2. 计算特征多项式 det(A - x*I)
    Polynomial char_poly = char_matrix.determinant();
    std::string char_eq_str = char_poly.toString() + " = 0";

    // 3. 求解特征方程
    Equation eq(char_eq_str);
    std::string solution = eq.solve();

    std::stringstream result_ss;
    result_ss << "Characteristic Equation: " << char_eq_str << "\n";
    result_ss << "Eigenvalues: " << solution;

    return result_ss.str();
}

} // namespace Algebra
