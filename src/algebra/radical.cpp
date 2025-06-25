#include "radical.h"
#include <sstream>
#include <stdexcept>

namespace Algebra {

// 辅助函数：化简整数的平方根
// 返回 {k, m} 使得 n = k^2 * m 且 m 无平方数因子
static std::pair<BigInt, BigInt> simplify_integer_sqrt(BigInt n) {
    if (n < 0) throw std::runtime_error("无法化简负数的平方根");
    if (n == 0) return {0, 1};
    BigInt k = 1;
    BigInt m = n;
    
    // 检查因子 2
    while (m % 4 == 0) {
        m /= 4;
        k *= 2;
    }

    // 检查奇数因子
    for (BigInt i = 3; i * i <= m; i += 2) {
        while (m % (i * i) == 0) {
            m /= (i * i);
            k *= i;
        }
    }
    return {k, m};
}

SimplifiedRadical::SimplifiedRadical(const Fraction& c, const Fraction& r) : coefficient(c), radicand(r) {}

bool SimplifiedRadical::isZero() const {
    return coefficient.getNumerator() == 0;
}

bool SimplifiedRadical::isRational() const {
    return radicand.getNumerator() == 1 && radicand.getDenominator() == 1;
}

Fraction SimplifiedRadical::getRationalValue() const {
    if (!isRational()) {
        throw std::logic_error("Radical is not a rational number.");
    }
    return coefficient;
}

SimplifiedRadical simplify_sqrt(const Fraction& f) {
    if (f.getNumerator() < 0) {
        throw std::runtime_error("无法计算负数的平方根。");
    }
    if (f.getNumerator() == 0) {
        return {Fraction(0), Fraction(1)};
    }

    // 有理化分母: sqrt(n/d) = sqrt(n*d) / d
    BigInt rationalized_num = f.getNumerator() * f.getDenominator();
    BigInt den = f.getDenominator();

    auto simplified_pair = simplify_integer_sqrt(rationalized_num);
    BigInt outside_part = simplified_pair.first;
    BigInt inside_part = simplified_pair.second;

    return {Fraction(outside_part, den), Fraction(inside_part)};
}

std::string SimplifiedRadical::toString() const {
    if (isZero()) {
        return "0";
    }
    if (isRational()) {
        return coefficient.toString();
    }

    std::stringstream ss;
    bool coeff_is_one = (coefficient.getNumerator() == 1 && coefficient.getDenominator() == 1);
    bool coeff_is_neg_one = (coefficient.getNumerator() == -1 && coefficient.getDenominator() == 1);

    if (coeff_is_neg_one) {
        ss << "-";
    } else if (!coeff_is_one) {
        ss << coefficient.toString() << "*";
    }
    
    ss << "sqrt(" << radicand.toString() << ")";
    return ss.str();
}

SimplifiedRadical operator+(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    if (a.isZero()) return b;
    if (b.isZero()) return a;
    if (a.radicand != b.radicand) {
        throw std::runtime_error("Cannot add radicals with different radicands.");
    }
    return {a.coefficient + b.coefficient, a.radicand};
}

SimplifiedRadical operator-(const SimplifiedRadical& a) {
    return {-a.coefficient, a.radicand};
}

SimplifiedRadical operator-(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    return a + (-b);
}

SimplifiedRadical operator*(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    Fraction new_coeff = a.coefficient * b.coefficient;
    Fraction new_radicand_unsimplified = a.radicand * b.radicand;
    
    SimplifiedRadical simplified_part = simplify_sqrt(new_radicand_unsimplified);

    return {new_coeff * simplified_part.coefficient, simplified_part.radicand};
}

bool operator==(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    return a.coefficient == b.coefficient && a.radicand == b.radicand;
}

bool operator!=(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    return !(a == b);
}

bool operator==(const SimplifiedRadical& a, const Fraction& b) {
    return a.isRational() && a.coefficient == b;
}

bool operator!=(const SimplifiedRadical& a, const Fraction& b) {
    return !(a == b);
}

bool operator==(const Fraction& a, const SimplifiedRadical& b) {
    return b == a;
}

bool operator!=(const Fraction& a, const SimplifiedRadical& b) {
    return !(a == b);
}

bool operator<(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    if (a.radicand != b.radicand) {
        return a.radicand < b.radicand;
    }
    return a.coefficient < b.coefficient;
}

SimplifiedRadical pow_frac(const Fraction& base, const Fraction& exp) {
    if (exp.getDenominator() == 1) {
        return SimplifiedRadical(pow(base, exp.getNumerator().convert_to<long long>()));
    }

    if (exp == Fraction(1, 2)) { // Special case for sqrt
        if (is_perfect_square(base)) {
            return SimplifiedRadical(sqrt(base));
        }
        return simplify_sqrt(base);
    }
    
    // For other n-th roots, we can extend this logic. For now, only support sqrt.
    throw std::runtime_error("Only square roots are supported for fractional exponents on constants.");
}

SimplifiedRadical pow(const SimplifiedRadical& base, long long exp) {
    if (exp == 0) return SimplifiedRadical(Fraction(1));
    if (exp == 1) return base;
    if (base.isZero()) return SimplifiedRadical(Fraction(0));

    Fraction new_coeff_part = pow(base.coefficient, exp);
    
    // Now handle the radicand part: (sqrt(r))^exp
    if (base.isRational()) {
        return SimplifiedRadical(new_coeff_part);
    }

    if (exp % 2 == 0) {
        // (sqrt(r))^(2k) = r^k
        Fraction radicand_part = pow(base.radicand, exp / 2);
        return SimplifiedRadical(new_coeff_part * radicand_part);
    } else {
        // (sqrt(r))^(2k+1) = r^k * sqrt(r)
        Fraction radicand_part = pow(base.radicand, (exp - 1) / 2);
        return SimplifiedRadical(new_coeff_part * radicand_part, base.radicand);
    }
}

} // namespace Algebra
