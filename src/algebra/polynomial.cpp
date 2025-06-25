#include "polynomial.h"
#include "../fraction.h"
#include "radical.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <map>
#include <boost/multiprecision/integer.hpp>

namespace Algebra
{

    // 内部使用的递归下降解析器
    namespace
    {
        class ExpressionParser
        {
        public:
            ExpressionParser(const std::string &text, std::string &var_name_ref)
                : expression(text), pos(0), variable_name(var_name_ref) {}

            Polynomial parse()
            {
                if (expression.empty())
                    return Polynomial();
                Polynomial result = parse_expression();
                if (pos < expression.length())
                {
                    throw std::runtime_error("Unexpected character encountered in input.");
                }
                return result;
            }

        private:
            std::string expression;
            size_t pos;
            std::string &variable_name;

            char peek()
            {
                skip_whitespace();
                if (pos >= expression.length())
                    return '\0';
                return expression[pos];
            }

            char get()
            {
                skip_whitespace();
                if (pos >= expression.length())
                    return '\0';
                return expression[pos++];
            }

            void skip_whitespace()
            {
                while (pos < expression.length() && isspace(expression[pos]))
                {
                    pos++;
                }
            }

            Fraction parse_number()
            {
                skip_whitespace();
                size_t start_pos = pos; // For backtracking on error

                std::string sign;
                if (pos < expression.length() && (expression[pos] == '+' || expression[pos] == '-'))
                {
                    sign += expression[pos];
                    pos++;
                }

                skip_whitespace(); // Handle spaces like in "- 1"

                size_t num_start = pos;
                while (pos < expression.length() && (isdigit(expression[pos]) || expression[pos] == '/'))
                {
                    pos++;
                }

                if (pos == num_start)
                {                    // No digits were read
                    pos = start_pos; // backtrack
                    throw std::runtime_error("Invalid number format.");
                }

                std::string num_val = expression.substr(num_start, pos - num_start);
                return Fraction(sign + num_val);
            }

            Fraction parse_exponent()
            {
                skip_whitespace();
                if (peek() == '(')
                {
                    get(); // consume '('
                    // 递归调用 parse_expression 来处理括号内的完整表达式
                    Polynomial exponent_poly = parse_expression();
                    if (peek() != ')')
                    {
                        throw std::runtime_error("Mismatched parentheses in exponent.");
                    }
                    get(); // consume ')'

                    // 指数必须是一个可以求值为常数的表达式
                    if (!exponent_poly.isConstant())
                    {
                        throw std::runtime_error("Exponent must evaluate to a constant value.");
                    }

                    // 获取常数值并检查是否为有理数
                    SimplifiedRadical exponent_value = exponent_poly.getConstantValue();
                    if (!exponent_value.isRational())
                    {
                        throw std::runtime_error("Exponent must be a rational number, not a radical.");
                    }

                    return exponent_value.getRationalValue();
                }
                else
                {
                    // 对于没有括号的指数，只解析一个简单的数字
                    return parse_number();
                }
            }

            Polynomial parse_expression()
            {
                Polynomial result = parse_term();
                while (peek() == '+' || peek() == '-')
                {
                    char op = get();
                    if (op == '+')
                    {
                        result = result + parse_term();
                    }
                    else
                    {
                        result = result - parse_term();
                    }
                }
                return result;
            }

            Polynomial parse_term()
            {
                Polynomial result = parse_factor();
                while (peek() == '*')
                {
                    get(); // consume '*'
                    result = result * parse_factor();
                }
                return result;
            }

