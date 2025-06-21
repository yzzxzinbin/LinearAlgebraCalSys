#pragma once
#include "matrix.h"
#include "result.h"

// 计算两个向量组的线性表示关系，返回Result类型
Result rep_vecset(const Matrix& set1, const Matrix& set2);

// 向量组联合最简行阶梯形变换，返回变换后的第二个矩阵
Matrix unionrref(const Matrix& A, const Matrix& B);
