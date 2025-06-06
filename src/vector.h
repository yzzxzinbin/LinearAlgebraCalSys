#pragma once
#include <vector>
#include <iostream>
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
};
