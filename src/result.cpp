#include "result.h"
#include <sstream>
#include <stdexcept>
#include <iomanip>

// ==== 新增：序列化分隔符与转义辅助 ====
namespace {
    const std::string RESULT_DELIMITER = "<!RES_FIELD_SEP!>";
    const std::string ESCAPED_DELIMITER_REPR = "<!ESC_SEP!>";
    const std::string NEWLINE_REPR = "<!NL!>";

    std::string escapeString(const std::string& s) {
        std::string result = s;
        size_t pos = 0;
        while ((pos = result.find(RESULT_DELIMITER, pos)) != std::string::npos) {
            result.replace(pos, RESULT_DELIMITER.length(), ESCAPED_DELIMITER_REPR);
            pos += ESCAPED_DELIMITER_REPR.length();
        }
        pos = 0;
        while ((pos = result.find("\n", pos)) != std::string::npos) {
            result.replace(pos, 1, NEWLINE_REPR);
            pos += NEWLINE_REPR.length();
        }
        return result;
    }
    std::string unescapeString(const std::string& s) {
        std::string result = s;
        size_t pos = 0;
        while ((pos = result.find(NEWLINE_REPR, pos)) != std::string::npos) {
            result.replace(pos, NEWLINE_REPR.length(), "\n");
            pos += 1;
        }
        pos = 0;
        while ((pos = result.find(ESCAPED_DELIMITER_REPR, pos)) != std::string::npos) {
            result.replace(pos, ESCAPED_DELIMITER_REPR.length(), RESULT_DELIMITER);
            pos += RESULT_DELIMITER.length();
        }
        return result;
    }
    std::vector<std::string> splitStringByDelimiter(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = s.find(delimiter);
        while (end != std::string::npos) {
            tokens.push_back(s.substr(start, end - start));
            start = end + delimiter.length();
            end = s.find(delimiter, start);
        }
        tokens.push_back(s.substr(start));
        return tokens;
    }
}
// ==== 结束辅助 ====

Result::Result() : type_(Type::SCALAR), scalarValue_("0"), rows_(0), cols_(0) {}

Result::Result(const std::string& scalar) 
    : type_(Type::SCALAR), scalarValue_(scalar), rows_(1), cols_(1) {}

Result::Result(const std::vector<std::string>& vector) 
    : type_(Type::VECTOR), vectorValues_(vector), rows_(1), cols_(vector.size()) {}

Result::Result(const std::vector<std::vector<std::string>>& matrix) 
    : type_(Type::MATRIX), matrixValues_(matrix), 
      rows_(matrix.size()), cols_(matrix.empty() ? 0 : matrix[0].size()) {}

// 新增：字符串类型构造
Result::Result(Type type, const std::string& str)
    : type_(type), rows_(1), cols_(1), stringValue_(str) {}

Result Result::fromString(const std::string& str) {
    return Result(Type::STRING, str);
}

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

// 新增：获取字符串
const std::string& Result::getString() const {
    if (type_ != Type::STRING) {
        throw std::runtime_error("Result is not a string");
    }
    return stringValue_;
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
        case Type::STRING:
            os << stringValue_;
            break;
    }
}

// ==== 修改：纯字符串序列化 ====
std::string Result::serialize() const {
    std::ostringstream oss;
    switch (type_) {
        case Type::SCALAR:
            oss << "SCALAR" << RESULT_DELIMITER << escapeString(scalarValue_);
            break;
        case Type::VECTOR:
            oss << "VECTOR" << RESULT_DELIMITER;
            for (size_t i = 0; i < vectorValues_.size(); ++i) {
                oss << escapeString(vectorValues_[i]);
                if (i < vectorValues_.size() - 1) oss << RESULT_DELIMITER;
            }
            break;
        case Type::MATRIX:
            oss << "MATRIX" << RESULT_DELIMITER << rows_ << RESULT_DELIMITER << cols_ << RESULT_DELIMITER;
            for (size_t r = 0; r < rows_; ++r) {
                for (size_t c = 0; c < cols_; ++c) {
                    oss << escapeString(matrixValues_[r][c]);
                    if (r < rows_ - 1 || c < cols_ - 1) oss << RESULT_DELIMITER;
                }
            }
            break;
        case Type::STRING:
            oss << "STRING" << RESULT_DELIMITER << escapeString(stringValue_);
            break;
    }
    return oss.str();
}

