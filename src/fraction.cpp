#include "fraction.h"
#include <stdexcept> // For std::invalid_argument
#include <numeric>   // For std::gcd

// 辅助函数：计算最大公约数 (C++17 标准库中有 std::gcd)
// 如果使用 C++17之前的版本，可能需要自己实现或使用 __gcd (GNU extension)
// long long gcd(long long a, long long b) {
//     return std::gcd(a, b);
// }

void Fraction::simplify() {
    if (denominator == 0) {
        // 通常不应该发生，因为构造函数和除法会检查
        throw std::runtime_error("Denominator cannot be zero in simplify.");
    }
    if (numerator == 0) {
        denominator = 1;
        return;
    }

    long long common_divisor = std::gcd(numerator, denominator);
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

Fraction::Fraction(long long num) : numerator(num), denominator(1) {}

Fraction::Fraction(long long num, long long den) : numerator(num), denominator(den) {
    if (den == 0) {
        throw std::invalid_argument("Denominator cannot be zero.");
    }
    simplify();
}

// 获取分子和分母
long long Fraction::getNumerator() const {
    return numerator;
}

long long Fraction::getDenominator() const {
    return denominator;
}

// 算术运算符重载
Fraction Fraction::operator+(const Fraction& other) const {
    long long new_num = numerator * other.denominator + other.numerator * denominator;
    long long new_den = denominator * other.denominator;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator-(const Fraction& other) const {
    long long new_num = numerator * other.denominator - other.numerator * denominator;
    long long new_den = denominator * other.denominator;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator*(const Fraction& other) const {
    long long new_num = numerator * other.numerator;
    long long new_den = denominator * other.denominator;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator/(const Fraction& other) const {
    if (other.numerator == 0) {
        throw std::runtime_error("Division by zero fraction.");
    }
    long long new_num = numerator * other.denominator;
    long long new_den = denominator * other.numerator;
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


// 输出流运算符重载
std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    if (f.denominator == 1) {
        os << f.numerator;
    } else {
        os << f.numerator << "/" << f.denominator;
    }
    return os;
}
