#include "equationset.h"
#include "matrix_operations.h"
#include <sstream>
#include <iomanip>
#include <vector> // 新增
#include <stdexcept> // 新增

// EquationSolution 实现
EquationSolution::EquationSolution() : type(SolutionType::UNDETERMINED),
                                     particularSolution(0, 0),
                                     homogeneousSolutions(0, 0),
                                     initialAugmentedMatrix(0,0) {} // 初始化新增成员

void EquationSolution::setSolutionType(SolutionType t) {
    type = t;
}

void EquationSolution::setSystemInfo(const EquationSystemInfo& info) {
    systemInfo = info;
}

void EquationSolution::setParticularSolution(const Matrix& solution) {
    particularSolution = solution;
}

void EquationSolution::setHomogeneousSolutions(const Matrix& solutions) {
    homogeneousSolutions = solutions;
}

void EquationSolution::setDetailedDescription(const std::string& desc) {
    detailedDescription = desc;
}

void EquationSolution::setInitialAugmentedMatrix(const Matrix& augMatrix) { // 新增实现
    initialAugmentedMatrix = augMatrix;
}

SolutionType EquationSolution::getSolutionType() const {
    return type;
}

const EquationSystemInfo& EquationSolution::getSystemInfo() const {
    return systemInfo;
}

const Matrix& EquationSolution::getParticularSolution() const {
    return particularSolution;
}

const Matrix& EquationSolution::getHomogeneousSolutions() const {
    return homogeneousSolutions;
}

std::string EquationSolution::getDetailedDescription() const {
    return detailedDescription;
}

const Matrix& EquationSolution::getInitialAugmentedMatrix() const { // 新增实现
    return initialAugmentedMatrix;
}

bool EquationSolution::hasSolution() const {
    return type != SolutionType::NO_SOLUTION;
}

bool EquationSolution::hasUniqueSolution() const {
    return type == SolutionType::UNIQUE_SOLUTION;
}

bool EquationSolution::hasInfiniteSolutions() const {
    return type == SolutionType::INFINITE_SOLUTIONS;
}

void EquationSolution::print(std::ostream& os) const {
    os << "=== 线性方程组求解结果 ===\n\n";
    
    // 输出系统信息 - 修改为显示初始增广矩阵
    os << "方程组 (增广矩阵形式):\n";
    if (initialAugmentedMatrix.rowCount() > 0 || initialAugmentedMatrix.colCount() > 0) {
        initialAugmentedMatrix.print(os);
    } else {
        os << "  (未提供初始方程组矩阵)\n";
    }
    os << "\n";
    
    // 输出解的类型
    os << "解的性质: ";
    switch (type) {
        case SolutionType::UNIQUE_SOLUTION:
            os << "唯一解\n\n";
            os << "解向量:\n";
            particularSolution.print(os);
            break;
            
        case SolutionType::INFINITE_SOLUTIONS:
            os << "无穷多解\n\n";
            if (particularSolution.rowCount() > 0) {
                os << "特解:\n";
                particularSolution.print(os);
                os << "\n";
            }
            if (homogeneousSolutions.rowCount() > 0) {
                os << "齐次解的基础解系:\n";
                homogeneousSolutions.print(os);
                os << "\n";
                os << "通解形式: x = 特解 + k1*基础解1 + k2*基础解2 + ...\n";
                os << "其中 k1, k2, ... 为任意常数\n";
            }
            break;
            
        case SolutionType::NO_SOLUTION:
            os << "无解\n";
            os << "原因: 增广矩阵的秩大于系数矩阵的秩\n";
            break;
            
        default:
            os << "未确定\n";
            break;
    }
    
    if (!detailedDescription.empty()) {
        os << "\n详细说明:\n" << detailedDescription << "\n"; // 详细说明中已包含秩和变量数等信息
    }
}

