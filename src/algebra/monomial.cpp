#include "monomial.h"
#include <sstream>
#include <utility>

namespace Algebra {

Monomial::Monomial(const SimplifiedRadical& coeff, std::string var, const Fraction& p)
    : coefficient(coeff), variable(std::move(var)), power(p) {}

Monomial::Monomial(const Fraction& coeff, std::string var, const Fraction& p)
    : coefficient(SimplifiedRadical(coeff)), variable(std::move(var)), power(p) {}


std::string Monomial::toString() const {
    if (coefficient.isZero()) {
        return "0";
    }

    std::stringstream ss;

    if (power == Fraction(0)) {
        return coefficient.toString();
    }

    if (coefficient.toString() == "-1") {
        ss << "-";
    } else if (coefficient.toString() != "1") {
        ss << coefficient.toString() << "*";
    }

    ss << variable;
    if (power != Fraction(1)) {
        // 对于分数或负数指数，使用括号
        if (power.getDenominator() != 1 || power < Fraction(0)) {
            ss << "^(" << power.toString() << ")";
        } else {
            ss << "^" << power.toString();
        }
    }

    return ss.str();
}

} // namespace Algebra
