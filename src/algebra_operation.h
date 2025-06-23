#ifndef ALGEBRA_OPERATION_H
#define ALGEBRA_OPERATION_H

#include <string>

// 此文件现在作为代数运算模块的公共接口。
// 底层实现已移至 src/algebra/ 文件夹中。

namespace Algebra {

// 公开的接口函数，供解释器调用
std::string simplifyExpression(const std::string& expr);
std::string factorExpression(const std::string& expr);
std::string solveExpression(const std::string& expr);

} // namespace Algebra

#endif // ALGEBRA_OPERATION_H