// 辅助函数：分割字符串
static std::vector<std::string> splitStringByDelimiter(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = s.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + delimiter.length();
        end = s.find(delimiter, start);
    }
    tokens.push_back(s.substr(start)); // Add the last token
    return tokens;
}

// 辅助函数：转义字符串中的特殊字符
static const std::string EQ_SOL_DELIMITER = "<!EQ_FIELD_SEP!>";
static const std::string ESCAPED_DELIMITER_REPR = "<!ESC_SEP!>"; // 表示转义后的分隔符
static const std::string NEWLINE_REPR = "<!NL!>";             // 表示转义后的换行符

static std::string escapeString(const std::string& s) {
    std::string result = s;
    // 1. 先替换分隔符本身
    size_t pos = 0;
    while ((pos = result.find(EQ_SOL_DELIMITER, pos)) != std::string::npos) {
        result.replace(pos, EQ_SOL_DELIMITER.length(), ESCAPED_DELIMITER_REPR);
        pos += ESCAPED_DELIMITER_REPR.length();
    }
    // 2. 再替换换行符
    pos = 0;
    while ((pos = result.find("\n", pos)) != std::string::npos) {
        result.replace(pos, 1, NEWLINE_REPR);
        pos += NEWLINE_REPR.length();
    }
    return result;
}

static std::string unescapeString(const std::string& s) {
    std::string result = s;
    // 1. 先还原换行符
    size_t pos = 0;
    while ((pos = result.find(NEWLINE_REPR, pos)) != std::string::npos) {
        result.replace(pos, NEWLINE_REPR.length(), "\n");
        pos += 1; 
    }
    // 2. 再还原分隔符
    pos = 0;
    while ((pos = result.find(ESCAPED_DELIMITER_REPR, pos)) != std::string::npos) {
        result.replace(pos, ESCAPED_DELIMITER_REPR.length(), EQ_SOL_DELIMITER);
        pos += EQ_SOL_DELIMITER.length();
    }
    return result;
}


std::string EquationSolution::serialize() const {
    std::ostringstream oss;
    oss << static_cast<int>(type) << EQ_SOL_DELIMITER;
    oss << particularSolution.serialize() << EQ_SOL_DELIMITER;
    oss << homogeneousSolutions.serialize() << EQ_SOL_DELIMITER;
    oss << systemInfo.coefficientRank << EQ_SOL_DELIMITER;
    oss << systemInfo.augmentedRank << EQ_SOL_DELIMITER;
    oss << systemInfo.numVariables << EQ_SOL_DELIMITER;
    oss << systemInfo.numEquations << EQ_SOL_DELIMITER;
    oss << static_cast<int>(systemInfo.solutionType) << EQ_SOL_DELIMITER;
    oss << escapeString(systemInfo.description) << EQ_SOL_DELIMITER;
    oss << escapeString(detailedDescription) << EQ_SOL_DELIMITER;
    oss << initialAugmentedMatrix.serialize();
    return oss.str();
}

EquationSolution EquationSolution::deserialize(const std::string& s) {
    std::vector<std::string> parts = splitStringByDelimiter(s, EQ_SOL_DELIMITER);

    if (parts.size() != 11) { // 确保有11个部分
        throw std::runtime_error("Invalid EquationSolution serialized string: incorrect number of parts. Expected 11, got " + std::to_string(parts.size()));
    }

    EquationSolution sol;
    int partIdx = 0;
    try {
        sol.type = static_cast<SolutionType>(std::stoi(parts[partIdx++]));
        sol.particularSolution = Matrix::deserialize(parts[partIdx++]);
        sol.homogeneousSolutions = Matrix::deserialize(parts[partIdx++]);
        
        EquationSystemInfo sysInfo;
        sysInfo.coefficientRank = std::stoi(parts[partIdx++]);
        sysInfo.augmentedRank = std::stoi(parts[partIdx++]);
        sysInfo.numVariables = std::stoi(parts[partIdx++]);
        sysInfo.numEquations = std::stoi(parts[partIdx++]);
        sysInfo.solutionType = static_cast<SolutionType>(std::stoi(parts[partIdx++]));
        sysInfo.description = unescapeString(parts[partIdx++]);
        sol.systemInfo = sysInfo;

        sol.detailedDescription = unescapeString(parts[partIdx++]);
        sol.initialAugmentedMatrix = Matrix::deserialize(parts[partIdx++]);

    } catch (const std::exception& e) {
        throw std::runtime_error("Error deserializing EquationSolution part " + std::to_string(partIdx) + ": " + e.what() + ". Original string: " + s);
    }
    return sol;
}


