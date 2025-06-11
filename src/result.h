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
        MATRIX   // 矩阵
    };

private:
    Type type_;
    std::string scalarValue_;                    // 标量值
    std::vector<std::string> vectorValues_;     // 向量值
    std::vector<std::vector<std::string>> matrixValues_; // 矩阵值
    size_t rows_, cols_;                         // 矩阵维度

public:
    // 构造函数
    Result();
    Result(const std::string& scalar);  // 标量构造
    Result(const std::vector<std::string>& vector);  // 向量构造
    Result(const std::vector<std::vector<std::string>>& matrix);  // 矩阵构造

    // 获取类型
    Type getType() const;

    // 获取数据
    const std::string& getScalar() const;
    const std::vector<std::string>& getVector() const;
    const std::vector<std::vector<std::string>>& getMatrix() const;

    // 获取维度信息
    size_t getRows() const;
    size_t getCols() const;
    size_t getVectorSize() const;

    // 显示方法
    void print(std::ostream& os = std::cout) const;

    // 序列化方法（用于导出）
    std::string serialize() const;
    static Result deserialize(const std::string& data);

    // 输出流运算符重载
    friend std::ostream& operator<<(std::ostream& os, const Result& result);
};

#endif // RESULT_H
