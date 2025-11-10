/**
 * @file simple_function_test.cpp
 * @brief Test basic XAD tape recording
 *
 * This is a minimal test to verify XAD is working correctly
 * and to understand the tape structure.
 */

#include <XAD/XAD.hpp>
#include <iostream>

int main() {
    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    std::cout << "Simple XAD Function Test\n";
    std::cout << "========================\n\n";

    tape_type tape;

    // Create inputs
    AD x = 0.0, y = 0.0;
    value(x) = 3.0;
    value(y) = 4.0;

    // Register and record
    tape.registerInput(x);
    tape.registerInput(y);
    tape.newRecording();

    // Compute: f(x,y) = x^2 + y^2
    AD result = x * x + y * y;

    tape.registerOutput(result);

    std::cout << "Forward pass:\n";
    std::cout << "  x = " << value(x) << "\n";
    std::cout << "  y = " << value(y) << "\n";
    std::cout << "  f(x,y) = x^2 + y^2 = " << value(result) << "\n";
    std::cout << "  Expected: 3^2 + 4^2 = 25\n\n";

    // Compute gradients
    derivative(result) = 1.0;
    tape.computeAdjoints();

    std::cout << "Reverse pass (gradients):\n";
    std::cout << "  df/dx = " << derivative(x) << " (expected: 2*x = 6)\n";
    std::cout << "  df/dy = " << derivative(y) << " (expected: 2*y = 8)\n\n";

    // Verify
    bool x_correct = std::abs(derivative(x) - 6.0) < 1e-10;
    bool y_correct = std::abs(derivative(y) - 8.0) < 1e-10;
    bool f_correct = std::abs(value(result) - 25.0) < 1e-10;

    if (x_correct && y_correct && f_correct) {
        std::cout << "✓ All tests passed!\n";
        return 0;
    } else {
        std::cout << "✗ Tests failed!\n";
        return 1;
    }
}
