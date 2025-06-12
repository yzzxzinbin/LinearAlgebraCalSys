#include "result.h"
#include <sstream>
#include <stdexcept>
#include <iomanip>

Result::Result() : type_(Type::SCALAR), scalarValue_("0"), rows_(0), cols_(0) {}

Result::Result(const std::string& scalar) 
    : type_(Type::SCALAR), scalarValue_(scalar), rows_(1), cols_(1) {}

Result::Result(const std::vector<std::string>& vector) 
    : type_(Type::VECTOR), vectorValues_(vector), rows_(1), cols_(vector.size()) {}

Result::Result(const std::vector<std::vector<std::string>>& matrix) 
    : type_(Type::MATRIX), matrixValues_(matrix), 
      rows_(matrix.size()), cols_(matrix.empty() ? 0 : matrix[0].size()) {}

Result::Type Result::getType() const {
    return type_;
}

const std::string& Result::getScalar() const {
    if (type_ != Type::SCALAR) {
        throw std::runtime_error("Result is not a scalar");
    }
    return scalarValue_;
}

const std::vector<std::string>& Result::getVector() const {
    if (type_ != Type::VECTOR) {
        throw std::runtime_error("Result is not a vector");
    }
    return vectorValues_;
}

const std::vector<std::vector<std::string>>& Result::getMatrix() const {
    if (type_ != Type::MATRIX) {
        throw std::runtime_error("Result is not a matrix");
    }
    return matrixValues_;
}

size_t Result::getRows() const {
    return rows_;
}

size_t Result::getCols() const {
    return cols_;
}

size_t Result::getVectorSize() const {
    if (type_ != Type::VECTOR) {
        throw std::runtime_error("Result is not a vector");
    }
    return vectorValues_.size();
}

void Result::print(std::ostream& os) const {
    switch (type_) {
        case Type::SCALAR:
            os << scalarValue_;
            break;
        case Type::VECTOR:
            os << "[";
            for (size_t i = 0; i < vectorValues_.size(); ++i) {
                os << vectorValues_[i];
                if (i < vectorValues_.size() - 1) {
                    os << ", ";
                }
            }
            os << "]";
            break;
        case Type::MATRIX:
            for (size_t r = 0; r < rows_; ++r) {
                os << "| ";
                for (size_t c = 0; c < cols_; ++c) {
                    os << std::setw(12) << matrixValues_[r][c] << " ";
                }
                os << "|";
                if (r < rows_ - 1) {
                    os << "\n";
                }
            }
            break;
    }
}

std::string Result::serialize() const {
    std::ostringstream oss;
    switch (type_) {
        case Type::SCALAR:
            oss << "SCALAR:" << scalarValue_;
            break;
        case Type::VECTOR:
            oss << "VECTOR:";
            for (size_t i = 0; i < vectorValues_.size(); ++i) {
                oss << vectorValues_[i];
                if (i < vectorValues_.size() - 1) {
                    oss << ",";
                }
            }
            break;
        case Type::MATRIX:
            oss << "MATRIX:" << rows_ << "," << cols_ << ":";
            for (size_t r = 0; r < rows_; ++r) {
                for (size_t c = 0; c < cols_; ++c) {
                    oss << matrixValues_[r][c];
                    if (r < rows_ - 1 || c < cols_ - 1) {
                        oss << ",";
                    }
                }
            }
            break;
    }
    return oss.str();
}

Result Result::deserialize(const std::string& data) {
    size_t colonPos = data.find(':');
    if (colonPos == std::string::npos) {
        throw std::invalid_argument("Invalid result format");
    }
    
    std::string typeStr = data.substr(0, colonPos);
    std::string valueStr = data.substr(colonPos + 1);
    
    if (typeStr == "SCALAR") {
        return Result(valueStr);
    } else if (typeStr == "VECTOR") {
        std::vector<std::string> values;
        std::stringstream ss(valueStr);
        std::string item;
        while (std::getline(ss, item, ',')) {
            values.push_back(item);
        }
        return Result(values);
    } else if (typeStr == "MATRIX") {
        size_t commaPos = valueStr.find(',');
        size_t secondColonPos = valueStr.find(':', commaPos);
        
        if (commaPos == std::string::npos || secondColonPos == std::string::npos) {
            throw std::invalid_argument("Invalid matrix result format");
        }
        
        size_t rows = std::stoul(valueStr.substr(0, commaPos));
        size_t cols = std::stoul(valueStr.substr(commaPos + 1, secondColonPos - commaPos - 1));
        
        std::string elementsStr = valueStr.substr(secondColonPos + 1);
        std::vector<std::string> elements;
        std::stringstream ss(elementsStr);
        std::string item;
        while (std::getline(ss, item, ',')) {
            elements.push_back(item);
        }
        
        std::vector<std::vector<std::string>> matrix(rows, std::vector<std::string>(cols));
        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < cols; ++c) {
                matrix[r][c] = elements[r * cols + c];
            }
        }
        
        return Result(matrix);
    } else {
        throw std::invalid_argument("Unknown result type: " + typeStr);
    }
}

std::string Result::toCsvString() const {
    std::ostringstream oss;
    switch (type_) {
        case Type::SCALAR:
            oss << "\"" << scalarValue_ << "\""; // 将标量值用引号括起来
            break;
        case Type::VECTOR:
            for (size_t i = 0; i < vectorValues_.size(); ++i) {
                oss << "\"" << vectorValues_[i] << "\""; // Enclose in quotes for safety
                if (i < vectorValues_.size() - 1) {
                    oss << ",";
                }
            }
            break;
        case Type::MATRIX:
            for (size_t r = 0; r < rows_; ++r) {
                for (size_t c = 0; c < cols_; ++c) {
                    oss << "\"" << matrixValues_[r][c] << "\""; // Enclose in quotes
                    if (c < cols_ - 1) {
                        oss << ",";
                    }
                }
                if (r < rows_ - 1) {
                    oss << "\n";
                }
            }
            break;
    }
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Result& result) {
    result.print(os);
    return os;
}
