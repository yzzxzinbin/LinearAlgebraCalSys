#ifndef ALGEBRA_MONOMIAL_H
#define ALGEBRA_MONOMIAL_H

#include "../fraction.h"
#include <string>

namespace Algebra {

/**
 * @class Monomial
 * @brief 表示单项式，如 5x^2。
 */
class Monomial {
public:
    Fraction coefficient;
    std::string variable;
    Fraction power; // 指数已从 int 更改为 Fraction

    // 构造函数更新以接受 Fraction 类型的指数
    Monomial(const Fraction& coeff = Fraction(0), std::string var = "", const Fraction& p = Fraction(0));

    std::string toString() const;
};

} // namespace Algebra

#endif // ALGEBRA_MONOMIAL_H