            Polynomial parse_factor()
            {
                Polynomial result = parse_primary();
                while (peek() == '^')
                {
                    get(); // consume '^'
                    Fraction exponent_frac = parse_exponent();

                    // If the base is a constant, we can handle fractional/negative exponents
                    if (result.isConstant())
                    {
                        if (result.isEmpty())
                        { // base is 0
                            if (exponent_frac.getNumerator() <= 0)
                            {
                                throw std::runtime_error("0 cannot be raised to a non-positive power.");
                            }
                            // 0 to a positive power is 0, result is already correct (empty polynomial)
                        }
                        else
                        { // base is a non-zero constant
                            SimplifiedRadical base_val = result.getConstantValue();
                            if (!base_val.isRational())
                            {
                                throw std::runtime_error("Raising a radical to a power is not supported yet.");
                            }
                            SimplifiedRadical new_coeff = pow_frac(base_val.getRationalValue(), exponent_frac);
                            result = Polynomial(Monomial(new_coeff, "", 0));
                        }
                    }
                    else
                    { // Base is a variable polynomial
                        // If it's a multi-term polynomial, e.g., (x+1), we can only handle positive integer exponents.
                        if (result.getTermCount() > 1)
                        {
                            if (exponent_frac.getDenominator() != 1)
                            {
                                throw std::runtime_error("Fractional exponents on multi-term polynomials are not supported.");
                            }
                            long long exponent;
                            try
                            {
                                exponent = exponent_frac.getNumerator().convert_to<long long>();
                            }
                            catch (const std::overflow_error &)
                            {
                                throw std::runtime_error("Exponent is too large.");
                            }
                            // This will call the function that throws on negative exponents.
                            result = pow(result, static_cast<int>(exponent));
                        }
                        else
                        { // It's a single-term polynomial (a monomial)
                            Monomial m = result.getMonomial();
                            Fraction new_power = m.power * exponent_frac;
                            SimplifiedRadical new_coeff = m.coefficient; // Start with original coefficient

                            if (m.coefficient != SimplifiedRadical(Fraction(1)))
                            {
                                // If coefficient is not 1, we need to raise it to the power too.
                                // This is only generally possible for integer exponents.
                                if (exponent_frac.getDenominator() != 1)
                                {
                                    // Could add special cases like sqrt here if needed
                                    throw std::runtime_error("Fractional exponents on non-unit coefficients are not supported.");
                                }
                                long long exponent;
                                try
                                {
                                    exponent = exponent_frac.getNumerator().convert_to<long long>();
                                }
                                catch (const std::overflow_error &)
                                {
                                    throw std::runtime_error("Exponent is too large.");
                                }
                                new_coeff = pow(m.coefficient, exponent);
                            }
                            // If coefficient is 1, it remains 1.

                            result = Polynomial(Monomial(new_coeff, m.variable, new_power));
                        }
                    }
                }
                return result;
            }

            Polynomial parse_primary()
            {
                char p = peek();
                if (p == '(')
                {
                    get(); // consume '('
                    Polynomial result = parse_expression();
                    if (get() != ')')
                    {
                        throw std::runtime_error("Mismatched parentheses.");
                    }
                    return result;
                }
                if (isdigit(p) || (p == '-' && isdigit(expression[pos + 1])))
                {
                    return Polynomial(Monomial(parse_number(), "", 0));
                }
                if (isalpha(p))
                {
                    return parse_variable_term();
                }
                if (p == '+')
                { // Unary plus
                    get();
                    return parse_primary();
                }
                if (p == '-')
                { // Unary minus
                    get();
                    return Polynomial(Monomial(Fraction(-1), "", 0)) * parse_primary();
                }
                throw std::runtime_error("Unexpected token in expression.");
            }

            Polynomial parse_variable_term()
            {
                std::string var_name = std::string(1, get()); // Consumes only the variable name

                if (variable_name.empty())
                {
                    variable_name = var_name;
                }
                else if (variable_name != var_name)
                {
                    throw std::runtime_error("Multi-variable polynomials not supported.");
                }

                // Exponent is always 1 here. It's handled by parse_factor.
                return Polynomial(Monomial(Fraction(1), var_name, Fraction(1)));
            }
        };
    } // anonymous namespace

    // Helper functions
    BigInt multi_gcd(const std::vector<BigInt> &numbers)
    {
        if (numbers.empty())
            return 0;
        BigInt result = numbers[0];
        for (size_t i = 1; i < numbers.size(); ++i)
        {
            result = boost::multiprecision::gcd(result, numbers[i]);
        }
        return result;
    }

