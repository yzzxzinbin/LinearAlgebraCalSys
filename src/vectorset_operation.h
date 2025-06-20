#pragma once
#include "matrix.h"
#include "result.h"

// 计算两个向量组的线性表示关系，返回Result类型
Result rep_vecset(const Matrix& set1, const Matrix& set2);
