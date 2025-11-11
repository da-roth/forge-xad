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
    const auto& op_types = tape.getOpTypes();

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

        // Get operation type directly from tape (no inference needed!)
        xad::OpCode xad_opcode = op_types[stmt_idx];

        forge::NodeId result_node_id;

        // Handle special XAD opcodes that don't map directly to Forge
        if (xad_opcode == xad::OpCode::Assign && operands.size() == 1) {
            // Assignment: just pass through the existing node
            slot_to_node[lhs_slot] = slot_to_node[operands[0].slot];
            continue;
        }

        // Handle scalar operations by converting to binary op with constant
        if ((xad_opcode == xad::OpCode::ScalarMul || xad_opcode == xad::OpCode::ScalarAdd ||
             xad_opcode == xad::OpCode::ScalarSub1 || xad_opcode == xad::OpCode::ScalarSub2 ||
             xad_opcode == xad::OpCode::ScalarDiv1 || xad_opcode == xad::OpCode::ScalarDiv2) &&
            operands.size() == 1) {

            double scalar_value = operands[0].multiplier;
            forge::NodeId operand_id = slot_to_node[operands[0].slot];

            // Create constant node for scalar
            forge::Node const_node;
            const_node.op = forge::OpCode::Constant;
            const_node.a = 0;
            const_node.b = 0;
            const_node.c = 0;
            const_node.imm = static_cast<double>(result.graph.constPool.size());
            const_node.isActive = false;
            const_node.isDead = false;
            const_node.needsGradient = false;

            forge::NodeId const_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(const_node);
            result.graph.constPool.push_back(scalar_value);

            // Create binary operation node
            forge::Node binary_node;
            if (xad_opcode == xad::OpCode::ScalarMul) {
                binary_node.op = forge::OpCode::Mul;
                binary_node.a = const_node_id;
                binary_node.b = operand_id;
            } else if (xad_opcode == xad::OpCode::ScalarAdd) {
                binary_node.op = forge::OpCode::Add;
                binary_node.a = operand_id;
                binary_node.b = const_node_id;
            } else if (xad_opcode == xad::OpCode::ScalarSub1) {
                // c - x
                binary_node.op = forge::OpCode::Sub;
                binary_node.a = const_node_id;
                binary_node.b = operand_id;
            } else if (xad_opcode == xad::OpCode::ScalarSub2) {
                // x - c
                binary_node.op = forge::OpCode::Sub;
                binary_node.a = operand_id;
                binary_node.b = const_node_id;
            } else if (xad_opcode == xad::OpCode::ScalarDiv1) {
                // c / x
                binary_node.op = forge::OpCode::Div;
                binary_node.a = const_node_id;
                binary_node.b = operand_id;
            } else { // ScalarDiv2
                // x / c
                binary_node.op = forge::OpCode::Div;
                binary_node.a = operand_id;
                binary_node.b = const_node_id;
            }
            binary_node.c = 0;
            binary_node.imm = 0.0;
            binary_node.isActive = true;
            binary_node.isDead = false;
            binary_node.needsGradient = result.graph.nodes[operand_id].needsGradient;

            result_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(binary_node);

            // Map this slot to the result node
            slot_to_node[lhs_slot] = result_node_id;
            continue;
        }

        forge::OpCode opcode = static_cast<forge::OpCode>(static_cast<uint16_t>(xad_opcode));

        // Handle different operation types
        if (opcode == forge::OpCode::Neg ||
            opcode == forge::OpCode::Exp || opcode == forge::OpCode::Log ||
            opcode == forge::OpCode::Sqrt ||
            opcode == forge::OpCode::Sin || opcode == forge::OpCode::Cos ||
            opcode == forge::OpCode::Tan ||
            opcode == forge::OpCode::Abs || opcode == forge::OpCode::Square ||
            opcode == forge::OpCode::Recip) {
            // Unary operations
            forge::NodeId operand_id = slot_to_node[operands[0].slot];

            forge::Node unary_node;
            unary_node.op = opcode;
            unary_node.a = operand_id;
            unary_node.b = 0;
            unary_node.c = 0;
            unary_node.imm = 0.0;
            unary_node.isActive = true;
            unary_node.isDead = false;
            // Forward propagation: node needs gradient if operand needs gradient
            unary_node.needsGradient = result.graph.nodes[operand_id].needsGradient;

            result_node_id = static_cast<forge::NodeId>(result.graph.nodes.size());
            result.graph.nodes.push_back(unary_node);
        }
        else if (opcode == forge::OpCode::Add || opcode == forge::OpCode::Sub ||
                 opcode == forge::OpCode::Mul || opcode == forge::OpCode::Div ||
                 opcode == forge::OpCode::Pow ||
                 opcode == forge::OpCode::Max || opcode == forge::OpCode::Min) {
            // Binary operations
            if (operands.size() != 2) {
                std::cerr << "Warning: Binary operation with " << operands.size() << " operands" << std::endl;
                continue;
            }

            forge::NodeId a_id = slot_to_node[operands[0].slot];
            forge::NodeId b_id = slot_to_node[operands[1].slot];

            forge::Node binary_node;
            binary_node.op = opcode;
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
        else {
            // Unsupported operation - throw exception
            std::string error_msg = "Unsupported XAD operation OpCode=" +
                                  std::to_string(static_cast<int>(xad_opcode)) +
                                  " (Forge OpCode=" + std::to_string(static_cast<int>(opcode)) + ")" +
                                  " with " + std::to_string(operands.size()) + " operands. " +
                                  "This operation is not yet supported in Forge.";
            throw std::runtime_error(error_msg);
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
