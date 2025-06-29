#include "fraction.h"
#include <stdexcept>
#include <sstream>
#include <boost/multiprecision/integer.hpp>

// 计算最大公约数的函数，适用于 cpp_int
BigInt gcd(const BigInt& a, const BigInt& b) {
    BigInt abs_a = abs(a);
    BigInt abs_b = abs(b);
    
    while (abs_b != 0) {
        BigInt temp = abs_b;
        abs_b = abs_a % abs_b;
        abs_a = temp;
    }
    return abs_a;
}

// 新增：自定义的整数n次方根函数
// 返回 floor(n^(1/r))
static BigInt integer_nth_root(const BigInt& n, unsigned int r) {
    if (n < 0) {
        throw std::runtime_error("Nth root of a negative number is not supported in this context.");
    }
    if (n == 0) return 0;
    if (r == 1) return n;
    if (r == 0) throw std::runtime_error("Cannot compute 0th root.");

    BigInt low = 1;
    BigInt high = n;
    BigInt root = 1;

    while (low <= high) {
        BigInt mid = low + (high - low) / 2;
        if (mid == 0) { // Should not happen if low starts at 1
            break;
        }
        
        BigInt p = boost::multiprecision::pow(mid, r);

        if (p > n) {
            high = mid - 1;
        } else if (p < n) {
            root = mid;
            low = mid + 1;
        } else { // p == n
            return mid;
        }
    }
    return root;
}

void Fraction::simplify() {
    if (denominator == 0) {
        throw std::runtime_error("Denominator cannot be zero in simplify.");
    }
    if (numerator == 0) {
        denominator = 1;
        return;
    }

    BigInt common_divisor = gcd(numerator, denominator);
    numerator /= common_divisor;
    denominator /= common_divisor;

    // 确保分母为正
    if (denominator < 0) {
        numerator = -numerator;
        denominator = -denominator;
    }
}

// 构造函数
Fraction::Fraction() : numerator(0), denominator(1) {}

Fraction::Fraction(const BigInt& num) : numerator(num), denominator(1) {}

Fraction::Fraction(const BigInt& num, const BigInt& den) : numerator(num), denominator(den) {
    if (den == 0) {
        throw std::invalid_argument("Denominator cannot be zero.");
    }
    simplify();
}

// 为了向后兼容的构造函数
Fraction::Fraction(long long num) : numerator(BigInt(num)), denominator(1) {}

Fraction::Fraction(long long num, long long den) : numerator(BigInt(num)), denominator(BigInt(den)) {
    if (den == 0) {
        throw std::invalid_argument("Denominator cannot be zero.");
    }
    simplify();
}

// 新增：从字符串构造
Fraction::Fraction(const std::string& s) {
    std::string temp_s = s;
    // 移除可能存在的前后空格
    auto first = temp_s.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) { // string is all whitespace or empty
        numerator = 0;
        denominator = 1;
        return;
    }
    auto last = temp_s.find_last_not_of(" \t\n\r");
    temp_s = temp_s.substr(first, (last - first + 1));

    size_t slash_pos = temp_s.find('/');
    if (slash_pos == std::string::npos) {
        // 没有斜杠，是整数
        numerator = BigInt(temp_s);
        denominator = 1;
    } else {
        // 有斜杠，是分数
        std::string num_str = temp_s.substr(0, slash_pos);
        std::string den_str = temp_s.substr(slash_pos + 1);
        
        if (den_str.empty() || num_str.empty()) {
            throw std::invalid_argument("Numerator or denominator cannot be empty in fraction string.");
        }

        numerator = BigInt(num_str);
        denominator = BigInt(den_str);
        if (denominator == 0) {
            throw std::invalid_argument("Denominator cannot be zero.");
        }
        simplify();
    }
}

// 获取分子和分母
const BigInt& Fraction::getNumerator() const {
    return numerator;
}

const BigInt& Fraction::getDenominator() const {
    return denominator;
}