    BigInt multi_lcm(const std::vector<BigInt> &numbers)
    {
        if (numbers.empty())
            return 1;
        BigInt result = numbers[0];
        for (size_t i = 1; i < numbers.size(); ++i)
        {
            if (numbers[i] == 0 || result == 0)
                return 0;
            result = (result * numbers[i]) / boost::multiprecision::gcd(result, numbers[i]);
        }
        return result;
    }

    Polynomial::Polynomial(const std::string &expression) : variable_name("")
    {
        if (!expression.empty())
        {
            parse(expression);
        }
    }

    Polynomial::Polynomial(const Monomial &m) : variable_name(m.variable)
    {
        if (!m.coefficient.isZero())
        {
            terms.push_back(m);
        }
    }

    void Polynomial::parse(std::string expr)
    {
        // 使用新的解析器
        ExpressionParser parser(expr, this->variable_name);
        Polynomial result = parser.parse();
        this->terms = result.terms;
        // variable_name is already set by reference in the parser
        simplify();
    }

    void Polynomial::simplify()
    {
        if (terms.empty())
            return;

        std::map<Fraction, std::vector<SimplifiedRadical>> power_to_coeffs;
        std::string var;
        for (const auto &term : terms)
        {
            if (!term.variable.empty())
            {
                var = term.variable;
            }
            power_to_coeffs[term.power].push_back(term.coefficient);
        }

        terms.clear();
        for (auto &pair : power_to_coeffs)
        {
            Fraction power = pair.first;
            auto &coeffs = pair.second;

            // 按根式的 (radicand, degree) 分组合并同类项
            std::map<std::pair<Fraction, Fraction>, Fraction> combined_coeffs;
            for (const auto &coeff : coeffs)
            {
                if (!coeff.isZero())
                {
                    auto key = std::make_pair(coeff.radicand, coeff.degree);
                    combined_coeffs[key] += coeff.coefficient;
                }
            }

            for (const auto &final_coeff_pair : combined_coeffs)
            {
                auto key = final_coeff_pair.first;
                Fraction coeff_part = final_coeff_pair.second;
                if (coeff_part.getNumerator() != 0)
                {
                    SimplifiedRadical simplified_coeff(coeff_part, key.first, key.second);
                    terms.emplace_back(simplified_coeff, var, power);
                }
            }
        }

        std::sort(terms.begin(), terms.end(), [](const Monomial &a, const Monomial &b)
                  { return a.power > b.power; });
    }

    Polynomial Polynomial::operator+(const Polynomial &other) const
    {
        Polynomial result = *this;
        for (const auto &term : other.terms)
        {
            result.terms.push_back(term);
        }
        if (result.variable_name.empty())
            result.variable_name = other.variable_name;
        result.simplify();
        return result;
    }

    Polynomial Polynomial::operator-(const Polynomial &other) const
    {
        Polynomial result = *this;
        for (const auto &term : other.terms)
        {
            result.terms.emplace_back(-term.coefficient, term.variable, term.power);
        }
        if (result.variable_name.empty())
            result.variable_name = other.variable_name;
        result.simplify();
        return result;
    }

    Polynomial Polynomial::operator*(const Polynomial &other) const
    {
        Polynomial result;
        if (this->variable_name.empty() && !other.variable_name.empty())
        {
            result.variable_name = other.variable_name;
        }
        else
        {
            result.variable_name = this->variable_name;
        }

        for (const auto &t1 : this->terms)
        {
            for (const auto &t2 : other.terms)
            {
                result.terms.emplace_back(
                    t1.coefficient * t2.coefficient,
                    result.variable_name,
                    t1.power + t2.power);
            }
        }
        result.simplify();
        return result;
    }

    Polynomial pow(const Polynomial &base, int exp)
    {
        if (exp < 0)
            throw std::runtime_error("Negative exponents on polynomials not supported.");
        if (exp == 0)
            return Polynomial("1");
        Polynomial result = base;
        for (int i = 1; i < exp; ++i)
        {
            result = result * base;
        }
        return result;
    }

