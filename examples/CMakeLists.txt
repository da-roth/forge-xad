# Example programs for Forge-XAD integration

# Simple XAD example (baseline - no JIT)
add_executable(xad_baseline_example
    xad_baseline.cpp
)
target_link_libraries(xad_baseline_example PRIVATE
    xad
)

# Forge-XAD integration example (with JIT - TO BE IMPLEMENTED)
add_executable(xad_forge_example
    xad_forge_integration.cpp
)
target_link_libraries(xad_forge_example PRIVATE
    forge_xad_bridge
)

# Simple function test
add_executable(simple_function_test
    simple_function_test.cpp
)
target_link_libraries(simple_function_test PRIVATE
    forge_xad_bridge
)

# Converter test
add_executable(test_converter
    test_converter.cpp
)
target_link_libraries(test_converter PRIVATE
    forge_xad_bridge
)

# JIT Tape example (full version with kernel execution)
add_executable(jit_tape_example
    jit_tape_example.cpp
)
target_link_libraries(jit_tape_example PRIVATE
    forge_xad_bridge
)

# JIT Tape simple test (without kernel execution)
add_executable(jit_tape_simple
    jit_tape_simple.cpp
)
target_link_libraries(jit_tape_simple PRIVATE
    forge_xad_bridge
)

# JIT Tape minimal test (isolate construction issue)
add_executable(jit_tape_minimal
    jit_tape_minimal.cpp
)
target_link_libraries(jit_tape_minimal PRIVATE
    forge_xad_bridge
)

# JIT Tape debug test (pinpoint crash location)
add_executable(jit_tape_debug
    jit_tape_debug.cpp
)
target_link_libraries(jit_tape_debug PRIVATE
    forge_xad_bridge
)

# Debug tape recording (understand how XAD records operations)
add_executable(debug_tape_recording
    debug_tape_recording.cpp
)
target_link_libraries(debug_tape_recording PRIVATE
    xad
)
