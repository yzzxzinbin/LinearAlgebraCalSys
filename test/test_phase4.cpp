#include <iostream>
#include <vector>
#include <windows.h>
#include "../src/fraction.h"
#include "../src/matrix.h"
#include "../src/vector.h"
#include "../src/matrix_operations.h"
#include "../src/operation_step.h"
#include "../src/determinant_expansion.h"

// 测试逆矩阵计算 - 伴随矩阵法
void testInverseByAdjugate() {
    std::cout << "\n=== 测试逆矩阵计算 (伴随矩阵法) ===\n" << std::endl;
    
    // 创建测试矩阵
    Matrix A(3, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(0); A.at(1, 1) = Fraction(1); A.at(1, 2) = Fraction(4);
    A.at(2, 0) = Fraction(5); A.at(2, 1) = Fraction(6); A.at(2, 2) = Fraction(0);
    
    std::cout << "原始矩阵 A:" << std::endl;
    A.print();
    
    // 计算行列式
    Fraction det = MatrixOperations::determinant(A);
    std::cout << "行列式 = " << det << std::endl;
    
    // 如果行列式非零，则矩阵可逆
    if (det != Fraction(0)) {
        // 创建历史记录对象
        OperationHistory history;
        
        try {
            // 计算逆矩阵
            Matrix inv = MatrixOperations::inverse(A, history);
            
            // 打印计算过程
            std::cout << "\n逆矩阵计算过程 (伴随矩阵法):" << std::endl;
            history.printAll();
            
            std::cout << "A的逆矩阵:" << std::endl;
            inv.print();
            
            // 验证 A * A^(-1) = I
            Matrix I = A * inv;
            std::cout << "\n验证 A * A^(-1) = I:" << std::endl;
            I.print();
        } catch (const std::exception& e) {
            std::cout << "错误: " << e.what() << std::endl;
        }
    } else {
        std::cout << "矩阵不可逆 (行列式为0)" << std::endl;
    }
}

// 测试逆矩阵计算 - 高斯-若尔当消元法
void testInverseByGaussJordan() {
    std::cout << "\n=== 测试逆矩阵计算 (高斯-若尔当消元法) ===\n" << std::endl;
    
    // 创建测试矩阵
    Matrix A(3, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(0); A.at(1, 1) = Fraction(1); A.at(1, 2) = Fraction(4);
    A.at(2, 0) = Fraction(5); A.at(2, 1) = Fraction(6); A.at(2, 2) = Fraction(0);
    
    std::cout << "原始矩阵 A:" << std::endl;
    A.print();
    
    // 创建历史记录对象
    OperationHistory history;
    
    try {
        // 计算逆矩阵
        Matrix inv = MatrixOperations::inverseGaussJordan(A, history);
        
        // 打印计算过程
        std::cout << "\n逆矩阵计算过程 (高斯-若尔当消元法):" << std::endl;
        history.printAll();
        
        std::cout << "A的逆矩阵:" << std::endl;
        inv.print();
        
        // 验证 A * A^(-1) = I
        Matrix I = A * inv;
        std::cout << "\n验证 A * A^(-1) = I:" << std::endl;
        I.print();
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
}

// 测试不可逆矩阵
void testSingularMatrix() {
    std::cout << "\n=== 测试不可逆矩阵 ===\n" << std::endl;
    
    // 创建奇异矩阵(行列式为0的矩阵)
    Matrix S(3, 3);
    S.at(0, 0) = Fraction(1); S.at(0, 1) = Fraction(2); S.at(0, 2) = Fraction(3);
    S.at(1, 0) = Fraction(4); S.at(1, 1) = Fraction(5); S.at(1, 2) = Fraction(6);
    S.at(2, 0) = Fraction(7); S.at(2, 1) = Fraction(8); S.at(2, 2) = Fraction(9);
    
    std::cout << "奇异矩阵 S:" << std::endl;
    S.print();
    
    // 计算行列式
    Fraction det = MatrixOperations::determinant(S);
    std::cout << "行列式 = " << det << std::endl;
    
    // 尝试计算逆矩阵
    std::cout << "\n尝试使用伴随矩阵法计算逆矩阵:" << std::endl;
    try {
        Matrix inv1 = MatrixOperations::inverse(S);
        inv1.print(); // 如果没有抛出异常则打印结果
    } catch (const std::exception& e) {
        std::cout << "预期的错误: " << e.what() << std::endl;
    }
    
    std::cout << "\n尝试使用高斯-若尔当消元法计算逆矩阵:" << std::endl;
    try {
        Matrix inv2 = MatrixOperations::inverseGaussJordan(S);
        inv2.print(); // 如果没有抛出异常则打印结果
    } catch (const std::exception& e) {
        std::cout << "预期的错误: " << e.what() << std::endl;
    }
}

// 测试比较两种方法的计算结果
void testCompareInverseMethods() {
    std::cout << "\n=== 比较两种逆矩阵计算方法 ===\n" << std::endl;
    
    // 创建测试矩阵
    Matrix A(3, 3);
    A.at(0, 0) = Fraction(2); A.at(0, 1) = Fraction(1); A.at(0, 2) = Fraction(0);
    A.at(1, 0) = Fraction(3); A.at(1, 1) = Fraction(2); A.at(1, 2) = Fraction(0);
    A.at(2, 0) = Fraction(1); A.at(2, 1) = Fraction(1); A.at(2, 2) = Fraction(1);
    
    std::cout << "测试矩阵 A:" << std::endl;
    A.print();
    
    try {
        // 使用伴随矩阵法计算
        Matrix inv1 = MatrixOperations::inverse(A);
        std::cout << "\n伴随矩阵法计算的逆矩阵:" << std::endl;
        inv1.print();
        
        // 使用高斯-若尔当消元法计算
        Matrix inv2 = MatrixOperations::inverseGaussJordan(A);
        std::cout << "\n高斯-若尔当消元法计算的逆矩阵:" << std::endl;
        inv2.print();
        
        // 验证两个结果是否相同
        bool isSame = true;
        for (size_t i = 0; i < inv1.rowCount() && isSame; ++i) {
            for (size_t j = 0; j < inv1.colCount() && isSame; ++j) {
                if (inv1.at(i, j) != inv2.at(i, j)) {
                    isSame = false;
                }
            }
        }
        
        std::cout << "\n两种方法的计算结果" << (isSame ? "相同" : "不同") << std::endl;
        
        // 验证 A * A^(-1) = I
        Matrix I1 = A * inv1;
        std::cout << "\n伴随矩阵法: A * A^(-1) = " << std::endl;
        I1.print();
        
        Matrix I2 = A * inv2;
        std::cout << "\n高斯-若尔当消元法: A * A^(-1) = " << std::endl;
        I2.print();
        
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
}

// 测试单位矩阵的逆
void testIdentityMatrixInverse() {
    std::cout << "\n=== 测试单位矩阵的逆 ===\n" << std::endl;
    
    // 创建单位矩阵
    Matrix I = Matrix::identity(3);
    std::cout << "单位矩阵 I:" << std::endl;
    I.print();
    
    try {
        // 计算逆矩阵
        Matrix inv = MatrixOperations::inverse(I);
        std::cout << "\n单位矩阵的逆矩阵:" << std::endl;
        inv.print();
        
        // 验证 I * I^(-1) = I
        Matrix result = I * inv;
        std::cout << "\n验证 I * I^(-1) = I:" << std::endl;
        result.print();
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
}

int main() {
    SetConsoleCP(65001);       // 设置控制台输入为UTF-8编码
    SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
    
    std::cout << "线性代数计算系统 - 第四阶段测试：逆矩阵计算\n" << std::endl;
    
    testInverseByAdjugate();
    testInverseByGaussJordan();
    testSingularMatrix();
    testCompareInverseMethods();
    testIdentityMatrixInverse();
    
    return 0;
}
