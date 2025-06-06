#include "matrix.h"
#include <iomanip>

Matrix::Matrix(size_t r, size_t c) : rows(r), cols(c), data(r, std::vector<Fraction>(c)) {}

Matrix::Matrix(const std::vector<std::vector<Fraction>>& d)
    : rows(d.size()), cols(d.empty() ? 0 : d[0].size()), data(d) {}

size_t Matrix::rowCount() const { return rows; }
size_t Matrix::colCount() const { return cols; }

Fraction& Matrix::at(size_t r, size_t c) { return data[r][c]; }
const Fraction& Matrix::at(size_t r, size_t c) const { return data[r][c]; }

void Matrix::input(std::istream& is) {
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j) {
            long long num, den = 1;
            char ch;
            is >> num;
            if (is.peek() == '/') {
                is >> ch >> den;
            }
            data[i][j] = Fraction(num, den);
        }
}

void Matrix::print(std::ostream& os) const {
    for (size_t i = 0; i < rows; ++i) {
        os << "| ";
        for (size_t j = 0; j < cols; ++j) {
            os << std::setw(8) << data[i][j] << " ";
        }
        os << "|\n";
    }
}

Matrix Matrix::operator+(const Matrix& rhs) const {
    Matrix res(rows, cols);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[i][j] = data[i][j] + rhs.data[i][j];
    return res;
}

Matrix Matrix::operator-(const Matrix& rhs) const {
    Matrix res(rows, cols);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[i][j] = data[i][j] - rhs.data[i][j];
    return res;
}

Matrix Matrix::operator*(const Fraction& k) const {
    Matrix res(rows, cols);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[i][j] = data[i][j] * k;
    return res;
}

Matrix Matrix::transpose() const {
    Matrix res(cols, rows);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[j][i] = data[i][j];
    return res;
}
