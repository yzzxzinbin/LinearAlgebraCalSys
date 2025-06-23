#include "fraction.h"
#include <stdexcept>
#include <sstream>

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
