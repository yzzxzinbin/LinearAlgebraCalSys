#include "similar_matrix_operations.h"

namespace SimilarMatrixOperations {

Matrix createDiagonalMatrix(const std::vector<Fraction>& diagonalElements) {
    size_t n = diagonalElements.size();
    if (n == 0) {
        return Matrix(0, 0); // 或者抛出异常，取决于设计决策
    }
    Matrix diagMatrix(n, n); // 创建一个 n x n 的零矩阵
    for (size_t i = 0; i < n; ++i) {
        diagMatrix.at(i, i) = diagonalElements[i];
    }
    return diagMatrix;
}

} // namespace SimilarMatrixOperations
