/**
 * @file xad_forge_integration.cpp
 * @brief Example showing XAD + Forge integration with minimal code changes
 *
 * GOAL: Demonstrate the "Option 1 Less Invasive" approach from docs
 *
 * User only changes 1 LINE:
 *   FROM: tape_type tape;
 *   TO:   forge_xad::JITTape<tape_type> tape;
 *
 * Everything else stays the same! The wrapper auto-detects:
 * - First iteration: records + compiles
 * - Subsequent iterations: executes compiled kernel
 */

#include <XAD/XAD.hpp>
// #include <forge_xad/jit_tape.hpp>  // TODO: Create this wrapper
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

    std::cout << "XAD + Forge Integration (Minimal Invasive Wrapper)\n";
    std::cout << "===================================================\n\n";

    std::cout << "STATUS: NOT YET IMPLEMENTED\n";
    std::cout << "Vision: User changes ONLY 1 LINE of code!\n\n";

    // GOAL: This should be the final user code (nearly identical to baseline)
    // ======================================================================
    /*
    // Only change: Use JITTape wrapper instead of regular tape
    forge_xad::JITTape<tape_type> tape;  // ← ONLY LINE THAT CHANGES!
    AD x, y;

    const int num_iterations = 1000;
    for (int i = 0; i < num_iterations; ++i) {
        // Everything below is COMPLETELY UNCHANGED from baseline XAD code!
        value(x) = 1.0 + i * 0.01;
        value(y) = 2.0 + i * 0.01;

        tape.registerInput(x);
        tape.registerInput(y);
        tape.newRecording();

        AD result = simpleFunction(x, y);

        tape.registerOutput(result);

        derivative(result) = 1.0;
        tape.computeAdjoints();

        double output = value(result);
        double grad_x = derivative(x);

        tape.clearAll();
    }
    */
    // ======================================================================

    std::cout << "TODO: Implement JITTape wrapper that:\n";
    std::cout << "1. First iteration (i=0):\n";
    std::cout << "   - newRecording() → start normal tape recording\n";
    std::cout << "   - registerOutput() → trigger: convert tape to Forge graph + compile\n";
    std::cout << "   - computeAdjoints() → execute normally first time\n";
    std::cout << "\n";
    std::cout << "2. Subsequent iterations (i=1...N):\n";
    std::cout << "   - newRecording() → intercepted, do nothing\n";
    std::cout << "   - registerOutput() → intercepted, do nothing\n";
    std::cout << "   - computeAdjoints() → execute compiled kernel instead!\n";
    std::cout << "\n";
    std::cout << "3. Value synchronization:\n";
    std::cout << "   - value(x) = ... → update kernel workspace\n";
    std::cout << "   - value(result) → read from kernel workspace\n";
    std::cout << "\n";

    std::cout << "\nKey files to create:\n";
    std::cout << "- include/forge_xad/jit_tape.hpp (wrapper class)\n";
    std::cout << "- src/jit_tape.cpp (implementation)\n";
    std::cout << "- Complete xad_tape_converter.cpp (tape → graph)\n";
    std::cout << "\n";

    std::cout << "See: docs/xadRefactor/option1IdeaLessInvasive.md for full design\n";

    return 0;
}
