#include "radical.h"
#include <sstream>

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
    if (coefficient.getNumerator() == 0 || radicand.getNumerator() == 0) {
        return "0";
    }
    if (radicand.getNumerator() == 1 && radicand.getDenominator() == 1) {
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
    
    ss << "(" << radicand.toString() << ")^(1/2)";
    return ss.str();
}

} // namespace Algebra
