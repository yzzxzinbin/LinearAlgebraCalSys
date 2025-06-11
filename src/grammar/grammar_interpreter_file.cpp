#include "grammar_parser.h"
#include "grammar_interpreter.h"
#include <stdexcept>
#include <sstream>
#include <fstream> // 需要包含 fstream
#include <deque>
#include "../utils/logger.h" // 确保包含 logger

const std::string Interpreter::HISTORY_MARKER = "HISTORY_ENTRY:"; // 定义历史记录标记

std::vector<std::string> Interpreter::splitString(const std::string &s, char delimiter) const
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

Fraction Interpreter::parseFractionString(const std::string &s) const
{
    if (s.empty())
        throw std::invalid_argument("空字符串无法解析为分数");
    size_t slash_pos = s.find('/');
    if (slash_pos == std::string::npos)
    {
        return Fraction(std::stoll(s));
    }
    else
    {
        long long num = std::stoll(s.substr(0, slash_pos));
        long long den = std::stoll(s.substr(slash_pos + 1));
        return Fraction(num, den);
    }
}

std::string Interpreter::serializeVariable(const std::string &name, const Variable &var) const
{
    std::ostringstream oss;
    oss << name << ":";
    switch (var.type)
    {
    case VariableType::FRACTION:
        oss << "FRACTION:" << var.fractionValue.getNumerator() << "/" << var.fractionValue.getDenominator();
        break;
    case VariableType::VECTOR:
        oss << "VECTOR:";
        for (size_t i = 0; i < var.vectorValue.size(); ++i)
        {
            oss << var.vectorValue.at(i).getNumerator() << "/" << var.vectorValue.at(i).getDenominator();
            if (i < var.vectorValue.size() - 1)
            {
                oss << ",";
            }
        }
        break;
    case VariableType::MATRIX:
        oss << "MATRIX:" << var.matrixValue.rowCount() << "," << var.matrixValue.colCount() << ":";
        for (size_t r = 0; r < var.matrixValue.rowCount(); ++r)
        {
            for (size_t c = 0; c < var.matrixValue.colCount(); ++c)
            {
                oss << var.matrixValue.at(r, c).getNumerator() << "/" << var.matrixValue.at(r, c).getDenominator();
                if (r < var.matrixValue.rowCount() - 1 || c < var.matrixValue.colCount() - 1)
                {
                    oss << ",";
                }
            }
        }
        break;
    case VariableType::RESULT:  // 新增：序列化Result类型
        oss << "RESULT:" << var.resultValue.serialize();
        break;
    }
    return oss.str();
}

// std::pair<std::string, Variable> Interpreter::deserializeLine(const std::string &line) const
// {
//     std::vector<std::string> parts = splitString(line, ':');
//     if (parts.size() != 3)
//     {
//         throw std::runtime_error("无效的文件行格式: " + line);
//     }
//     std::string name = parts[0];
//     std::string typeStr = parts[1];
//     std::string dataStr = parts[2];
//     Variable var;

//     if (typeStr == "FRACTION")
//     {
//         var = Variable(parseFractionString(dataStr));
//     }
//     else if (typeStr == "VECTOR")
//     {
//         std::vector<Fraction> vecData;
//         std::vector<std::string> fracStrings = splitString(dataStr, ',');
//         if (fracStrings.empty() && !dataStr.empty())
//         { // 处理单个元素向量且无逗号的情况
//             vecData.push_back(parseFractionString(dataStr));
//         }
//         else
//         {
//             for (const auto &fs : fracStrings)
//             {
//                 if (!fs.empty())
//                     vecData.push_back(parseFractionString(fs));
//             }
//         }
//         var = Variable(Vector(vecData));
//     }
//     else if (typeStr == "MATRIX")
//     {
//         size_t first_comma = dataStr.find(',');
//         size_t first_colon_after_dims = dataStr.find(':', first_comma + 1);
//         if (first_comma == std::string::npos || first_colon_after_dims == std::string::npos)
//         {
//             throw std::runtime_error("无效的矩阵数据格式 (维度): " + dataStr);
//         }

//         size_t rows = std::stoul(dataStr.substr(0, first_comma));
//         size_t cols = std::stoul(dataStr.substr(first_comma + 1, first_colon_after_dims - (first_comma + 1)));

//         std::string elementsStr = dataStr.substr(first_colon_after_dims + 1);
//         std::vector<std::string> fracStrings = splitString(elementsStr, ',');

//         if (fracStrings.size() != rows * cols && !(rows * cols == 0 && fracStrings.size() <= 1 && (fracStrings.empty() || fracStrings[0].empty())))
//         { // 允许空矩阵的fracStrings为空或只有一个空元素
//             throw std::runtime_error("矩阵元素数量与维度不匹配. 期望 " + std::to_string(rows * cols) + ", 得到 " + std::to_string(fracStrings.size()));
//         }

//         Matrix mat(rows, cols);
//         if (rows > 0 && cols > 0)
//         {
//             for (size_t r = 0; r < rows; ++r)
//             {
//                 for (size_t c = 0; c < cols; ++c)
//                 {
//                     mat.at(r, c) = parseFractionString(fracStrings[r * cols + c]);
//                 }
//             }
//         }
//         var = Variable(mat);
//     }
//     else
//     {
//         throw std::runtime_error("未知变量类型: " + typeStr);
//     }
//     return {name, var};
// }

