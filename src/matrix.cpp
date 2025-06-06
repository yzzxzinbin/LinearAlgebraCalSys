#include "matrix.h"
#include "determinant_expansion.h"
#include <iomanip>
#include <algorithm>
#include <sstream>

Matrix::Matrix(size_t r, size_t c) : rows(r), cols(c), data(r, std::vector<Fraction>(c)) {}

Matrix::Matrix(const std::vector<std::vector<Fraction>>& d)
    : rows(d.size()), cols(d.empty() ? 0 : d[0].size()), data(d) {}

size_t Matrix::rowCount() const { return rows; }
size_t Matrix::colCount() const { return cols; }

Fraction& Matrix::at(size_t r, size_t c) { return data[r][c]; }
const Fraction& Matrix::at(size_t r, size_t c) const { return data[r][c]; }

void Matrix::input(std::istream& is) {
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j) {
            long long num, den = 1;
            char ch;
            is >> num;
            if (is.peek() == '/') {
                is >> ch >> den;
            }
            data[i][j] = Fraction(num, den);
        }
}

void Matrix::print(std::ostream& os) const {
    const int field_width = 8; // Define field width for each element

    for (size_t i = 0; i < rows; ++i) {
        os << "| ";
        for (size_t j = 0; j < cols; ++j) {
            std::ostringstream oss;
            oss << data[i][j];
            std::string s = oss.str();

            int len = s.length();
            int padding_total = field_width - len;

            if (padding_total < 0) { // String is wider than field
                os << s; 
            } else {
                int padding_left = padding_total / 2;
                int padding_right = padding_total - padding_left;

                for (int k = 0; k < padding_left; ++k) {
                    os << ' ';
                }
                os << s;
                for (int k = 0; k < padding_right; ++k) {
                    os << ' ';
                }
            }
            os << " "; // Separator space after the element
        }
        os << "|\n";
    }
}

Matrix Matrix::operator+(const Matrix& rhs) const {
    Matrix res(rows, cols);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[i][j] = data[i][j] + rhs.data[i][j];
    return res;
}

Matrix Matrix::operator-(const Matrix& rhs) const {
    Matrix res(rows, cols);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[i][j] = data[i][j] - rhs.data[i][j];
    return res;
}

Matrix Matrix::operator*(const Fraction& k) const {
    Matrix res(rows, cols);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[i][j] = data[i][j] * k;
    return res;
}

Matrix Matrix::operator*(const Matrix& rhs) const {
    if (cols != rhs.rows) {
        throw std::invalid_argument("Matrix multiplication error: dimensions mismatch.");
    }
    
    Matrix result(rows, rhs.cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < rhs.cols; ++j) {
            Fraction sum;
            for (size_t k = 0; k < cols; ++k) {
                sum += data[i][k] * rhs.data[k][j];
            }
            result.data[i][j] = sum;
        }
    }
    return result;
}

Matrix Matrix::transpose() const {
    Matrix res(cols, rows);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[j][i] = data[i][j];
    return res;
}

// 获取去掉指定行和列的子矩阵
Matrix Matrix::getSubMatrix(size_t excludeRow, size_t excludeCol) const {
    if (excludeRow >= rows || excludeCol >= cols) {
        throw std::out_of_range("Row or column index out of range in getSubMatrix");
    }
    
    Matrix result(rows - 1, cols - 1);
    size_t r = 0;
    for (size_t i = 0; i < rows; ++i) {
        if (i == excludeRow) continue;
        
        size_t c = 0;
        for (size_t j = 0; j < cols; ++j) {
            if (j == excludeCol) continue;
            result.at(r, c) = data[i][j];
            ++c;
        }
        ++r;
    }
    return result;
}

