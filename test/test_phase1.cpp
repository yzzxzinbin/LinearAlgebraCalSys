#include <iostream>
#include <vector>
#include <windows.h>
#include "../src/fraction.h"
#include "../src/matrix.h"
#include "../src/vector.h"

// 用于测试的简单断言宏
#define ASSERT(condition, message)                                 \
    if (!(condition))                                              \
    {                                                              \
        std::cerr << "Assertion failed: " << message << std::endl; \
        return false;                                              \
    }

// 测试Fraction类的基本功能
bool testFraction()
{
    std::cout << "=== 测试Fraction类 ===" << std::endl;

    // 构造函数测试
    Fraction f1;
    ASSERT(f1.getNumerator() == 0 && f1.getDenominator() == 1, "默认构造函数应创建0/1分数");

    Fraction f2(5);
    ASSERT(f2.getNumerator() == 5 && f2.getDenominator() == 1, "整数构造函数应创建n/1分数");

    Fraction f3(10, 15); // 应简化为2/3
    ASSERT(f3.getNumerator() == 2 && f3.getDenominator() == 3, "分数构造函数应自动化简");

    // 算术运算测试
    Fraction f4 = f2 + f3; // 5/1 + 2/3 = 15/3 + 2/3 = 17/3
    ASSERT(f4.getNumerator() == 17 && f4.getDenominator() == 3, "加法操作失败");

    Fraction f5 = f2 - f3; // 5/1 - 2/3 = 15/3 - 2/3 = 13/3
    ASSERT(f5.getNumerator() == 13 && f5.getDenominator() == 3, "减法操作失败");

    Fraction f6 = f2 * f3; // 5/1 * 2/3 = 10/3
    ASSERT(f6.getNumerator() == 10 && f6.getDenominator() == 3, "乘法操作失败");

    Fraction f7 = f2 / f3; // 5/1 / 2/3 = 5/1 * 3/2 = 15/2
    ASSERT(f7.getNumerator() == 15 && f7.getDenominator() == 2, "除法操作失败");

    Fraction f8 = f6 * f7; // 10/3 * 15/2 = 150/6 = 25/1
    ASSERT(f8.getNumerator() == 25 && f8.getDenominator() == 1, "复合运算失败");

    // 输出测试
    std::cout << "f1: " << f1 << std::endl;
    std::cout << "f2: " << f2 << std::endl;
    std::cout << "f3: " << f3 << std::endl;
    std::cout << "f4 (f2+f3): " << f4 << std::endl;
    std::cout << "f5 (f2-f3): " << f5 << std::endl;
    std::cout << "f6 (f2*f3): " << f6 << std::endl;
    std::cout << "f7 (f2/f3): " << f7 << std::endl;
    std::cout << "f8 (f6*f7): " << f8 << std::endl;

    return true;
}

// 测试Matrix类的基本功能
bool testMatrix()
{
    std::cout << "\n=== 测试Matrix类 ===" << std::endl;

    // 构造函数测试
    Matrix m1(2, 3); // 2x3矩阵，默认元素为0

    // 填充矩阵
    for (size_t i = 0; i < m1.rowCount(); ++i)
    {
        for (size_t j = 0; j < m1.colCount(); ++j)
        {
            m1.at(i, j) = Fraction(i + j + 1);
        }
    }

    std::cout << "矩阵m1:" << std::endl;
    m1.print();

    // 使用向量构造测试
    std::vector<std::vector<Fraction>> data = {
        {Fraction(1), Fraction(2), Fraction(3)},
        {Fraction(4), Fraction(5), Fraction(6)}};
    Matrix m2(data);

    std::cout << "矩阵m2:" << std::endl;
    m2.print();

    // 矩阵加法测试
    Matrix m3 = m1 + m2;
    std::cout << "矩阵m3 (m1+m2):" << std::endl;
    m3.print();

    // 矩阵减法测试
    Matrix m4 = m2 - m1;
    std::cout << "矩阵m4 (m2-m1):" << std::endl;
    m4.print();

    // 矩阵数乘测试
    Matrix m5 = m1 * Fraction(2);
    std::cout << "矩阵m5 (m1*2):" << std::endl;
    m5.print();

    // 矩阵转置测试
    Matrix m6 = m1.transpose();
    std::cout << "矩阵m6 (m1的转置):" << std::endl;
    m6.print();

    // 矩阵乘法测试
    Matrix m7(2, 2);
    m7.at(0, 0) = Fraction(1);
    m7.at(0, 1) = Fraction(2);
    m7.at(1, 0) = Fraction(3);
    m7.at(1, 1) = Fraction(4);

    Matrix m8(2, 2);
    m8.at(0, 0) = Fraction(2);
    m8.at(0, 1) = Fraction(0);
    m8.at(1, 0) = Fraction(1);
    m8.at(1, 1) = Fraction(3);

    std::cout << "矩阵m7:" << std::endl;
    m7.print();

    std::cout << "矩阵m8:" << std::endl;
    m8.print();

    Matrix m9 = m7 * m8;
    std::cout << "矩阵m9 (m7*m8):" << std::endl;
    m9.print();
    // 验证矩阵乘法结果
    // m7 * m8 = [1 2] * [2 0] = [4 6]
    //           [3 4]   [1 3]   [10 12]
    Matrix m10 = m6 * m1;
    std::cout << "矩阵m10 (m6*m1):" << std::endl;
    m10.print();

    ASSERT(m9.at(0, 0).getNumerator() == 4 && m9.at(0, 0).getDenominator() == 1, "矩阵乘法结果不正确");
    ASSERT(m9.at(0, 1).getNumerator() == 6 && m9.at(0, 1).getDenominator() == 1, "矩阵乘法结果不正确");
    ASSERT(m9.at(1, 0).getNumerator() == 10 && m9.at(1, 0).getDenominator() == 1, "矩阵乘法结果不正确");
    ASSERT(m9.at(1, 1).getNumerator() == 12 && m9.at(1, 1).getDenominator() == 1, "矩阵乘法结果不正确");

    return true;
}