// EquationSolver 实现

// 新增：实现 solve(Matrix, Vector)
EquationSolution EquationSolver::solve(const Matrix& A, const Vector& b) {
    OperationHistory dummy;
    return solve(A, b, dummy);
}

// 新增：实现 solve(Matrix, Vector, OperationHistory)
EquationSolution EquationSolver::solve(const Matrix& A, const Vector& b, OperationHistory& history) {
    // 将 Vector b 转换为 Matrix (列向量)
    if (A.rowCount() != b.size() && b.size() != 0) { // b.size() == 0 可能是空向量，后续会处理
        throw std::invalid_argument("系数矩阵的行数必须与向量 b 的元素个数匹配");
    }
    Matrix b_matrix(b.size(), 1);
    for (size_t i = 0; i < b.size(); ++i) {
        b_matrix.at(i, 0) = b.at(i);
    }
    // 调用原有的 solve 方法
    return solve(A, b_matrix, history);
}

EquationSolution EquationSolver::solve(const Matrix& A, const Matrix& b) {
    OperationHistory dummy;
    return solve(A, b, dummy);
}

EquationSolution EquationSolver::solve(const Matrix& A, const Matrix& b, OperationHistory& history) {
    EquationSolution solution;
    
    // 检查输入有效性
    if (A.rowCount() != b.rowCount()) {
        throw std::invalid_argument("系数矩阵和常数向量的行数不匹配");
    }
    
    if (b.colCount() != 1) {
        throw std::invalid_argument("常数项必须是列向量(nx1矩阵)");
    }
    
    // 分析方程组
    EquationSystemInfo info = analyzeSystem(A, b);
    solution.setSystemInfo(info);
    
    history.addStep(OperationStep(
        OperationType::INITIAL_STATE,
        "开始求解线性方程组 Ax = b",
        A
    ));
    
    // 创建增广矩阵 [A|b]
    Matrix augmented = A.augment(b);
    solution.setInitialAugmentedMatrix(augmented); // 设置初始增广矩阵
    
    history.addStep(OperationStep(
        OperationType::RESULT_STATE,
        "构造增广矩阵 [A|b]:",
        augmented
    ));
    
    // 化为最简行阶梯形
    MatrixOperations::toReducedRowEchelonForm(augmented, history);
    
    // 分离系数部分和常数部分
    Matrix A_rref = augmented.extractRightPart(0);
    A_rref.resize(A_rref.rowCount(), A.colCount()); // 只保留系数部分
    
    Matrix b_rref(b.rowCount(), 1);
    for (size_t i = 0; i < b.rowCount(); ++i) {
        b_rref.at(i, 0) = augmented.at(i, A.colCount());
    }
    
    // 查找主元列
    std::vector<int> pivotCols = findPivotColumns(A_rref);
    
    // 根据分析结果确定解的类型
    solution.setSolutionType(info.solutionType);
    
    switch (info.solutionType) {
        case SolutionType::NO_SOLUTION:
            history.addStep(OperationStep(
                OperationType::RESULT_STATE,
                "方程组无解: rank(A) < rank([A|b])",
                augmented
            ));
            break;
            
        case SolutionType::UNIQUE_SOLUTION: {
            Matrix x = findParticularSolution(A_rref, b_rref, pivotCols);
            solution.setParticularSolution(x);
            
            history.addStep(OperationStep(
                OperationType::RESULT_STATE,
                "方程组有唯一解: rank(A) = rank([A|b]) = n",
                x
            ));
            break;
        }
        
        case SolutionType::INFINITE_SOLUTIONS: {
            Matrix x_particular = findParticularSolution(A_rref, b_rref, pivotCols);
            Matrix x_homogeneous = findHomogeneousSolutions(A_rref, pivotCols);
            
            solution.setParticularSolution(x_particular);
            solution.setHomogeneousSolutions(x_homogeneous);
            
            std::stringstream ss;
            ss << "方程组有无穷多解: rank(A) = rank([A|b]) < n\n";
            ss << "自由变量个数: " << (info.numVariables - info.coefficientRank);
            
            history.addStep(OperationStep(
                OperationType::RESULT_STATE,
                ss.str(),
                augmented
            ));
            break;
        }
        
        default:
            break;
    }
    
    solution.setDetailedDescription(generateSolutionDescription(solution));
    return solution;
}

