#include "vectorset_operation.h"
#include "equationset.h"
#include "matrix_operations.h"
#include "./utils/tui_utils.h"
#include <sstream>
#include <vector>
#include <string>

// 辅助：生成 α1, α2, ... 或 β1, β2, ...
static std::vector<std::string> genNames(const std::string& prefix, size_t n) {
    std::vector<std::string> names;
    for (size_t i = 1; i <= n; ++i) {
        names.push_back(prefix + std::to_string(i));
    }
    return names;
}

// 辅助：判断setA能否线性表示setB的所有列，返回系数矩阵和可表示性
static std::pair<bool, Matrix> canRepresent(const Matrix& setA, const Matrix& setB) {
    size_t n = setB.colCount();
    size_t m = setA.colCount();
    Matrix coeffs(m, n); // 每列为setB的一个向量的系数
    bool allUnique = true;
    // 原有解法
    for (size_t j = 0; j < n; ++j) {
        Matrix b_col(setB.rowCount(), 1);
        for (size_t i = 0; i < setB.rowCount(); ++i)
            b_col.at(i, 0) = setB.at(i, j);
        EquationSolver solver;
        EquationSolution sol = solver.solve(setA, b_col);
        if (!sol.hasSolution()) return {false, Matrix(0,0)};
        // 只支持唯一解
        if (!sol.hasUniqueSolution()) return {false, Matrix(0,0)};
        for (size_t k = 0; k < m; ++k)
            coeffs.at(k, j) = sol.getParticularSolution().at(k, 0);
    }
    // 联合行最简化简法
    Matrix setA_rref = setA;
    Matrix setB_rref = setB;
    // 做联合rref
    setB_rref = unionrref(setA, setB);
    setA_rref = setA;
    unionrref(setA_rref, setB); // setA_rref 变成rref
    // 检查setA_rref是否为单位阵
    bool isIdentity = true;
    size_t rows = setA_rref.rowCount(), cols = setA_rref.colCount();
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            if ((i == j && setA_rref.at(i, j) != Fraction(1)) || (i != j && setA_rref.at(i, j) != Fraction(0))) {
                isIdentity = false;
                break;
            }
        }
        if (!isIdentity) break;
    }
    if (isIdentity && setB_rref.rowCount() == m) {
        // setB_rref 的每一列就是线性表示系数
        // 检查与原有解法是否一致
        bool match = true;
        for (size_t j = 0; j < n && match; ++j) {
            for (size_t i = 0; i < m; ++i) {
                if (coeffs.at(i, j) != setB_rref.at(i, j)) {
                    match = false;
                    break;
                }
            }
        }
        if (match) {
            return {true, setB_rref}; // 优先返回联合rref结果
        } else {
            // 不一致，提示冲突
            // 可以在外层输出提示
            return {true, coeffs};
        }
    } else {
        // 不能用联合rref法，保留原有解法
        return {true, coeffs};
    }
}

// 新增：对A做最简行阶梯形变换并同步对B做相同行变换，返回B的变换结果
Matrix unionrref(const Matrix& A, const Matrix& B) {
    if (A.rowCount() != B.rowCount()) {
        throw std::invalid_argument("unionrref: 两个矩阵的行数必须相同");
    }
    Matrix a = A;
    Matrix b = B;
    size_t rowCount = a.rowCount();
    size_t colCount = a.colCount();
    size_t lead = 0;
    for (size_t r = 0; r < rowCount; ++r) {
        if (lead >= colCount) break;
        size_t i = r;
        while (i < rowCount && a.at(i, lead) == Fraction(0)) ++i;
        if (i == rowCount) {
            ++lead;
            --r;
            continue;
        }
        // 交换行
        if (i != r) {
            for (size_t j = 0; j < colCount; ++j) std::swap(a.at(r, j), a.at(i, j));
            for (size_t j = 0; j < b.colCount(); ++j) std::swap(b.at(r, j), b.at(i, j));
        }
        // 主元归一化
        Fraction pivot = a.at(r, lead);
        if (pivot != Fraction(1)) {
            for (size_t j = 0; j < colCount; ++j) a.at(r, j) = a.at(r, j) / pivot;
            for (size_t j = 0; j < b.colCount(); ++j) b.at(r, j) = b.at(r, j) / pivot;
        }
        // 消去下方
        for (size_t k = r + 1; k < rowCount; ++k) {
            Fraction factor = a.at(k, lead);
            if (factor != Fraction(0)) {
                for (size_t j = 0; j < colCount; ++j) a.at(k, j) = a.at(k, j) - a.at(r, j) * factor;
                for (size_t j = 0; j < b.colCount(); ++j) b.at(k, j) = b.at(k, j) - b.at(r, j) * factor;
            }
        }
        ++lead;
    }
    // 上消元
    for (int r = (int)rowCount - 1; r >= 0; --r) {
        int lead = -1;
        for (size_t j = 0; j < colCount; ++j) {
            if (a.at(r, j) == Fraction(1)) { lead = (int)j; break; }
        }
        if (lead == -1) continue;
        for (int i = r - 1; i >= 0; --i) {
            Fraction factor = a.at(i, lead);
            if (factor != Fraction(0)) {
                for (size_t j = 0; j < colCount; ++j) a.at(i, j) = a.at(i, j) - a.at(r, j) * factor;
                for (size_t j = 0; j < b.colCount(); ++j) b.at(i, j) = b.at(i, j) - b.at(r, j) * factor;
            }
        }
    }
    return b;
}

