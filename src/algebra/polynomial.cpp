#include "polynomial.h"
#include "../fraction.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <map>
#include <boost/multiprecision/integer.hpp>

namespace Algebra {

// Helper functions
BigInt multi_gcd(const std::vector<BigInt>& numbers) {
    if (numbers.empty()) return 0;
    BigInt result = numbers[0];
    for (size_t i = 1; i < numbers.size(); ++i) {
        result = boost::multiprecision::gcd(result, numbers[i]);
    }
    return result;
}

BigInt multi_lcm(const std::vector<BigInt>& numbers) {
    if (numbers.empty()) return 1;
    BigInt result = numbers[0];
    for (size_t i = 1; i < numbers.size(); ++i) {
        if (numbers[i] == 0 || result == 0) return 0;
        result = (result * numbers[i]) / boost::multiprecision::gcd(result, numbers[i]);
    }
    return result;
}

Polynomial::Polynomial(const std::string& expression) : variable_name("") {
    if (!expression.empty()) {
        parse(expression);
        simplify();
    }
}

void Polynomial::parse(std::string expr) {
    expr.erase(std::remove_if(expr.begin(), expr.end(), ::isspace), expr.end());
    if (expr.empty()) return;

    for (size_t i = 1; i < expr.length(); ++i) {
        if (expr[i] == '-' && expr[i-1] != '^') {
            expr.replace(i, 1, "+-");
            i++;
        }
    }

    std::stringstream ss(expr);
    std::string term_str;
    while (std::getline(ss, term_str, '+')) {
        if (term_str.empty()) continue;

        size_t var_pos = term_str.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        
        Fraction coeff(1);
        std::string var_name = "";
        int power = 0;

        if (var_pos != std::string::npos) {
            var_name = term_str.substr(var_pos, 1);
            if (variable_name.empty()) variable_name = var_name;
            else if (variable_name != var_name) throw std::runtime_error("Multi-variable polynomials not supported.");

            size_t pow_pos = term_str.find('^');
            power = (pow_pos != std::string::npos) ? std::stoi(term_str.substr(pow_pos + 1)) : 1;
            
            std::string coeff_str = term_str.substr(0, var_pos);
            if (coeff_str.empty() || coeff_str == "+") {
                coeff = 1;
            } else if (coeff_str == "-") {
                coeff = -1;
            } else {
                if (coeff_str.back() == '*') coeff_str.pop_back();
                coeff = Fraction(coeff_str);
            }
        } else {
            coeff = Fraction(term_str);
            power = 0;
        }
        
        if (coeff.getNumerator() != 0) {
            terms.emplace_back(coeff, var_name, power);
        }
    }
    if (variable_name.empty()) {
        for(const auto& term : terms) {
            if(term.power > 0) {
                variable_name = term.variable;
                break;
            }
        }
    }
}

void Polynomial::simplify() {
    if (terms.empty()) return;

    std::map<int, Fraction> power_to_coeff;
    std::string var;
    for (const auto& term : terms) {
        if (!term.variable.empty()) {
            var = term.variable;
        }
        power_to_coeff[term.power] += term.coefficient;
    }

    terms.clear();
    for (const auto& pair : power_to_coeff) {
        if (pair.second.getNumerator() != 0) {
            terms.emplace_back(pair.second, var, pair.first);
        }
    }

    std::sort(terms.begin(), terms.end(), [](const Monomial& a, const Monomial& b) {
        return a.power > b.power;
    });
}

std::string Polynomial::toString() const {
    if (terms.empty()) return "0";

    std::stringstream ss;
    bool first = true;
    for (const auto& term : terms) {
        Fraction c = term.coefficient;
        
        if (!first) {
            if (c.getNumerator() > 0) {
                ss << " + ";
            } else {
                ss << " - ";
                c = -c;
            }
        } else if (c.getNumerator() < 0) {
            ss << "-";
            c = -c;
        }
        
        bool coeff_is_one = (c.getNumerator() == 1 && c.getDenominator() == 1);
        
        if (!coeff_is_one || term.power == 0) {
            ss << c.toString();
        }
        
        if (term.power > 0) {
            if (!coeff_is_one && !ss.str().empty() && ss.str().back() != '-') ss << "*";
            ss << term.variable;
            if (term.power > 1) ss << "^" << term.power;
        }
        first = false;
    }
    return ss.str();
}

int Polynomial::getDegree() const {
    if (terms.empty()) return -1;
    return terms.front().power;
}

std::vector<Polynomial> Polynomial::perform_factorization() const {
    std::vector<Polynomial> factors;
    if (terms.empty()) return factors;

    Polynomial current_poly = *this;

    std::vector<BigInt> nums, dens;
    for(const auto& term : current_poly.terms) {
        nums.push_back(abs(term.coefficient.getNumerator()));
        dens.push_back(term.coefficient.getDenominator());
    }
    BigInt num_gcd = multi_gcd(nums);
    BigInt den_lcm = multi_lcm(dens);
    Fraction common_coeff(num_gcd, den_lcm);

    int min_power = current_poly.terms.empty() ? 0 : current_poly.terms.back().power;

    if (common_coeff.getNumerator() != 1 || common_coeff.getDenominator() != 1 || min_power > 0) {
        Polynomial common_factor;
        if (common_coeff.getNumerator() != 0) {
             common_factor.terms.emplace_back(common_coeff, variable_name, min_power);
             factors.push_back(common_factor);
        }

        Polynomial remaining_poly;
        for (const auto& term : current_poly.terms) {
            remaining_poly.terms.emplace_back(term.coefficient / common_coeff, variable_name, term.power - min_power);
        }
        remaining_poly.simplify();
        current_poly = remaining_poly;
    }

    if (current_poly.getDegree() == 2) {
        Fraction a, b, c;
        for(const auto& term : current_poly.terms) {
            if(term.power == 2) a = term.coefficient;
            else if(term.power == 1) b = term.coefficient;
            else if(term.power == 0) c = term.coefficient;
        }
        
        Fraction discriminant = b*b - Fraction(4)*a*c;
        if (is_perfect_square(discriminant)) {
            Fraction sqrt_d = sqrt(discriminant);
            Fraction r1 = (-b + sqrt_d) / (Fraction(2)*a);
            Fraction r2 = (-b - sqrt_d) / (Fraction(2)*a);

            Polynomial factor_a;
            factor_a.terms.emplace_back(a, "", 0);
            factors.push_back(factor_a);

            Polynomial factor1;
            factor1.terms.emplace_back(1, variable_name, 1);
            factor1.terms.emplace_back(-r1, "", 0);
            factors.push_back(factor1);

            Polynomial factor2;
            factor2.terms.emplace_back(1, variable_name, 1);
            factor2.terms.emplace_back(-r2, "", 0);
            factors.push_back(factor2);
            
            return factors;
        }
    }
    
    if (!current_poly.terms.empty()) {
        factors.push_back(current_poly);
    }
    return factors;
}

std::string Polynomial::factor() const {
    if (getDegree() <= 0) return toString();

    auto factors = perform_factorization();
    std::stringstream ss;
    for (size_t i = 0; i < factors.size(); ++i) {
        const auto& f = factors[i];
        if (f.terms.empty()) continue;
        if (f.terms.size() == 1 && f.terms[0].power == 0 && f.terms[0].coefficient == Fraction(1)) continue;

        bool needs_paren = (f.getDegree() > 0 && f.terms.size() > 1);
        if (needs_paren) ss << "(";
        ss << f.toString();
        if (needs_paren) ss << ")";
        
        if (i < factors.size() - 1) {
            const auto& next_f = factors[i+1];
            if (next_f.terms.empty()) continue;
            if (next_f.terms.size() == 1 && next_f.terms[0].power == 0 && next_f.terms[0].coefficient == Fraction(1)) continue;
            ss << " * ";
        }
    }
    std::string result = ss.str();
    if (result.empty()) return "1";
    if (result.rfind(" * ") == result.length() - 3) {
        result = result.substr(0, result.length() - 3);
    }
    return result;
}

std::string Polynomial::solve() const {
    if (getDegree() <= 0) {
        if (terms.empty() || terms[0].coefficient.getNumerator() != 0) return "无解";
        return variable_name + " 可以是任意数";
    }

    auto factors = perform_factorization();
    std::stringstream ss;
    bool first_sol = true;

    for (const auto& f : factors) {
        if (f.getDegree() == 1) {
            Fraction a, b;
            for(const auto& term : f.terms) {
                if(term.power == 1) a = term.coefficient;
                else if(term.power == 0) b = term.coefficient;
            }
            Fraction sol = -b / a;
            if (!first_sol) ss << ", ";
            ss << variable_name << " = " << sol.toString();
            first_sol = false;
        } else if (f.getDegree() > 0 && f.terms.size() == 1) {
            if (!first_sol) ss << ", ";
            ss << variable_name << " = 0";
            first_sol = false;
        }
    }
    if (first_sol) return "无法求解（或无有理数解）";
    return ss.str();
}

} // namespace Algebra