    std::string Polynomial::toString() const
    {
        if (terms.empty())
            return "0";

        std::stringstream ss;
        bool first = true;
        for (const auto &term : terms)
        {
            SimplifiedRadical c = term.coefficient;

            if (!first)
            {
                if (c.coefficient.getNumerator() > 0)
                {
                    ss << " + ";
                }
                else
                {
                    ss << " - ";
                    c = -c;
                }
            }
            else if (c.coefficient.getNumerator() < 0)
            {
                ss << "-";
                c = -c;
            }

            bool coeff_is_one = c.isRational() && c.coefficient == Fraction(1);

            if (!coeff_is_one || term.power == Fraction(0))
            {
                ss << c.toString();
            }

            if (term.power != Fraction(0))
            {
                if (!coeff_is_one && !ss.str().empty() && ss.str().back() != '-')
                    ss << "*";
                ss << term.variable;
                if (term.power != Fraction(1))
                {
                    if (term.power.getDenominator() != 1 || term.power < Fraction(0))
                    {
                        ss << "^(" << term.power.toString() << ")";
                    }
                    else
                    {
                        ss << "^" << term.power.toString();
                    }
                }
            }
            first = false;
        }
        return ss.str();
    }

    // 新增：公有查询方法的实现
    bool Polynomial::isConstant() const
    {
        return getDegree() <= Fraction(0);
    }

    bool Polynomial::isEmpty() const
    {
        return terms.empty();
    }

    SimplifiedRadical Polynomial::getConstantValue() const
    {
        if (!isConstant())
        {
            throw std::logic_error("Polynomial is not a constant.");
        }
        if (isEmpty())
        {
            return SimplifiedRadical(Fraction(0));
        }
        return terms[0].coefficient;
    }

    bool Polynomial::hasOnlyRationalCoefficients() const
    {
        for (const auto &term : terms)
        {
            if (!term.coefficient.isRational())
            {
                return false;
            }
        }
        return true;
    }

    Fraction Polynomial::getDegree() const
    {
        if (terms.empty())
            return Fraction(-1);
        return terms.front().power;
    }

    size_t Polynomial::getTermCount() const
    {
        return terms.size();
    }

    Monomial Polynomial::getMonomial() const
    {
        if (terms.size() != 1)
        {
            throw std::logic_error("Polynomial is not a monomial.");
        }
        return terms[0];
    }

    const std::vector<Monomial> &Polynomial::getTerms() const
    {
        return terms;
    }

