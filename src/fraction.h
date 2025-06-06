#ifndef FRACTION_H
#define FRACTION_H

#include <iostream>
#include <numeric> // For std::gcd

class Fraction {
private:
    long long numerator;
    long long denominator;

    void simplify(); // 用于化简分数

public:
    // 构造函数
    Fraction();
    Fraction(long long num);
    Fraction(long long num, long long den);

    // 获取分子和分母
    long long getNumerator() const;
    long long getDenominator() const;

    // 算术运算符重载
    Fraction operator+(const Fraction& other) const;
    Fraction operator-(const Fraction& other) const;
    Fraction operator*(const Fraction& other) const;
    Fraction operator/(const Fraction& other) const;

    // 复合赋值运算符
    Fraction& operator+=(const Fraction& other);
    Fraction& operator-=(const Fraction& other);
    Fraction& operator*=(const Fraction& other);
    Fraction& operator/=(const Fraction& other);

    // 比较运算符
    bool operator==(const Fraction& other) const;
    bool operator!=(const Fraction& other) const;
    bool operator<(const Fraction& other) const;
    bool operator<=(const Fraction& other) const;
    bool operator>(const Fraction& other) const;
    bool operator>=(const Fraction& other) const;
    
    // 一元负号运算符
    Fraction operator-() const;

    // 输出流运算符重载
    friend std::ostream& operator<<(std::ostream& os, const Fraction& f);
};

#endif // FRACTION_H
