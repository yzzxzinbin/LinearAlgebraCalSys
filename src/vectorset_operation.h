#pragma once
#include "matrix.h"
#include "vector.h"
#include "result.h"

// 计算两个向量组的线性表示关系，返回Result类型
Result RS_rep_vecset(const Matrix& set1, const Matrix& set2);

// 新增：判断setA能否线性表示v，返回系数列或全0列
Matrix rep_vecsingle(const Matrix& setA, const Vector& v);

// 向量组联合最简行阶梯形变换，返回变换后的第二个矩阵
Matrix unionrref(const Matrix& A, const Matrix& B);

// 新增：计算矩阵的极大线性无关列向量组，返回对应的子矩阵（原矩阵的部分列）
Matrix max_independentset_col(const Matrix& mat);

// 新增：计算矩阵的极大线性无关行向量组，返回对应的子矩阵（原矩阵的部分行）
Matrix max_independentset_row(const Matrix& mat);
