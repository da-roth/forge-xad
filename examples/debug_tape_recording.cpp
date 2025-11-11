// Debug program to understand how XAD records operations to tape
// This helps us understand what patterns to look for in the converter

#include <XAD/XAD.hpp>
#include <iostream>
#include <iomanip>

using AD = xad::AReal<double>;
using tape_type = xad::Tape<double>;

void printTapeContents(const tape_type& tape, const std::string& operation_name) {
    std::cout << "\n=== Operation: " << operation_name << " ===" << std::endl;

    auto& statements = tape.getStatements();
    auto& operations = tape.getOperations();
    auto& op_types = tape.getOpTypes();

    std::cout << "Statements: " << statements.size() << std::endl;
    std::cout << "Operations: " << operations.size() << std::endl;
    std::cout << "OpCodes: " << op_types.size() << std::endl;

    // Print each statement
    for (size_t i = 0; i < statements.size(); ++i) {
        auto [op_idx, lhs_slot] = statements[i];
        std::cout << "Statement " << i << ": op_idx=" << op_idx
                  << ", lhs_slot=" << lhs_slot;

        if (lhs_slot == tape.INVALID_SLOT) {
            std::cout << " (INVALID)" << std::endl;
            continue;
        }

        // Get OpCode for this statement
        xad::OpCode opcode = op_types[i];
        std::cout << ", OpCode=" << static_cast<int>(opcode);

        // Get operations for this statement
        unsigned int op_start = (i > 0) ? statements[i-1].first : 0;
        unsigned int op_end = op_idx;

        std::cout << " -> operations[" << op_start << ":" << op_end << "]: ";
        for (unsigned int j = op_start; j < op_end; ++j) {
            auto [mult, slot] = operations[j];
            std::cout << "(" << mult << ", slot=" << slot << ") ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << std::fixed << std::setprecision(6);

    // Test 1: Addition z = x + y
    {
        tape_type tape;
        AD x = 3.0, y = 4.0;

        tape.registerInput(x);
        tape.registerInput(y);
        tape.newRecording();

        AD z = x + y;

        tape.registerOutput(z);
        printTapeContents(tape, "z = x + y");
        std::cout << "Result: z = " << value(z) << std::endl;
    }

    // Test 2: Multiplication z = x * y
    {
        tape_type tape;
        AD x = 3.0, y = 4.0;

        tape.registerInput(x);
        tape.registerInput(y);
        tape.newRecording();

        AD z = x * y;

        tape.registerOutput(z);
        printTapeContents(tape, "z = x * y");
        std::cout << "Result: z = " << value(z) << std::endl;
    }

    // Test 3: Division z = x / y
    {
        tape_type tape;
        AD x = 12.0, y = 4.0;

        tape.registerInput(x);
        tape.registerInput(y);
        tape.newRecording();

        AD z = x / y;

        tape.registerOutput(z);
        printTapeContents(tape, "z = x / y");
        std::cout << "Result: z = " << value(z) << std::endl;
    }

    // Test 4: Exp z = exp(x)
    {
        tape_type tape;
        AD x = 2.0;

        tape.registerInput(x);
        tape.newRecording();

        AD z = exp(x);

        tape.registerOutput(z);
        printTapeContents(tape, "z = exp(x)");
        std::cout << "Result: z = " << value(z) << " (expected ~7.389)" << std::endl;
    }

    // Test 5: Log z = log(x)
    {
        tape_type tape;
        AD x = 2.7182818;  // e

        tape.registerInput(x);
        tape.newRecording();

        AD z = log(x);

        tape.registerOutput(z);
        printTapeContents(tape, "z = log(x)");
        std::cout << "Result: z = " << value(z) << " (expected ~1.0)" << std::endl;
    }

    // Test 6: Sqrt z = sqrt(x)
    {
        tape_type tape;
        AD x = 16.0;

        tape.registerInput(x);
        tape.newRecording();

        AD z = sqrt(x);

        tape.registerOutput(z);
        printTapeContents(tape, "z = sqrt(x)");
        std::cout << "Result: z = " << value(z) << " (expected 4.0)" << std::endl;
    }

    // Test 7: Complex expression z = x * y + sqrt(x)
    {
        tape_type tape;
        AD x = 9.0, y = 2.0;

        tape.registerInput(x);
        tape.registerInput(y);
        tape.newRecording();

        AD z = x * y + sqrt(x);

        tape.registerOutput(z);
        printTapeContents(tape, "z = x * y + sqrt(x)");
        std::cout << "Result: z = " << value(z) << " (expected 21.0)" << std::endl;
    }

    return 0;
}
