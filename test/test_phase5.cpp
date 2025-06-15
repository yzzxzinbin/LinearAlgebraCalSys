#include <iostream>
#include <vector>
#include <windows.h>
#include "../src/fraction.h"
#include "../src/matrix.h"
#include "../src/vector.h"
#include "../src/matrix_operations.h"
#include "../src/operation_step.h"
#include "../src/equationset.h"

// 测试唯一解的线性方程组
void testUniqueQEquation() {
    std::cout << "\n=== 测试唯一解的线性方程组 ===\n" << std::endl;
    
    // 创建系数矩阵 A: 2x + 3y = 7, x - y = 1
    Matrix A(2, 2);
    A.at(0, 0) = Fraction(2); A.at(0, 1) = Fraction(3);
    A.at(1, 0) = Fraction(1); A.at(1, 1) = Fraction(-1);
    
    // 创建常数向量 b
    Matrix b(2, 1);
    b.at(0, 0) = Fraction(7);
    b.at(1, 0) = Fraction(1);
    
    std::cout << "系数矩阵 A:" << std::endl;
    A.print();
    
    std::cout << "常数向量 b:" << std::endl;
    b.print();
    
    try {
        // 创建操作历史对象
        OperationHistory history;
        
        // 求解方程组
        EquationSolution solution = EquationSolver::solve(A, b, history);
        
        // 打印求解过程
        std::cout << "\n方程组求解过程:" << std::endl;
        history.printAll();
        
        // 打印解
        std::cout << "\n";
        solution.print();
        
        // 验证解
        if (solution.hasUniqueSolution()) {
            Matrix x = solution.getParticularSolution();
            Matrix Ax = A * x;
            std::cout << "\n验证 Ax = b:" << std::endl;
            std::cout << "Ax = " << std::endl;
            Ax.print();
            std::cout << "b = " << std::endl;
            b.print();
        }
        
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
}

// 测试无穷多解的线性方程组
void testInfiniteSolutionEquation() {
    std::cout << "\n=== 测试无穷多解的线性方程组 ===\n" << std::endl;
    
    // 创建系数矩阵 A: x + 2y + 3z = 6, 2x + 4y + 6z = 12
    Matrix A(2, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(2); A.at(1, 1) = Fraction(4); A.at(1, 2) = Fraction(6);
    
    // 创建常数向量 b
    Matrix b(2, 1);
    b.at(0, 0) = Fraction(6);
    b.at(1, 0) = Fraction(12);
    
    std::cout << "系数矩阵 A:" << std::endl;
    A.print();
    
    std::cout << "常数向量 b:" << std::endl;
    b.print();
    
    try {
        // 求解方程组
        EquationSolution solution = EquationSolver::solve(A, b);
        
        // 打印解
        solution.print();
        
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
}

// 测试无解的线性方程组
void testNoSolutionEquation() {
    std::cout << "\n=== 测试无解的线性方程组 ===\n" << std::endl;
    
    // 创建系数矩阵 A: x + y = 1, x + y = 2
    Matrix A(2, 2);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(1);
    A.at(1, 0) = Fraction(1); A.at(1, 1) = Fraction(1);
    
    // 创建常数向量 b
    Matrix b(2, 1);
    b.at(0, 0) = Fraction(1);
    b.at(1, 0) = Fraction(2);
    
    std::cout << "系数矩阵 A:" << std::endl;
    A.print();
    
    std::cout << "常数向量 b:" << std::endl;
    b.print();
    
    try {
        // 求解方程组
        EquationSolution solution = EquationSolver::solve(A, b);
        
        // 打印解
        solution.print();
        
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
}

// 测试齐次线性方程组
void testHomogeneousEquation() {
    std::cout << "\n=== 测试齐次线性方程组 ===\n" << std::endl;
    
    // 创建系数矩阵 A: x + 2y + 3z = 0, 4x + 5y + 6z = 0
    Matrix A(2, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(4); A.at(1, 1) = Fraction(5); A.at(1, 2) = Fraction(6);
    
    std::cout << "系数矩阵 A (齐次方程组 Ax = 0):" << std::endl;
    A.print();
    
    try {
        // 创建操作历史对象
        OperationHistory history;
        
        // 求解齐次方程组
        EquationSolution solution = EquationSolver::solveHomogeneous(A, history);
        
        // 打印求解过程
        std::cout << "\n齐次方程组求解过程:" << std::endl;
        history.printAll();
        
        // 打印解
        std::cout << "\n";
        solution.print();
        
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
}

// 测试方程组性质分析
void testEquationSystemAnalysis() {
    std::cout << "\n=== 测试方程组性质分析 ===\n" << std::endl;
    
    // 测试几种不同的方程组
    std::vector<std::pair<Matrix, Matrix>> testCases;
    
    // 案例1: 唯一解
    Matrix A1(2, 2);
    A1.at(0, 0) = Fraction(1); A1.at(0, 1) = Fraction(2);
    A1.at(1, 0) = Fraction(3); A1.at(1, 1) = Fraction(4);
    Matrix b1(2, 1);
    b1.at(0, 0) = Fraction(5);
    b1.at(1, 0) = Fraction(11);
    testCases.push_back({A1, b1});
    
    // 案例2: 无穷多解
    Matrix A2(2, 3);
    A2.at(0, 0) = Fraction(1); A2.at(0, 1) = Fraction(2); A2.at(0, 2) = Fraction(3);
    A2.at(1, 0) = Fraction(2); A2.at(1, 1) = Fraction(4); A2.at(1, 2) = Fraction(6);
    Matrix b2(2, 1);
    b2.at(0, 0) = Fraction(6);
    b2.at(1, 0) = Fraction(12);
    testCases.push_back({A2, b2});
    
    // 案例3: 无解
    Matrix A3(2, 2);
    A3.at(0, 0) = Fraction(1); A3.at(0, 1) = Fraction(1);
    A3.at(1, 0) = Fraction(1); A3.at(1, 1) = Fraction(1);
    Matrix b3(2, 1);
    b3.at(0, 0) = Fraction(1);
    b3.at(1, 0) = Fraction(2);
    testCases.push_back({A3, b3});
    
    for (size_t i = 0; i < testCases.size(); ++i) {
        std::cout << "\n--- 案例 " << (i + 1) << " ---" << std::endl;
        
        const Matrix& A = testCases[i].first;
        const Matrix& b = testCases[i].second;
        
        std::cout << "系数矩阵 A:" << std::endl;
        A.print();
        std::cout << "常数向量 b:" << std::endl;
        b.print();
        
        EquationSystemInfo info = EquationSolver::analyzeSystem(A, b);
        
        std::cout << "分析结果:" << std::endl;
        std::cout << "  方程个数: " << info.numEquations << std::endl;
        std::cout << "  变量个数: " << info.numVariables << std::endl;
        std::cout << "  系数矩阵的秩: " << info.coefficientRank << std::endl;
        std::cout << "  增广矩阵的秩: " << info.augmentedRank << std::endl;
        std::cout << "  解的性质: " << info.description << std::endl;
    }
}

int main() {
    SetConsoleCP(65001);       // 设置控制台输入为UTF-8编码
    SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
    
    std::cout << "线性代数计算系统 - 第五阶段测试：线性方程组求解\n" << std::endl;
    
    testUniqueQEquation();
    testInfiniteSolutionEquation();
    testNoSolutionEquation();
    testHomogeneousEquation();
    testEquationSystemAnalysis();
    
    return 0;
}
