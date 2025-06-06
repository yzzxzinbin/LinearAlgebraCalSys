#include <iostream>
#include <vector>
#include <windows.h>
#include "../src/fraction.h"
#include "../src/matrix.h"
#include "../src/vector.h"
#include "../src/matrix_operations.h"
#include "../src/operation_step.h"
#include "../src/determinant_expansion.h"

// 测试代数余子式矩阵和伴随矩阵
void testCofactorAndAdjugate() {
    std::cout << "\n=== 测试代数余子式矩阵和伴随矩阵 ===\n" << std::endl;
    
    // 创建测试矩阵
    Matrix m(3, 3);
    m.at(0, 0) = Fraction(1); m.at(0, 1) = Fraction(2); m.at(0, 2) = Fraction(3);
    m.at(1, 0) = Fraction(0); m.at(1, 1) = Fraction(4); m.at(1, 2) = Fraction(5);
    m.at(2, 0) = Fraction(1); m.at(2, 1) = Fraction(0); m.at(2, 2) = Fraction(6);
    
    std::cout << "原始矩阵:" << std::endl;
    m.print();
    
    // 计算代数余子式矩阵
    Matrix cofactorMat = MatrixOperations::cofactorMatrix(m);
    std::cout << "\n代数余子式矩阵:" << std::endl;
    cofactorMat.print();
    
    // 计算伴随矩阵
    Matrix adjugateMat = MatrixOperations::adjugate(m);
    std::cout << "\n伴随矩阵 (代数余子式矩阵的转置):" << std::endl;
    adjugateMat.print();
}

// 测试按行列展开计算行列式
void testDeterminantByExpansion() {
    std::cout << "\n=== 测试按行列展开计算行列式 ===\n" << std::endl;
    
    // 创建测试矩阵 - 包含较多零元素
    Matrix m(4, 4);
    m.at(0, 0) = Fraction(3); m.at(0, 1) = Fraction(0); m.at(0, 2) = Fraction(2); m.at(0, 3) = Fraction(0);
    m.at(1, 0) = Fraction(0); m.at(1, 1) = Fraction(1); m.at(1, 2) = Fraction(-5); m.at(1, 3) = Fraction(0);
    m.at(2, 0) = Fraction(0); m.at(2, 1) = Fraction(3); m.at(2, 2) = Fraction(4); m.at(2, 3) = Fraction(2);
    m.at(3, 0) = Fraction(1); m.at(3, 1) = Fraction(0); m.at(3, 2) = Fraction(0); m.at(3, 3) = Fraction(5);
    
    std::cout << "计算行列式的矩阵:" << std::endl;
    m.print();
    
    // 创建历史记录对象
    ExpansionHistory history;
    
    // 计算行列式
    Fraction det = MatrixOperations::determinantByExpansion(m, history);
    
    // 打印计算过程
    std::cout << "\n按行列展开计算行列式的步骤:" << std::endl;
    history.printAll();
    
    std::cout << "行列式的值: " << det << std::endl;
    
    // 与高斯消元法计算的结果比较
    OperationHistory gaussHistory;
    Fraction detGauss = MatrixOperations::determinant(m, gaussHistory);
    std::cout << "\n使用高斯消元法计算的行列式值: " << detGauss << std::endl;
}

int main() {
    SetConsoleCP(65001);       // 设置控制台输入为UTF-8编码
    SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
    
    std::cout << "线性代数计算系统 - 第三阶段测试\n" << std::endl;
    
    testCofactorAndAdjugate();
    testDeterminantByExpansion();
    
    return 0;
}
