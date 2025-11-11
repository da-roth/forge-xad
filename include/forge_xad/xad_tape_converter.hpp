#pragma once

#include <XAD/XAD.hpp>
#include <forge/graph/graph.hpp>
#include <memory>
#include <unordered_map>

namespace forge_xad {

/**
 * @brief Converts an XAD tape to a Forge graph for JIT compilation
 *
 * This class bridges XAD's tape-based automatic differentiation with
 * Forge's graph-based JIT compilation system.
 */
class XadTapeConverter {
public:
    /**
     * @brief Convert an XAD tape to a Forge graph
     *
     * @tparam Real The scalar type (e.g., double)
     * @tparam N The tape dimension
     * @param tape The XAD tape to convert
     * @return A Forge graph ready for compilation
     */
    template<class Real, std::size_t N = 1>
    static forge::Graph convertTapeToGraph(const xad::Tape<Real, N>& tape);

    /**
     * @brief Get mapping from XAD slot to Forge node ID
     *
     * This is needed to synchronize values between XAD variables and
     * the compiled kernel's workspace.
     */
    const std::unordered_map<unsigned int, forge::NodeId>& getSlotToNodeMap() const {
        return slot_to_node_;
    }

    /**
     * @brief Get input node IDs in order of registration
     */
    const std::vector<forge::NodeId>& getInputNodes() const {
        return input_nodes_;
    }

    /**
     * @brief Get output node IDs in order of registration
     */
    const std::vector<forge::NodeId>& getOutputNodes() const {
        return output_nodes_;
    }

private:
    std::unordered_map<unsigned int, forge::NodeId> slot_to_node_;
    std::vector<forge::NodeId> input_nodes_;
    std::vector<forge::NodeId> output_nodes_;
};

/**
 * @brief Result of tape conversion including the graph and metadata
 */
struct ConversionResult {
    forge::Graph graph;
    std::unordered_map<unsigned int, forge::NodeId> slot_to_node;
    std::vector<forge::NodeId> input_nodes;
    std::vector<forge::NodeId> output_nodes;
};

/**
 * @brief Convert XAD tape to Forge graph (standalone function)
 *
 * @tparam Real The scalar type
 * @tparam N The tape dimension
 * @param tape The XAD tape
 * @return Conversion result with graph and mappings
 */
template<class Real, std::size_t N = 1>
ConversionResult convertXadTapeToForge(const xad::Tape<Real, N>& tape);

} // namespace forge_xad
