#ifndef SEMON_MATH_SYNTHESIZER_HPP
#define SEMON_MATH_SYNTHESIZER_HPP

#include "circuit/CircuitTopology.hpp"
#include "circuit/Virtual3PinGate.hpp"
#include "math/MathModel.hpp"

namespace semon {
namespace math {

class Synthesizer {
public:
    Synthesizer() = delete;

    static bool compileModelToCircuit(const MathModel& model, circuit::Circuit& target);

private:
    static bool emitExpression(const MathModel& model,
                               const ExpressionNode* node,
                               circuit::Circuit& target,
                               circuit::Pin** output_pin);
    static bool emitBinaryGate(const MathModel& model,
                               const ExpressionNode* node,
                               circuit::Circuit& target,
                               circuit::Pin* left_pin,
                               circuit::Pin* right_pin,
                               circuit::Pin** output_pin);
};

} // namespace math
} // namespace semon

#endif
