#include "forge_xad/operation_inference.hpp"
#include <cmath>

namespace forge_xad {

forge::OpCode OperationInference::inferOpCode(const std::vector<Operand>& operands) {
    // Infer operation type from XAD tape's (multiplier, slot) patterns
    //
    // Note: XAD stores operations as weighted sums during recording.
    // Some operations (like Mul, Div, transcendentals) are NOT directly
    // detectable from this format and will require different handling.

    if (operands.empty()) {
        // No operands = constant value
        return forge::OpCode::Constant;
    }

    if (operands.size() == 1) {
        // Single operand: could be negation, scalar multiply, or identity
        double m = operands[0].multiplier;

        if (isApproximately(m, -1.0)) {
            return forge::OpCode::Neg;  // -x
        }

        if (isApproximately(m, 1.0)) {
            // Identity: just pass through the value
            // This happens for simple assignments like z = x
            return forge::OpCode::Input;
        }

        // Scalar multiplication: m * x where m != 1.0 and m != -1.0
        // Example: z = 2.0 * x → [(2.0, slot_x)]
        // We'll need to handle this specially in the converter
        // by creating a Mul node with a Constant node
        return forge::OpCode::Mul;
    }

    if (operands.size() == 2) {
        // Two operands: binary operation
        double m0 = operands[0].multiplier;
        double m1 = operands[1].multiplier;

        if (isApproximately(m0, 1.0) && isApproximately(m1, 1.0)) {
            // z = 1.0*x + 1.0*y → z = x + y
            return forge::OpCode::Add;
        }

        if (isApproximately(m0, 1.0) && isApproximately(m1, -1.0)) {
            // z = 1.0*x + (-1.0)*y → z = x - y
            return forge::OpCode::Sub;
        }

        // Weighted sum: z = m0*x + m1*y
        // This is a linear combination, treat as Add
        // The converter will need to insert scalar multiply nodes first
        return forge::OpCode::Add;
    }

    // More than 2 operands: linear combination
    // Example: z = m0*x + m1*y + m2*w
    // Treat as nested additions in the converter
    return forge::OpCode::Add;
}

bool OperationInference::isUnaryOp(const std::vector<Operand>& operands) {
    return operands.size() == 1;
}

bool OperationInference::isBinaryOp(const std::vector<Operand>& operands) {
    return operands.size() == 2;
}

bool OperationInference::isNegation(const std::vector<Operand>& operands) {
    if (operands.size() != 1) return false;
    return isApproximately(operands[0].multiplier, -1.0);
}

bool OperationInference::isAddition(const std::vector<Operand>& operands) {
    if (operands.size() != 2) return false;
    return isApproximately(operands[0].multiplier, 1.0) &&
           isApproximately(operands[1].multiplier, 1.0);
}

bool OperationInference::isSubtraction(const std::vector<Operand>& operands) {
    if (operands.size() != 2) return false;
    return isApproximately(operands[0].multiplier, 1.0) &&
           isApproximately(operands[1].multiplier, -1.0);
}

bool OperationInference::isMultiplication(const std::vector<Operand>& operands) {
    // Multiplication is harder to detect from tape format
    // This is a placeholder - needs more sophisticated logic
    return false;
}

bool OperationInference::isScalarMultiplication(const std::vector<Operand>& operands, double& constant) {
    if (operands.size() != 1) return false;
    constant = operands[0].multiplier;
    return !isApproximately(constant, 1.0) && !isApproximately(constant, -1.0);
}

bool OperationInference::hasWeightedOperands(const std::vector<Operand>& operands) {
    // Check if any operand has a multiplier != 1.0 or -1.0
    // This indicates we need to expand with scalar multiply nodes
    for (const auto& op : operands) {
        if (!isApproximately(op.multiplier, 1.0) &&
            !isApproximately(op.multiplier, -1.0)) {
            return true;
        }
    }
    return false;
}

} // namespace forge_xad
