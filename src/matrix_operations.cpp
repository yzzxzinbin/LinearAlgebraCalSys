#include "matrix_operations.h"
#include <sstream>

// 实现初等行变换 - 返回新矩阵
Matrix MatrixOperations::swapRows(const Matrix& mat, size_t row1, size_t row2) {
    if (row1 >= mat.rowCount() || row2 >= mat.rowCount()) {
        throw std::out_of_range("Row index out of range in swapRows");
    }
    
    Matrix result = mat;
    for (size_t j = 0; j < mat.colCount(); ++j) {
        Fraction temp = result.at(row1, j);
        result.at(row1, j) = result.at(row2, j);
        result.at(row2, j) = temp;
    }
    return result;
}

Matrix MatrixOperations::scaleRow(const Matrix& mat, size_t row, const Fraction& scalar) {
    if (row >= mat.rowCount()) {
        throw std::out_of_range("Row index out of range in scaleRow");
    }
    if (scalar == Fraction(0)) {
        throw std::invalid_argument("Scalar cannot be zero in scaleRow");
    }
    
    Matrix result = mat;
    for (size_t j = 0; j < mat.colCount(); ++j) {
        result.at(row, j) = result.at(row, j) * scalar;
    }
    return result;
}

Matrix MatrixOperations::addScaledRow(const Matrix& mat, size_t targetRow, size_t sourceRow, const Fraction& scalar) {
    if (targetRow >= mat.rowCount() || sourceRow >= mat.rowCount()) {
        throw std::out_of_range("Row index out of range in addScaledRow");
    }
    
    Matrix result = mat;
    for (size_t j = 0; j < mat.colCount(); ++j) {
        result.at(targetRow, j) = result.at(targetRow, j) + result.at(sourceRow, j) * scalar;
    }
    return result;
}

// 带操作历史记录的初等行变换实现
void MatrixOperations::swapRows(Matrix& mat, size_t row1, size_t row2, OperationHistory& history) {
    if (row1 >= mat.rowCount() || row2 >= mat.rowCount()) {
        throw std::out_of_range("Row index out of range in swapRows");
    }
    
    for (size_t j = 0; j < mat.colCount(); ++j) {
        Fraction temp = mat.at(row1, j);
        mat.at(row1, j) = mat.at(row2, j);
        mat.at(row2, j) = temp;
    }
    
    std::stringstream ss;
    ss << "交换第 " << (row1 + 1) << " 行和第 " << (row2 + 1) << " 行";
    
    history.addStep(OperationStep(
        OperationType::SWAP_ROWS,
        ss.str(),
        mat,
        row1, row2
    ));
}

void MatrixOperations::scaleRow(Matrix& mat, size_t row, const Fraction& scalar, OperationHistory& history) {
    if (row >= mat.rowCount()) {
        throw std::out_of_range("Row index out of range in scaleRow");
    }
    if (scalar == Fraction(0)) {
        throw std::invalid_argument("Scalar cannot be zero in scaleRow");
    }
    
    for (size_t j = 0; j < mat.colCount(); ++j) {
        mat.at(row, j) = mat.at(row, j) * scalar;
    }
    
    std::stringstream ss;
    ss << "将第 " << (row + 1) << " 行乘以 " << scalar;
    
    history.addStep(OperationStep(
        OperationType::SCALE_ROW,
        ss.str(),
        mat,
        row, -1, scalar
    ));
}

void MatrixOperations::addScaledRow(Matrix& mat, size_t targetRow, size_t sourceRow, const Fraction& scalar, OperationHistory& history) {
    if (targetRow >= mat.rowCount() || sourceRow >= mat.rowCount()) {
        throw std::out_of_range("Row index out of range in addScaledRow");
    }
    
    for (size_t j = 0; j < mat.colCount(); ++j) {
        mat.at(targetRow, j) = mat.at(targetRow, j) + mat.at(sourceRow, j) * scalar;
    }
    
    std::stringstream ss;
    ss << "将第 " << (sourceRow + 1) << " 行乘以 " << scalar << " 加到第 " << (targetRow + 1) << " 行";
    
    history.addStep(OperationStep(
        OperationType::ADD_SCALED_ROW,
        ss.str(),
        mat,
        targetRow, sourceRow, scalar
    ));
}

// 化简为行阶梯形（高斯消元法）
Matrix MatrixOperations::toRowEchelonForm(const Matrix& mat) {
    Matrix result = mat;
    OperationHistory dummy; // 不记录历史
    toRowEchelonForm(result, dummy);
    return result;
}

