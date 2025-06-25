#include "polynomial.h"
#include "../fraction.h"
#include "radical.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <boost/multiprecision/integer.hpp>

namespace Algebra
{
    // 计算多项式在x处的值
    Fraction Polynomial::evaluate(const Fraction &x) const
    {
        if (terms.empty())
            return Fraction(0);

        Fraction result(0);
        for (const auto &term : terms)
        {
            if (!term.coefficient.isRational())
            {
                throw std::runtime_error("Cannot evaluate polynomial with radical coefficients.");
            }

            Fraction term_value = term.coefficient.getRationalValue();
            if (term.power.getDenominator() != 1)
            {
                throw std::runtime_error("Cannot evaluate polynomial with fractional powers.");
            }

            long long power = term.power.getNumerator().convert_to<long long>();
            if (power > 0)
            {
                term_value = term_value * pow(x, power);
            }
            result = result + term_value;
        }
        return result;
    }

    // 综合除法 - 用于除以 (x - root)
    Polynomial Polynomial::synthetic_division(const Fraction &root) const
    {
        if (!hasOnlyRationalCoefficients())
        {
            throw std::runtime_error("Synthetic division requires rational coefficients.");
        }
        if (terms.empty())
            return Polynomial();

        // 将多项式按降幂排列，并填充缺失的项
        if (getDegree().getDenominator() != 1 || getDegree() < Fraction(1))
        {
            throw std::runtime_error("Synthetic division requires positive integer degree.");
        }

        long long degree = getDegree().getNumerator().convert_to<long long>();
        std::vector<Fraction> coeffs(degree + 1, Fraction(0));

        // 填充系数数组
        for (const auto &term : terms)
        {
            if (term.power.getDenominator() != 1)
                continue;
            long long power = term.power.getNumerator().convert_to<long long>();
            if (power >= 0 && power <= degree)
            {
                coeffs[degree - power] = term.coefficient.getRationalValue();
            }
        }

        // 执行综合除法
        if (degree <= 0)
            return Polynomial();
        std::vector<Fraction> result_coeffs(degree);
        result_coeffs[0] = coeffs[0];

        for (size_t i = 1; i < static_cast<size_t>(degree); ++i)
        {
            result_coeffs[i] = coeffs[i] + root * result_coeffs[i - 1];
        }

        // 计算余数，但不抛出异常（允许小的数值误差）
        Fraction remainder = coeffs[degree] + root * result_coeffs[degree - 1];
        // 只在余数确实很大时才抛出异常
        if (abs(remainder.getNumerator()) > 1000)
        {
            throw std::runtime_error("Division remainder is too large - likely not a valid root.");
        }

        // 构造结果多项式
        Polynomial result;
        result.variable_name = variable_name;
        for (size_t i = 0; i < result_coeffs.size(); ++i)
        {
            if (result_coeffs[i].getNumerator() != 0)
            {
                long long power = degree - 1 - static_cast<long long>(i);
                result.terms.emplace_back(SimplifiedRadical(result_coeffs[i]), variable_name, Fraction(power));
            }
        }

        result.simplify();
        return result;
    }

    // 获取整数n的所有因数（正负）
    std::vector<Fraction> Polynomial::get_all_factors(const Fraction &n) const
    {
        std::vector<Fraction> factors;
        if (n.getDenominator() != 1)
            return factors;

        BigInt num = abs(n.getNumerator());
        if (num == 0)
            return {Fraction(0)};

        for (BigInt i = 1; i * i <= num; ++i)
        {
            if (num % i == 0)
            {
                factors.push_back(Fraction(i));
                factors.push_back(Fraction(-i));
                if (i != num / i)
                {
                    factors.push_back(Fraction(num / i));
                    factors.push_back(Fraction(-(num / i)));
                }
            }
        }

        return factors;
    }

