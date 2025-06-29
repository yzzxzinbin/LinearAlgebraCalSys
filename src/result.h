#ifndef RESULT_H
#define RESULT_H

#include <iostream>
#include <string>
#include <vector>

// Result类型，用于存储格式化后的字符串结果
class Result {
public:
    enum class Type {
        SCALAR,  // 标量（单个数值）
        VECTOR,  // 向量
        MATRIX,  // 矩阵
        STRING   // 新增：字符串类型
    };

private:
    Type type_;
    std::string scalarValue_;                    // 标量值
    std::vector<std::string> vectorValues_;     // 向量值
    std::vector<std::vector<std::string>> matrixValues_; // 矩阵值
    size_t rows_, cols_;                         // 矩阵维度
    std::string stringValue_;                    // 新增：字符串值

public:
    // 构造函数
    Result();
    Result(const std::string& scalar);  // 标量构造
    Result(const std::vector<std::string>& vector);  // 向量构造
    Result(const std::vector<std::vector<std::string>>& matrix);  // 矩阵构造
    Result(Type type, const std::string& str); // 新增：字符串类型构造

    // 新增：字符串类型专用构造
    static Result fromString(const std::string& str);

    // 获取类型
    Type getType() const;

    // 获取数据
    const std::string& getScalar() const;
    const std::vector<std::string>& getVector() const;
    const std::vector<std::vector<std::string>>& getMatrix() const;
    const std::string& getString() const; // 新增

    // 获取维度信息
    size_t getRows() const;
    size_t getCols() const;
    size_t getVectorSize() const;

    // 显示方法
    void print(std::ostream& os = std::cout) const;

    // 纯字符串序列化方法（支持转义，适合保存/导出）
    std::string serialize() const;
    static Result deserialize(const std::string& data);

    // CSV导出方法
    std::string toCsvString() const;

    // 输出流运算符重载
    friend std::ostream& operator<<(std::ostream& os, const Result& result);
};

#endif // RESULT_H
