#include "convert_utils.h"
#include "../matrix.h"
#include "../vector.h"
#include "../fraction.h"
#include "../equationset.h"
#include "../result.h"
#include <stdexcept>
#include <sstream>
#include <vector>

namespace
{
    Matrix parse_matrix_from_block(std::stringstream &ss, bool parse_right_part, bool has_separator)
    {
        std::string line;
        std::vector<std::vector<Fraction>> matrix_data;

        while (std::getline(ss, line))
        {
            // Trim
            size_t first = line.find_first_not_of(" \t");
            if (std::string::npos == first)
                continue;
            size_t last = line.find_last_not_of(" \t\r\n");
            line = line.substr(first, (last - first + 1));

            if (line.rfind("|", 0) != 0)
            { // check for starting '|'
                if (!matrix_data.empty())
                    break; // Matrix block ended
                continue;
            }

            if (line.find("α") != std::string::npos || line.find("β") != std::string::npos)
            {
                break; // Variable name row
            }

            // remove start and end '|'
            line = line.substr(1, line.length() - 2);

            std::string part_to_parse;
            if (has_separator)
            {
                const std::string sep = " ┆ ";
                size_t sep_pos = line.find(sep);
                if (sep_pos == std::string::npos)
                    continue;

                if (parse_right_part)
                {
                    part_to_parse = line.substr(sep_pos + sep.length());
                }
                else
                {
                    part_to_parse = line.substr(0, sep_pos);
                }
            }
            else
            {
                part_to_parse = line;
            }

            std::stringstream line_ss(part_to_parse);
            std::string val_str;
            std::vector<Fraction> row_data;
            while (line_ss >> val_str)
            {
                try
                {
                    row_data.push_back(Fraction(val_str));
                }
                catch (...)
                { /* ignore parse errors */
                }
            }
            if (!row_data.empty())
            {
                matrix_data.push_back(row_data);
            }
        }

        if (matrix_data.empty())
            return Matrix(0, 0);

        size_t rows = matrix_data.size();
        size_t cols = matrix_data[0].size();
        for (const auto &row : matrix_data)
        {
            if (row.size() != cols)
                return Matrix(0, 0); // non-uniform
        }

        Matrix result(rows, cols);
        for (size_t i = 0; i < rows; ++i)
        {
            for (size_t j = 0; j < cols; ++j)
            {
                result.at(i, j) = matrix_data[i][j];
            }
        }
        return result;
    }
} // anonymous namespace

namespace ConvertUtils
{

