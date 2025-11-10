/**
 * @file xad_baseline.cpp
 * @brief Baseline XAD example without JIT compilation
 *
 * This demonstrates standard XAD usage where the tape is re-recorded
 * for every iteration. This is the "before" state.
 */

#include <XAD/XAD.hpp>
#include <iostream>
#include <chrono>

// Simple test function: f(x, y) = x^2 + y^2
template<typename T>
T simpleFunction(const T& x, const T& y) {
    return x * x + y * y;
}

int main() {
    using mode = xad::adj<double>;
    using tape_type = mode::tape_type;
    using AD = mode::active_type;

    tape_type tape;

    std::cout << "XAD Baseline (Re-recording tape each iteration)\n";
    std::cout << "================================================\n\n";

    const int num_iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_iterations; ++i) {
        // Create inputs
        AD x = 0.0, y = 0.0;

        // Set values
        value(x) = 1.0 + i * 0.01;
        value(y) = 2.0 + i * 0.01;

        // Register inputs
        tape.registerInput(x);
        tape.registerInput(y);

        // Start recording
        tape.newRecording();

        // Compute function
        AD result = simpleFunction(x, y);

        // Register output
        tape.registerOutput(result);

        // Seed adjoint
        derivative(result) = 1.0;

        // Compute gradients
        tape.computeAdjoints();

        // Extract results (only print first few)
        if (i < 5 || i == num_iterations - 1) {
            std::cout << "Iteration " << i << ":\n";
            std::cout << "  f(" << value(x) << ", " << value(y) << ") = " << value(result) << "\n";
            std::cout << "  df/dx = " << derivative(x) << "\n";
            std::cout << "  df/dy = " << derivative(y) << "\n\n";
        }

        // Clear for next iteration
        tape.clearAll();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Total time for " << num_iterations << " iterations: "
              << duration.count() << " ms\n";
    std::cout << "Average time per iteration: "
              << (double)duration.count() / num_iterations << " ms\n";

    return 0;
}
