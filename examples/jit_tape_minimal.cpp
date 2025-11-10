// Minimal test to isolate JITTape construction issue

#include <iostream>
#include "forge_xad/jit_tape.hpp"

int main() {
    std::cout << "Step 1: Starting minimal test\n" << std::flush;

    try {
        using mode = xad::adj<double>;
        using tape_type = mode::tape_type;

        std::cout << "Step 2: About to construct JITTape\n" << std::flush;

        forge_xad::JITTape<tape_type> tape;

        std::cout << "Step 3: JITTape constructed successfully!\n" << std::flush;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n" << std::flush;
        return 1;
    }
}