EquationSolution EquationSolver::solveHomogeneous(const Matrix& A) {
    OperationHistory dummy;
    return solveHomogeneous(A, dummy);
}

EquationSolution EquationSolver::solveHomogeneous(const Matrix& A, OperationHistory& history) {
    // 创建零向量作为常数项
    Matrix zero_b(A.rowCount(), 1);
    for (size_t i = 0; i < A.rowCount(); ++i) {
        zero_b.at(i, 0) = Fraction(0);
    }
    
    history.addStep(OperationStep(
        OperationType::INITIAL_STATE,
        "求解齐次线性方程组 Ax = 0",
        A
    ));
    
    return solve(A, zero_b, history);
}

EquationSystemInfo EquationSolver::analyzeSystem(const Matrix& A, const Matrix& b) {
    EquationSystemInfo info;
    
    info.numEquations = A.rowCount();
    info.numVariables = A.colCount();
    info.coefficientRank = MatrixOperations::rank(A);
    
    // 计算增广矩阵的秩
    Matrix augmented = A.augment(b);
    info.augmentedRank = MatrixOperations::rank(augmented);
    
    // 判断解的性质
    if (info.coefficientRank < info.augmentedRank) {
        info.solutionType = SolutionType::NO_SOLUTION;
        info.description = "无解: 系数矩阵的秩小于增广矩阵的秩";
    } else if (info.coefficientRank == info.augmentedRank && 
               info.coefficientRank == info.numVariables) {
        info.solutionType = SolutionType::UNIQUE_SOLUTION;
        info.description = "唯一解: 系数矩阵的秩等于变量个数";
    } else if (info.coefficientRank == info.augmentedRank && 
               info.coefficientRank < info.numVariables) {
        info.solutionType = SolutionType::INFINITE_SOLUTIONS;
        info.description = "无穷多解: 系数矩阵的秩小于变量个数";
    } else {
        info.solutionType = SolutionType::UNDETERMINED;
        info.description = "未确定的情况";
    }
    
    return info;
}

EquationSystemInfo EquationSolver::analyzeHomogeneousSystem(const Matrix& A) {
    EquationSystemInfo info;
    
    info.numEquations = A.rowCount();
    info.numVariables = A.colCount();
    info.coefficientRank = MatrixOperations::rank(A);
    info.augmentedRank = info.coefficientRank; // 齐次系统增广矩阵秩等于系数矩阵秩
    
    // 齐次系统总是有解(至少有零解)
    if (info.coefficientRank == info.numVariables) {
        info.solutionType = SolutionType::UNIQUE_SOLUTION;
        info.description = "仅有零解";
    } else {
        info.solutionType = SolutionType::INFINITE_SOLUTIONS;
        info.description = "有非零解(无穷多解)";
    }
    
    return info;
}

