#include "forge_xad/xad_tape_converter.hpp"
#include "forge_xad/operation_inference.hpp"
#include <stdexcept>
#include <iostream>

namespace forge_xad {

template<class Real, std::size_t N>
ConversionResult convertXadTapeToForge(const xad::Tape<Real, N>& tape) {
    ConversionResult result;

    // Map XAD slot IDs to Forge node IDs
    std::unordered_map<unsigned int, forge::NodeId> slot_to_node;

    // Step 1: Create input nodes
    const auto& input_slots = tape.getInputSlots();
    for (auto slot : input_slots) {
        forge::NodeId node_id = static_cast<forge::NodeId>(result.graph.nodes.size());

        forge::Node input_node;
        input_node.op = forge::OpCode::Input;
        input_node.a = 0;
        input_node.b = 0;
        input_node.c = 0;
        input_node.imm = 0.0;
        input_node.isActive = true;
        input_node.isDead = false;
        input_node.needsGradient = true;  // All inputs need gradients for AD

        result.graph.nodes.push_back(input_node);
        slot_to_node[slot] = node_id;
        result.input_nodes.push_back(node_id);

        // Mark input for differentiation so buffer allocates gradients
        result.graph.diff_inputs.push_back(node_id);
    }

    // Step 2: Process statements
    const auto& statements = tape.getStatements();
    const auto& operations = tape.getOperations();


    // Skip first statement (it's a dummy entry from XAD)
    for (size_t stmt_idx = 1; stmt_idx < statements.size(); ++stmt_idx) {
        auto statement = statements[stmt_idx];
        unsigned int op_end_idx = statement.first;  // Operations END at this statement's index
        unsigned int lhs_slot = statement.second;

        // Skip invalid statements
        if (lhs_slot == tape.INVALID_SLOT) {
            continue;
        }

        // Operations for this statement are from previous statement to current
        unsigned int op_start_idx = statements[stmt_idx - 1].first;

        std::vector<OperationInference::Operand> operands;
        for (unsigned int op_idx = op_start_idx; op_idx < op_end_idx; ++op_idx) {
            auto op_pair = operations[op_idx];
            operands.push_back({op_pair.first, op_pair.second});
        }

        // Skip empty operations (shouldn't happen, but be safe)
        if (operands.empty()) {
            continue;
        }

        // Infer operation type
        forge::OpCode opcode = OperationInference::inferOpCode(operands);

        forge::NodeId result_node_id;

        // Handle different operation types
        if (opcode == forge::OpCode::Neg) {
            // Unary negation: -x
            forge::NodeId operand_id = slot_to_node[operands[0].slot];

            forge::Node neg_node;
            neg_node.op = forge::OpCode::Neg;
            neg_node.a = operand_id;
            neg_node.b = 0;
            neg_node.c = 0;
            neg_node.imm = 0.0;
            neg_node.isActive = true;
            neg_node.isDead = false;
            // Forward propagation: node needs gradient if operand needs gradient
            neg_node.needsGradient = result.graph.nodes[operand_id].needsGradient;

            result_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(neg_node);
        }
        else if ((opcode == forge::OpCode::Add || opcode == forge::OpCode::Sub) && operands.size() == 2) {
            // Binary addition or subtraction
            forge::OpCode actual_op = OperationInference::isSubtraction(operands)
                ? forge::OpCode::Sub
                : forge::OpCode::Add;

            forge::NodeId a_id = slot_to_node[operands[0].slot];
            forge::NodeId b_id = slot_to_node[operands[1].slot];

            forge::Node binary_node;
            binary_node.op = actual_op;
            binary_node.a = a_id;
            binary_node.b = b_id;
            binary_node.c = 0;
            binary_node.imm = 0.0;
            binary_node.isActive = true;
            binary_node.isDead = false;
            // Forward propagation: node needs gradient if ANY operand needs gradient
            binary_node.needsGradient = result.graph.nodes[a_id].needsGradient ||
                                        result.graph.nodes[b_id].needsGradient;

            result_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(binary_node);
        }
        else if (opcode == forge::OpCode::Mul && operands.size() == 1) {
            // Scalar multiplication: m * x
            double multiplier = operands[0].multiplier;
            forge::NodeId operand_id = slot_to_node[operands[0].slot];

            // Create constant node for multiplier
            forge::Node const_node;
            const_node.op = forge::OpCode::Constant;
            const_node.a = 0;
            const_node.b = 0;
            const_node.c = 0;
            const_node.imm = static_cast<double>(result.graph.constPool.size());
            const_node.isActive = false;  // Constants are inactive
            const_node.isDead = false;
            const_node.needsGradient = false;  // Constants never need gradients

            forge::NodeId const_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(const_node);
            result.graph.constPool.push_back(multiplier);

            // Create multiply node
            forge::Node mul_node;
            mul_node.op = forge::OpCode::Mul;
            mul_node.a = operand_id;
            mul_node.b = const_node_id;
            mul_node.c = 0;
            mul_node.imm = 0.0;
            mul_node.isActive = true;
            mul_node.isDead = false;
            // Forward propagation: multiply needs gradient if operand needs gradient
            // (constant never needs gradient, so we only check the operand)
            mul_node.needsGradient = result.graph.nodes[operand_id].needsGradient;

            result_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(mul_node);
        }
        else if (opcode == forge::OpCode::Input) {
            // Identity: just pass through
            result_node_id = slot_to_node[operands[0].slot];
        }
        else {
            // Unsupported operation for now
            std::cerr << "Warning: Unsupported operation pattern with "
                      << operands.size() << " operands" << std::endl;

            // Create a dummy node to keep going
            forge::Node dummy_node;
            dummy_node.op = forge::OpCode::Input;
            dummy_node.a = 0;
            dummy_node.b = 0;
            dummy_node.c = 0;
            dummy_node.imm = 0.0;
            dummy_node.isActive = false;
            dummy_node.isDead = true;
            dummy_node.needsGradient = false;

            result_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(dummy_node);
        }

        // Map this slot to the result node
        slot_to_node[lhs_slot] = result_node_id;
    }

    // Step 3: Mark outputs
    const auto& output_slots = tape.getOutputSlots();
    for (auto slot : output_slots) {
        if (slot_to_node.find(slot) != slot_to_node.end()) {
            forge::NodeId output_node_id = slot_to_node[slot];
            result.output_nodes.push_back(output_node_id);
            result.graph.outputs.push_back(output_node_id);
        }
    }

    // Store the mapping
    result.slot_to_node = slot_to_node;

    return result;
}

// Explicit template instantiation for common types
template ConversionResult convertXadTapeToForge<double, 1>(const xad::Tape<double, 1>&);

} // namespace forge_xad
