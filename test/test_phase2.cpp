#include <iostream>
#include <vector>
#include <windows.h>
#include "../src/fraction.h"
#include "../src/matrix.h"
#include "../src/vector.h"
#include "../src/matrix_operations.h"
#include "../src/operation_step.h"

// 测试初等行变换
void testRowOperations() {
    std::cout << "\n=== 测试初等行变换 ===\n" << std::endl;
    
    // 创建测试矩阵
    Matrix m(3, 3);
    m.at(0, 0) = Fraction(1); m.at(0, 1) = Fraction(2); m.at(0, 2) = Fraction(3);
    m.at(1, 0) = Fraction(4); m.at(1, 1) = Fraction(5); m.at(1, 2) = Fraction(6);
    m.at(2, 0) = Fraction(7); m.at(2, 1) = Fraction(8); m.at(2, 2) = Fraction(9);
    
    std::cout << "原始矩阵:" << std::endl;
    m.print();
    
    // 创建操作历史对象
    OperationHistory history;
    
    // 测试交换行
    MatrixOperations::swapRows(m, 0, 1, history);
    
    // 测试行乘以常数
    MatrixOperations::scaleRow(m, 0, Fraction(2), history);
    
    // 测试一行加上另一行的倍数
    MatrixOperations::addScaledRow(m, 2, 0, Fraction(1, 2), history);
    
    // 打印所有操作历史
    std::cout << "\n操作历史:" << std::endl;
    history.printAll();
}

// 测试行阶梯形变换
void testRowEchelonForm() {
    std::cout << "\n=== 测试行阶梯形变换 ===\n" << std::endl;
    
    // 创建测试矩阵
    Matrix m(3, 4);
    m.at(0, 0) = Fraction(1); m.at(0, 1) = Fraction(3); m.at(0, 2) = Fraction(-2); m.at(0, 3) = Fraction(0);
    m.at(1, 0) = Fraction(2); m.at(1, 1) = Fraction(6); m.at(1, 2) = Fraction(-5); m.at(1, 3) = Fraction(-2);
    m.at(2, 0) = Fraction(0); m.at(2, 1) = Fraction(0); m.at(2, 2) = Fraction(5); m.at(2, 3) = Fraction(10);
    
    std::cout << "原始矩阵:" << std::endl;
    m.print();
    
    // 创建操作历史对象
    OperationHistory history;
    
    // 转换为行阶梯形
    MatrixOperations::toRowEchelonForm(m, history);
    
    // 打印所有操作历史
    std::cout << "\n行阶梯形变换操作历史:" << std::endl;
    history.printAll();
}

// 测试最简行阶梯形变换
void testReducedRowEchelonForm() {
    std::cout << "\n=== 测试最简行阶梯形变换 ===\n" << std::endl;
    
    // 创建测试矩阵
    Matrix m(3, 4);
    m.at(0, 0) = Fraction(1); m.at(0, 1) = Fraction(3); m.at(0, 2) = Fraction(-2); m.at(0, 3) = Fraction(0);
    m.at(1, 0) = Fraction(2); m.at(1, 1) = Fraction(6); m.at(1, 2) = Fraction(-5); m.at(1, 3) = Fraction(-2);
    m.at(2, 0) = Fraction(0); m.at(2, 1) = Fraction(0); m.at(2, 2) = Fraction(5); m.at(2, 3) = Fraction(10);
    
    std::cout << "原始矩阵:" << std::endl;
    m.print();
    
    // 创建操作历史对象
    OperationHistory history;
    
    // 转换为最简行阶梯形
    MatrixOperations::toReducedRowEchelonForm(m, history);
    
    // 打印所有操作历史
    std::cout << "\n最简行阶梯形变换操作历史:" << std::endl;
    history.printAll();
}

// 测试矩阵秩的计算
void testMatrixRank() {
    std::cout << "\n=== 测试矩阵秩计算 ===\n" << std::endl;
    
    // 创建满秩矩阵
    Matrix m1(3, 3);
    m1.at(0, 0) = Fraction(1); m1.at(0, 1) = Fraction(0); m1.at(0, 2) = Fraction(0);
    m1.at(1, 0) = Fraction(0); m1.at(1, 1) = Fraction(1); m1.at(1, 2) = Fraction(0);
    m1.at(2, 0) = Fraction(0); m1.at(2, 1) = Fraction(0); m1.at(2, 2) = Fraction(1);
    
    std::cout << "矩阵1 (应为3阶满秩):" << std::endl;
    m1.print();
    
    int rank1 = MatrixOperations::rank(m1);
    std::cout << "矩阵1的秩: " << rank1 << std::endl;
    
    // 创建秩为2的矩阵
    Matrix m2(3, 3);
    m2.at(0, 0) = Fraction(1); m2.at(0, 1) = Fraction(2); m2.at(0, 2) = Fraction(3);
    m2.at(1, 0) = Fraction(4); m2.at(1, 1) = Fraction(5); m2.at(1, 2) = Fraction(6);
    m2.at(2, 0) = Fraction(7); m2.at(2, 1) = Fraction(8); m2.at(2, 2) = Fraction(9);
    
    std::cout << "\n矩阵2 (应为秩2):" << std::endl;
    m2.print();
    
    int rank2 = MatrixOperations::rank(m2);
    std::cout << "矩阵2的秩: " << rank2 << std::endl;
}

// 测试行列式计算
void testDeterminant() {
    std::cout << "\n=== 测试行列式计算 ===\n" << std::endl;
    
    // 创建2x2矩阵
    Matrix m1(2, 2);
    m1.at(0, 0) = Fraction(4); m1.at(0, 1) = Fraction(6);
    m1.at(1, 0) = Fraction(3); m1.at(1, 1) = Fraction(8);
    
    std::cout << "2x2矩阵:" << std::endl;
    m1.print();
    
    Fraction det1 = MatrixOperations::determinant(m1);
    std::cout << "行列式值: " << det1 << std::endl;
    
    // 创建3x3矩阵
    Matrix m2(3, 3);
    m2.at(0, 0) = Fraction(6); m2.at(0, 1) = Fraction(1); m2.at(0, 2) = Fraction(1);
    m2.at(1, 0) = Fraction(4); m2.at(1, 1) = Fraction(-2); m2.at(1, 2) = Fraction(5);
    m2.at(2, 0) = Fraction(2); m2.at(2, 1) = Fraction(8); m2.at(2, 2) = Fraction(7);
    
    std::cout << "\n3x3矩阵:" << std::endl;
    m2.print();
    
    OperationHistory history;
    Fraction det2 = MatrixOperations::determinant(m2, history);
    
    std::cout << "行列式计算步骤:" << std::endl;
    history.printAll();
    
    std::cout << "行列式值: " << det2 << std::endl;
}

int main() {
    SetConsoleCP(65001);       // 设置控制台输入为UTF-8编码
    SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
    
    std::cout << "线性代数计算系统 - 第二阶段测试\n" << std::endl;
    
    testRowOperations();
    testRowEchelonForm();
    testReducedRowEchelonForm();
    testMatrixRank();
    testDeterminant();
    
    return 0;
}
