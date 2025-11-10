#pragma once

#include <graph/graph.hpp>
#include <vector>
#include <utility>

namespace forge_xad {

/**
 * @brief Infers Forge OpCode from XAD tape operation data
 *
 * XAD stores operations as (multiplier, slot) pairs without explicit
 * operation types. This class uses pattern matching to infer the
 * likely operation type.
 */
class OperationInference {
public:
    /**
     * @brief A single operand in XAD's tape format
     */
    struct Operand {
        double multiplier;
        unsigned int slot;
    };

    /**
     * @brief Infer OpCode from operation pattern
     *
     * @param operands The (multiplier, slot) pairs from XAD tape
     * @return Inferred Forge OpCode
     */
    static forge::OpCode inferOpCode(const std::vector<Operand>& operands);

    /**
     * @brief Check if pattern matches a unary operation
     */
    static bool isUnaryOp(const std::vector<Operand>& operands);

    /**
     * @brief Check if pattern matches a binary operation
     */
    static bool isBinaryOp(const std::vector<Operand>& operands);

    /**
     * @brief Check if pattern represents negation (x * -1.0)
     */
    static bool isNegation(const std::vector<Operand>& operands);

    /**
     * @brief Check if pattern represents addition (x * 1.0 + y * 1.0)
     */
    static bool isAddition(const std::vector<Operand>& operands);

    /**
     * @brief Check if pattern represents subtraction (x * 1.0 + y * -1.0)
     */
    static bool isSubtraction(const std::vector<Operand>& operands);

    /**
     * @brief Check if pattern represents multiplication (x * y)
     */
    static bool isMultiplication(const std::vector<Operand>& operands);

    /**
     * @brief Check if pattern represents scalar multiplication (x * constant)
     */
    static bool isScalarMultiplication(const std::vector<Operand>& operands, double& constant);

    /**
     * @brief Check if any operands have non-unity multipliers (weighted sum)
     *
     * @return true if we need to expand with scalar multiply nodes
     */
    static bool hasWeightedOperands(const std::vector<Operand>& operands);

private:
    static constexpr double EPSILON = 1e-14;

    static bool isApproximately(double a, double b) {
        return std::abs(a - b) < EPSILON;
    }
};

} // namespace forge_xad
