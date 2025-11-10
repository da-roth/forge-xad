/**
 * @brief Debug test to pinpoint where jit_tape_simple crashes
 */

#include "forge_xad/jit_tape.hpp"
#include <iostream>

int main() {
    std::cout << "Step 1: Construct JITTape\n" << std::flush;

    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    forge_xad::JITTape<tape_type> tape;
    std::cout << "Step 2: JITTape constructed\n" << std::flush;

    // Create inputs
    AD x = 0.0, y = 0.0;
    xad::value(x) = 3.0;
    xad::value(y) = 4.0;

    std::cout << "Step 3: Created AD variables\n" << std::flush;

    // Register inputs
    tape.registerInput(x);
    std::cout << "Step 4: Registered first input\n" << std::flush;

    tape.registerInput(y);
    std::cout << "Step 5: Registered second input\n" << std::flush;

    // Start recording
    tape.newRecording();
    std::cout << "Step 6: Started recording\n" << std::flush;

    // Compute function: f(x, y) = x + y
    AD result = x + y;
    std::cout << "Step 7: Computed result = x + y\n" << std::flush;

    // Register output (triggers compilation attempt)
    std::cout << "Step 8: About to register output (will trigger compilation)...\n" << std::flush;

    try {
        tape.registerOutput(result);
        std::cout << "Step 9: registerOutput() completed successfully!\n" << std::flush;
    } catch (const std::exception& e) {
        std::cerr << "ERROR in registerOutput: " << e.what() << "\n" << std::flush;
        return 1;
    }

    std::cout << "Step 10: Test completed successfully!\n" << std::flush;
    std::cout << "Compiled: " << (tape.isCompiled() ? "Yes" : "No") << "\n" << std::flush;

    // Now test computeAdjoints (this is where jit_tape_simple might be crashing)
    std::cout << "Step 11: Setting derivative of output...\n" << std::flush;
    xad::derivative(result) = 1.0;

    std::cout << "Step 12: About to call computeAdjoints()...\n" << std::flush;
    try {
        tape.computeAdjoints();
        std::cout << "Step 13: computeAdjoints() completed!\n" << std::flush;
    } catch (const std::exception& e) {
        std::cerr << "ERROR in computeAdjoints: " << e.what() << "\n" << std::flush;
        return 1;
    }

    std::cout << "Step 14: Extracting gradients...\n" << std::flush;
    double grad_x = xad::derivative(x);
    double grad_y = xad::derivative(y);

    std::cout << "Results:\n";
    std::cout << "  df/dx = " << grad_x << "\n";
    std::cout << "  df/dy = " << grad_y << "\n";
    std::cout << "Step 15: All done!\n" << std::flush;

    return 0;
}
