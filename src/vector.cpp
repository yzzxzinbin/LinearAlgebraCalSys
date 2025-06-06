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
    Fraction sum;
    for (const auto& value : data) {
        sum += value * value;
    }
    
    // 注意：由于我们使用Fraction类，这里无法直接计算平方根
    // 对于分数来说，只有在分子、分母都是完全平方数的情况下才能准确表示
    // 这里返回的是向量长度的平方
    // 如果需要精确的平方根，我们需要扩展Fraction类或采用其他方法
    return sum;
}

// 向量归一化
Vector Vector::normalize() const {
    Fraction length_squared = norm();
    if (length_squared == Fraction(0)) {
        throw std::runtime_error("Cannot normalize a zero vector.");
    }
    
    // 由于无法直接计算分数的平方根，这里我们返回的是向量除以其长度的平方
    // 如果需要真正的单位向量，需要扩展Fraction类以支持平方根计算
    Vector result = *this;
    for (size_t i = 0; i < data.size(); ++i) {
        result.data[i] = result.data[i] / length_squared;
    }
    return result;
}
