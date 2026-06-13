#ifndef SEMON_CIRCUIT_VIRTUAL_GATE_HPP
#define SEMON_CIRCUIT_VIRTUAL_GATE_HPP

#include <cstdint>

#include "circuit/CircuitTopology.hpp"
#include "math/MathTypes.hpp"

namespace semon {
namespace circuit {

class Virtual3PinGate {
public:
    static constexpr std::uint8_t TRUTH_AND = 0x87U;
    static constexpr std::uint8_t TRUTH_XOR = 0x69U;
    static constexpr std::uint8_t TRUTH_OR = 0xEFU;
    static constexpr std::uint8_t TRUTH_EQUAL = 0x96U;
    static constexpr std::uint8_t TRUTH_BUFFER = 0x09U;
    static constexpr std::uint8_t TRUTH_INVERT = 0x06U;
    static constexpr std::uint8_t TRUTH_UNIVERSAL_FREEDOM = 0xFFU;
    static constexpr std::uint8_t TRUTH_UNIVERSAL_CONTRADICTION = 0x00U;

    Virtual3PinGate();

    bool initialize(std::uint32_t uid,
                    Pin* pin_a,
                    Pin* pin_b,
                    Pin* pin_c,
                    std::uint8_t truth_byte,
                    Circuit* parent);

    std::uint32_t uid() const;
    Pin* pinA() const;
    Pin* pinB() const;
    Pin* pinC() const;
    std::uint8_t truthByte() const;
    Circuit* parentCircuit() const;
    bool isReleased() const;
    void markReleased();
    Virtual3PinGate* nextInCircuit() const;
    Virtual3PinGate* previousInCircuit() const;
    void setNextInCircuit(Virtual3PinGate* next);
    void setPreviousInCircuit(Virtual3PinGate* previous);

    static std::uint8_t invertPinA(std::uint8_t truth_byte);
    static std::uint8_t invertPinB(std::uint8_t truth_byte);
    static std::uint8_t invertPinC(std::uint8_t truth_byte);
    static bool combinationAllowed(std::uint8_t truth_byte, bool a, bool b, bool c);
    static std::uint8_t truthByteFor(math::MathOperation operation);

private:
    static constexpr std::uint8_t MASK_A = 0x55U;
    static constexpr std::uint8_t MASK_B = 0x33U;
    static constexpr std::uint8_t MASK_C = 0x0FU;
    static constexpr std::uint8_t MASK_EVEN_A = 0xAAU;
    static constexpr std::uint8_t MASK_EVEN_B = 0xCCU;

    std::uint32_t uid_;
    Pin* pin_a_;
    Pin* pin_b_;
    Pin* pin_c_;
    Virtual3PinGate* next_in_circuit_;
    Virtual3PinGate* previous_in_circuit_;
    std::uint8_t truth_byte_;
    Circuit* parent_;
    bool released_;
};

} // namespace circuit
} // namespace semon

#endif
