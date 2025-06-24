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
    Fraction getConstantValue() const; // 如果不是常量则抛出异常
    Fraction getDegree() const;
    size_t getTermCount() const;
    Monomial getMonomial() const;
    const std::vector<Monomial>& getTerms() const;
    std::vector<Polynomial> perform_factorization() const;

private:
    std::vector<Monomial> terms;
    std::string variable_name;

    void parse(std::string expression);
    void simplify();

};

Polynomial pow(const Polynomial& base, int exp);

} // namespace Algebra

#endif // ALGEBRA_POLYNOMIAL_H
