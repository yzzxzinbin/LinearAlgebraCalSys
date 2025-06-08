#pragma once
#include <vector>
#include <iostream>
#include <cmath>
#include <sstream>
#include "fraction.h"

class Vector {
private:
    std::vector<Fraction> data;
public:
    Vector(size_t n);
    Vector(const std::vector<Fraction>& d);

    size_t size() const;
    Fraction& at(size_t i);
    const Fraction& at(size_t i) const;

    void input(std::istream& is = std::cin);
    void print(std::ostream& os = std::cout) const;

    Vector operator+(const Vector& rhs) const;
    Vector operator-(const Vector& rhs) const;
    Vector operator*(const Fraction& k) const;
    
    // 新增：向量点乘（返回标量）
    Fraction dot(const Vector& rhs) const;
    
    // 新增：向量叉乘（仅适用于三维向量，返回向量）
    Vector cross(const Vector& rhs) const;
    
    // 新增：计算向量的模（长度/范数）
    Fraction norm() const;
    
    // 新增：向量归一化（返回单位向量）
    Vector normalize() const;

    void resize(size_t newSize);
};
