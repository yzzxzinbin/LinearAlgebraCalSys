#ifndef ALGEBRA_EQUATION_H
#define ALGEBRA_EQUATION_H

#include "polynomial.h"
#include <string>

namespace Algebra {

class Equation {
public:
    explicit Equation(const std::string& expr);

    std::string solve();

private:
    Polynomial poly_form; // 方程的标准形式 P(x) = 0
    std::string variable_name;

    void parse(const std::string& expr);
    std::string solve_linear();
    std::string solve_quadratic();
    std::string solve_by_factoring();
};

} // namespace Algebra

#endif // ALGEBRA_EQUATION_H
