/**
 * @file test_converter.cpp
 * @brief Test XAD tape to Forge graph conversion
 *
 * Tests the basic converter functionality with simple operations.
 */

#include "forge_xad/xad_tape_converter.hpp"
#include "forge_xad/operation_inference.hpp"
#include <XAD/XAD.hpp>
#include <iostream>
#include <iomanip>

void printGraph(const forge::Graph& graph) {
    std::cout << "\nForge Graph Structure:\n";
    std::cout << "=====================\n";
    std::cout << "Total nodes: " << graph.nodes.size() << "\n";
    std::cout << "Constant pool size: " << graph.constPool.size() << "\n";
    std::cout << "Outputs: " << graph.outputs.size() << "\n\n";

    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        const auto& node = graph.nodes[i];
        std::cout << "Node " << i << ": ";

        switch (node.op) {
            case forge::OpCode::Input:
                std::cout << "Input";
                break;
            case forge::OpCode::Constant:
                std::cout << "Constant (value=" << graph.constPool[static_cast<size_t>(node.imm)] << ")";
                break;
            case forge::OpCode::Add:
                std::cout << "Add (a=" << node.a << ", b=" << node.b << ")";
                break;
            case forge::OpCode::Sub:
                std::cout << "Sub (a=" << node.a << ", b=" << node.b << ")";
                break;
            case forge::OpCode::Mul:
                std::cout << "Mul (a=" << node.a << ", b=" << node.b << ")";
                break;
            case forge::OpCode::Neg:
                std::cout << "Neg (a=" << node.a << ")";
                break;
            default:
                std::cout << "Unknown";
        }

        std::cout << " [active=" << node.isActive
                  << ", dead=" << node.isDead << "]\n";
    }

    std::cout << "\nOutput nodes: ";
    for (auto out : graph.outputs) {
        std::cout << out << " ";
    }
    std::cout << "\n";
}

bool testSimpleAddition() {
    std::cout << "\n=== Test 1: Simple Addition (z = x + y) ===\n";

    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    tape_type tape;

    // Create inputs
    AD x = 0.0, y = 0.0;
    value(x) = 3.0;
    value(y) = 4.0;

    // Register inputs and start recording
    tape.registerInput(x);
    tape.registerInput(y);
    tape.newRecording();

    // Perform operation: z = x + y
    AD z = x + y;

    tape.registerOutput(z);

    std::cout << "XAD computation: x=" << value(x) << ", y=" << value(y)
              << ", z=" << value(z) << "\n";

    // Debug: Check tape contents before conversion
    std::cout << "XAD tape info:\n";
    std::cout << "  Input slots: " << tape.getInputSlots().size() << "\n";
    std::cout << "  Output slots: " << tape.getOutputSlots().size() << "\n";
    std::cout << "  Statements: " << tape.getStatements().size() << "\n";
    std::cout << "  Operations: " << tape.getOperations().size() << "\n";

    // Convert tape to Forge graph
    auto result = forge_xad::convertXadTapeToForge(tape);

    // Print the graph
    printGraph(result.graph);

    // Verify structure
    std::cout << "\nVerification:\n";
    bool ok = true;

    if (result.input_nodes.size() != 2) {
        std::cout << "✗ Expected 2 input nodes, got " << result.input_nodes.size() << "\n";
        ok = false;
    } else {
        std::cout << "✓ Correct number of input nodes (2)\n";
    }

    if (result.output_nodes.size() != 1) {
        std::cout << "✗ Expected 1 output node, got " << result.output_nodes.size() << "\n";
        ok = false;
    } else {
        std::cout << "✓ Correct number of output nodes (1)\n";
    }

    // Should have at least: 2 inputs + 1 add operation = 3 nodes minimum
    if (result.graph.nodes.size() < 3) {
        std::cout << "✗ Expected at least 3 nodes, got " << result.graph.nodes.size() << "\n";
        ok = false;
    } else {
        std::cout << "✓ Graph has sufficient nodes\n";
    }

    return ok;
}

bool testSimpleSubtraction() {
    std::cout << "\n=== Test 2: Simple Subtraction (z = x - y) ===\n";

    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    tape_type tape;

    AD x = 0.0, y = 0.0;
    value(x) = 10.0;
    value(y) = 3.0;

    tape.registerInput(x);
    tape.registerInput(y);
    tape.newRecording();

    AD z = x - y;

    tape.registerOutput(z);

    std::cout << "XAD computation: x=" << value(x) << ", y=" << value(y)
              << ", z=" << value(z) << "\n";

    auto result = forge_xad::convertXadTapeToForge(tape);
    printGraph(result.graph);

    std::cout << "\nVerification:\n";
    bool ok = true;

    if (result.input_nodes.size() != 2) {
        std::cout << "✗ Expected 2 input nodes\n";
        ok = false;
    } else {
        std::cout << "✓ Correct number of input nodes\n";
    }

    if (result.output_nodes.size() != 1) {
        std::cout << "✗ Expected 1 output node\n";
        ok = false;
    } else {
        std::cout << "✓ Correct number of output nodes\n";
    }

    return ok;
}

bool testNegation() {
    std::cout << "\n=== Test 3: Negation (z = -x) ===\n";

    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    tape_type tape;

    AD x = 0.0;
    value(x) = 5.0;

    tape.registerInput(x);
    tape.newRecording();

    AD z = -x;

    tape.registerOutput(z);

    std::cout << "XAD computation: x=" << value(x) << ", z=" << value(z) << "\n";

    auto result = forge_xad::convertXadTapeToForge(tape);
    printGraph(result.graph);

    std::cout << "\nVerification:\n";
    bool ok = true;

    if (result.input_nodes.size() != 1) {
        std::cout << "✗ Expected 1 input node\n";
        ok = false;
    } else {
        std::cout << "✓ Correct number of input nodes\n";
    }

    return ok;
}

bool testScalarMultiplication() {
    std::cout << "\n=== Test 4: Scalar Multiplication (z = 2.5 * x) ===\n";

    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    tape_type tape;

    AD x = 0.0;
    value(x) = 4.0;

    tape.registerInput(x);
    tape.newRecording();

    AD z = 2.5 * x;

    tape.registerOutput(z);

    std::cout << "XAD computation: x=" << value(x) << ", z=" << value(z) << "\n";

    auto result = forge_xad::convertXadTapeToForge(tape);
    printGraph(result.graph);

    std::cout << "\nVerification:\n";
    bool ok = true;

    if (result.input_nodes.size() != 1) {
        std::cout << "✗ Expected 1 input node\n";
        ok = false;
    } else {
        std::cout << "✓ Correct number of input nodes\n";
    }

    // Should have: 1 input + 1 constant + 1 multiply = 3 nodes
    if (result.graph.constPool.size() != 1) {
        std::cout << "✗ Expected 1 constant (2.5)\n";
        ok = false;
    } else {
        std::cout << "✓ Constant pool has 1 entry (value="
                  << result.graph.constPool[0] << ")\n";
    }

    return ok;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "XAD Tape to Forge Graph Converter Tests\n";
    std::cout << "========================================\n";

    bool all_passed = true;

    all_passed &= testSimpleAddition();
    all_passed &= testSimpleSubtraction();
    all_passed &= testNegation();
    all_passed &= testScalarMultiplication();

    std::cout << "\n========================================\n";
    if (all_passed) {
        std::cout << "✓ All converter tests passed!\n";
        return 0;
    } else {
        std::cout << "✗ Some tests failed!\n";
        return 1;
    }
}
