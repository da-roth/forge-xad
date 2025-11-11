#pragma once

#include <XAD/XAD.hpp>
#include "forge_xad/xad_tape_converter.hpp"
#include <forge/compiler/forge_engine.hpp>
#include <forge/compiler/compiler_config.hpp>
#include <forge/compiler/node_value_buffers/node_value_buffer.hpp>
#include <memory>
#include <iostream>

namespace forge_xad {

/**
 * @brief JIT-accelerated wrapper around XAD tape
 *
 * This is a drop-in replacement for xad::Tape that automatically
 * compiles the tape to native code on first use and uses the
 * compiled kernel for subsequent gradient computations.
 *
 * Usage:
 *   // Instead of: xad::Tape<double> tape;
 *   forge_xad::JITTape<xad::Tape<double>> tape;
 *
 * The wrapper transparently delegates all operations to the underlying
 * tape but intercepts computeAdjoints() to use the compiled kernel.
 */
template<class BaseTape>
class JITTape {
public:
    using active_type = typename BaseTape::active_type;
    using slot_type = typename BaseTape::slot_type;
    using derivative_type = typename BaseTape::derivative_type;
    using size_type = typename BaseTape::size_type;
    using position_type = typename BaseTape::position_type;

    static constexpr slot_type INVALID_SLOT = BaseTape::INVALID_SLOT;

    JITTape() : tape_(), compiled_(false), kernel_(nullptr) {}

    // ===== Delegate to underlying tape =====

    void registerInput(active_type& inp) {
        tape_.registerInput(inp);

        // Store reference to input variable for value synchronization
        if (!compiled_) {
            input_vars_.push_back(&inp);
        }
    }

    void registerOutput(active_type& outp) {
        tape_.registerOutput(outp);

        // Store reference to output variable for gradient synchronization
        if (!compiled_) {
            output_vars_.push_back(&outp);
        }

        // After first registerOutput, trigger compilation
        if (!compiled_) {
            tryCompile();
        }
    }

    void newRecording() {
        tape_.newRecording();
    }

    void computeAdjoints() {
        if (compiled_ && kernel_) {
            // Use compiled kernel (SSE2 scalar mode for simplicity)
            executeCompiledKernel();
        } else {
            // Fall back to tape-based adjoints
            tape_.computeAdjoints();
        }
    }

    void clearAll() {
        tape_.clearAll();
        // Note: Keep compiled kernel - it's still valid for same structure
    }

    // Accessor methods
    const auto& getInputSlots() const { return tape_.getInputSlots(); }
    const auto& getOutputSlots() const { return tape_.getOutputSlots(); }
    const auto& getStatements() const { return tape_.getStatements(); }
    const auto& getOperations() const { return tape_.getOperations(); }

    size_type getNumVariables() const { return tape_.getNumVariables(); }
    size_type getNumOperations() const { return tape_.getNumOperations(); }
    size_type getNumStatements() const { return tape_.getNumStatements(); }

    position_type getPosition() const { return tape_.getPosition(); }
    void clearDerivativesAfter(position_type pos) { tape_.clearDerivativesAfter(pos); }
    void resetTo(position_type pos) { tape_.resetTo(pos); }
    void computeAdjointsTo(position_type pos) {
        if (compiled_ && kernel_) {
            // TODO: Implement partial adjoints with kernel
            tape_.computeAdjointsTo(pos);
        } else {
            tape_.computeAdjointsTo(pos);
        }
    }

    // Active tape management
    static void activate() { BaseTape::activate(); }
    static void deactivate() { BaseTape::deactivate(); }
    static void deactivateAll() { BaseTape::deactivateAll(); }

    // Get underlying tape for advanced use
    BaseTape& getTape() { return tape_; }
    const BaseTape& getTape() const { return tape_; }

    // Check if compiled
    bool isCompiled() const { return compiled_; }

private:
    BaseTape tape_;
    bool compiled_;
    std::unique_ptr<forge::StitchedKernel> kernel_;
    std::unique_ptr<forge::INodeValueBuffer> buffer_;
    ConversionResult conversion_result_;

    // Store references to input/output variables for value synchronization
    std::vector<active_type*> input_vars_;
    std::vector<active_type*> output_vars_;

    void tryCompile() {
        try {
            std::cout << "[JITTape] Converting tape to Forge graph...\n";

            // Convert XAD tape to Forge graph
            conversion_result_ = convertXadTapeToForge(tape_);

            std::cout << "[JITTape] Graph: "
                      << conversion_result_.graph.nodes.size() << " nodes, "
                      << conversion_result_.input_nodes.size() << " inputs, "
                      << conversion_result_.output_nodes.size() << " outputs\n";

            // Compile the graph using ForgeEngine with SSE2 scalar mode (no SIMD)
            std::cout << "[JITTape] Compiling to native code (SSE2 scalar)...\n";
            forge::CompilerConfig config = forge::CompilerConfig::Default();
            config.instructionSet = forge::CompilerConfig::InstructionSet::SSE2_SCALAR;
            forge::ForgeEngine engine(config);
            kernel_ = engine.compile(conversion_result_.graph);

            // Create buffer for value storage
            buffer_ = forge::NodeValueBufferFactory::create(conversion_result_.graph, *kernel_);

            compiled_ = true;
            std::cout << "[JITTape] Compilation successful!\n";
            std::cout << "[JITTape] Buffer created: " << buffer_->getNumNodes() << " nodes\n";

        } catch (const std::exception& e) {
            std::cerr << "[JITTape] Compilation failed: " << e.what() << "\n";
            std::cerr << "[JITTape] Falling back to tape-based computation\n";
            compiled_ = false;
        }
    }

    void executeCompiledKernel() {
        // Step 1: Scatter - sync input values from XAD variables to Forge buffer
        for (size_t i = 0; i < input_vars_.size(); ++i) {
            forge::NodeId node_id = conversion_result_.input_nodes[i];
            double val = xad::value(*input_vars_[i]);
            buffer_->setValue(node_id, val);
        }

        // Step 2: Clear all gradients in buffer
        buffer_->clearGradients();

        // Step 3: Seed output gradients from XAD (reverse mode AD initialization)
        double* gradients = buffer_->getGradientsPtr();
        for (size_t i = 0; i < output_vars_.size(); ++i) {
            forge::NodeId node_id = conversion_result_.output_nodes[i];
            double grad = xad::derivative(*output_vars_[i]);
            gradients[node_id] = grad;
        }

        // Step 4: Execute kernel to backpropagate gradients
        kernel_->executeDirect(
            buffer_->getValuesPtr(),
            buffer_->getGradientsPtr(),
            buffer_->getNumNodes());

        // Step 5: Gather - sync input gradients from Forge buffer back to XAD
        for (size_t i = 0; i < input_vars_.size(); ++i) {
            forge::NodeId node_id = conversion_result_.input_nodes[i];
            double grad = buffer_->getGradient(node_id);
            xad::derivative(*input_vars_[i]) = grad;
        }

        // Step 6: Sync output values back to XAD (for correct forward pass values)
        for (size_t i = 0; i < output_vars_.size(); ++i) {
            forge::NodeId node_id = conversion_result_.output_nodes[i];
            double val = buffer_->getValue(node_id);
            xad::value(*output_vars_[i]) = val;
        }
    }
};

} // namespace forge_xad
