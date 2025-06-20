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
    return {true, coeffs};
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
    oss << "\033[36m① 增广矩阵 [set1 ┆ set2]：\033[0m\n";
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
    oss << "\033[36m② set1 ────────> set2：\033[0m\n";
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
        oss << "\n\033[33m系数矩阵 C (set2 = set1 * C):\033[0m\n";
        coeff12.print(oss);
        oss << "\n";
    } else {
        oss << "\033[33mset1 不能线性表示 set2\033[0m\n\n";
    }

    // 6. set2能否表示set1
    auto [can21, coeff21] = canRepresent(set2, set1);
    oss << "\033[36m③ set2 ────────> set1：\033[0m\n";
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
        oss << "\n\033[33m系数矩阵 D (set1 = set2 * D):\033[0m\n";
        coeff21.print(oss);
        oss << "\n";
    } else {
        oss << "\033[33mset2 不能线性表示 set1\033[0m\n\n";
    }

    if (can12 && can21) {
        oss << "\033[32mset1 与 set2 互相可线性表示，说明它们张成的子空间相同。\033[0m\n";
    }

    return Result::fromString(oss.str());
}
