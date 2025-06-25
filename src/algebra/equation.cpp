#include "equation.h"
#include "../fraction.h"
#include "radical.h"
#include <sstream>
#include <vector>

namespace Algebra {

// 下标数字映射辅助函数
static std::string to_subscript(int n) {
    static const char* subs[] = {"₀","₁","₂","₃","₄","₅","₆","₇","₈","₉"};
    std::string result;
    if (n == 0) return subs[0];
    while (n > 0) {
        int digit = n % 10;
        result = subs[digit] + result;
        n /= 10;
    }
    return result;
}

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
        std::vector<std::string> sols = {solve_linear()};
        // 统一下标输出
        std::stringstream ss;
        for (size_t i = 0; i < sols.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << variable_name << to_subscript(i + 1) << " = " << sols[i];
        }
        return ss.str();
    }

    if (poly_form.getDegree() == Fraction(2)) {
        std::vector<std::string> sols = solve_quadratic_all();
        std::stringstream ss;
        for (size_t i = 0; i < sols.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << variable_name << to_subscript(i + 1) << " = " << sols[i];
        }
        return ss.str();
    }

    // 对于更高次，尝试因式分解
    std::vector<std::string> sols = solve_by_factoring_all();
    if (sols.empty()) return "无法求解（或无有理数解）";
    std::stringstream ss;
    for (size_t i = 0; i < sols.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << variable_name << to_subscript(i + 1) << " = " << sols[i];
    }
    return ss.str();
}

// 保留原有的单解接口供内部调用
std::string Equation::solve_linear() {
    // ax + b = 0
    SimplifiedRadical a, b(Fraction(0));
    for(const auto& term : poly_form.getTerms()) {
        if(term.power == Fraction(1)) a = term.coefficient;
        else if(term.power == Fraction(0)) b = term.coefficient;
    }
    
    if (!a.isRational() || !b.isRational()) {
        return "Linear equation solving with radical coefficients is not fully supported.";
    }
    
    Fraction sol = -b.getRationalValue() / a.getRationalValue();
    return sol.toString(); // 只返回解的值，不包含变量名
}

// 返回所有解的字符串（二次方程只有一个解）
std::vector<std::string> Equation::solve_quadratic_all() {
    // ax^2 + bx + c = 0
    SimplifiedRadical a, b(Fraction(0)), c(Fraction(0));
    for(const auto& term : poly_form.getTerms()) {
        if(term.power == Fraction(2)) a = term.coefficient;
        else if(term.power == Fraction(1)) b = term.coefficient;
        else if(term.power == Fraction(0)) c = term.coefficient;
    }

    if (!a.isRational() || !b.isRational() || !c.isRational()) {
        return { "Quadratic equation solving with radical coefficients is not fully supported." };
    }

    Fraction a_rat = a.getRationalValue();
    Fraction b_rat = b.getRationalValue();
    Fraction c_rat = c.getRationalValue();
    
    Fraction discriminant = b_rat*b_rat - Fraction(4)*a_rat*c_rat;

    if (is_perfect_square(discriminant)) {
        Fraction sqrt_d = sqrt(discriminant);
        Fraction r1 = (-b_rat + sqrt_d) / (Fraction(2)*a_rat);
        Fraction r2 = (-b_rat - sqrt_d) / (Fraction(2)*a_rat);
        if (r1 == r2) {
            return { r1.toString() };
        }
        return { r1.toString(), r2.toString() };
    } else {
        // 处理无理数解，使用任意阶根式
        Fraction two_a = Fraction(2) * a_rat;
        Fraction neg_b_over_2a = -b_rat / two_a;

        SimplifiedRadical sqrt_d_simplified = simplify_sqrt(discriminant);
        sqrt_d_simplified.coefficient = sqrt_d_simplified.coefficient / two_a;

        std::string s1, s2;
        if (neg_b_over_2a.getNumerator() != 0) {
            if (sqrt_d_simplified.coefficient.getNumerator() > 0) {
                s1 = neg_b_over_2a.toString() + " + " + sqrt_d_simplified.toString();
                s2 = neg_b_over_2a.toString() + " - " + sqrt_d_simplified.toString();
            } else {
                s1 = neg_b_over_2a.toString() + " - " + (-sqrt_d_simplified).toString();
                s2 = neg_b_over_2a.toString() + " + " + (-sqrt_d_simplified).toString();
            }
        } else {
            s1 = sqrt_d_simplified.toString();
            s2 = "-" + sqrt_d_simplified.toString();
        }
        return { s1, s2 };
    }
}

// 返回所有解的字符串（因式分解法）
std::vector<std::string> Equation::solve_by_factoring_all() {
    try {
        // 使用新的完整因式分解算法
        return poly_form.solve_all_roots();
    } catch (const std::exception&) {
        // 如果新算法失败，回退到原来的方法
        auto factors = poly_form.perform_factorization();
        std::vector<std::string> sols;
        for (const auto& f : factors) {
            if (f.getDegree() == Fraction(1) && f.hasOnlyRationalCoefficients()) {
                Fraction a, b(0);
                for(const auto& term : f.getTerms()) {
                    if(term.power == Fraction(1)) a = term.coefficient.getRationalValue();
                    else if(term.power == Fraction(0)) b = term.coefficient.getRationalValue();
                }
                Fraction sol = -b / a;
                sols.push_back(sol.toString());
            } else if (f.getDegree() > Fraction(0) && f.getTermCount() == 1) {
                sols.push_back("0");
            }
        }
        return sols;
    }
}

} // namespace Algebra