std::pair<std::string, Variable> Interpreter::deserializeLine(const std::string &line) const
{
    size_t first_colon_pos = line.find(':');
    if (first_colon_pos == std::string::npos)
    {
        throw std::runtime_error("无效的文件行格式 (缺少名称分隔符): " + line);
    }
    std::string name = line.substr(0, first_colon_pos);
    std::string rest_after_name = line.substr(first_colon_pos + 1);

    size_t second_colon_pos = rest_after_name.find(':');
    if (second_colon_pos == std::string::npos)
    {
        throw std::runtime_error("无效的文件行格式 (缺少类型分隔符): " + line);
    }
    std::string typeStr = rest_after_name.substr(0, second_colon_pos);
    std::string dataStr = rest_after_name.substr(second_colon_pos + 1);
    
    Variable var;

    if (typeStr == "FRACTION")
    {
        var = Variable(parseFractionString(dataStr));
    }
    else if (typeStr == "VECTOR")
    {
        std::vector<Fraction> vecData;
        std::vector<std::string> fracStrings = splitString(dataStr, ',');
        if (fracStrings.empty() && !dataStr.empty())
        { // 处理单个元素向量且无逗号的情况
            vecData.push_back(parseFractionString(dataStr));
        }
        else
        {
            for (const auto &fs : fracStrings)
            {
                if (!fs.empty())
                    vecData.push_back(parseFractionString(fs));
            }
        }
        var = Variable(Vector(vecData));
    }
    else if (typeStr == "MATRIX")
    {
        size_t first_comma = dataStr.find(',');
        // The colon separating dimensions from elements is the first colon in dataStr for matrices.
        size_t dims_elements_separator_colon = dataStr.find(':'); 

        if (first_comma == std::string::npos || dims_elements_separator_colon == std::string::npos || first_comma > dims_elements_separator_colon)
        {
            throw std::runtime_error("无效的矩阵数据格式 (维度格式错误): " + dataStr);
        }

        size_t rows = std::stoul(dataStr.substr(0, first_comma));
        size_t cols = std::stoul(dataStr.substr(first_comma + 1, dims_elements_separator_colon - (first_comma + 1)));

        std::string elementsStr = dataStr.substr(dims_elements_separator_colon + 1);
        std::vector<std::string> fracStrings = splitString(elementsStr, ',');

        if (fracStrings.size() != rows * cols && !(rows * cols == 0 && fracStrings.size() <= 1 && (fracStrings.empty() || fracStrings[0].empty())))
        { // 允许空矩阵的fracStrings为空或只有一个空元素
            throw std::runtime_error("矩阵元素数量与维度不匹配. 期望 " + std::to_string(rows * cols) + ", 得到 " + std::to_string(fracStrings.size()));
        }

        Matrix mat(rows, cols);
        if (rows > 0 && cols > 0)
        {
            for (size_t r = 0; r < rows; ++r)
            {
                for (size_t c = 0; c < cols; ++c)
                {
                    mat.at(r, c) = parseFractionString(fracStrings[r * cols + c]);
                }
            }
        }
        var = Variable(mat);
    }
    else if (typeStr == "RESULT")  // 新增：反序列化Result类型
    {
        var = Variable(Result::deserialize(dataStr));
    }
    else
    {
        throw std::runtime_error("未知变量类型: " + typeStr);
    }
    return {name, var};
}

std::string Interpreter::exportVariables(const std::string &filename, const std::deque<std::string>& commandHistory)
{
    std::ofstream outFile(filename);
    if (!outFile.is_open())
    {
        LOG_ERROR("无法打开文件进行导出: " + filename);
        return "错误: 无法打开文件 '" + filename + "' 进行导出。";
    }

    // 序列化变量
    for (const auto &pair : variables)
    {
        try
        {
            outFile << serializeVariable(pair.first, pair.second) << std::endl;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("序列化变量 " + pair.first + " 时出错: " + e.what());
            outFile.close(); 
            return "错误: 序列化变量 " + pair.first + " 时出错: " + e.what();
        }
    }

    // 序列化命令历史 (从最新到最旧)
    LOG_INFO("开始导出命令历史到 " + filename);
    for (const auto& command : commandHistory) { // deque 迭代器从头到尾 (即最新到最旧)
        outFile << HISTORY_MARKER << command << std::endl;
    }
    
    outFile.close();
    LOG_INFO("变量和命令历史已成功导出到 " + filename);
    return "变量和命令历史已成功导出到 " + filename;
}

std::pair<std::string, std::vector<std::string>> Interpreter::importVariables(const std::string &filename)
{
    std::vector<std::string> importedHistoryCommands;
    std::ifstream inFile(filename);
    if (!inFile.is_open())
    {
        LOG_ERROR("无法打开文件进行导入: " + filename);
        return {"错误: 无法打开文件 '" + filename + "' 进行导入。", importedHistoryCommands};
    }
    std::string line;
    int lineNum = 0;
    while (std::getline(inFile, line))
    {
        lineNum++;
        if (line.empty() || line[0] == '#') // 跳过空行和注释
            continue; 

        // 检查行是否以 HISTORY_MARKER 开头
        if (line.rfind(HISTORY_MARKER, 0) == 0) { 
            importedHistoryCommands.push_back(line.substr(HISTORY_MARKER.length()));
        } else {
            try
            {
                auto pair = deserializeLine(line);
                variables[pair.first] = pair.second;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("导入文件 " + filename + " 第 " + std::to_string(lineNum) + " 行时出错: " + e.what());
                inFile.close();
                return {"错误: 导入文件 " + filename + " 第 " + std::to_string(lineNum) + " 行时出错: " + e.what(), importedHistoryCommands};
            }
        }
    }
    inFile.close();
    LOG_INFO("变量和命令历史已成功从 " + filename + " 导入");
    return {"变量和命令历史已成功从 " + filename + " 导入。", importedHistoryCommands};
}
