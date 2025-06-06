#include "vector.h"
#include <iomanip>

Vector::Vector(size_t n) : data(n) {}
Vector::Vector(const std::vector<Fraction>& d) : data(d) {}

size_t Vector::size() const { return data.size(); }
Fraction& Vector::at(size_t i) { return data[i]; }
const Fraction& Vector::at(size_t i) const { return data[i]; }

void Vector::input(std::istream& is) {
    for (size_t i = 0; i < data.size(); ++i) {
        long long num, den = 1;
        char ch;
        is >> num;
        if (is.peek() == '/') {
            is >> ch >> den;
        }
        data[i] = Fraction(num, den);
    }
}

void Vector::print(std::ostream& os) const {
    os << "[ ";
    for (const auto& v : data) {
        os << std::setw(8) << v << " ";
    }
    os << "]\n";
}

Vector Vector::operator+(const Vector& rhs) const {
    Vector res(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        res.data[i] = data[i] + rhs.data[i];
    return res;
}

Vector Vector::operator-(const Vector& rhs) const {
    Vector res(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        res.data[i] = data[i] - rhs.data[i];
    return res;
}

Vector Vector::operator*(const Fraction& k) const {
    Vector res(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        res.data[i] = data[i] * k;
    return res;
}