// 测试Vector类的基本功能
bool testVector()
{
    std::cout << "\n=== 测试Vector类 ===" << std::endl;

    // 构造函数测试
    Vector v1(3); // 3维向量，默认元素为0

    // 填充向量
    for (size_t i = 0; i < v1.size(); ++i)
    {
        v1.at(i) = Fraction(i + 1);
    }

    std::cout << "向量v1:" << std::endl;
    v1.print();

    // 使用vector构造测试
    std::vector<Fraction> data = {Fraction(4), Fraction(5), Fraction(6)};
    Vector v2(data);

    std::cout << "向量v2:" << std::endl;
    v2.print();

    // 向量加法测试
    Vector v3 = v1 + v2;
    std::cout << "向量v3 (v1+v2):" << std::endl;
    v3.print();

    // 向量减法测试
    Vector v4 = v2 - v1;
    std::cout << "向量v4 (v2-v1):" << std::endl;
    v4.print();

    // 向量数乘测试
    Vector v5 = v1 * Fraction(2);
    std::cout << "向量v5 (v1*2):" << std::endl;
    v5.print();

    // 向量点乘测试
    Fraction dotProduct = v1.dot(v2);
    std::cout << "v1·v2 = " << dotProduct << std::endl;
    // 验证点乘: (1*4 + 2*5 + 3*6) = 4 + 10 + 18 = 32
    ASSERT(dotProduct.getNumerator() == 32 && dotProduct.getDenominator() == 1, "向量点乘计算错误");

    // 创建三维向量进行叉乘测试
    Vector v3d1(v1);
    Vector v3d2(v2);
    
    Vector crossProduct = v3d1.cross(v3d2);
    std::cout << "v3d1×v3d2 = ";
    crossProduct.print();
    // 验证叉乘: [0,0,1]

    // 测试向量范数
    Fraction norm_squared = v1.norm();
    std::cout << "||v1||² = " << norm_squared << std::endl;
    // 验证范数平方: 1² + 2² + 3² = 1 + 4 + 9 = 14
    ASSERT(norm_squared.getNumerator() == 14 && norm_squared.getDenominator() == 1, "向量范数计算错误");

    return true;
}

int main()
{
    SetConsoleCP(65001);       // 设置控制台输入为UTF-8编码
    SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码
    std::cout << "线性代数计算系统 - 第一阶段测试\n"
              << std::endl;

    bool fractionTestPassed = testFraction();
    bool matrixTestPassed = testMatrix();
    bool vectorTestPassed = testVector();

    std::cout << "\n=== 测试结果汇总 ===" << std::endl;
    std::cout << "Fraction类测试: " << (fractionTestPassed ? "通过" : "失败") << std::endl;
    std::cout << "Matrix类测试: " << (matrixTestPassed ? "通过" : "失败") << std::endl;
    std::cout << "Vector类测试: " << (vectorTestPassed ? "通过" : "失败") << std::endl;

    return (fractionTestPassed && matrixTestPassed && vectorTestPassed) ? 0 : 1;
}
