#include "determinant_expansion.h"

// ExpansionStep 实现
ExpansionStep::ExpansionStep(
    ExpansionType type,
    const std::string& desc,
    const Matrix& matrix,
    int expIndex,
    int elemIndex,
    const Fraction& elem,
    const Fraction& cof,
    const Fraction& termVal,
    const Fraction& accVal
) : type(type), description(desc), matrixState(matrix),
    expansionIndex(expIndex), elementIndex(elemIndex),
    element(elem), cofactor(cof), termValue(termVal),
    accumulatedValue(accVal) {}

ExpansionType ExpansionStep::getType() const { return type; }
std::string ExpansionStep::getDescription() const { return description; }
const Matrix& ExpansionStep::getMatrixState() const { return matrixState; }
int ExpansionStep::getExpansionIndex() const { return expansionIndex; }
int ExpansionStep::getElementIndex() const { return elementIndex; }
Fraction ExpansionStep::getElement() const { return element; }
Fraction ExpansionStep::getCofactor() const { return cofactor; }
Fraction ExpansionStep::getTermValue() const { return termValue; }
Fraction ExpansionStep::getAccumulatedValue() const { return accumulatedValue; }

void ExpansionStep::print(std::ostream& os) const {
    os << description << "\n";
    
    if (type != ExpansionType::INITIAL_STATE && type != ExpansionType::RESULT_STATE) {
        if (type == ExpansionType::ROW_EXPANSION || type == ExpansionType::COLUMN_EXPANSION) {
            os << "展开" << (type == ExpansionType::ROW_EXPANSION ? "行" : "列") 
               << "索引: " << (expansionIndex + 1) << "\n";
        }
        
        if (elementIndex >= 0) {
            os << "处理元素索引: " << (elementIndex + 1) << "\n";
            os << "元素值: " << element << "\n";
            os << "代数余子式: " << cofactor << "\n";
            os << "项值: " << termValue << "\n";
            os << "当前累积和: " << accumulatedValue << "\n";
        }
    }
    
    matrixState.print(os);
    os << "\n";
}

// ExpansionHistory 实现
void ExpansionHistory::addStep(const ExpansionStep& step) {
    steps.push_back(step);
}

size_t ExpansionHistory::size() const {
    return steps.size();
}

const ExpansionStep& ExpansionHistory::getStep(size_t index) const {
    if (index >= steps.size()) {
        throw std::out_of_range("Step index out of range");
    }
    return steps[index];
}

void ExpansionHistory::printAll(std::ostream& os) const {
    if (steps.empty()) {
        os << "No expansion steps recorded.\n";
        return;
    }

    for (size_t i = 0; i < steps.size(); ++i) {
        os << "Step " << i << ": ";
        steps[i].print(os);
    }
}

void ExpansionHistory::printStep(size_t index, std::ostream& os) const {
    if (index >= steps.size()) {
        os << "Step index out of range.\n";
        return;
    }

    os << "Step " << index << ": ";
    steps[index].print(os);
}

void ExpansionHistory::clear() {
    steps.clear();
}
