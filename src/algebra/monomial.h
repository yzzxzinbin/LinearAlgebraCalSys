#ifndef ALGEBRA_MONOMIAL_H
#define ALGEBRA_MONOMIAL_H

#include "../fraction.h"
#include "radical.h"
#include <string>

namespace Algebra {

/**
 * @class Monomial
 * @brief 表示单项式，如 3*x^2。
 */
struct Monomial {
    SimplifiedRadical coefficient;
    std::string variable;
    Fraction power;

    Monomial(const SimplifiedRadical& coeff = {}, std::string var = "", const Fraction& p = 0);
    Monomial(const Fraction& coeff, std::string var = "", const Fraction& p = 0);


    std::string toString() const;
};

} // namespace Algebra

#endif // ALGEBRA_MONOMIAL_H