// 计算代数余子式
Fraction Matrix::cofactor(size_t row, size_t col) const {
    if (row >= rows || col >= cols) {
        throw std::out_of_range("Row or column index out of range in cofactor");
    }
    
    // 计算代数因子 (-1)^(i+j)
    int sign = ((row + col) % 2 == 0) ? 1 : -1;
    
    // 如果是1x1矩阵，直接返回元素值乘以代数因子
    if (rows == 1 && cols == 1) {
        return Fraction(sign) * data[0][0];
    }
    
    // 计算余子式（子矩阵的行列式）
    Matrix subMatrix = getSubMatrix(row, col);
    
    // 对于小矩阵，我们可以直接计算行列式
    Fraction det;
    if (subMatrix.rows == 1) {
        det = subMatrix.data[0][0];
    } else if (subMatrix.rows == 2) {
        det = subMatrix.data[0][0] * subMatrix.data[1][1] - 
              subMatrix.data[0][1] * subMatrix.data[1][0];
    } else {
        // 对于较大的矩阵，使用现有的行列式计算方法
        det = subMatrix.determinantByExpansion();
    }
    
    return Fraction(sign) * det;
}

// 计算代数余子式矩阵
Matrix Matrix::cofactorMatrix() const {
    if (rows != cols) {
        throw std::invalid_argument("Cofactor matrix can only be calculated for square matrices");
    }
    
    Matrix result(rows, cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            result.at(i, j) = cofactor(i, j);
        }
    }
    return result;
}

// 计算伴随矩阵（代数余子式矩阵的转置）
Matrix Matrix::adjugate() const {
    return cofactorMatrix().transpose();
}

// 查找零元素最多的行或列
std::pair<bool, size_t> Matrix::findOptimalExpansionIndex() const {
    if (rows != cols) {
        throw std::invalid_argument("Determinant can only be calculated for square matrices");
    }
    
    // 统计每行零元素数量
    std::vector<int> rowZeros(rows, 0);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            if (data[i][j] == Fraction(0)) {
                rowZeros[i]++;
            }
        }
    }
    
    // 统计每列零元素数量
    std::vector<int> colZeros(cols, 0);
    for (size_t j = 0; j < cols; ++j) {
        for (size_t i = 0; i < rows; ++i) {
            if (data[i][j] == Fraction(0)) {
                colZeros[j]++;
            }
        }
    }
    
    // 找出零元素最多的行
    auto maxRowIt = std::max_element(rowZeros.begin(), rowZeros.end());
    size_t maxRowIndex = std::distance(rowZeros.begin(), maxRowIt);
    int maxRowZeros = *maxRowIt;
    
    // 找出零元素最多的列
    auto maxColIt = std::max_element(colZeros.begin(), colZeros.end());
    size_t maxColIndex = std::distance(colZeros.begin(), maxColIt);
    int maxColZeros = *maxColIt;
    
    // 返回行或列展开的选择（true表示按行展开，false表示按列展开）
    // 以及对应的行或列索引
    if (maxRowZeros >= maxColZeros) {
        return {true, maxRowIndex};
    } else {
        return {false, maxColIndex};
    }
}

// 更改函数名称，与头文件定义保持一致
Fraction Matrix::determinantByExpansionRecursive(ExpansionHistory& history, int depth) const {
    // 实现递归计算行列式的方法
    // 这里应包含原来的递归实现代码
    // 但出于简洁考虑，此处使用按行列式展开计算的实现
    
    if (rows != cols) {
        throw std::invalid_argument("Determinant can only be calculated for square matrices");
    }
    
    // 递归的基本情况 - 1x1 矩阵
    if (rows == 1) {
        return data[0][0];
    }
    
    // 递归的基本情况 - 2x2 矩阵
    if (rows == 2) {
        return data[0][0] * data[1][1] - data[0][1] * data[1][0];
    }
    
    // 选择最优展开行/列
    auto [expandByRow, expandIndex] = findOptimalExpansionIndex();
    
    Fraction result;
    
    // 按行展开
    if (expandByRow) {
        for (size_t j = 0; j < cols; ++j) {
            Fraction element = data[expandIndex][j];
            
            // 如果元素为0，跳过（提高效率）
            if (element == Fraction(0)) {
                continue;
            }
            
            // 计算代数余子式
            int sign = ((expandIndex + j) % 2 == 0) ? 1 : -1;
            Matrix subMatrix = getSubMatrix(expandIndex, j);
            
            // 递归计算子矩阵的行列式
            Fraction subDet = subMatrix.determinantByExpansionRecursive(history, depth + 1);
            result += Fraction(sign) * element * subDet;
        }
    }
    // 按列展开
    else {
        for (size_t i = 0; i < rows; ++i) {
            Fraction element = data[i][expandIndex];
            
            // 如果元素为0，跳过（提高效率）
            if (element == Fraction(0)) {
                continue;
            }
            
            // 计算代数余子式
            int sign = ((i + expandIndex) % 2 == 0) ? 1 : -1;
            Matrix subMatrix = getSubMatrix(i, expandIndex);
            
            // 递归计算子矩阵的行列式
            Fraction subDet = subMatrix.determinantByExpansionRecursive(history, depth + 1);
            result += Fraction(sign) * element * subDet;
        }
    }
    
    return result;
}

