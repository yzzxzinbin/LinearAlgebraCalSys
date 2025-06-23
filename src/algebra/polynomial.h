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

    std::string toString() const;
    std::string factor() const;
    std::string solve() const;

private:
    std::vector<Monomial> terms;
    std::string variable_name;

    void parse(std::string expression);
    void simplify();
    int getDegree() const;

    std::vector<Polynomial> perform_factorization() const;
};

} // namespace Algebra

#endif // ALGEBRA_POLYNOMIAL_H
