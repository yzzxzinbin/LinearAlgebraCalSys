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
