#ifndef ALGEBRA_EQUATION_H
#define ALGEBRA_EQUATION_H

#include "polynomial.h"
#include <string>
#include <vector>

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
    std::vector<std::string> solve_quadratic_all();
    std::vector<std::string> solve_by_factoring_all();
};

} // namespace Algebra

#endif // ALGEBRA_EQUATION_H