Result Result::deserialize(const std::string& data) {
    // 拆分类型
    size_t delimPos = data.find(RESULT_DELIMITER);
    if (delimPos == std::string::npos) {
        // ==== 兼容老格式的 RESULT:MATRIX:4,4:... ====
        // 只处理 RESULT 类型的老格式
        // 例如: MATRIX:4,4:1.00,-5.00,...
        // 或者: SCALAR:xxx, VECTOR:xxx,xxx
        // 先判断类型
        if (data.rfind("SCALAR:", 0) == 0) {
            std::string val = data.substr(7);
            return Result(val);
        } else if (data.rfind("VECTOR:", 0) == 0) {
            std::string val = data.substr(7);
            std::vector<std::string> vec;
            if (!val.empty()) {
                std::stringstream ss(val);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    vec.push_back(item);
                }
            }
            return Result(vec);
        } else if (data.rfind("MATRIX:", 0) == 0) {
            std::string val = data.substr(7);
            size_t first_comma = val.find(',');
            size_t first_colon = val.find(':');
            if (first_comma == std::string::npos || first_colon == std::string::npos || first_comma > first_colon)
                throw std::invalid_argument("Invalid matrix result format (legacy)");
            size_t rows = std::stoul(val.substr(0, first_comma));
            size_t cols = std::stoul(val.substr(first_comma + 1, first_colon - (first_comma + 1)));
            std::string elementsStr = val.substr(first_colon + 1);
            std::vector<std::string> parts;
            std::stringstream ss(elementsStr);
            std::string item;
            while (std::getline(ss, item, ',')) {
                parts.push_back(item);
            }
            std::vector<std::vector<std::string>> matrix(rows, std::vector<std::string>(cols));
            size_t idx = 0;
            for (size_t r = 0; r < rows; ++r) {
                for (size_t c = 0; c < cols; ++c) {
                    if (idx < parts.size()) {
                        matrix[r][c] = parts[idx++];
                    } else {
                        matrix[r][c] = "";
                    }
                }
            }
            return Result(matrix);
        } else if (data.rfind("STRING:", 0) == 0) {
            std::string val = data.substr(7);
            return Result::fromString(val);
        }
        throw std::invalid_argument("Invalid result format (no delimiter)");
    }
    // ...existing code...
    std::string typeStr = data.substr(0, delimPos);
    std::string rest = data.substr(delimPos + RESULT_DELIMITER.length());

    if (typeStr == "SCALAR") {
        return Result(unescapeString(rest));
    } else if (typeStr == "VECTOR") {
        std::vector<std::string> values;
        if (!rest.empty()) {
            std::vector<std::string> parts = splitStringByDelimiter(rest, RESULT_DELIMITER);
            for (auto& s : parts) values.push_back(unescapeString(s));
        }
        return Result(values);
    } else if (typeStr == "MATRIX") {
        std::vector<std::string> parts = splitStringByDelimiter(rest, RESULT_DELIMITER);
        if (parts.size() < 2) throw std::invalid_argument("Invalid matrix result format");
        size_t rows = std::stoul(parts[0]);
        size_t cols = std::stoul(parts[1]);
        std::vector<std::vector<std::string>> matrix(rows, std::vector<std::string>(cols));
        size_t idx = 2;
        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < cols; ++c) {
                if (idx < parts.size()) {
                    matrix[r][c] = unescapeString(parts[idx++]);
                } else {
                    matrix[r][c] = "";
                }
            }
        }
        return Result(matrix);
    } else if (typeStr == "STRING") {
        return Result::fromString(unescapeString(rest));
    } else {
        throw std::invalid_argument("Unknown result type: " + typeStr);
    }
}
// ==== 结束修改 ====

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
        case Type::STRING:
            oss << "\"" << stringValue_ << "\"";
            break;
    }
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Result& result) {
    result.print(os);
    return os;
}
