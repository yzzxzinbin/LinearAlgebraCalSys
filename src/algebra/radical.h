#ifndef ALGEBRA_RADICAL_H
#define ALGEBRA_RADICAL_H

#include "../fraction.h"
#include <string>

namespace Algebra {

// 表示一个简化的根式: coefficient * (radicand)^(1/degree)
struct SimplifiedRadical {
    Fraction coefficient;
    Fraction radicand;
    Fraction degree; // 根式的阶数，默认为2（平方根）

    SimplifiedRadical(const Fraction& c = 0, const Fraction& r = 1, const Fraction& d = 2);
    std::string toString() const;
    bool isZero() const;
    bool isRational() const;
    Fraction getRationalValue() const; // 如果不是有理数则抛出异常
};

// 运算符重载
SimplifiedRadical operator+(const SimplifiedRadical& a, const SimplifiedRadical& b);
SimplifiedRadical operator-(const SimplifiedRadical& a);
SimplifiedRadical operator-(const SimplifiedRadical& a, const SimplifiedRadical& b);
SimplifiedRadical operator*(const SimplifiedRadical& a, const SimplifiedRadical& b);
bool operator==(const SimplifiedRadical& a, const SimplifiedRadical& b);
bool operator!=(const SimplifiedRadical& a, const SimplifiedRadical& b);
bool operator==(const SimplifiedRadical& a, const Fraction& b);
bool operator!=(const SimplifiedRadical& a, const Fraction& b);
bool operator==(const Fraction& a, const SimplifiedRadical& b);
bool operator!=(const Fraction& a, const SimplifiedRadical& b);
bool operator<(const SimplifiedRadical& a, const SimplifiedRadical& b);


// 将 sqrt(f) 化简为 k * sqrt(m) 的形式，其中 m 是无平方数因子的
SimplifiedRadical simplify_sqrt(const Fraction& f);

// 新增：将 nth_root(f) 化简为 k * nth_root(m) 的形式，其中 m 是无n次方因子的
SimplifiedRadical simplify_nth_root(const Fraction& f, const Fraction& degree);

// 新增：分数的分数次幂运算，返回根式
SimplifiedRadical pow_frac(const Fraction& base, const Fraction& exp);
SimplifiedRadical pow(const SimplifiedRadical& base, long long exp);

} // namespace Algebra

#endif // ALGEBRA_RADICAL_H
