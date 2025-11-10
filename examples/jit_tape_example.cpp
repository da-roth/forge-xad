/**
 * @file jit_tape_example.cpp
 * @brief Example demonstrating the JITTape wrapper
 *
 * Shows the "1-line change" API: just change the tape type
 * to get automatic JIT compilation.
 */

#include "forge_xad/jit_tape.hpp"
#include <iostream>
#include <iomanip>

int main() {
    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    std::cout << "========================================\n";
    std::cout << "JITTape Example: Auto-Compilation Demo\n";
    std::cout << "========================================\n\n";

    // THIS IS THE ONLY LINE THAT CHANGES!
    // Instead of: tape_type tape;
    forge_xad::JITTape<tape_type> tape;

    std::cout << "Running 5 iterations to demonstrate auto-compilation:\n\n";

    for (int iter = 0; iter < 5; ++iter) {
        std::cout << "--- Iteration " << iter << " ---\n";

        // Create inputs
        AD x = 0.0, y = 0.0;
        value(x) = 3.0 + iter;
        value(y) = 4.0 + iter;

        // Register inputs
        tape.registerInput(x);
        tape.registerInput(y);

        // Start recording
        tape.newRecording();

        // Compute function: f(x, y) = x + y
        AD result = x + y;

        // Register output (triggers compilation on first iteration)
        tape.registerOutput(result);

        // Compute gradients
        derivative(result) = 1.0;
        tape.computeAdjoints();

        // Extract results
        double output = value(result);
        double grad_x = derivative(x);
        double grad_y = derivative(y);

        std::cout << "  Inputs: x=" << value(x) << ", y=" << value(y) << "\n";
        std::cout << "  Output: f(x,y) = " << output << "\n";
        std::cout << "  Gradients: df/dx = " << grad_x << ", df/dy = " << grad_y << "\n";

        if (iter == 0) {
            std::cout << "  → First iteration: recorded and compiled\n";
        } else {
            std::cout << "  → Using compiled kernel (once implemented)\n";
        }

        // Clear for next iteration
        tape.clearAll();

        std::cout << "\n";
    }

    std::cout << "========================================\n";
    std::cout << "JITTape Status:\n";
    std::cout << "  Compiled: " << (tape.isCompiled() ? "Yes" : "No") << "\n";
    std::cout << "  Inputs: " << tape.getInputSlots().size() << "\n";
    std::cout << "  Outputs: " << tape.getOutputSlots().size() << "\n";
    std::cout << "========================================\n";

    return 0;
}
