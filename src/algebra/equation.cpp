#include "equation.h"
#include "../fraction.h"
#include "radical.h"
#include <sstream>
#include <vector>

namespace Algebra {

Equation::Equation(const std::string& expr) {
    parse(expr);
}

void Equation::parse(const std::string& expr) {
    size_t equals_pos = expr.find("==");
    if (equals_pos == std::string::npos) {
        equals_pos = expr.find("=");
    }

    if (equals_pos == std::string::npos) {
        // 没有 "="，视为 "expr = 0"
        poly_form = Polynomial(expr);
    } else {
        std::string lhs_str = expr.substr(0, equals_pos);
        std::string rhs_str = expr.substr(equals_pos + (expr[equals_pos+1] == '=' ? 2 : 1));
        
        Polynomial lhs(lhs_str);
        Polynomial rhs(rhs_str);

        poly_form = lhs - rhs; // 移项，得到 P(x) = 0 的形式
    }
    
    // 从结果多项式中提取变量名
    if (!poly_form.getTerms().empty()) {
        for(const auto& term : poly_form.getTerms()) {
            if (!term.variable.empty()) {
                variable_name = term.variable;
                break;
            }
        }
    }
}

std::string Equation::solve() {
    if (!poly_form.hasOnlyRationalCoefficients()) {
        return "Equation solving with radical coefficients is not supported.";
    }
    if (poly_form.getDegree() <= Fraction(0)) {
        if (poly_form.isEmpty() || poly_form.getConstantValue().coefficient.getNumerator() == 0) {
            // 0 = 0
            return variable_name.empty() ? "恒等式成立" : (variable_name + " 可以是任意数");
        } else {
            // c = 0, c != 0
            return "无解 (矛盾方程)";
        }
    }

    if (poly_form.getDegree() == Fraction(1)) {
        return solve_linear();
    }

    if (poly_form.getDegree() == Fraction(2)) {
        return solve_quadratic();
    }

    // 对于更高次，尝试因式分解
    return solve_by_factoring();
}

std::string Equation::solve_linear() {
    // ax + b = 0
    Fraction a, b(0);
    for(const auto& term : poly_form.getTerms()) {
        if(term.power == Fraction(1)) a = term.coefficient.getRationalValue();
        else if(term.power == Fraction(0)) b = term.coefficient.getRationalValue();
    }
    Fraction sol = -b / a;
    return variable_name + " = " + sol.toString();
}

std::string Equation::solve_quadratic() {
    // ax^2 + bx + c = 0
    Fraction a, b(0), c(0);
    for(const auto& term : poly_form.getTerms()) {
        if(term.power == Fraction(2)) a = term.coefficient.getRationalValue();
        else if(term.power == Fraction(1)) b = term.coefficient.getRationalValue();
        else if(term.power == Fraction(0)) c = term.coefficient.getRationalValue();
    }

    Fraction discriminant = b*b - Fraction(4)*a*c;

    if (is_perfect_square(discriminant)) {
        Fraction sqrt_d = sqrt(discriminant);
        Fraction r1 = (-b + sqrt_d) / (Fraction(2)*a);
        Fraction r2 = (-b - sqrt_d) / (Fraction(2)*a);
        if (r1 == r2) {
            return variable_name + " = " + r1.toString();
        }
        return variable_name + " = " + r1.toString() + ", " + variable_name + " = " + r2.toString();
    } else {
        // 处理无理数解
        std::stringstream ss;
        Fraction two_a = Fraction(2) * a;
        Fraction neg_b_over_2a = -b / two_a;

        SimplifiedRadical sqrt_d_simplified = simplify_sqrt(discriminant);
        sqrt_d_simplified.coefficient = sqrt_d_simplified.coefficient / two_a;

        // 解1: -b/2a + sqrt(D)/2a
        ss << variable_name << " = ";
        if (neg_b_over_2a.getNumerator() != 0) {
            ss << neg_b_over_2a.toString();
            if (sqrt_d_simplified.coefficient.getNumerator() > 0) {
                ss << " + ";
            } else {
                ss << " - ";
                sqrt_d_simplified.coefficient = -sqrt_d_simplified.coefficient;
            }
        }
        ss << sqrt_d_simplified.toString();

        // 解2: -b/2a - sqrt(D)/2a
        ss << ", " << variable_name << " = ";
        if (neg_b_over_2a.getNumerator() != 0) {
            ss << neg_b_over_2a.toString() << " - ";
        } else {
            ss << "-";
        }
        ss << sqrt_d_simplified.toString();
        
        return ss.str();
    }
}

std::string Equation::solve_by_factoring() {
    auto factors = poly_form.perform_factorization();
    std::stringstream ss;
    bool first_sol = true;

    for (const auto& f : factors) {
        if (f.getDegree() == Fraction(1)) {
            Fraction a, b(0);
            for(const auto& term : f.getTerms()) {
                if(term.power == Fraction(1)) a = term.coefficient.getRationalValue();
                else if(term.power == Fraction(0)) b = term.coefficient.getRationalValue();
            }
            Fraction sol = -b / a;
            if (!first_sol) ss << ", ";
            ss << variable_name << " = " << sol.toString();
            first_sol = false;
        } else if (f.getDegree() > Fraction(0) && f.getTermCount() == 1) {
            if (!first_sol) ss << ", ";
            ss << variable_name << " = 0";
            first_sol = false;
        }
    }
    if (first_sol) return "无法求解（或无有理数解）";
    return ss.str();
}

} // namespace Algebra