    std::vector<Polynomial> Polynomial::perform_factorization() const
    {
        if (!hasOnlyRationalCoefficients())
        {
            throw std::runtime_error("Factorization of polynomials with radical coefficients is not supported.");
        }
        std::vector<Polynomial> factors;
        if (terms.empty())
            return factors;

        Polynomial current_poly = *this;

        // 提取公因子（只处理有理系数）
        std::vector<BigInt> nums, dens;
        for (const auto &term : current_poly.terms)
        {
            if (term.coefficient.isRational())
            {
                nums.push_back(abs(term.coefficient.coefficient.getNumerator()));
                dens.push_back(term.coefficient.coefficient.getDenominator());
            }
        }

        if (!nums.empty())
        {
            BigInt num_gcd = multi_gcd(nums);
            BigInt den_lcm = multi_lcm(dens);
            Fraction common_coeff(num_gcd, den_lcm);

            Fraction min_power = current_poly.terms.empty() ? Fraction(0) : current_poly.terms.back().power;

            if (common_coeff.getNumerator() != 1 || common_coeff.getDenominator() != 1 || min_power > Fraction(0))
            {
                Polynomial common_factor;
                if (common_coeff.getNumerator() != 0)
                {
                    common_factor.terms.emplace_back(SimplifiedRadical(common_coeff), variable_name, min_power);
                    factors.push_back(common_factor);
                }

                Polynomial remaining_poly;
                for (const auto &term : current_poly.terms)
                {
                    if (term.coefficient.isRational())
                    {
                        SimplifiedRadical new_coeff(term.coefficient.coefficient / common_coeff);
                        remaining_poly.terms.emplace_back(new_coeff, variable_name, term.power - min_power);
                    }
                    else
                    {
                        // 对于无理系数，保持原样
                        remaining_poly.terms.emplace_back(term.coefficient, variable_name, term.power - min_power);
                    }
                }
                remaining_poly.simplify();
                current_poly = remaining_poly;
            }
        }

        // 二次方程因式分解（只处理有理系数）
        if (current_poly.getDegree() == Fraction(2) && current_poly.hasOnlyRationalCoefficients())
        {
            Fraction a, b, c;
            for (const auto &term : current_poly.terms)
            {
                if (term.power == Fraction(2))
                    a = term.coefficient.getRationalValue();
                else if (term.power == Fraction(1))
                    b = term.coefficient.getRationalValue();
                else if (term.power == Fraction(0))
                    c = term.coefficient.getRationalValue();
            }

            Fraction discriminant = b * b - Fraction(4) * a * c;
            if (is_perfect_square(discriminant))
            {
                Fraction sqrt_d = sqrt(discriminant);
                Fraction r1 = (-b + sqrt_d) / (Fraction(2) * a);
                Fraction r2 = (-b - sqrt_d) / (Fraction(2) * a);

                Polynomial factor_a;
                factor_a.terms.emplace_back(SimplifiedRadical(a), "", Fraction(0));
                factors.push_back(factor_a);

                Polynomial factor1;
                factor1.terms.emplace_back(SimplifiedRadical(Fraction(1)), variable_name, Fraction(1));
                factor1.terms.emplace_back(SimplifiedRadical(-r1), "", Fraction(0));
                factors.push_back(factor1);

                Polynomial factor2;
                factor2.terms.emplace_back(SimplifiedRadical(Fraction(1)), variable_name, Fraction(1));
                factor2.terms.emplace_back(SimplifiedRadical(-r2), "", Fraction(0));
                factors.push_back(factor2);

                return factors;
            }
        }

        if (!current_poly.terms.empty())
        {
            factors.push_back(current_poly);
        }
        return factors;
    }

    std::string Polynomial::factor() const
    {
        if (!hasOnlyRationalCoefficients())
        {
            return toString() + " (factorization with radicals not supported)";
        }
        if (getDegree() <= Fraction(0))
            return toString();

        auto factors = complete_factorization(); // 使用 complete_factorization()

        // 进一步处理剩余的因子，特别是二次因子
        std::vector<Polynomial> final_factors;
        for (const auto &factor : factors)
        {
            if (factor.getDegree() == Fraction(2))
            {
                // 对二次因子尝试进一步分解
                try
                {
                    auto sub_factors = factor.complete_factorization();
                    if (sub_factors.size() > 1)
                    {
                        // 二次因子可以进一步分解
                        for (const auto &sub_factor : sub_factors)
                        {
                            final_factors.push_back(sub_factor);
                        }
                    }
                    else
                    {
                        // 二次因子不能进一步分解
                        final_factors.push_back(factor);
                    }
                }
                catch (const std::exception &)
                {
                    // 如果分解失败，保持原因子
                    final_factors.push_back(factor);
                }
            }
            else
            {
                final_factors.push_back(factor);
            }
        }

        std::stringstream ss;
        bool first = true;

        for (const auto &f : final_factors)
        {
            if (f.terms.empty())
                continue;

            // 跳过系数为1的常数因子
            if (f.terms.size() == 1 && f.terms[0].power == Fraction(0) && f.terms[0].coefficient == SimplifiedRadical(Fraction(1)))
            {
                continue;
            }

            if (!first)
            {
                ss << " * ";
            }
            first = false;

            // 处理常数因子
            if (f.getDegree() == Fraction(0))
            {
                ss << f.toString();
            }
            else
            {
                // 处理线性和高次因子
                bool needs_paren = (f.getDegree() > Fraction(0) && f.terms.size() > 1);
                if (needs_paren)
                    ss << "(";
                ss << f.toString();
                if (needs_paren)
                    ss << ")";
            }
        }

        std::string result = ss.str();
        if (result.empty())
            return "1";
        return result;
    }
}