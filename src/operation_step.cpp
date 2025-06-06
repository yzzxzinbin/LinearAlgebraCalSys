#include "operation_step.h"

OperationStep::OperationStep(OperationType type, const std::string& desc, const Matrix& matrix, 
                            int r1, int r2, const Fraction& scalar)
    : type(type), description(desc), matrixState(matrix), row1(r1), row2(r2), scalar(scalar) {}

OperationType OperationStep::getType() const { return type; }
std::string OperationStep::getDescription() const { return description; }
const Matrix& OperationStep::getMatrixState() const { return matrixState; }
int OperationStep::getRow1() const { return row1; }
int OperationStep::getRow2() const { return row2; }
Fraction OperationStep::getScalar() const { return scalar; }

void OperationStep::print(std::ostream& os) const {
    os << description << "\n";
    matrixState.print(os);
    os << "\n";
}

// OperationHistory 实现
void OperationHistory::addStep(const OperationStep& step) {
    steps.push_back(step);
}

size_t OperationHistory::size() const {
    return steps.size();
}

const OperationStep& OperationHistory::getStep(size_t index) const {
    if (index >= steps.size()) {
        throw std::out_of_range("Step index out of range");
    }
    return steps[index];
}

void OperationHistory::printAll(std::ostream& os) const {
    if (steps.empty()) {
        os << "No operations recorded.\n";
        return;
    }

    for (size_t i = 0; i < steps.size(); ++i) {
        os << "Step " << i << ": ";
        steps[i].print(os);
    }
}

void OperationHistory::printStep(size_t index, std::ostream& os) const {
    if (index >= steps.size()) {
        os << "Step index out of range.\n";
        return;
    }

    os << "Step " << index << ": ";
    steps[index].print(os);
}

void OperationHistory::clear() {
    steps.clear();
}
