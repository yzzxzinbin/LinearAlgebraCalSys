#ifndef FRACTION_H
#define FRACTION_H

#include <iostream>
#include <boost/multiprecision/cpp_int.hpp>

// 使用 Boost 的任意精度整数类型
using BigInt = boost::multiprecision::cpp_int;

class Fraction {
private:
    BigInt numerator;
    BigInt denominator;

    void simplify(); // 用于化简分数

public:
    // 构造函数
    Fraction();
    Fraction(const BigInt& num);
    Fraction(const BigInt& num, const BigInt& den);
    
    // 为了向后兼容，保留 long long 构造函数
    Fraction(long long num);
    Fraction(long long num, long long den);

    // 新增：从字符串构造
    explicit Fraction(const std::string& s);

    // 获取分子和分母
    const BigInt& getNumerator() const;
    const BigInt& getDenominator() const;

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
    
    // 新增：转换为字符串，用于更好的显示
    std::string toString() const;
};

// 新增：数学函数
Fraction pow(const Fraction& base, long long exp);
Fraction sqrt(const Fraction& f);
bool is_perfect_square(const Fraction& f);

#endif // FRACTION_H