// 算术运算符重载
Fraction Fraction::operator+(const Fraction& other) const {
    BigInt new_num = numerator * other.denominator + other.numerator * denominator;
    BigInt new_den = denominator * other.denominator;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator-(const Fraction& other) const {
    BigInt new_num = numerator * other.denominator - other.numerator * denominator;
    BigInt new_den = denominator * other.denominator;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator*(const Fraction& other) const {
    BigInt new_num = numerator * other.numerator;
    BigInt new_den = denominator * other.denominator;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator/(const Fraction& other) const {
    if (other.numerator == 0) {
        throw std::runtime_error("Division by zero fraction.");
    }
    BigInt new_num = numerator * other.denominator;
    BigInt new_den = denominator * other.numerator;
    return Fraction(new_num, new_den);
}

// 复合赋值运算符
Fraction& Fraction::operator+=(const Fraction& other) {
    *this = *this + other;
    return *this;
}

Fraction& Fraction::operator-=(const Fraction& other) {
    *this = *this - other;
    return *this;
}

Fraction& Fraction::operator*=(const Fraction& other) {
    *this = *this * other;
    return *this;
}

Fraction& Fraction::operator/=(const Fraction& other) {
    *this = *this / other;
    return *this;
}

// 比较运算符
bool Fraction::operator==(const Fraction& other) const {
    return numerator == other.numerator && denominator == other.denominator;
}

bool Fraction::operator!=(const Fraction& other) const {
    return !(*this == other);
}

bool Fraction::operator<(const Fraction& other) const {
    return numerator * other.denominator < other.numerator * denominator;
}

bool Fraction::operator<=(const Fraction& other) const {
    return numerator * other.denominator <= other.numerator * denominator;
}

bool Fraction::operator>(const Fraction& other) const {
    return numerator * other.denominator > other.numerator * denominator;
}

bool Fraction::operator>=(const Fraction& other) const {
    return numerator * other.denominator >= other.numerator * denominator;
}

// 一元负号运算符
Fraction Fraction::operator-() const {
    return Fraction(-numerator, denominator);
}

// 转换为字符串
std::string Fraction::toString() const {
    if (denominator == 1) {
        return numerator.str();
    } else {
        return numerator.str() + "/" + denominator.str();
    }
}

// 输出流运算符重载
std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    os << f.toString();
    return os;
}

// 新增：数学函数
// 帮助函数：检查 BigInt 是否为完美平方数
static bool is_perfect_square_bigint(const BigInt& n) {
    if (n < 0) return false;
    if (n == 0) return true;
    BigInt root = boost::multiprecision::sqrt(n);
    return root * root == n;
}

// 新增：分数幂运算
Fraction pow(const Fraction& base, long long exp) {
    if (exp == 0) {
        return Fraction(1);
    }
    if (base.getNumerator() == 0 && exp < 0) {
        throw std::runtime_error("Division by zero: 0 to a negative power.");
    }

    if (exp > 0) {
        BigInt num = boost::multiprecision::pow(base.getNumerator(), static_cast<unsigned int>(exp));
        BigInt den = boost::multiprecision::pow(base.getDenominator(), static_cast<unsigned int>(exp));
        return Fraction(num, den);
    } else { // exp < 0
        BigInt num = boost::multiprecision::pow(base.getDenominator(), static_cast<unsigned int>(-exp));
        BigInt den = boost::multiprecision::pow(base.getNumerator(), static_cast<unsigned int>(-exp));
        return Fraction(num, den);
    }
}

// 分数开方
Fraction sqrt(const Fraction& f) {
    if (f.getNumerator() < 0) {
        throw std::runtime_error("Cannot compute square root of a negative number.");
    }
    if (!is_perfect_square_bigint(f.getNumerator()) || !is_perfect_square_bigint(f.getDenominator())) {
        throw std::runtime_error("Result of square root is not a rational number.");
    }
    return Fraction(boost::multiprecision::sqrt(f.getNumerator()), boost::multiprecision::sqrt(f.getDenominator()));
}

// 检查分数是否为完美平方数
bool is_perfect_square(const Fraction& f) {
    if (f.getNumerator() < 0) return false;
    return is_perfect_square_bigint(f.getNumerator()) && is_perfect_square_bigint(f.getDenominator());
}

// 新增：n次方根
Fraction nth_root(const Fraction& f, long long n) {
    if (f.getNumerator() < 0 && n % 2 == 0) {
        throw std::runtime_error("Cannot compute even root of a negative number.");
    }
    if (!is_perfect_nth_root(f, n)) {
        throw std::runtime_error("Result of nth root is not a rational number.");
    }
    
    BigInt num_root = integer_nth_root(abs(f.getNumerator()), n);
    BigInt den_root = integer_nth_root(f.getDenominator(), n);
    
    // 处理负数的奇次根
    if (f.getNumerator() < 0 && n % 2 == 1) {
        num_root = -num_root;
    }
    
    return Fraction(num_root, den_root);
}

// 检查分数是否为完美n次方数
bool is_perfect_nth_root(const Fraction& f, long long n) {
    if (f.getNumerator() < 0 && n % 2 == 0) return false;
    
    BigInt num_root = integer_nth_root(abs(f.getNumerator()), n);
    BigInt den_root = integer_nth_root(f.getDenominator(), n);
    
    return boost::multiprecision::pow(num_root, n) == abs(f.getNumerator()) &&
           boost::multiprecision::pow(den_root, n) == f.getDenominator();
}