// Simple test to check if OpCode is accessible from expression types
#include <XAD/XAD.hpp>
#include <iostream>

using AD = xad::AReal<double>;
using tape_type = xad::Tape<double>;

int main() {
    std::cout << "=== Testing OpCode Accessibility ===" << std::endl;

    // Test if we can access OpCode from functor
    std::cout << "add_op::opcode = " << static_cast<int>(xad::add_op<double>::opcode) << std::endl;
    std::cout << "prod_op::opcode = " << static_cast<int>(xad::prod_op<double>::opcode) << std::endl;
    std::cout << "exp_op::opcode = " << static_cast<int>(xad::exp_op<double>::opcode) << std::endl;

    // Test if we can access OpCode from BinaryExpr (not constructing AReal yet)
    AD x = 3.0, y = 4.0;
    auto expr = x + y;  // This creates a BinaryExpr
    using ExprType = decltype(expr);
    std::cout << "BinaryExpr (x+y)::getOpCode() = " << static_cast<int>(ExprType::getOpCode()) << std::endl;

    std::cout << "\n=== Testing AReal Construction and Tape Recording ===" << std::endl;

    // Now test actual AReal construction with tape recording
    tape_type tape;
    AD a = 3.0, b = 4.0;

    tape.registerInput(a);
    tape.registerInput(b);
    tape.newRecording();

    // Test binary operation
    AD c = a + b;  // This should call AReal constructor with BinaryExpr

    tape.registerOutput(c);

    // Check what was recorded
    auto& op_types = tape.getOpTypes();
    std::cout << "Number of operations recorded: " << op_types.size() << std::endl;
    if (op_types.size() > 0) {
        std::cout << "Last operation OpCode: " << static_cast<int>(op_types[op_types.size()-1]) << std::endl;
    }

    return 0;
}
