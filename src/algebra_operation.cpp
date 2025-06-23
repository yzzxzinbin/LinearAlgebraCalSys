#include "algebra_operation.h"
#include "algebra/polynomial.h" // 包含新的多项式实现

namespace Algebra {

// 公开接口实现
std::string simplifyExpression(const std::string& expr) {
    return Polynomial(expr).toString();
}

std::string factorExpression(const std::string& expr) {
    return Polynomial(expr).factor();
}

std::string solveExpression(const std::string& expr) {
    return Polynomial(expr).solve();
}

} // namespace Algebra
