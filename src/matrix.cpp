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
    const int field_width = 8; // Define field width for each element

    for (size_t i = 0; i < rows; ++i) {
        os << "| ";
        for (size_t j = 0; j < cols; ++j) {
            std::ostringstream oss;
            oss << data[i][j];
            std::string s = oss.str();

            int len = s.length();
            int padding_total = field_width - len;

            if (padding_total < 0) { // String is wider than field
                os << s; 
            } else {
                int padding_left = padding_total / 2;
                int padding_right = padding_total - padding_left;

                for (int k = 0; k < padding_left; ++k) {
                    os << ' ';
                }
                os << s;
                for (int k = 0; k < padding_right; ++k) {
                    os << ' ';
                }
            }
            os << " "; // Separator space after the element
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

Matrix Matrix::operator*(const Matrix& rhs) const {
    if (cols != rhs.rows) {
        throw std::invalid_argument("Matrix multiplication error: dimensions mismatch.");
    }
    
    Matrix result(rows, rhs.cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < rhs.cols; ++j) {
            Fraction sum;
            for (size_t k = 0; k < cols; ++k) {
                sum += data[i][k] * rhs.data[k][j];
            }
            result.data[i][j] = sum;
        }
    }
    return result;
}

Matrix Matrix::transpose() const {
    Matrix res(cols, rows);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            res.data[j][i] = data[i][j];
    return res;
}
