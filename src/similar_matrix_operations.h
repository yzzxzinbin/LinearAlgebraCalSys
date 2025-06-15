#ifndef SIMILAR_MATRIX_OPERATIONS_H
#define SIMILAR_MATRIX_OPERATIONS_H

#include "matrix.h"
#include "vector.h"
#include "fraction.h"
#include <vector>

namespace SimilarMatrixOperations {

/**
 * @brief 创建一个对角矩阵。
 *
 * @param diagonalElements 对角线上的元素列表。
 * @return Matrix 对角矩阵。
 */
Matrix createDiagonalMatrix(const std::vector<Fraction>& diagonalElements);

} // namespace SimilarMatrixOperations

#endif // SIMILAR_MATRIX_OPERATIONS_H