Matrix EquationSolver::findParticularSolution(const Matrix& rref, const Matrix& b_rref, 
                                             const std::vector<int>& pivotCols) {
    size_t n = rref.colCount();
    Matrix solution(n, 1);
    
    // 初始化所有变量为0
    for (size_t i = 0; i < n; ++i) {
        solution.at(i, 0) = Fraction(0);
    }
    
    // 对每个主元列设置对应的解
    for (size_t i = 0; i < pivotCols.size() && i < rref.rowCount(); ++i) {
        if (pivotCols[i] >= 0 && pivotCols[i] < static_cast<int>(n)) {
            solution.at(pivotCols[i], 0) = b_rref.at(i, 0);
        }
    }
    
    return solution;
}

Matrix EquationSolver::findHomogeneousSolutions(const Matrix& rref, 
                                               const std::vector<int>& pivotCols) {
    size_t n = rref.colCount();
    size_t numPivots = pivotCols.size();
    
    // 找到自由变量
    std::vector<int> freeVars;
    std::vector<bool> isPivot(n, false);
    
    for (int col : pivotCols) {
        if (col >= 0 && col < static_cast<int>(n)) {
            isPivot[col] = true;
        }
    }
    
    for (size_t j = 0; j < n; ++j) {
        if (!isPivot[j]) {
            freeVars.push_back(j);
        }
    }
    
    if (freeVars.empty()) {
        // 没有自由变量，返回空矩阵
        return Matrix(n, 0);
    }
    
    // 为每个自由变量构造一个基础解
    Matrix solutions(n, freeVars.size());
    
    for (size_t k = 0; k < freeVars.size(); ++k) {
        // 初始化当前解向量
        for (size_t i = 0; i < n; ++i) {
            solutions.at(i, k) = Fraction(0);
        }
        
        // 自由变量设为1
        solutions.at(freeVars[k], k) = Fraction(1);
        
        // 计算基本变量的值
        for (size_t i = 0; i < numPivots && i < rref.rowCount(); ++i) {
            if (pivotCols[i] >= 0 && pivotCols[i] < static_cast<int>(n)) {
                Fraction value(0);
                for (size_t j = pivotCols[i] + 1; j < n; ++j) {
                    value = value - rref.at(i, j) * solutions.at(j, k);
                }
                solutions.at(pivotCols[i], k) = value;
            }
        }
    }
    
    return solutions;
}

std::vector<int> EquationSolver::findPivotColumns(const Matrix& rref) {
    std::vector<int> pivotCols;
    
    for (size_t i = 0; i < rref.rowCount(); ++i) {
        for (size_t j = 0; j < rref.colCount(); ++j) {
            if (rref.at(i, j) != Fraction(0)) {
                pivotCols.push_back(j);
                break;
            }
        }
    }
    
    return pivotCols;
}

std::string EquationSolver::generateSolutionDescription(const EquationSolution& solution) {
    std::stringstream ss;
    const auto& info = solution.getSystemInfo();
    
    ss << "方程组分析:\n"; // 此处已包含方程数、变量数、秩等信息
    ss << "- 方程个数: " << info.numEquations << "\n";
    ss << "- 变量个数: " << info.numVariables << "\n";
    ss << "- 系数矩阵的秩: " << info.coefficientRank << "\n";
    ss << "- 增广矩阵的秩: " << info.augmentedRank << "\n";
    
    switch (solution.getSolutionType()) {
        case SolutionType::UNIQUE_SOLUTION:
            ss << "- 结论: 方程组有唯一解\n";
            break;
        case SolutionType::INFINITE_SOLUTIONS:
            ss << "- 结论: 方程组有无穷多解\n";
            ss << "- 自由变量个数: " << (info.numVariables - info.coefficientRank) << "\n";
            break;
        case SolutionType::NO_SOLUTION:
            ss << "- 结论: 方程组无解\n";
            break;
        default:
            ss << "- 结论: 未确定\n";
            break;
    }
    
    return ss.str();
}
