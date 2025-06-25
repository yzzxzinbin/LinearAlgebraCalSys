#include "radical.h"
#include <sstream>
#include <stdexcept>

namespace Algebra {

// 辅助函数：化简整数的n次方根
// 返回 {k, m} 使得 n = k^degree * m 且 m 无degree次方数因子
static std::pair<BigInt, BigInt> simplify_integer_nth_root(BigInt n, long long degree) {
    if (n < 0 && degree % 2 == 0) {
        throw std::runtime_error("无法化简负数的偶次方根");
    }
    if (n == 0) return {0, 1};
    if (degree <= 0) throw std::runtime_error("根式的阶数必须为正数");
    
    BigInt k = 1;
    BigInt m = abs(n);
    
    // 检查因子 2
    BigInt two_power = static_cast<BigInt>(1) << degree; // 2^degree
    while (m % two_power == 0) {
        m /= two_power;
        k *= 2;
    }

    // 检查奇数因子
    for (BigInt i = 3; boost::multiprecision::pow(i, degree) <= m; i += 2) {
        BigInt i_power = boost::multiprecision::pow(i, degree);
        while (m % i_power == 0) {
            m /= i_power;
            k *= i;
        }
    }
    
    // 如果原数是负数且是奇次根，保持符号
    if (n < 0 && degree % 2 == 1) {
        k = -k;
    }
    
    return {k, m};
}

SimplifiedRadical::SimplifiedRadical(const Fraction& c, const Fraction& r, const Fraction& d) 
    : coefficient(c), radicand(r), degree(d) {
    if (d.getNumerator() <= 0) {
        throw std::runtime_error("根式的阶数必须为正数");
    }
}

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
    return simplify_nth_root(f, Fraction(2));
}

SimplifiedRadical simplify_nth_root(const Fraction& f, const Fraction& deg) {
    if (deg.getDenominator() != 1) {
        throw std::runtime_error("根式的阶数必须是整数");
    }
    
    long long degree = deg.getNumerator().convert_to<long long>();
    
    if (f.getNumerator() < 0 && degree % 2 == 0) {
        throw std::runtime_error("无法计算负数的偶次方根");
    }
    if (f.getNumerator() == 0) {
        return {Fraction(0), Fraction(1), deg};
    }

    // 有理化分母: nth_root(n/d) = nth_root(n*d^(degree-1)) / d
    BigInt rationalized_num = f.getNumerator() * boost::multiprecision::pow(f.getDenominator(), degree - 1);
    BigInt den = f.getDenominator();

    auto simplified_pair = simplify_integer_nth_root(rationalized_num, degree);
    BigInt outside_part = simplified_pair.first;
    BigInt inside_part = simplified_pair.second;

    return {Fraction(outside_part, den), Fraction(inside_part), deg};
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
    
    if (degree == Fraction(2)) {
        ss << "sqrt(" << radicand.toString() << ")";
    } else if (degree == Fraction(3)) {
        ss << "cbrt(" << radicand.toString() << ")";
    } else {
        ss << "root(" << degree.toString() << ", " << radicand.toString() << ")";
    }
    return ss.str();
}

SimplifiedRadical operator+(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    if (a.isZero()) return b;
    if (b.isZero()) return a;
    if (a.radicand != b.radicand || a.degree != b.degree) {
        throw std::runtime_error("Cannot add radicals with different radicands or degrees.");
    }
    return {a.coefficient + b.coefficient, a.radicand, a.degree};
}

SimplifiedRadical operator-(const SimplifiedRadical& a) {
    return {-a.coefficient, a.radicand, a.degree};
}

SimplifiedRadical operator-(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    return a + (-b);
}

SimplifiedRadical operator*(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    if (a.degree != b.degree) {
        // 不同阶的根式相乘比较复杂，暂时不支持
        throw std::runtime_error("Multiplication of radicals with different degrees is not supported.");
    }
    
    Fraction new_coeff = a.coefficient * b.coefficient;
    Fraction new_radicand_unsimplified = a.radicand * b.radicand;
    
    SimplifiedRadical simplified_part = simplify_nth_root(new_radicand_unsimplified, a.degree);

    return {new_coeff * simplified_part.coefficient, simplified_part.radicand, a.degree};
}

bool operator==(const SimplifiedRadical& a, const SimplifiedRadical& b) {
    return a.coefficient == b.coefficient && a.radicand == b.radicand && a.degree == b.degree;
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
    if (a.degree != b.degree) {
        return a.degree < b.degree;
    }
    if (a.radicand != b.radicand) {
        return a.radicand < b.radicand;
    }
    return a.coefficient < b.coefficient;
}

SimplifiedRadical pow_frac(const Fraction& base, const Fraction& exp) {
    if (exp.getDenominator() == 1) {
        return SimplifiedRadical(pow(base, exp.getNumerator().convert_to<long long>()));
    }

    // 处理分数指数 p/q
    BigInt p = exp.getNumerator();
    BigInt q = exp.getDenominator();
    
    // base^(p/q) = (base^p)^(1/q)
    if (p == 1) {
        // base^(1/q) = q-th root of base
        if (is_perfect_nth_root(base, q.convert_to<long long>())) {
            return SimplifiedRadical(nth_root(base, q.convert_to<long long>()));
        }
        return simplify_nth_root(base, Fraction(q));
    } else {
        // 先计算 base^p
        Fraction base_to_p = pow(base, p.convert_to<long long>());
        // 然后计算 (base^p)^(1/q)
        return simplify_nth_root(base_to_p, Fraction(q));
    }
}

SimplifiedRadical pow(const SimplifiedRadical& base, long long exp) {
    if (exp == 0) return SimplifiedRadical(Fraction(1));
    if (exp == 1) return base;
    if (base.isZero()) return SimplifiedRadical(Fraction(0));

    Fraction new_coeff_part = pow(base.coefficient, exp);
    
    // 处理根式部分：(radicand^(1/degree))^exp = radicand^(exp/degree)
    if (base.isRational()) {
        return SimplifiedRadical(new_coeff_part);
    }

    // 计算新的指数 exp/degree
    Fraction new_exp = Fraction(exp) / base.degree;
    
    if (new_exp.getDenominator() == 1) {
        // 指数是整数，直接计算
        Fraction radicand_part = pow(base.radicand, new_exp.getNumerator().convert_to<long long>());
        return SimplifiedRadical(new_coeff_part * radicand_part);
    } else {
        // 指数是分数，需要化简根式
        BigInt num = new_exp.getNumerator();
        BigInt den = new_exp.getDenominator();
        
        Fraction radicand_power = pow(base.radicand, num.convert_to<long long>());
        SimplifiedRadical result = simplify_nth_root(radicand_power, Fraction(den));
        
        return {new_coeff_part * result.coefficient, result.radicand, result.degree};
    }
}

} // namespace Algebra
