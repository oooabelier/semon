#ifndef SEMON_CIRCUIT_SOLVER_HPP
#define SEMON_CIRCUIT_SOLVER_HPP

#include <cstddef>
#include <cstdint>

#include "circuit/CircuitTopology.hpp"

namespace semon {
namespace circuit {

typedef bool (*PropagationHook)(std::uint64_t current_cycle,
                                std::size_t active_gates_count,
                                void* userdata);

struct DiagnosticStepInfo {
    std::uint32_t implication_depth;
    std::uint32_t gate_uid;
    std::uint8_t assigned_mask;
    const Pin* active_pin;
    std::uint64_t diagnostic_cycle;
    void* userdata;
};

typedef bool (*DiagnosticHook)(const DiagnosticStepInfo* info, void* userdata);

class Solver {
public:
    Solver() = delete;

    static bool propagate(Circuit& circuit,
                          PropagationHook hook = static_cast<PropagationHook>(0),
                          void* userdata = static_cast<void*>(0),
                          DiagnosticHook diagnostic_hook = static_cast<DiagnosticHook>(0),
                          void* diagnostic_userdata = static_cast<void*>(0));
};

} // namespace circuit
} // namespace semon

#endif