    // 寻找所有可能的有理根（有理根定理）
    std::vector<Fraction> Polynomial::find_rational_roots() const
    {
        if (!hasOnlyRationalCoefficients())
        {
            throw std::runtime_error("Rational root theorem requires rational coefficients.");
        }
        if (terms.empty())
            return {};

        // 获取首项系数和末项系数
        Fraction leading_coeff, constant_term(0);
        bool has_leading = false, has_constant = false;

        for (const auto &term : terms)
        {
            if (term.power == getDegree())
            {
                leading_coeff = term.coefficient.getRationalValue();
                has_leading = true;
            }
            if (term.power == Fraction(0))
            {
                constant_term = term.coefficient.getRationalValue();
                has_constant = true;
            }
        }

        if (!has_leading)
            return {};
        if (!has_constant || constant_term.getNumerator() == 0)
        {
            return {Fraction(0)}; // 如果没有常数项或常数项为0，0是根
        }

        // 获取常数项的因数和首项系数的因数
        auto p_factors = get_all_factors(constant_term);
        auto q_factors = get_all_factors(leading_coeff);

        // 生成所有可能的有理根 p/q
        std::vector<Fraction> possible_roots;
        for (const auto &p : p_factors)
        {
            for (const auto &q : q_factors)
            {
                if (q.getNumerator() != 0)
                {
                    Fraction root = p / q;
                    // 避免重复
                    bool found = false;
                    for (const auto &existing : possible_roots)
                    {
                        if (root == existing)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        possible_roots.push_back(root);
                    }
                }
            }
        }

        // 对可能的根进行排序，优先测试小的整数根
        std::sort(possible_roots.begin(), possible_roots.end(), [](const Fraction &a, const Fraction &b)
                  {
        // 整数优先，然后按绝对值排序
        bool a_is_int = (a.getDenominator() == 1);
        bool b_is_int = (b.getDenominator() == 1);
        if (a_is_int && !b_is_int) return true;
        if (!a_is_int && b_is_int) return false;
        return abs(a.getNumerator()) < abs(b.getNumerator()); });

        return possible_roots;
    }

    // 求解所有根
    std::vector<std::string> Polynomial::solve_all_roots() const
    {
        std::vector<std::string> roots;

        try
        {
            auto factors = complete_factorization();

            for (const auto &factor : factors)
            {
                if (factor.getDegree() == Fraction(0))
                {
                    // 常数因子，跳过
                    continue;
                }
                else if (factor.getDegree() == Fraction(1))
                {
                    // 线性因子：ax + b = 0
                    Fraction a, b(0);
                    for (const auto &term : factor.getTerms())
                    {
                        if (term.power == Fraction(1))
                            a = term.coefficient.getRationalValue();
                        else if (term.power == Fraction(0))
                            b = term.coefficient.getRationalValue();
                    }
                    if (a.getNumerator() != 0)
                    {
                        roots.push_back((-b / a).toString());
                    }
                }
                else if (factor.getDegree() == Fraction(2))
                {
                    // 二次因子，使用求根公式
                    Fraction a, b(0), c(0);
                    for (const auto &term : factor.getTerms())
                    {
                        if (term.power == Fraction(2))
                            a = term.coefficient.getRationalValue();
                        else if (term.power == Fraction(1))
                            b = term.coefficient.getRationalValue();
                        else if (term.power == Fraction(0))
                            c = term.coefficient.getRationalValue();
                    }

                    if (a.getNumerator() != 0)
                    {
                        Fraction discriminant = b * b - Fraction(4) * a * c;

                        if (discriminant.getNumerator() == 0)
                        {
                            // 重根，只添加一个解
                            Fraction root = -b / (Fraction(2) * a);
                            roots.push_back(root.toString());
                            roots.push_back(root.toString()); // 二次重根，添加两次
                        }
                        else if (is_perfect_square(discriminant))
                        {
                            // 有理根
                            Fraction sqrt_d = sqrt(discriminant);
                            Fraction r1 = (-b + sqrt_d) / (Fraction(2) * a);
                            Fraction r2 = (-b - sqrt_d) / (Fraction(2) * a);
                            roots.push_back(r1.toString());
                            roots.push_back(r2.toString());
                        }
                        else
                        {
                            // 无理根
                            Fraction two_a = Fraction(2) * a;
                            Fraction neg_b_over_2a = -b / two_a;

                            SimplifiedRadical sqrt_d_simplified = simplify_sqrt(discriminant);
                            sqrt_d_simplified.coefficient = sqrt_d_simplified.coefficient / two_a;

                            std::string s1, s2;
                            if (neg_b_over_2a.getNumerator() != 0)
                            {
                                s1 = neg_b_over_2a.toString() + " + " + sqrt_d_simplified.toString();
                                s2 = neg_b_over_2a.toString() + " - " + sqrt_d_simplified.toString();
                            }
                            else
                            {
                                s1 = sqrt_d_simplified.toString();
                                s2 = "-" + sqrt_d_simplified.toString();
                            }
                            roots.push_back(s1);
                            roots.push_back(s2);
                        }
                    }
                }
                else
                {
                    // 更高次的不可分解因子
                    long long degree = factor.getDegree().getNumerator().convert_to<long long>();
                    for (long long i = 0; i < degree; ++i)
                    {
                        roots.push_back("CANT_SOLVE");
                    }
                }
            }
        }
        catch (const std::exception &)
        {
            // 如果因式分解失败，返回错误信息
            long long degree = getDegree().getNumerator().convert_to<long long>();
            for (long long i = 0; i < degree; ++i)
            {
                roots.push_back("CANT_SOLVE");
            }
        }

        return roots;
    }