void MatrixOperations::toRowEchelonForm(Matrix& mat, OperationHistory& history) {
    // 记录初始状态
    history.addStep(OperationStep(
        OperationType::INITIAL_STATE,
        "初始矩阵:",
        mat
    ));
    
    size_t lead = 0;
    size_t rowCount = mat.rowCount();
    size_t colCount = mat.colCount();
    
    for (size_t r = 0; r < rowCount; ++r) {
        if (lead >= colCount) {
            break;
        }
        
        size_t i = r;
        
        // 找到当前列中第一个非零元素的行
        while (i < rowCount && mat.at(i, lead) == Fraction(0)) {
            ++i;
        }
        
        if (i == rowCount) {
            // 当前列全为0，处理下一列
            ++lead;
            --r; // 重新处理当前行
            continue;
        }
        
        // 如果主元不在当前行，交换行
        if (i != r) {
            swapRows(mat, r, i, history);
        }
        
        // 将主元归一化
        Fraction pivot = mat.at(r, lead);
        if (pivot != Fraction(1)) {
            scaleRow(mat, r, Fraction(1) / pivot, history);
        }
        
        // 消去下方行的对应元素
        for (size_t i = r + 1; i < rowCount; ++i) {
            Fraction factor = mat.at(i, lead);
            if (factor != Fraction(0)) {
                addScaledRow(mat, i, r, -factor, history);
            }
        }
        
        ++lead;
    }
    
    // 记录最终状态
    history.addStep(OperationStep(
        OperationType::RESULT_STATE,
        "行阶梯形矩阵:",
        mat
    ));
}

// 化简为最简行阶梯形（高斯-若尔当消元法）
Matrix MatrixOperations::toReducedRowEchelonForm(const Matrix& mat) {
    Matrix result = mat;
    OperationHistory dummy; // 不记录历史
    toReducedRowEchelonForm(result, dummy);
    return result;
}

void MatrixOperations::toReducedRowEchelonForm(Matrix& mat, OperationHistory& history) {
    // 先转化为行阶梯形
    toRowEchelonForm(mat, history);
    
    // 从下往上消元，使每个主元上方的元素都为0
    size_t rowCount = mat.rowCount();
    size_t colCount = mat.colCount();
    
    for (int r = rowCount - 1; r >= 0; --r) {
        // 找到当前行的主元位置
        int lead = -1;
        for (size_t j = 0; j < colCount; ++j) {
            if (mat.at(r, j) == Fraction(1)) {
                lead = j;
                break;
            }
        }
        
        if (lead == -1) {
            continue; // 当前行没有主元
        }
        
        // 消去该主元列上方的所有元素
        for (int i = r - 1; i >= 0; --i) {
            Fraction factor = mat.at(i, lead);
            if (factor != Fraction(0)) {
                addScaledRow(mat, i, r, -factor, history);
            }
        }
    }
    
    // 记录最终状态
    history.addStep(OperationStep(
        OperationType::RESULT_STATE,
        "最简行阶梯形矩阵:",
        mat
    ));
}

// 计算矩阵的秩
int MatrixOperations::rank(const Matrix& mat) {
    Matrix rref = toReducedRowEchelonForm(mat);
    int rank = 0;
    
    // 计算非零行的数量
    for (size_t i = 0; i < rref.rowCount(); ++i) {
        bool isZeroRow = true;
        for (size_t j = 0; j < rref.colCount(); ++j) {
            if (rref.at(i, j) != Fraction(0)) {
                isZeroRow = false;
                break;
            }
        }
        if (!isZeroRow) {
            ++rank;
        }
    }
    
    return rank;
}

// 计算方阵的行列式
Fraction MatrixOperations::determinant(const Matrix& mat) {
    OperationHistory dummy; // 不记录历史
    return determinant(mat, dummy);
}

