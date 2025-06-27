#include "convert_utils.h"
#include "../matrix.h"
#include "../vector.h"
#include "../fraction.h"
#include "../equationset.h"
#include <stdexcept>

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
        else if (targetTypeFlag == "-m")
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
            else
            {
                throw std::runtime_error("不支持从此变量类型转换为分数。");
            }
        }
        else
        {
            throw std::runtime_error("无效的转换标志。请使用 -m, -v, 或 -f。");
        }
    }

} // namespace ConvertUtils
