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
    int power;

    Monomial(const Fraction& coeff = Fraction(0), std::string var = "", int p = 0);

    std::string toString() const;
};

} // namespace Algebra

#endif // ALGEBRA_MONOMIAL_H