    // 完整的因式分解
    std::vector<Polynomial> Polynomial::complete_factorization() const
    {
        if (!hasOnlyRationalCoefficients())
        {
            throw std::runtime_error("Complete factorization requires rational coefficients.");
        }

        std::vector<Polynomial> factors;
        Polynomial current = *this;

        // 首先处理零多项式
        if (current.terms.empty())
            return factors;

        // 提取常数因子和最小次幂
        auto basic_factors = current.perform_factorization();
        if (basic_factors.size() > 1)
        {
            // 如果基本因式分解找到了因子，先处理它们
            for (size_t i = 0; i < basic_factors.size() - 1; ++i)
            {
                factors.push_back(basic_factors[i]);
            }
            current = basic_factors.back();
        }

        // 对剩余的多项式使用有理根定理
        int max_iterations = 20; // 防止无限循环
        int iteration_count = 0;

        while (current.getDegree() > Fraction(2) && current.hasOnlyRationalCoefficients() && !current.terms.empty() && iteration_count < max_iterations)
        {
            auto possible_roots = current.find_rational_roots();
            bool found_root = false;

            // 添加调试信息
            std::vector<Fraction> actual_roots;
            for (const auto &root : possible_roots)
            {
                try
                {
                    Fraction eval_result = current.evaluate(root);
                    if (eval_result.getNumerator() == 0)
                    {
                        actual_roots.push_back(root);
                    }
                }
                catch (const std::exception &)
                {
                    continue;
                }
            }

            // 尝试找到并使用第一个有效根
            for (const auto &root : actual_roots)
            {
                try
                {
                    // 找到一个根，执行综合除法
                    Polynomial linear_factor;
                    linear_factor.variable_name = current.variable_name;
                    linear_factor.terms.emplace_back(SimplifiedRadical(Fraction(1)), current.variable_name, Fraction(1));
                    linear_factor.terms.emplace_back(SimplifiedRadical(-root), "", Fraction(0));
                    factors.push_back(linear_factor);

                    current = current.synthetic_division(root);
                    found_root = true;
                    break;
                }
                catch (const std::exception &)
                {
                    // 如果综合除法失败，继续尝试下一个根
                    continue;
                }
            }

            if (!found_root)
            {
                // 没有找到有理根，跳出循环
                break;
            }

            iteration_count++;
        }

        // 处理剩余的因子
        if (!current.terms.empty())
        {
            factors.push_back(current);
        }

        return factors;
    }
}