/**
 * @file jit_tape_simple.cpp
 * @brief Simplified JITTape test without Forge execution
 *
 * Tests that JITTape compiles and can record/convert tapes.
 * Does not execute compiled kernels yet (Phase 2.4).
 */

#include "forge_xad/jit_tape.hpp"
#include <iostream>

int main() {
    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    std::cout << "========================================\n";
    std::cout << "JITTape Simple Test (No Kernel Execution)\n";
    std::cout << "========================================\n\n";

    // Use JITTape wrapper
    forge_xad::JITTape<tape_type> tape;

    std::cout << "Test 1: Basic tape operations\n";

    // Create inputs
    AD x = 0.0, y = 0.0;
    value(x) = 3.0;
    value(y) = 4.0;

    // Register inputs
    tape.registerInput(x);
    tape.registerInput(y);

    // Start recording
    tape.newRecording();

    // Compute function: f(x, y) = x + y
    AD result = x + y;

    // Register output (triggers compilation attempt)
    std::cout << "Registering output (will attempt compilation)...\n";
    tape.registerOutput(result);

    // Use tape-based computation (kernel execution not yet implemented)
    derivative(result) = 1.0;
    tape.computeAdjoints();

    // Extract results
    double output = value(result);
    double grad_x = derivative(x);
    double grad_y = derivative(y);

    std::cout << "\nResults:\n";
    std::cout << "  f(" << value(x) << ", " << value(y) << ") = " << output << "\n";
    std::cout << "  df/dx = " << grad_x << "\n";
    std::cout << "  df/dy = " << grad_y << "\n";

    std::cout << "\n========================================\n";
    std::cout << "JITTape Status:\n";
    std::cout << "  Compiled: " << (tape.isCompiled() ? "Yes" : "No") << "\n";
    std::cout << "  Input slots: " << tape.getInputSlots().size() << "\n";
    std::cout << "  Output slots: " << tape.getOutputSlots().size() << "\n";
    std::cout << "========================================\n";

    if (std::abs(output - 7.0) < 1e-10 &&
        std::abs(grad_x - 1.0) < 1e-10 &&
        std::abs(grad_y - 1.0) < 1e-10) {
        std::cout << "\n✓ Test passed! JITTape wrapper works correctly.\n";
        std::cout << "  (Kernel execution will be implemented in Phase 2.4)\n";
        return 0;
    } else {
        std::cout << "\n✗ Test failed!\n";
        return 1;
    }
}