Fraction MatrixOperations::determinant(const Matrix& mat, OperationHistory& history) {
    if (mat.rowCount() != mat.colCount()) {
        throw std::invalid_argument("Determinant can only be calculated for square matrices");
    }
    
    size_t n = mat.rowCount();
    
    // 特殊情况处理
    if (n == 1) {
        Fraction result = mat.at(0, 0);
        history.addStep(OperationStep(
            OperationType::RESULT_STATE,
            "行列式为: " + std::to_string(result.getNumerator()) + 
            (result.getDenominator() != 1 ? "/" + std::to_string(result.getDenominator()) : ""),
            mat
        ));
        return result;
    }
    
    if (n == 2) {
        Fraction result = mat.at(0, 0) * mat.at(1, 1) - mat.at(0, 1) * mat.at(1, 0);
        std::stringstream ss;
        ss << "2x2行列式计算: " << mat.at(0, 0) << " * " << mat.at(1, 1) << " - " 
           << mat.at(0, 1) << " * " << mat.at(1, 0) << " = " << result;
        history.addStep(OperationStep(
            OperationType::RESULT_STATE,
            ss.str(),
            mat
        ));
        return result;
    }
    
    // 对于较大的矩阵，使用高斯消元法计算行列式
    Matrix copy = mat;
    
    std::stringstream initial;
    initial << "计算行列式的初始矩阵 (当前因子: 1)";
    history.addStep(OperationStep(
        OperationType::INITIAL_STATE,
        initial.str(),
        copy
    ));
    
    Fraction det(1); // 行列式的值
    int sign = 1;    // 交换行时的符号
    
    size_t lead = 0;
    for (size_t r = 0; r < n; ++r) {
        if (lead >= n) {
            break;
        }
        
        // 找到当前列最大绝对值的元素（为了数值稳定性）
        size_t maxRow = r;
        for (size_t i = r + 1; i < n; ++i) {
            // 这里我们没有真正比较绝对值，因为Fraction没有实现abs
            // 在实际应用中，应该比较绝对值
            if (copy.at(i, lead) > copy.at(maxRow, lead) || 
                copy.at(maxRow, lead) == Fraction(0)) {
                maxRow = i;
            }
        }
        
        if (copy.at(maxRow, lead) == Fraction(0)) {
            // 如果主元为0，则行列式为0
            std::stringstream ss;
            ss << "主元为0，行列式为0 (当前累积因子: " << (sign > 0 ? "" : "-") << det << ")";
            history.addStep(OperationStep(
                OperationType::RESULT_STATE,
                ss.str(),
                copy
            ));
            return Fraction(0);
        }
        
        // 交换行
        if (maxRow != r) {
            for (size_t j = 0; j < n; ++j) {
                Fraction temp = copy.at(r, j);
                copy.at(r, j) = copy.at(maxRow, j);
                copy.at(maxRow, j) = temp;
            }
            
            sign = -sign; // 交换行改变行列式符号
            
            std::stringstream ss;
            ss << "交换第 " << (r + 1) << " 行和第 " << (maxRow + 1) 
               << " 行 (符号变为: " << (sign > 0 ? "+" : "-") 
               << ", 当前累积因子: " << (sign > 0 ? "" : "-") << det << ")";
            history.addStep(OperationStep(
                OperationType::SWAP_ROWS,
                ss.str(),
                copy,
                r, maxRow
            ));
        }
        
        // 当前主元
        Fraction pivot = copy.at(r, lead);
        det = det * pivot; // 累乘主元得到行列式
        
        std::stringstream ss_pivot;
        ss_pivot << "主元 " << pivot << " 加入计算 (当前累积因子: " 
                 << (sign > 0 ? "" : "-") << det << ")";
        history.addStep(OperationStep(
            OperationType::RESULT_STATE,
            ss_pivot.str(),
            copy
        ));
        
        // 归一化当前行（仅用于显示，不影响行列式计算）
        for (size_t j = lead; j < n; ++j) {
            copy.at(r, j) = copy.at(r, j) / pivot;
        }
        
        std::stringstream ss;
        ss << "将第 " << (r + 1) << " 行除以主元 " << pivot;
        history.addStep(OperationStep(
            OperationType::SCALE_ROW,
            ss.str(),
            copy,
            r, -1, Fraction(1) / pivot
        ));
        
        // 消元
        for (size_t i = r + 1; i < n; ++i) {
            Fraction factor = copy.at(i, lead);
            for (size_t j = lead; j < n; ++j) {
                copy.at(i, j) = copy.at(i, j) - copy.at(r, j) * factor;
            }
            
            if (factor != Fraction(0)) {
                std::stringstream ss2;
                ss2 << "将第 " << (r + 1) << " 行乘以 " << -factor << " 加到第 " << (i + 1) << " 行";
                history.addStep(OperationStep(
                    OperationType::ADD_SCALED_ROW,
                    ss2.str(),
                    copy,
                    i, r, -factor
                ));
            }
        }
        
        ++lead;
    }
    
    Fraction finalDet = Fraction(sign) * det;
    std::stringstream final;
    final << "行列式计算完成，值为: " << finalDet 
          << " (" << (sign > 0 ? "" : "-") << det << ")";
    history.addStep(OperationStep(
        OperationType::RESULT_STATE,
        final.str(),
        copy
    ));
    
    return finalDet;
}

// 计算代数余子式矩阵
Matrix MatrixOperations::cofactorMatrix(const Matrix& mat) {
    return mat.cofactorMatrix();
}

// 计算伴随矩阵
Matrix MatrixOperations::adjugate(const Matrix& mat) {
    return mat.adjugate();
}

// 按行列展开计算行列式（不带历史记录）
Fraction MatrixOperations::determinantByExpansion(const Matrix& mat) {
    return mat.determinantByExpansion();
}

// 按行列展开计算行列式（带历史记录）
Fraction MatrixOperations::determinantByExpansion(const Matrix& mat, ExpansionHistory& history) {
    return mat.determinantByExpansion(history);
}
