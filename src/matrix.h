#pragma once
#include <vector>
#include <iostream>
#include "fraction.h"

class Matrix {
private:
    std::vector<std::vector<Fraction>> data;
    size_t rows, cols;
public:
    Matrix(size_t r, size_t c);
    Matrix(const std::vector<std::vector<Fraction>>& d);

    size_t rowCount() const;
    size_t colCount() const;

    Fraction& at(size_t r, size_t c);
    const Fraction& at(size_t r, size_t c) const;

    void input(std::istream& is = std::cin);
    void print(std::ostream& os = std::cout) const;

    Matrix operator+(const Matrix& rhs) const;
    Matrix operator-(const Matrix& rhs) const;
    Matrix operator*(const Fraction& k) const;
    Matrix operator*(const Matrix& rhs) const; // 添加矩阵乘法运算符
    Matrix transpose() const;
};
