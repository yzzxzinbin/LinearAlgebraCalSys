#ifndef ALGEBRA_POLYNOMIAL_H
#define ALGEBRA_POLYNOMIAL_H

#include "monomial.h"
#include <vector>
#include <string>

namespace Algebra {

/**
 * @class Polynomial
 * @brief 表示单变量多项式。
 */
class Polynomial {
public:
    explicit Polynomial(const std::string& expression = "");
    explicit Polynomial(const Monomial& m);

    std::string toString() const;
    std::string factor() const;

    Polynomial operator+(const Polynomial& other) const;
    Polynomial operator-(const Polynomial& other) const;
    Polynomial operator*(const Polynomial& other) const;

    // 新增：公有查询方法
    bool isConstant() const;
    bool isEmpty() const;
    SimplifiedRadical getConstantValue() const; // 如果不是常量则抛出异常
    bool hasOnlyRationalCoefficients() const;
    Fraction getDegree() const;
    size_t getTermCount() const;
    Monomial getMonomial() const;
    const std::vector<Monomial>& getTerms() const;
    std::vector<Polynomial> perform_factorization() const;

    // 新增：多项式除法和因式分解相关方法
    std::pair<Polynomial, Polynomial> divide(const Polynomial& divisor) const; // 返回 {商, 余}
    Polynomial synthetic_division(const Fraction& root) const; // 综合除法，返回商多项式
    Fraction evaluate(const Fraction& x) const; // 计算多项式在x处的值
    std::vector<Fraction> find_rational_roots() const; // 寻找所有可能的有理根
    std::vector<Fraction> get_all_factors(const Fraction& n) const; // 获取n的所有因数
    std::vector<Polynomial> complete_factorization() const; // 完整因式分解
    std::vector<std::string> solve_all_roots() const; // 求解所有根

private:
    std::vector<Monomial> terms;
    std::string variable_name;

    void parse(std::string expression);
    void simplify();

};

Polynomial pow(const Polynomial& base, int exp);

} // namespace Algebra

#endif // ALGEBRA_POLYNOMIAL_H
