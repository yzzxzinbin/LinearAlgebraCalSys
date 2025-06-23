#include "monomial.h"
#include <sstream>
#include <utility>

namespace Algebra {

Monomial::Monomial(const Fraction& coeff, std::string var, int p)
    : coefficient(coeff), variable(std::move(var)), power(p) {}

std::string Monomial::toString() const {
    if (coefficient.getNumerator() == 0) {
        return "0";
    }

    std::stringstream ss;
    bool coeff_is_one = (coefficient.getNumerator() == 1 && coefficient.getDenominator() == 1);
    bool coeff_is_neg_one = (coefficient.getNumerator() == -1 && coefficient.getDenominator() == 1);

    if (power == 0) {
        return coefficient.toString();
    }

    if (coeff_is_neg_one) {
        ss << "-";
    } else if (!coeff_is_one) {
        ss << coefficient.toString() << "*";
    }

    ss << variable;
    if (power > 1) {
        ss << "^" << power;
    }

    return ss.str();
}

} // namespace Algebra