Result rep_vecset(const Matrix& set1, const Matrix& set2) {
    std::ostringstream oss;
    size_t n1 = set1.colCount(), n2 = set2.colCount();
    size_t rows = set1.rowCount();

    // 1. 计算每列最大宽度（视觉宽度），用于对齐
    std::vector<size_t> colWidths1(n1, 0), colWidths2(n2, 0);
    for (size_t j = 0; j < n1; ++j)
        for (size_t i = 0; i < rows; ++i) {
            std::ostringstream tmp; tmp << set1.at(i, j);
            size_t w = TuiUtils::calculateUtf8VisualWidth(tmp.str());
            colWidths1[j] = std::max(colWidths1[j], w);
        }
    for (size_t j = 0; j < n2; ++j)
        for (size_t i = 0; i < rows; ++i) {
            std::ostringstream tmp; tmp << set2.at(i, j);
            size_t w = TuiUtils::calculateUtf8VisualWidth(tmp.str());
            colWidths2[j] = std::max(colWidths2[j], w);
        }
    // 变量名宽度也要考虑（视觉宽度）
    auto alphaNames = genNames("α", n1);
    auto betaNames = genNames("β", n2);
    for (size_t j = 0; j < n1; ++j)
        colWidths1[j] = std::max(colWidths1[j], TuiUtils::calculateUtf8VisualWidth(alphaNames[j]));
    for (size_t j = 0; j < n2; ++j)
        colWidths2[j] = std::max(colWidths2[j], TuiUtils::calculateUtf8VisualWidth(betaNames[j]));

    // 2. 彩色编号和分隔线
    oss << "\033[36m① 增广矩阵 [set1 ┆ set2]:\033[0m\n";
    // 分隔线
    oss << std::string(2, ' ');
    for (size_t j = 0; j < n1; ++j) oss << std::string(colWidths1[j], '-') << " ";
    oss << "┆ ";
    for (size_t j = 0; j < n2; ++j) oss << std::string(colWidths2[j], '-') << " ";
    oss << "\n";

    // 3. 打印矩阵内容
    for (size_t i = 0; i < rows; ++i) {
        oss << "| ";
        for (size_t j = 0; j < n1; ++j) {
            std::ostringstream tmp; tmp << set1.at(i, j);
            std::string s = tmp.str();
            size_t w = TuiUtils::calculateUtf8VisualWidth(s);
            oss << std::string(colWidths1[j] > w ? colWidths1[j] - w : 0, ' ') << s << " ";
        }
        oss << "┆ ";
        for (size_t j = 0; j < n2; ++j) {
            std::ostringstream tmp; tmp << set2.at(i, j);
            std::string s = tmp.str();
            size_t w = TuiUtils::calculateUtf8VisualWidth(s);
            oss << std::string(colWidths2[j] > w ? colWidths2[j] - w : 0, ' ') << s << " ";
        }
        oss << "|\n";
    }

    // 4. 打印变量名对齐（严格视觉宽度对齐）
    oss << "| ";
    for (size_t j = 0; j < n1; ++j) {
        std::string s = alphaNames[j];
        size_t w = TuiUtils::calculateUtf8VisualWidth(s);
        oss << std::string(colWidths1[j] > w ? colWidths1[j] - w : 0, ' ') << s << " ";
    }
    oss << "┆ ";
    for (size_t j = 0; j < n2; ++j) {
        std::string s = betaNames[j];
        size_t w = TuiUtils::calculateUtf8VisualWidth(s);
        oss << std::string(colWidths2[j] > w ? colWidths2[j] - w : 0, ' ') << s << " ";
    }
    oss << "|\n";

    // 分隔线
    oss << std::string(2, ' ');
    for (size_t j = 0; j < n1; ++j) oss << std::string(colWidths1[j], '-') << " ";
    oss << "┆ ";
    for (size_t j = 0; j < n2; ++j) oss << std::string(colWidths2[j], '-') << " ";
    oss << "\n\n";

    // 5. set1能否表示set2
    auto [can12, coeff12] = canRepresent(set1, set2);
    // 新增：联合rref增广矩阵输出
    Matrix set1_rref = MatrixOperations::toReducedRowEchelonForm(set1);
    Matrix set2_rref = unionrref(set1, set2); // set2_rref已同步行变换

    oss << "\033[36m② set1 ────────> set2:\033[0m\n";
    if (can12) {
        oss << "\033[33mset1 可以线性表示 set2:\033[0m\n";
        for (size_t j = 0; j < n2; ++j) {
            oss << betaNames[j] << " = ";
            bool first = true;
            for (size_t k = 0; k < n1; ++k) {
                const auto& coef = coeff12.at(k, j);
                if (coef == Fraction(0)) continue;
                if (!first) oss << " + ";
                oss << coef << "·" << alphaNames[k];
                first = false;
            }
            if (first) oss << "0";
            oss << "\n";
        }
        // 输出联合rref增广矩阵
        oss << "\n\033[33m联合最简行阶梯形增广矩阵 [rref(set1) ┆ set2']:\033[0m\n";
        // 计算宽度
        std::vector<size_t> colW1(n1, 0), colW2(n2, 0);
        for (size_t j = 0; j < n1; ++j) {
            for (size_t i = 0; i < set1_rref.rowCount(); ++i) {
                std::ostringstream tmp; tmp << set1_rref.at(i, j);
                size_t w = TuiUtils::calculateUtf8VisualWidth(tmp.str());
                colW1[j] = std::max(colW1[j], w);
            }
            // 变量名宽度也要考虑
            colW1[j] = std::max(colW1[j], TuiUtils::calculateUtf8VisualWidth(alphaNames[j]));
        }
        for (size_t j = 0; j < n2; ++j) {
            for (size_t i = 0; i < set2_rref.rowCount(); ++i) {
                std::ostringstream tmp; tmp << set2_rref.at(i, j);
                size_t w = TuiUtils::calculateUtf8VisualWidth(tmp.str());
                colW2[j] = std::max(colW2[j], w);
            }
            // 变量名宽度也要考虑
            colW2[j] = std::max(colW2[j], TuiUtils::calculateUtf8VisualWidth(betaNames[j]));
        }
        // 分隔线
        oss << std::string(2, ' ');
        for (size_t j = 0; j < n1; ++j) oss << std::string(colW1[j], '-') << " ";
        oss << "┆ ";
        for (size_t j = 0; j < n2; ++j) oss << std::string(colW2[j], '-') << " ";
        oss << "\n";
        // 内容
        for (size_t i = 0; i < set1_rref.rowCount(); ++i) {
            oss << "| ";
            for (size_t j = 0; j < n1; ++j) {
                std::ostringstream tmp; tmp << set1_rref.at(i, j);
                std::string s = tmp.str();
                size_t w = TuiUtils::calculateUtf8VisualWidth(s);
                oss << std::string(colW1[j] > w ? colW1[j] - w : 0, ' ') << s << " ";
            }
            oss << "┆ ";
            for (size_t j = 0; j < n2; ++j) {
                std::ostringstream tmp; tmp << set2_rref.at(i, j);
                std::string s = tmp.str();
                size_t w = TuiUtils::calculateUtf8VisualWidth(s);
                oss << std::string(colW2[j] > w ? colW2[j] - w : 0, ' ') << s << " ";
            }
            oss << "|\n";
        }
        // 变量名
        oss << "| ";
        for (size_t j = 0; j < n1; ++j) {
            std::string s = alphaNames[j];
            size_t w = TuiUtils::calculateUtf8VisualWidth(s);
            oss << std::string(colW1[j] > w ? colW1[j] - w : 0, ' ') << s << " ";
        }
        oss << "┆ ";
        for (size_t j = 0; j < n2; ++j) {
            std::string s = betaNames[j];
            size_t w = TuiUtils::calculateUtf8VisualWidth(s);
            oss << std::string(colW2[j] > w ? colW2[j] - w : 0, ' ') << s << " ";
        }
        oss << "|\n";
        // 分隔线
        oss << std::string(2, ' ');
        for (size_t j = 0; j < n1; ++j) oss << std::string(colW1[j], '-') << " ";
        oss << "┆ ";
        for (size_t j = 0; j < n2; ++j) oss << std::string(colW2[j], '-') << " ";
        oss << "\n";
    } else {
        oss << "\033[33mset1 不能线性表示 set2\033[0m\n\n";
    }

    // 6. set2能否表示set1
    auto [can21, coeff21] = canRepresent(set2, set1);
    // 新增：联合rref增广矩阵输出
    Matrix set1_rref2(0,0);
    set2_rref = MatrixOperations::toReducedRowEchelonForm(set2);
    set1_rref2 = unionrref(set2, set1);

    oss << "\033[36m③ set2 ────────> set1:\033[0m\n";
    if (can21) {
        oss << "\033[33mset2 可以线性表示 set1:\033[0m\n";
        for (size_t j = 0; j < n1; ++j) {
            oss << alphaNames[j] << " = ";
            bool first = true;
            for (size_t k = 0; k < n2; ++k) {
                const auto& coef = coeff21.at(k, j);
                if (coef == Fraction(0)) continue;
                if (!first) oss << " + ";
                oss << coef << "·" << betaNames[k];
                first = false;
            }
            if (first) oss << "0";
            oss << "\n";
        }
        // 输出联合rref增广矩阵，始终左A右B
        oss << "\n\033[33m联合最简行阶梯形增广矩阵 [ set1' ┆ rref(set2) ]:\033[0m\n";
        // 计算宽度
        std::vector<size_t> colW1b(n1, 0), colW2b(n2, 0);
        for (size_t j = 0; j < n1; ++j) {
            for (size_t i = 0; i < set1_rref2.rowCount(); ++i) {
                std::ostringstream tmp; tmp << set1_rref2.at(i, j);
                size_t w = TuiUtils::calculateUtf8VisualWidth(tmp.str());
                colW1b[j] = std::max(colW1b[j], w);
            }
            // 变量名宽度也要考虑
            colW1b[j] = std::max(colW1b[j], TuiUtils::calculateUtf8VisualWidth(alphaNames[j]));
        }
        for (size_t j = 0; j < n2; ++j) {
            for (size_t i = 0; i < set2_rref.rowCount(); ++i) {
                std::ostringstream tmp; tmp << set2_rref.at(i, j);
                size_t w = TuiUtils::calculateUtf8VisualWidth(tmp.str());
                colW2b[j] = std::max(colW2b[j], w);
            }
            // 变量名宽度也要考虑
            colW2b[j] = std::max(colW2b[j], TuiUtils::calculateUtf8VisualWidth(betaNames[j]));
        }
        // 分隔线
        oss << std::string(2, ' ');
        for (size_t j = 0; j < n1; ++j) oss << std::string(colW1b[j], '-') << " ";
        oss << "┆ ";
        for (size_t j = 0; j < n2; ++j) oss << std::string(colW2b[j], '-') << " ";
        oss << "\n";
        // 内容
        for (size_t i = 0; i < set1_rref2.rowCount(); ++i) {
            oss << "| ";
            for (size_t j = 0; j < n1; ++j) {
                std::ostringstream tmp; tmp << set1_rref2.at(i, j);
                std::string s = tmp.str();
                size_t w = TuiUtils::calculateUtf8VisualWidth(s);
                oss << std::string(colW1b[j] > w ? colW1b[j] - w : 0, ' ') << s << " ";
            }
            oss << "┆ ";
            for (size_t j = 0; j < n2; ++j) {
                std::ostringstream tmp; tmp << set2_rref.at(i, j);
                std::string s = tmp.str();
                size_t w = TuiUtils::calculateUtf8VisualWidth(s);
                oss << std::string(colW2b[j] > w ? colW2b[j] - w : 0, ' ') << s << " ";
            }
            oss << "|\n";
        }
        // 变量名
        oss << "| ";
        for (size_t j = 0; j < n1; ++j) {
            std::string s = alphaNames[j];
            size_t w = TuiUtils::calculateUtf8VisualWidth(s);
            oss << std::string(colW1b[j] > w ? colW1b[j] - w : 0, ' ') << s << " ";
        }
        oss << "┆ ";
        for (size_t j = 0; j < n2; ++j) {
            std::string s = betaNames[j];
            size_t w = TuiUtils::calculateUtf8VisualWidth(s);
            oss << std::string(colW2b[j] > w ? colW2b[j] - w : 0, ' ') << s << " ";
        }
        oss << "|\n";
        // 分隔线
        oss << std::string(2, ' ');
        for (size_t j = 0; j < n1; ++j) oss << std::string(colW1b[j], '-') << " ";
        oss << "┆ ";
        for (size_t j = 0; j < n2; ++j) oss << std::string(colW2b[j], '-') << " ";
        oss << "\n";
    } else {
        oss << "\033[33mset2 不能线性表示 set1\033[0m\n\n";
    }

    if (can12 && can21) {
        oss << "\033[32mset1 与 set2 互相可线性表示，说明它们张成的子空间相同。\033[0m\n";
    }

    return Result::fromString(oss.str());
}
