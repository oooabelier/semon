#include "circuit/Solver.hpp"
#include "circuit/Virtual3PinGate.hpp"

#include "core/SystemSettings.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace semon {
namespace circuit {

namespace {

bool knownValue(const Pin* pin, bool* value)
{
    if (pin == static_cast<const Pin*>(0) || value == static_cast<bool*>(0)) {
        return false;
    }

    if (pin->state == SignalState::ZERO) {
        *value = false;
        return true;
    }
    if (pin->state == SignalState::ONE) {
        *value = true;
        return true;
    }
    return false;
}

bool assignPin(Pin* pin, bool value, DiagnosticHook diagnostic_hook, void* diagnostic_userdata)
{
    if (pin == static_cast<Pin*>(0)) {
        return true;
    }

    DiagnosticStepInfo info;
    info.implication_depth = 0U;
    info.gate_uid = 0U;
    info.assigned_mask = value ? 1U : 0U;
    info.active_pin = pin;
    info.diagnostic_cycle = diagnostics::Diagnostics::cycleCount();
    info.userdata = diagnostic_userdata;

    if (diagnostic_hook != static_cast<DiagnosticHook>(0) && !diagnostic_hook(&info, diagnostic_userdata)) {
        return false;
    }

    if (pin->locked && pin->value != value) {
        pin->state = SignalState::CONFLICT;
        diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::SOLVER,
                         "Solver::assignPin",
                         static_cast<void*>(0),
                         0U,
                         "locked pin contradiction");
        return false;
    }

    pin->value = value;
    pin->state = value ? SignalState::ONE : SignalState::ZERO;
    return true;
}

bool pinValueForIndex(std::uint8_t truth_index, std::size_t pin_index, bool* value)
{
    const std::uint8_t shifted = static_cast<std::uint8_t>(truth_index >> pin_index);
    *value = (shifted & 1U) != 0U;
    return true;
}

} // namespace

bool Solver::propagate(Circuit& circuit,
                       PropagationHook hook,
                       void* userdata,
                       DiagnosticHook diagnostic_hook,
                       void* diagnostic_userdata)
{
    std::uint64_t cycle = 0U;

    while (cycle < core::SystemSettings::MAX_LOOP_CYCLES) {
        SEMON_LOOP();
        bool changed = false;
        bool contradiction = false;
        std::size_t unstable_gates = 0U;

        for (Virtual3PinGate* gate = circuit.firstGateForSolver(); gate != static_cast<Virtual3PinGate*>(0); gate = gate->nextInCircuit()) {
            (void)gate;
        }

        for (Virtual3PinGate* gate = circuit.firstGateForSolver(); gate != static_cast<Virtual3PinGate*>(0); gate = gate->nextInCircuit()) {
            if (gate->isReleased() || gate->truthByte() == Virtual3PinGate::TRUTH_UNIVERSAL_FREEDOM) {
                continue;
            }
            if (gate->truthByte() == Virtual3PinGate::TRUTH_UNIVERSAL_CONTRADICTION) {
                diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::SOLVER,
                                 "Solver::propagate",
                                 static_cast<void*>(0),
                                 0U,
                                 "universal contradiction");
                return false;
            }

            unstable_gates += 1U;
            Pin* pins[3] = {gate->pinA(), gate->pinB(), gate->pinC()};

            bool known_pins[3] = {false, false, false};
            bool known_values[3] = {false, false, false};
            std::size_t known_count = 0U;
            for (std::size_t pin_index = 0U; pin_index < 3U; ++pin_index) {
                if (knownValue(pins[pin_index], &known_values[pin_index])) {
                    known_pins[pin_index] = true;
                    known_count += 1U;
                }
            }

            if (known_count == 0U) {
                continue;
            }

            bool possible[3][2] = {{false, false}, {false, false}, {false, false}};

            for (std::uint8_t index = 0U; index < 8U; ++index) {
                if (!Virtual3PinGate::combinationAllowed(gate->truthByte(),
                                                         (index & 1U) != 0U,
                                                         (index & 2U) != 0U,
                                                         (index & 4U) != 0U)) {
                    continue;
                }

                bool matches_known = true;
                for (std::size_t pin_index = 0U; pin_index < 3U; ++pin_index) {
                    if (!known_pins[pin_index]) {
                        continue;
                    }
                    bool indexed_value = false;
                    (void)pinValueForIndex(index, pin_index, &indexed_value);
                    if (indexed_value != known_values[pin_index]) {
                        matches_known = false;
                        break;
                    }
                }
                if (!matches_known) {
                    continue;
                }

                for (std::size_t pin_index = 0U; pin_index < 3U; ++pin_index) {
                    if (pins[pin_index] == static_cast<Pin*>(0) || known_pins[pin_index]) {
                        continue;
                    }
                    bool candidate = false;
                    (void)pinValueForIndex(index, pin_index, &candidate);
                    possible[pin_index][candidate ? 1 : 0] = true;
                }
            }

            for (std::size_t pin_index = 0U; pin_index < 3U; ++pin_index) {
                Pin* target = pins[pin_index];
                if (target == static_cast<Pin*>(0) || known_pins[pin_index] || target->locked) {
                    continue;
                }

                if (possible[pin_index][0] && !possible[pin_index][1]) {
                    if (!assignPin(target, false, diagnostic_hook, diagnostic_userdata)) {
                        return false;
                    }
                    changed = true;
                } else if (possible[pin_index][1] && !possible[pin_index][0]) {
                    if (!assignPin(target, true, diagnostic_hook, diagnostic_userdata)) {
                        return false;
                    }
                    changed = true;
                } else if (!possible[pin_index][0] && !possible[pin_index][1]) {
                    target->state = SignalState::CONFLICT;
                    contradiction = true;
                }
            }
        }

        if (hook != static_cast<PropagationHook>(0) && !hook(diagnostics::Diagnostics::cycleCount(), unstable_gates, userdata)) {
            return false;
        }

        if (contradiction) {
            diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::SOLVER,
                             "Solver::propagate",
                             static_cast<void*>(0),
                             0U,
                             "local contradiction");
            return false;
        }

        if (!changed) {
            return true;
        }
        cycle += 1U;
    }

    diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::LIMIT,
                     "Solver::propagate",
                     static_cast<void*>(0),
                     0U,
                     "MAX_LOOP_CYCLES");
    return false;
}

} // namespace circuit
} // namespace semon
