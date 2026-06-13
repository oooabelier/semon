#include "circuit/Virtual3PinGate.hpp"

namespace semon {
namespace circuit {

Virtual3PinGate::Virtual3PinGate()
    : uid_(0U),
      pin_a_(static_cast<Pin*>(0)),
      pin_b_(static_cast<Pin*>(0)),
      pin_c_(static_cast<Pin*>(0)),
      next_in_circuit_(static_cast<Virtual3PinGate*>(0)),
      previous_in_circuit_(static_cast<Virtual3PinGate*>(0)),
      truth_byte_(TRUTH_UNIVERSAL_FREEDOM),
      parent_(static_cast<Circuit*>(0)),
      released_(false)
{
}

bool Virtual3PinGate::initialize(std::uint32_t uid,
                                 Pin* pin_a,
                                 Pin* pin_b,
                                 Pin* pin_c,
                                 std::uint8_t truth_byte,
                                 Circuit* parent)
{
    uid_ = uid;
    pin_a_ = pin_a;
    pin_b_ = pin_b;
    pin_c_ = pin_c;
    next_in_circuit_ = static_cast<Virtual3PinGate*>(0);
    previous_in_circuit_ = static_cast<Virtual3PinGate*>(0);
    truth_byte_ = truth_byte;
    parent_ = parent;
    released_ = false;
    return parent != static_cast<Circuit*>(0);
}

std::uint32_t Virtual3PinGate::uid() const
{
    return uid_;
}

Pin* Virtual3PinGate::pinA() const
{
    return pin_a_;
}

Pin* Virtual3PinGate::pinB() const
{
    return pin_b_;
}

Pin* Virtual3PinGate::pinC() const
{
    return pin_c_;
}

std::uint8_t Virtual3PinGate::truthByte() const
{
    return truth_byte_;
}

Circuit* Virtual3PinGate::parentCircuit() const
{
    return parent_;
}

bool Virtual3PinGate::isReleased() const
{
    return released_;
}

void Virtual3PinGate::markReleased()
{
    released_ = true;
    truth_byte_ = TRUTH_UNIVERSAL_FREEDOM;
}

std::uint8_t Virtual3PinGate::invertPinA(std::uint8_t truth_byte)
{
    return static_cast<std::uint8_t>(((truth_byte & MASK_EVEN_A) >> 1U) | ((truth_byte & MASK_A) << 1U));
}

std::uint8_t Virtual3PinGate::invertPinB(std::uint8_t truth_byte)
{
    return static_cast<std::uint8_t>(((truth_byte & MASK_EVEN_B) >> 2U) | ((truth_byte & MASK_B) << 2U));
}

std::uint8_t Virtual3PinGate::invertPinC(std::uint8_t truth_byte)
{
    return static_cast<std::uint8_t>((truth_byte >> 4U) | (truth_byte << 4U));
}

bool Virtual3PinGate::combinationAllowed(std::uint8_t truth_byte, bool a, bool b, bool c)
{
    const std::uint8_t index = static_cast<std::uint8_t>((c ? 4U : 0U) | (b ? 2U : 0U) | (a ? 1U : 0U));
    return (truth_byte & (static_cast<std::uint8_t>(1U) << index)) != 0U;
}

std::uint8_t Virtual3PinGate::truthByteFor(math::MathOperation operation)
{
    switch (operation) {
    case math::MathOperation::AND:
    case math::MathOperation::BAND:
        return TRUTH_AND;
    case math::MathOperation::XOR:
    case math::MathOperation::BXOR:
    case math::MathOperation::NE:
        return TRUTH_XOR;
    case math::MathOperation::OR:
    case math::MathOperation::BOR:
        return TRUTH_OR;
    case math::MathOperation::EQ:
        return TRUTH_EQUAL;
    case math::MathOperation::NOT:
        return TRUTH_INVERT;
    case math::MathOperation::ADD:
    case math::MathOperation::SUB:
    case math::MathOperation::MUL:
    case math::MathOperation::DIV:
    case math::MathOperation::MOD:
    case math::MathOperation::SHL:
    case math::MathOperation::SHR:
    case math::MathOperation::LT:
    case math::MathOperation::GT:
    case math::MathOperation::LE:
    case math::MathOperation::GE:
    case math::MathOperation::CONCAT:
    case math::MathOperation::SLICE:
    case math::MathOperation::FUNCTION_CALL:
    case math::MathOperation::NONE:
        return TRUTH_XOR;
    }
    return TRUTH_XOR;
}

Virtual3PinGate* Virtual3PinGate::nextInCircuit() const
{
    return next_in_circuit_;
}

Virtual3PinGate* Virtual3PinGate::previousInCircuit() const
{
    return previous_in_circuit_;
}

void Virtual3PinGate::setNextInCircuit(Virtual3PinGate* next)
{
    next_in_circuit_ = next;
}

void Virtual3PinGate::setPreviousInCircuit(Virtual3PinGate* previous)
{
    previous_in_circuit_ = previous;
}

} // namespace circuit
} // namespace semon
