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

    // 处理系数显示
    bool coeff_is_one = (coefficient.coefficient.getNumerator() == 1 &&
                        coefficient.coefficient.getDenominator() == 1 &&
                        coefficient.isRational());
    bool coeff_is_neg_one = (coefficient.coefficient.getNumerator() == -1 &&
                             coefficient.coefficient.getDenominator() == 1 &&
                             coefficient.isRational());

    if (coeff_is_neg_one) {
        ss << "-";
    } else if (!coeff_is_one) {
        ss << coefficient.toString();
        if (!variable.empty()) {
            ss << "*";
        }
    }

    // 处理变量部分
    if (!variable.empty()) {
        ss << variable;
        if (power != Fraction(1)) {
            // 对于分数或负数指数，使用括号
            if (power.getDenominator() != 1 || power < Fraction(0)) {
                ss << "^(" << power.toString() << ")";
            } else {
                ss << "^" << power.toString();
            }
        }
    }

    return ss.str();
}

// 新增：单项式运算符重载
Monomial operator+(const Monomial& a, const Monomial& b) {
    if (a.variable != b.variable || a.power != b.power) {
        throw std::runtime_error("Cannot add monomials with different variables or powers.");
    }
    return Monomial(a.coefficient + b.coefficient, a.variable, a.power);
}

Monomial operator-(const Monomial& a) {
    return Monomial(-a.coefficient, a.variable, a.power);
}

Monomial operator*(const Monomial& a, const Monomial& b) {
    std::string result_var = a.variable.empty() ? b.variable : a.variable;
    if (!a.variable.empty() && !b.variable.empty() && a.variable != b.variable) {
        throw std::runtime_error("Cannot multiply monomials with different variables.");
    }
    return Monomial(a.coefficient * b.coefficient, result_var, a.power + b.power);
}

bool operator==(const Monomial& a, const Monomial& b) {
    return a.coefficient == b.coefficient && a.variable == b.variable && a.power == b.power;
}

// 新增：单项式幂运算
Monomial pow(const Monomial& base, int exp) {
    if (exp < 0) {
        throw std::runtime_error("Negative exponents on monomials are not supported in this context.");
    }
    if (exp == 0) {
        return Monomial(Fraction(1), "", 0);
    }
    if (exp == 1) {
        return base;
    }
    if (base.coefficient.isZero()) {
        if (exp > 0) return Monomial(Fraction(0), "", 0);
        else throw std::runtime_error("0^0 or 0 to a negative power is undefined.");
    }

    SimplifiedRadical new_coeff = pow(base.coefficient, static_cast<long long>(exp));
    Fraction new_power = base.power * Fraction(exp);
    return Monomial(new_coeff, base.variable, new_power);
}

} // namespace Algebra