    Variable convertVariable(const Variable &sourceVar, const std::string &targetTypeFlag)
    {
        VariableType sourceType = sourceVar.type;

        if (targetTypeFlag == "-v")
        { // Convert to Vector
            if (sourceType == VariableType::MATRIX)
            {
                const Matrix &m = sourceVar.matrixValue;
                if (m.colCount() != 1)
                {
                    throw std::runtime_error("无法转换为向量：矩阵必须只有一列。");
                }
                Vector v(m.rowCount());
                for (size_t i = 0; i < m.rowCount(); ++i)
                {
                    v.at(i) = m.at(i, 0);
                }
                return Variable(v);
            }
            else if (sourceType == VariableType::EQUATION_SOLUTION)
            {
                const EquationSolution &sol = sourceVar.equationSolutionValue;
                if (sol.getSolutionType() == SolutionType::NO_SOLUTION)
                {
                    throw std::runtime_error("无法转换为向量：方程组无解。");
                }
                const Matrix &p_sol = sol.getParticularSolution();
                if (p_sol.colCount() != 1)
                {
                    throw std::runtime_error("无法转换为向量：特解不是列向量。");
                }
                Vector v(p_sol.rowCount());
                for (size_t i = 0; i < p_sol.rowCount(); ++i)
                {
                    v.at(i) = p_sol.at(i, 0);
                }
                return Variable(v);
            }
            else
            {
                throw std::runtime_error("不支持从此变量类型转换为向量。");
            }
        }
        else if (targetTypeFlag == "-m" || targetTypeFlag == "-m1" || targetTypeFlag == "-m2")
        { // Convert to Matrix
            if (sourceType == VariableType::VECTOR)
            {
                const Vector &v = sourceVar.vectorValue;
                Matrix m(v.size(), 1);
                for (size_t i = 0; i < v.size(); ++i)
                {
                    m.at(i, 0) = v.at(i);
                }
                return Variable(m);
            }
            else if (sourceType == VariableType::EQUATION_SOLUTION)
            {
                const EquationSolution &sol = sourceVar.equationSolutionValue;
                if( sol.getSolutionType() == SolutionType::NO_SOLUTION )
                {
                    throw std::runtime_error("无法转换为基础解系矩阵：方程组无解。");
                }else if( sol.getSolutionType() == SolutionType::UNIQUE_SOLUTION )
                {
                    const Matrix &p_sol = sol.getParticularSolution();
                    return Variable(p_sol);
                }
                // 返回齐次解的基础解系
                return Variable(sol.getHomogeneousSolutions());
            }
            else if (sourceType == VariableType::RESULT)
            {
                const Result &res = sourceVar.resultValue;
                if (res.getType() != Result::Type::STRING)
                {
                    throw std::runtime_error("不支持从此结果类型转换为矩阵。");
                }
                const std::string &str = res.getString();

                const std::string header1 = "联合最简行阶梯形增广矩阵 [rref(set1) ┆ set2']:";
                const std::string header2 = "联合最简行阶梯形增广矩阵 [ set1' ┆ rref(set2) ]:";
                const std::string header3 = "系数矩阵 D (set1 = set2 * D):";

                Matrix m(0, 0);

                if (targetTypeFlag == "-m1") {
                    size_t pos = str.find(header1);
                    if (pos != std::string::npos) {
                        std::stringstream ss(str.substr(pos + header1.length()));
                        m = parse_matrix_from_block(ss, true, true);
                    } else {
                        throw std::runtime_error("无法从结果字符串中提取矩阵: 未找到 '" + header1 + "'");
                    }
                } else if (targetTypeFlag == "-m2") {
                    size_t pos = str.find(header2);
                    if (pos != std::string::npos) {
                        std::stringstream ss(str.substr(pos + header2.length()));
                        m = parse_matrix_from_block(ss, false, true);
                    } else {
                        throw std::runtime_error("无法从结果字符串中提取矩阵: 未找到 '" + header2 + "'");
                    }
                } else { // targetTypeFlag == "-m"
                    size_t pos1 = str.find(header1);
                    size_t pos2 = str.find(header2);

                    if (pos1 != std::string::npos && pos2 != std::string::npos) {
                        throw std::runtime_error("该结果为线性表示分析，其中包含多个可转换矩阵，请使用 -m1 或 -m2 指定。");
                    }

                    if (pos1 != std::string::npos) {
                        std::stringstream ss(str.substr(pos1 + header1.length()));
                        m = parse_matrix_from_block(ss, true, true);
                    } else if (pos2 != std::string::npos) {
                        std::stringstream ss(str.substr(pos2 + header2.length()));
                        m = parse_matrix_from_block(ss, false, true);
                    } else {
                        size_t pos3 = str.find(header3);
                        if (pos3 != std::string::npos) {
                            std::stringstream ss(str.substr(pos3 + header3.length()));
                            m = parse_matrix_from_block(ss, false, false); // parse left (whole thing), no separator
                        }
                    }
                }

                if (m.rowCount() > 0)
                {
                    return Variable(m);
                }
                else
                {
                    throw std::runtime_error("无法从结果字符串中提取系数矩阵。");
                }
            }
            else
            {
                throw std::runtime_error("不支持从此变量类型转换为矩阵。");
            }
        }
        else if (targetTypeFlag == "-f")
        { // Convert to Fraction
            if (sourceType == VariableType::MATRIX)
            {
                const Matrix &m = sourceVar.matrixValue;
                if (m.rowCount() == 1 && m.colCount() == 1)
                {
                    return Variable(m.at(0, 0));
                }
                else
                {
                    throw std::runtime_error("无法转换为分数：矩阵必须是 1x1。");
                }
            }
            else if (sourceType == VariableType::VECTOR)
            {
                const Vector &v = sourceVar.vectorValue;
                if (v.size() == 1)
                {
                    return Variable(v.at(0));
                }
                else
                {
                    throw std::runtime_error("无法转换为分数：向量必须只有 1 个元素。");
                }
            }
            else if (sourceType == VariableType::RESULT)
            {
                const Result &res = sourceVar.resultValue;
                if (res.getType() == Result::Type::SCALAR)
                {
                    const std::string &scalarStr = res.getScalar();
                    try
                    {
                        return Variable(Fraction(scalarStr));
                    }
                    catch (const std::exception &e)
                    {
                        throw std::runtime_error("无法将结果标量转换为分数: '" + scalarStr + "'");
                    }
                }
                else
                {
                    throw std::runtime_error("无法转换为分数：结果类型必须是标量。");
                }
            }
            else
            {
                throw std::runtime_error("不支持从此变量类型转换为分数。");
            }
        }
        else
        {
            throw std::runtime_error("无效的转换标志。请使用 -m, -m1, -m2, -v, 或 -f。");
        }
    }

} // namespace ConvertUtils