// 按行列式展开计算行列式（不带历史记录）
Fraction Matrix::determinantByExpansion() const {
    if (rows != cols) {
        throw std::invalid_argument("Determinant can only be calculated for square matrices");
    }
    
    ExpansionHistory dummy;
    return determinantByExpansion(dummy);
}

// 按行列式展开计算行列式（带历史记录）
Fraction Matrix::determinantByExpansion(ExpansionHistory& history) const {
    if (rows != cols) {
        throw std::invalid_argument("Determinant can only be calculated for square matrices");
    }
    
    // 记录初始状态
    std::stringstream ss;
    ss << "计算 " << rows << "x" << cols << " 矩阵行列式 (按行列展开)";
    history.addStep(ExpansionStep(
        ExpansionType::INITIAL_STATE,
        ss.str(),
        *this
    ));
    
    // 特殊情况处理
    if (rows == 1) {
        Fraction result = data[0][0];
        history.addStep(ExpansionStep(
            ExpansionType::RESULT_STATE,
            "1x1矩阵行列式 = " + std::to_string(result.getNumerator()) + 
            (result.getDenominator() != 1 ? "/" + std::to_string(result.getDenominator()) : ""),
            *this,
            0, 0, result, Fraction(1), result, result
        ));
        return result;
    }
    
    if (rows == 2) {
        Fraction result = data[0][0] * data[1][1] - data[0][1] * data[1][0];
        std::stringstream ss2;
        ss2 << "2x2行列式 = " << data[0][0] << " * " << data[1][1] << " - " 
           << data[0][1] << " * " << data[1][0] << " = " << result;
        history.addStep(ExpansionStep(
            ExpansionType::RESULT_STATE,
            ss2.str(),
            *this
        ));
        return result;
    }
    
    // 选择最优展开行/列
    auto [expandByRow, expandIndex] = findOptimalExpansionIndex();
    
    std::stringstream ssExpand;
    ssExpand << "选择" << (expandByRow ? "行" : "列") << " " << (expandIndex + 1) 
             << " 进行展开 (含有最多的零元素)";
    history.addStep(ExpansionStep(
        expandByRow ? ExpansionType::ROW_EXPANSION : ExpansionType::COLUMN_EXPANSION,
        ssExpand.str(),
        *this,
        expandIndex
    ));
    
    Fraction result;
    
    // 按行展开
    if (expandByRow) {
        for (size_t j = 0; j < cols; ++j) {
            Fraction element = data[expandIndex][j];
            
            // 如果元素为0，跳过（提高效率）
            if (element == Fraction(0)) {
                std::stringstream ssZero;
                ssZero << "跳过 [" << (expandIndex + 1) << "," << (j + 1) << "] = 0 (不影响结果)";
                history.addStep(ExpansionStep(
                    ExpansionType::ROW_EXPANSION,
                    ssZero.str(),
                    *this,
                    expandIndex, j, element, Fraction(0), Fraction(0), result
                ));
                continue;
            }
            
            // 计算代数余子式
            int sign = ((expandIndex + j) % 2 == 0) ? 1 : -1;
            Matrix subMatrix = getSubMatrix(expandIndex, j);
            
            std::stringstream ssSubMat;
            ssSubMat << "计算元素 [" << (expandIndex + 1) << "," << (j + 1) << "] = " << element 
                     << " 的子矩阵行列式:";
            history.addStep(ExpansionStep(
                ExpansionType::SUBMATRIX_CALCULATION,
                ssSubMat.str(),
                subMatrix,
                expandIndex, j, element
            ));
            
            // 递归计算子矩阵的行列式 - 使用重命名后的递归方法
            Fraction subDet = subMatrix.determinantByExpansionRecursive(history);
            Fraction cofactorValue = Fraction(sign) * subDet;
            Fraction termValue = element * cofactorValue;
            result += termValue;
            
            std::stringstream ssTerm;
            ssTerm << "项 " << (j + 1) << ": " << element << " * " << cofactorValue << " = " << termValue 
                   << ", 累积和 = " << result;
            history.addStep(ExpansionStep(
                ExpansionType::ROW_EXPANSION,
                ssTerm.str(),
                *this,
                expandIndex, j, element, cofactorValue, termValue, result
            ));
        }
    }
    // 按列展开
    else {
        for (size_t i = 0; i < rows; ++i) {
            Fraction element = data[i][expandIndex];
            
            // 如果元素为0，跳过（提高效率）
            if (element == Fraction(0)) {
                std::stringstream ssZero;
                ssZero << "跳过 [" << (i + 1) << "," << (expandIndex + 1) << "] = 0 (不影响结果)";
                history.addStep(ExpansionStep(
                    ExpansionType::COLUMN_EXPANSION,
                    ssZero.str(),
                    *this,
                    expandIndex, i, element, Fraction(0), Fraction(0), result
                ));
                continue;
            }
            
            // 计算代数余子式
            int sign = ((i + expandIndex) % 2 == 0) ? 1 : -1;
            Matrix subMatrix = getSubMatrix(i, expandIndex);
            
            std::stringstream ssSubMat;
            ssSubMat << "计算元素 [" << (i + 1) << "," << (expandIndex + 1) << "] = " << element 
                     << " 的子矩阵行列式:";
            history.addStep(ExpansionStep(
                ExpansionType::SUBMATRIX_CALCULATION,
                ssSubMat.str(),
                subMatrix,
                expandIndex, i, element
            ));
            
            // 递归计算子矩阵的行列式
            Fraction subDet = subMatrix.determinantByExpansionRecursive(history);
            Fraction cofactorValue = Fraction(sign) * subDet;
            Fraction termValue = element * cofactorValue;
            result += termValue;
            
            std::stringstream ssTerm;
            ssTerm << "项 " << (i + 1) << ": " << element << " * " << cofactorValue << " = " << termValue 
                   << ", 累积和 = " << result;
            history.addStep(ExpansionStep(
                ExpansionType::COLUMN_EXPANSION,
                ssTerm.str(),
                *this,
                expandIndex, i, element, cofactorValue, termValue, result
            ));
        }
    }
    
    // 记录最终结果
    std::stringstream ssFinal;
    ssFinal << "行列式计算完成，值为: " << result;
    history.addStep(ExpansionStep(
        ExpansionType::RESULT_STATE,
        ssFinal.str(),
        *this,
        -1, -1, Fraction(0), Fraction(0), Fraction(0), result
    ));
    
    return result;
}

// 创建增广矩阵 [A|B]
Matrix Matrix::augment(const Matrix& B) const {
    if (rows != B.rows) {
        throw std::invalid_argument("Cannot augment matrices with different row counts");
    }
    
    Matrix result(rows, cols + B.cols);
    
    // 复制当前矩阵 A 到结果的左侧
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            result.at(i, j) = at(i, j);
        }
    }
    
    // 复制矩阵 B 到结果的右侧
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < B.cols; ++j) {
            result.at(i, cols + j) = B.at(i, j);
        }
    }
    
    return result;
}

// 创建与当前矩阵同维度的单位矩阵
Matrix Matrix::identity(size_t n) {
    Matrix result(n, n);
    
    for (size_t i = 0; i < n; ++i) {
        result.at(i, i) = Fraction(1);
    }
    
    return result;
}

// 从增广矩阵中提取右侧部分
Matrix Matrix::extractRightPart(size_t colStart) const {
    if (colStart >= cols) {
        throw std::invalid_argument("Starting column index out of range");
    }
    
    Matrix result(rows, cols - colStart);
    
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols - colStart; ++j) {
            result.at(i, j) = at(i, colStart + j);
        }
    }
    
    return result;
}
