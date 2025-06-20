#include "vector.h"
#include <iomanip>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

namespace { // anonymous namespace to limit scope to this file
    bool is_perfect_square(const BigInt& n) {
        if (n < 0) return false;
        if (n == 0) return true;
        BigInt root = boost::multiprecision::sqrt(n);
        return root * root == n;
    }
}

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
        std::ostringstream oss;
        oss << v;
        std::string element = oss.str();
        int width = 8;
        int padding = width - static_cast<int>(element.length());
        int left_padding = padding / 2;
        int right_padding = padding - left_padding;
        os << std::string(left_padding, ' ') << element << std::string(right_padding, ' ') << " ";
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

// 向量点乘实现
Fraction Vector::dot(const Vector& rhs) const {
    if (data.size() != rhs.data.size()) {
        throw std::invalid_argument("Vector dot product error: dimensions mismatch.");
    }
    
    Fraction result;
    for (size_t i = 0; i < data.size(); ++i) {
        result += data[i] * rhs.data[i];
    }
    return result;
}

// 向量叉乘实现（仅适用于三维向量）
Vector Vector::cross(const Vector& rhs) const {
    if (data.size() != 3 || rhs.data.size() != 3) {
        throw std::invalid_argument("Vector cross product only defined for 3D vectors.");
    }
    
    std::vector<Fraction> result(3);
    result[0] = data[1] * rhs.data[2] - data[2] * rhs.data[1];
    result[1] = data[2] * rhs.data[0] - data[0] * rhs.data[2];
    result[2] = data[0] * rhs.data[1] - data[1] * rhs.data[0];
    
    return Vector(result);
}

// 计算向量的模（长度/范数）
Fraction Vector::norm() const {
    Fraction sum_of_squares;
    for (const auto& value : data) {
        sum_of_squares += value * value;
    }

    // 检查分子和分母是否为完全平方数
    if (is_perfect_square(sum_of_squares.getNumerator()) && 
        is_perfect_square(sum_of_squares.getDenominator())) {
        // 返回精确平方根
        return Fraction(
            boost::multiprecision::sqrt(sum_of_squares.getNumerator()),
            boost::multiprecision::sqrt(sum_of_squares.getDenominator())
        );
    } else {
        // 高精度浮点数近似
        using HighPrecisionFloat = boost::multiprecision::cpp_dec_float_100;
        HighPrecisionFloat num_hp(sum_of_squares.getNumerator().str());
        HighPrecisionFloat den_hp(sum_of_squares.getDenominator().str());
        HighPrecisionFloat val_hp = num_hp / den_hp;
        HighPrecisionFloat sqrt_val_hp = boost::multiprecision::sqrt(val_hp);

        // 将 scale 转换为 HighPrecisionFloat 后再乘法
        BigInt scale = boost::multiprecision::pow(BigInt(10), 20);
        HighPrecisionFloat scaled_val = sqrt_val_hp * HighPrecisionFloat(scale);
        
        // 四舍五入并转换回 BigInt
        BigInt approx_num = static_cast<BigInt>(round(scaled_val));
        return Fraction(approx_num, scale);
    }
}

// 向量归一化
Vector Vector::normalize() const {
    Fraction length = this->norm();
    if (length == Fraction(0)) {
        throw std::runtime_error("Cannot normalize a zero vector.");
    }
    
    // 向量中的每个分量除以向量的模长
    Vector result = *this;
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] /= length;
    }
    return result;
}

void Vector::resize(size_t newSize) {
    data.resize(newSize);
}