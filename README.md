# 线性代数计算系统 API 文档

## 目录
1. [简介](#简介)
2. [系统架构](#系统架构)
3. [分数类 (Fraction)](#分数类-fraction)
4. [向量类 (Vector)](#向量类-vector)
5. [矩阵类 (Matrix)](#矩阵类-matrix)
6. [矩阵操作类 (MatrixOperations)](#矩阵操作类-matrixoperations)
7. [操作步骤与历史记录](#操作步骤与历史记录)
8. [语法解析系统](#语法解析系统)
9. [用户界面系统](#用户界面系统)
10. [文件导入导出](#文件导入导出)
11. [使用示例](#使用示例)

## 简介

本线性代数计算系统是一个功能完整的C++应用程序，提供精确的线性代数计算功能。系统采用分数表示法确保计算精度，支持交互式操作界面，并能详细记录和展示计算步骤。

### 主要特性

- **精确计算**: 使用任意精度分数避免浮点数误差
- **丰富功能**: 支持矩阵/向量的各种运算和高级操作
- **步骤展示**: 详细记录计算过程，支持步骤回放
- **交互界面**: 提供友好的终端用户界面(TUI)
- **表达式解析**: 支持自然数学表达式输入
- **变量管理**: 支持变量定义、存储和文件导入导出
- **扩展性**: 模块化设计，易于扩展新功能

### 支持的运算

**基础运算**: 矩阵/向量加减法、数乘、矩阵乘法、转置  
**高级运算**: 行列式计算、逆矩阵、代数余子式、伴随矩阵  
**矩阵变换**: 初等行变换、行阶梯形、最简行阶梯形  
**向量运算**: 点乘、叉乘、范数计算、归一化  
**特殊功能**: 矩阵秩计算、行列式展开、高斯消元法

## 系统架构

系统采用模块化设计，主要包含以下层次：

```
┌───────────────────────────────────────┐
│         用户界面层 (TUI)               │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │  应用主界面  │ │   矩阵编辑器     │  │
│  └─────────────┘ └─────────────────┘  │
└─────────────────┬─────────────────────┘
                  │
┌─────────────────┴─────────────────────┐
│       语法解析层                       │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │  词法分析器  │ │   语法分析器     │  │
│  └─────────────┘ └─────────────────┘  │
│  ┌─────────────────────────────────┐  │
│  │        表达式解释器              │  │
│  └─────────────────────────────────┘  │
└─────────────────┬─────────────────────┘
                  │
┌─────────────────┴─────────────────────┐
│       计算核心层                       │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │  矩阵操作类  │ │   历史记录系统   │  │
│  └─────────────┘ └─────────────────┘  │
└─────────────────┬─────────────────────┘
                  │
┌─────────────────┴─────────────────────┐
│       数据类型层                       │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │   分数类     │ │     矩阵类      │  │
│  └─────────────┘ └─────────────────┘  │
│  ┌─────────────┐ ┌─────────────────┐  │
│  │   向量类     │ │     结果类      │  │
│  └─────────────┘ └─────────────────┘  │
└───────────────────────────────────────┘
```

## 分数类 (Fraction)

`Fraction` 类实现了精确的分数计算，是系统的基础数据类型。

### 构造函数

```cpp
Fraction();                              // 默认构造函数，创建值为0的分数
Fraction(long long num);                 // 用整数创建分数 num/1
Fraction(long long num, long long den);  // 创建分数 num/den
```

### 基本操作

```cpp
long long getNumerator() const;          // 获取分子
long long getDenominator() const;        // 获取分母
```

### 算术运算

```cpp
Fraction operator+(const Fraction& other) const;  // 加法
Fraction operator-(const Fraction& other) const;  // 减法
Fraction operator*(const Fraction& other) const;  // 乘法
Fraction operator/(const Fraction& other) const;  // 除法

Fraction& operator+=(const Fraction& other);      // 复合赋值加法
Fraction& operator-=(const Fraction& other);      // 复合赋值减法
Fraction& operator*=(const Fraction& other);      // 复合赋值乘法
Fraction& operator/=(const Fraction& other);      // 复合赋值除法

Fraction operator-() const;                       // 取负
```

### 比较运算

```cpp
bool operator==(const Fraction& other) const;     // 相等比较
bool operator!=(const Fraction& other) const;     // 不等比较
bool operator<(const Fraction& other) const;      // 小于比较
bool operator<=(const Fraction& other) const;     // 小于等于比较
bool operator>(const Fraction& other) const;      // 大于比较
bool operator>=(const Fraction& other) const;     // 大于等于比较
```

### 输出

```cpp
friend std::ostream& operator<<(std::ostream& os, const Fraction& f);  // 输出格式化
```

## 向量类 (Vector)

`Vector` 类表示数学向量，支持各种向量运算。

### 构造函数

```cpp
Vector(size_t n);                            // 创建n维零向量
Vector(const std::vector<Fraction>& d);      // 用已有数据创建向量
```

### 基本操作

```cpp
size_t size() const;                         // 获取向量维度
Fraction& at(size_t i);                      // 访问/修改第i个元素
const Fraction& at(size_t i) const;          // 只读访问第i个元素

void input(std::istream& is = std::cin);     // 从输入流读取向量数据
void print(std::ostream& os = std::cout) const;  // 格式化输出向量到输出流
```

### 向量运算

```cpp
Vector operator+(const Vector& rhs) const;   // 向量加法
Vector operator-(const Vector& rhs) const;   // 向量减法
Vector operator*(const Fraction& k) const;   // 向量数乘

Fraction dot(const Vector& rhs) const;       // 向量点乘(内积)
Vector cross(const Vector& rhs) const;       // 向量叉乘(外积)，仅适用于3D向量
Fraction norm() const;                       // 计算向量的范数平方
Vector normalize() const;                    // 向量归一化(暂时未实现)
```

## 矩阵类 (Matrix)

`Matrix` 类表示数学矩阵，支持各种矩阵运算。

### 构造函数

```cpp
Matrix(size_t r, size_t c);                         // 创建r行c列的零矩阵
Matrix(const std::vector<std::vector<Fraction>>& d); // 用已有数据创建矩阵
```

### 基本操作

```cpp
size_t rowCount() const;                            // 获取行数
size_t colCount() const;                            // 获取列数

Fraction& at(size_t r, size_t c);                   // 访问/修改指定位置的元素
const Fraction& at(size_t r, size_t c) const;       // 只读访问指定位置的元素

void input(std::istream& is = std::cin);            // 从输入流读取矩阵数据
void print(std::ostream& os = std::cout) const;     // 格式化输出矩阵到输出流
```

### 矩阵运算

```cpp
Matrix operator+(const Matrix& rhs) const;          // 矩阵加法
Matrix operator-(const Matrix& rhs) const;          // 矩阵减法
Matrix operator*(const Fraction& k) const;          // 矩阵数乘
Matrix operator*(const Matrix& rhs) const;          // 矩阵乘法
Matrix transpose() const;                           // 矩阵转置

// 新增方法
Fraction cofactor(size_t row, size_t col) const;    // 计算指定位置的代数余子式
Matrix cofactorMatrix() const;                      // 计算代数余子式矩阵
Matrix adjugate() const;                            // 计算伴随矩阵
Fraction determinantByExpansion() const;            // 按行列式展开法计算行列式
Matrix augment(const Matrix& B) const;              // 创建增广矩阵 [A|B]
static Matrix identity(size_t n);                   // 创建n阶单位矩阵
Matrix extractRightPart(size_t colStart) const;     // 从增广矩阵中提取右侧部分
```

## 矩阵操作类 (MatrixOperations)

`MatrixOperations` 类提供高级矩阵操作，如行变换、行列式计算等。

### 初等行变换

```cpp
// 返回新矩阵的版本
static Matrix swapRows(const Matrix& mat, size_t row1, size_t row2);
static Matrix scaleRow(const Matrix& mat, size_t row, const Fraction& scalar);
static Matrix addScaledRow(const Matrix& mat, size_t targetRow, size_t sourceRow, const Fraction& scalar);

// 原地修改版本(带历史记录)
static void swapRows(Matrix& mat, size_t row1, size_t row2, OperationHistory& history);
static void scaleRow(Matrix& mat, size_t row, const Fraction& scalar, OperationHistory& history);
static void addScaledRow(Matrix& mat, size_t targetRow, size_t sourceRow, const Fraction& scalar, OperationHistory& history);
```

### 行阶梯形与最简行阶梯形

```cpp
// 返回新矩阵的版本
static Matrix toRowEchelonForm(const Matrix& mat);
static Matrix toReducedRowEchelonForm(const Matrix& mat);

// 原地修改版本(带历史记录)
static void toRowEchelonForm(Matrix& mat, OperationHistory& history);
static void toReducedRowEchelonForm(Matrix& mat, OperationHistory& history);
```

### 矩阵特性计算

```cpp
static int rank(const Matrix& mat);                           // 计算矩阵秩
static Fraction determinant(const Matrix& mat);               // 计算行列式(高斯消元法)
static Fraction determinant(const Matrix& mat, OperationHistory& history);  // 计算行列式(带历史记录)

// 新增方法
static Matrix cofactorMatrix(const Matrix& mat);              // 计算代数余子式矩阵
static Matrix adjugate(const Matrix& mat);                    // 计算伴随矩阵
static Fraction determinantByExpansion(const Matrix& mat);    // 按行列式展开法计算行列式
static Fraction determinantByExpansion(const Matrix& mat, ExpansionHistory& history); // 带历史记录
static Matrix inverse(const Matrix& mat);                     // 计算逆矩阵(伴随矩阵法)
static Matrix inverse(const Matrix& mat, OperationHistory& history); // 带历史记录
static Matrix inverseGaussJordan(const Matrix& mat);          // 计算逆矩阵(高斯-若尔当消元法)
static Matrix inverseGaussJordan(const Matrix& mat, OperationHistory& history); // 带历史记录
```

## 操作步骤与历史记录

`OperationStep` 和 `OperationHistory` 类用于记录和展示矩阵操作步骤。

### OperationStep 类

表示单个操作步骤。

```cpp
// 操作类型
enum class OperationType {
    SWAP_ROWS,           // 交换两行
    SCALE_ROW,           // 行乘以常数
    ADD_SCALED_ROW,      // 一行加上另一行的倍数
    INITIAL_STATE,       // 初始状态
    RESULT_STATE         // 最终结果状态
};

// 构造函数
OperationStep(OperationType type, const std::string& desc, const Matrix& matrix, 
             int r1 = -1, int r2 = -1, const Fraction& scalar = Fraction(1));

// 获取操作信息
OperationType getType() const;
std::string getDescription() const;
const Matrix& getMatrixState() const;
int getRow1() const;
int getRow2() const;
Fraction getScalar() const;

// 打印操作步骤和矩阵状态
void print(std::ostream& os = std::cout) const;
```

### OperationHistory 类

管理操作步骤的集合。

```cpp
// 添加操作步骤
void addStep(const OperationStep& step);

// 获取步骤数量
size_t size() const;

// 获取指定索引的步骤
const OperationStep& getStep(size_t index) const;

// 打印所有步骤
void printAll(std::ostream& os = std::cout) const;

// 打印指定索引的步骤
void printStep(size_t index, std::ostream& os = std::cout) const;

// 清空历史
void clear();
```

### 新增: 行列式展开历史记录

行列式展开过程中的步骤记录与展示。

#### ExpansionStep 类

表示行列式展开计算中的单个步骤。

```cpp
// 展开类型
enum class ExpansionType {
    ROW_EXPANSION,        // 按行展开
    COLUMN_EXPANSION,     // 按列展开
    SUBMATRIX_CALCULATION,// 计算子矩阵行列式
    INITIAL_STATE,        // 初始状态
    RESULT_STATE          // 最终结果状态
};

// 构造函数
ExpansionStep(
    ExpansionType type,
    const std::string& desc,
    const Matrix& matrix,
    int expIndex = -1,
    int elemIndex = -1,
    const Fraction& elem = Fraction(0),
    const Fraction& cof = Fraction(0),
    const Fraction& termVal = Fraction(0),
    const Fraction& accVal = Fraction(0)
);

// 获取步骤信息
ExpansionType getType() const;
std::string getDescription() const;
const Matrix& getMatrixState() const;
int getExpansionIndex() const;       // 获取展开的行或列索引
int getElementIndex() const;         // 获取当前处理的元素索引
Fraction getElement() const;         // 获取当前元素值
Fraction getCofactor() const;        // 获取当前元素的代数余子式
Fraction getTermValue() const;       // 获取当前项的值
Fraction getAccumulatedValue() const; // 获取累积和

// 打印步骤信息
void print(std::ostream& os = std::cout) const;
```

#### ExpansionHistory 类

管理行列式展开计算过程的历史记录。

```cpp
// 添加步骤
void addStep(const ExpansionStep& step);

// 获取步骤数量
size_t size() const;

// 获取指定索引的步骤
const ExpansionStep& getStep(size_t index) const;

// 打印所有步骤
void printAll(std::ostream& os = std::cout) const;

// 打印指定索引的步骤
void printStep(size_t index, std::ostream& os = std::cout) const;

// 清空历史
void clear();
```

## 使用示例

### 基本分数运算

```cpp
#include "fraction.h"
#include <iostream>

int main() {
    // 创建分数
    Fraction a(1, 2);    // 1/2
    Fraction b(3, 4);    // 3/4
    
    // 分数运算
    Fraction sum = a + b;           // 1/2 + 3/4 = 5/4
    Fraction difference = a - b;    // 1/2 - 3/4 = -1/4
    Fraction product = a * b;       // 1/2 * 3/4 = 3/8
    Fraction quotient = a / b;      // 1/2 / 3/4 = 2/3
    
    // 输出结果
    std::cout << "a = " << a << ", b = " << b << std::endl;
    std::cout << "a + b = " << sum << std::endl;
    std::cout << "a - b = " << difference << std::endl;
    std::cout << "a * b = " << product << std::endl;
    std::cout << "a / b = " << quotient << std::endl;
    
    return 0;
}
```

### 向量运算

```cpp
#include "vector.h"
#include <iostream>

int main() {
    // 创建向量
    std::vector<Fraction> data1 = {Fraction(1), Fraction(2), Fraction(3)};
    std::vector<Fraction> data2 = {Fraction(4), Fraction(5), Fraction(6)};
    Vector v1(data1);
    Vector v2(data2);
    
    // 向量运算
    Vector sum = v1 + v2;               // [5, 7, 9]
    Vector scaled = v1 * Fraction(2);   // [2, 4, 6]
    Fraction dot_product = v1.dot(v2);  // 1*4 + 2*5 + 3*6 = 32
    
    // 对于3D向量，计算叉积
    Vector cross_product = v1.cross(v2); // [2*6-3*5, 3*4-1*6, 1*5-2*4] = [-3, 6, -3]
    
    // 输出结果
    std::cout << "v1 = ";
    v1.print();
    std::cout << "v2 = ";
    v2.print();
    std::cout << "v1 + v2 = ";
    sum.print();
    std::cout << "v1 * 2 = ";
    scaled.print();
    std::cout << "v1 · v2 = " << dot_product << std::endl;
    std::cout << "v1 × v2 = ";
    cross_product.print();
    
    return 0;
}
```

### 矩阵运算

```cpp
#include "matrix.h"
#include <iostream>

int main() {
    // 创建矩阵
    Matrix A(2, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(4); A.at(1, 1) = Fraction(5); A.at(1, 2) = Fraction(6);
    
    Matrix B(3, 2);
    B.at(0, 0) = Fraction(7); B.at(0, 1) = Fraction(8);
    B.at(1, 0) = Fraction(9); B.at(1, 1) = Fraction(10);
    B.at(2, 0) = Fraction(11); B.at(2, 1) = Fraction(12);
    
    // 矩阵运算
    Matrix C = A * B;    // 矩阵乘法
    Matrix AT = A.transpose(); // 矩阵转置
    
    // 输出结果
    std::cout << "Matrix A:" << std::endl;
    A.print();
    std::cout << "Matrix B:" << std::endl;
    B.print();
    std::cout << "A * B = " << std::endl;
    C.print();
    std::cout << "A^T = " << std::endl;
    AT.print();
    
    return 0;
}
```

### 高级矩阵操作

```cpp
#include "matrix.h"
#include "matrix_operations.h"
#include "operation_step.h"
#include <iostream>

int main() {
    // 创建矩阵
    Matrix A(3, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(4); A.at(1, 1) = Fraction(5); A.at(1, 2) = Fraction(6);
    A.at(2, 0) = Fraction(7); A.at(2, 1) = Fraction(8); A.at(2, 2) = Fraction(9);
    
    std::cout << "原始矩阵 A:" << std::endl;
    A.print();
    
    // 计算行列式
    OperationHistory history;
    Fraction det = MatrixOperations::determinant(A, history);
    std::cout << "计算 A 的行列式:" << std::endl;
    history.printAll();
    std::cout << "行列式 = " << det << std::endl;
    
    // 计算秩
    int rank = MatrixOperations::rank(A);
    std::cout << "矩阵 A 的秩 = " << rank << std::endl;
    
    // 行阶梯形变换
    history.clear();
    Matrix REF = A;
    MatrixOperations::toRowEchelonForm(REF, history);
    std::cout << "计算 A 的行阶梯形:" << std::endl;
    history.printAll();
    
    // 最简行阶梯形变换
    history.clear();
    Matrix RREF = A;
    MatrixOperations::toReducedRowEchelonForm(RREF, history);
    std::cout << "计算 A 的最简行阶梯形:" << std::endl;
    history.printAll();
    
    return 0;
}
```

### 代数余子式与伴随矩阵

```cpp
#include "matrix.h"
#include "matrix_operations.h"
#include <iostream>

int main() {
    // 创建矩阵
    Matrix A(3, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(4); A.at(1, 1) = Fraction(5); A.at(1, 2) = Fraction(6);
    A.at(2, 0) = Fraction(7); A.at(2, 1) = Fraction(8); A.at(2, 2) = Fraction(9);
    
    std::cout << "原始矩阵 A:" << std::endl;
    A.print();
    
    // 计算(1,1)位置的代数余子式
    Fraction c11 = A.cofactor(0, 0);
    std::cout << "A的(1,1)位置的代数余子式 = " << c11 << std::endl;
    
    // 计算代数余子式矩阵
    Matrix C = MatrixOperations::cofactorMatrix(A);
    std::cout << "A的代数余子式矩阵:" << std::endl;
    C.print();
    
    // 计算伴随矩阵
    Matrix adj = MatrixOperations::adjugate(A);
    std::cout << "A的伴随矩阵:" << std::endl;
    adj.print();
    
    return 0;
}
```

### 行列式展开计算

```cpp
#include "matrix.h"
#include "matrix_operations.h"
#include "determinant_expansion.h"
#include <iostream>

int main() {
    // 创建矩阵
    Matrix A(4, 4);
    A.at(0, 0) = Fraction(3); A.at(0, 1) = Fraction(0); A.at(0, 2) = Fraction(2); A.at(0, 3) = Fraction(0);
    A.at(1, 0) = Fraction(0); A.at(1, 1) = Fraction(1); A.at(1, 2) = Fraction(-5); A.at(1, 3) = Fraction(0);
    A.at(2, 0) = Fraction(0); A.at(2, 1) = Fraction(0); A.at(2, 2) = Fraction(4); A.at(2, 3) = Fraction(2);
    A.at(3, 0) = Fraction(1); A.at(3, 1) = Fraction(0); A.at(3, 2) = Fraction(0); A.at(3, 3) = Fraction(5);
    
    std::cout << "矩阵 A:" << std::endl;
    A.print();
    
    // 创建历史记录对象
    ExpansionHistory history;
    
    // 按行列式展开法计算行列式
    Fraction det = MatrixOperations::determinantByExpansion(A, history);
    
    // 打印计算过程
    std::cout << "按行列式展开法计算行列式的过程:" << std::endl;
    history.printAll();
    
    std::cout << "行列式值 = " << det << std::endl;
    
    return 0;
}
```

### 逆矩阵计算示例

```cpp
#include "matrix.h"
#include "matrix_operations.h"
#include "operation_step.h"
#include <iostream>

int main() {
    // 创建矩阵
    Matrix A(3, 3);
    A.at(0, 0) = Fraction(1); A.at(0, 1) = Fraction(2); A.at(0, 2) = Fraction(3);
    A.at(1, 0) = Fraction(0); A.at(1, 1) = Fraction(1); A.at(1, 2) = Fraction(4);
    A.at(2, 0) = Fraction(5); A.at(2, 1) = Fraction(6); A.at(2, 2) = Fraction(0);
    
    std::cout << "原始矩阵 A:" << std::endl;
    A.print();
    
    // 使用伴随矩阵法计算逆矩阵
    OperationHistory history1;
    try {
        Matrix inv1 = MatrixOperations::inverse(A, history1);
        std::cout << "使用伴随矩阵法计算 A 的逆矩阵:" << std::endl;
        history1.printAll();
        std::cout << "A^(-1) = " << std::endl;
        inv1.print();
        
        // 验证 A * A^(-1) = I
        Matrix I1 = A * inv1;
        std::cout << "验证 A * A^(-1) = " << std::endl;
        I1.print();
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
    
    // 使用高斯-若尔当消元法计算逆矩阵
    OperationHistory history2;
    try {
        Matrix inv2 = MatrixOperations::inverseGaussJordan(A, history2);
        std::cout << "使用高斯-若尔当消元法计算 A 的逆矩阵:" << std::endl;
        history2.printAll();
        std::cout << "A^(-1) = " << std::endl;
        inv2.print();
        
        // 验证 A * A^(-1) = I
        Matrix I2 = A * inv2;
        std::cout << "验证 A * A^(-1) = " << std::endl;
        I2.print();
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }
    
    return 0;
}
```

> This document is mainly generated by Claude 4/Gemini 2.5 Pro by scanning the repository and then manually edited to ensure correctness and clarity. The code examples are simplified for demonstration purposes and may not include all necessary headers or error handling for a complete application. Please refer to the actual source code for full implementation details.
