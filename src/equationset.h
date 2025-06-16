#pragma once
#include "matrix.h"
#include "vector.h"
#include "fraction.h"
#include "result.h"
#include "operation_step.h"
#include <vector>
#include <string>

// 方程组解的类型
enum class SolutionType {
    UNIQUE_SOLUTION,      // 唯一解
    INFINITE_SOLUTIONS,   // 无穷多解
    NO_SOLUTION,          // 无解
    UNDETERMINED          // 未确定
};

// 方程组分析结果
struct EquationSystemInfo {
    int coefficientRank;     // 系数矩阵的秩
    int augmentedRank;       // 增广矩阵的秩
    int numVariables;        // 变量个数
    int numEquations;        // 方程个数
    SolutionType solutionType;
    std::string description; // 解的描述
    
    EquationSystemInfo() : coefficientRank(0), augmentedRank(0), 
                          numVariables(0), numEquations(0), 
                          solutionType(SolutionType::UNDETERMINED) {}
};

// 方程组求解结果
class EquationSolution {
private:
    SolutionType type;
    Matrix particularSolution;    // 特解
    Matrix homogeneousSolutions;  // 齐次解的基础解系
    EquationSystemInfo systemInfo;
    std::string detailedDescription;
    Matrix initialAugmentedMatrix; // 新增：存储初始增广矩阵

public:
    EquationSolution();
    
    // 设置解的类型和信息
    void setSolutionType(SolutionType t);
    void setSystemInfo(const EquationSystemInfo& info);
    void setParticularSolution(const Matrix& solution);
    void setHomogeneousSolutions(const Matrix& solutions);
    void setDetailedDescription(const std::string& desc);
    void setInitialAugmentedMatrix(const Matrix& augMatrix); // 新增 setter
    
    // 获取解的信息
    SolutionType getSolutionType() const;
    const EquationSystemInfo& getSystemInfo() const;
    const Matrix& getParticularSolution() const;
    const Matrix& getHomogeneousSolutions() const;
    std::string getDetailedDescription() const;
    const Matrix& getInitialAugmentedMatrix() const; // 新增 getter
    
    // 检查是否有解
    bool hasSolution() const;
    bool hasUniqueSolution() const;
    bool hasInfiniteSolutions() const;
    
    // 输出解的信息
    void print(std::ostream& os = std::cout) const;

    // 新增：序列化和反序列化方法
    std::string serialize() const;
    static EquationSolution deserialize(const std::string& s);
};

// 方程组求解器
class EquationSolver {
public:
    // 求解线性方程组 Ax = b (非齐次)
    static EquationSolution solve(const Matrix& A, const Matrix& b);
    static EquationSolution solve(const Matrix& A, const Matrix& b, OperationHistory& history);
    // 新增：重载以接受 Vector b
    static EquationSolution solve(const Matrix& A, const Vector& b);
    static EquationSolution solve(const Matrix& A, const Vector& b, OperationHistory& history);
    
    // 求解齐次线性方程组 Ax = 0
    static EquationSolution solveHomogeneous(const Matrix& A);
    static EquationSolution solveHomogeneous(const Matrix& A, OperationHistory& history);
    
    // 分析方程组性质
    static EquationSystemInfo analyzeSystem(const Matrix& A, const Matrix& b);
    static EquationSystemInfo analyzeHomogeneousSystem(const Matrix& A);
    
private:
    // 内部辅助方法
    static Matrix findParticularSolution(const Matrix& rref, const Matrix& b_rref, 
                                        const std::vector<int>& pivotCols);
    static Matrix findHomogeneousSolutions(const Matrix& rref, 
                                         const std::vector<int>& pivotCols);
    static std::vector<int> findPivotColumns(const Matrix& rref);
    static std::string generateSolutionDescription(const EquationSolution& solution);
};
