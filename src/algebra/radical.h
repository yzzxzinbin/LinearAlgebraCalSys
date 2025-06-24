#ifndef ALGEBRA_RADICAL_H
#define ALGEBRA_RADICAL_H

#include "../fraction.h"
#include <string>

namespace Algebra {

// 表示一个简化的根式: coefficient * (radicand)^(1/2)
struct SimplifiedRadical {
    Fraction coefficient;
    Fraction radicand;

    std::string toString() const;
};

// 将 sqrt(f) 化简为 k * sqrt(m) 的形式，其中 m 是无平方数因子的
SimplifiedRadical simplify_sqrt(const Fraction& f);

} // namespace Algebra

#endif // ALGEBRA_RADICAL_H
