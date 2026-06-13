#include "math/Synthesizer.hpp"

#include <cstdio>

namespace semon {
namespace math {

bool Synthesizer::compileModelToCircuit(const MathModel& model, circuit::Circuit& target)
{
    for (std::uint32_t variable_index = 0U; variable_index < model.variableCount(); ++variable_index) {
        const MathVariable variable_storage = model.variableByUid(variable_index + 1U) == static_cast<const MathVariable*>(0)
                                                  ? MathVariable()
                                                  : *model.variableByUid(variable_index + 1U);
        const MathVariable* variable = model.variableByUid(variable_storage.uid);
        if (variable == static_cast<const MathVariable*>(0)) {
            continue;
        }

        for (std::uint32_t bit = 0U; bit < variable->bit_width; ++bit) {
            char pin_name[96];
            (void)std::snprintf(pin_name, sizeof(pin_name), "%s:%u", variable->name, bit);
            circuit::Pin* pin = target.addPin(pin_name, variable->uid, bit);
            if (pin == static_cast<circuit::Pin*>(0)) {
                return false;
            }
        }
    }

    for (std::uint32_t equation_index = 0U; equation_index < model.equationCount(); ++equation_index) {
        const MathEquation* equation = model.equationByUid(equation_index + 1U);
        if (equation == static_cast<const MathEquation*>(0)) {
            continue;
        }

        circuit::Pin* lhs_pin = static_cast<circuit::Pin*>(0);
        circuit::Pin* rhs_pin = static_cast<circuit::Pin*>(0);
        if (!emitExpression(model, equation->lhs, target, &lhs_pin) ||
            !emitExpression(model, equation->rhs, target, &rhs_pin)) {
            return false;
        }

        circuit::Virtual3PinGate* gate = static_cast<circuit::Virtual3PinGate*>(0);
        if (!target.addGate(lhs_pin, rhs_pin, static_cast<circuit::Pin*>(0), circuit::Virtual3PinGate::TRUTH_EQUAL, &gate)) {
            return false;
        }
        if (gate != static_cast<circuit::Virtual3PinGate*>(0)) {
            (void)gate;
        }
    }

    return true;
}

bool Synthesizer::emitExpression(const MathModel& model,
                                 const ExpressionNode* node,
                                 circuit::Circuit& target,
                                 circuit::Pin** output_pin)
{
    if (output_pin == static_cast<circuit::Pin**>(0) || node == static_cast<const ExpressionNode*>(0)) {
        return false;
    }
    *output_pin = static_cast<circuit::Pin*>(0);

    if (node->node_type == MathNodeType::VARIABLE) {
        const MathVariable* variable = model.variableByUid(node->variable_uid);
        if (variable == static_cast<const MathVariable*>(0)) {
            return false;
        }
        const std::uint32_t bit_index = node->bit_start;
        if (bit_index >= variable->bit_width) {
            return false;
        }
        char pin_name[96];
        (void)std::snprintf(pin_name, sizeof(pin_name), "%s:%u", variable->name, bit_index);
        *output_pin = target.addPin(pin_name, variable->uid, bit_index);
        return *output_pin != static_cast<circuit::Pin*>(0);
    }

    if (node->node_type == MathNodeType::CONSTANT) {
        char pin_name[64];
        (void)std::snprintf(pin_name, sizeof(pin_name), "const_%llu", static_cast<unsigned long long>(node->value));
        circuit::Pin* pin = target.addPin(pin_name, 0U, 0U);
        if (pin == static_cast<circuit::Pin*>(0)) {
            return false;
        }
        const bool value = ((node->value & 1ULL) != 0ULL);
        (void)target.lockPin(pin, value);
        *output_pin = pin;
        return true;
    }

    if (node->node_type == MathNodeType::OPERATION) {
        if (node->operation == MathOperation::NOT) {
            circuit::Pin* input_pin = static_cast<circuit::Pin*>(0);
            if (!emitExpression(model, node->left, target, &input_pin)) {
                return false;
            }
            char pin_name[64];
            (void)std::snprintf(pin_name, sizeof(pin_name), "not_%u", static_cast<unsigned int>(target.gateCount()));
            circuit::Pin* result = target.addPin(pin_name, 0U, 0U);
            if (result == static_cast<circuit::Pin*>(0)) {
                return false;
            }
            return target.addGate(input_pin, static_cast<circuit::Pin*>(0), result, circuit::Virtual3PinGate::TRUTH_INVERT);
        }

        if (node->operation == MathOperation::SLICE) {
            return emitExpression(model, node->left, target, output_pin);
        }

        if (node->operation == MathOperation::CONCAT) {
            return emitExpression(model, node->left, target, output_pin);
        }

        if (node->operation == MathOperation::FUNCTION_CALL) {
            circuit::Pin* pin = target.addPin(node->name, 0U, 0U);
            if (pin == static_cast<circuit::Pin*>(0)) {
                return false;
            }
            (void)target.lockPin(pin, false);
            *output_pin = pin;
            return true;
        }

        circuit::Pin* left_pin = static_cast<circuit::Pin*>(0);
        circuit::Pin* right_pin = static_cast<circuit::Pin*>(0);
        if (!emitExpression(model, node->left, target, &left_pin) ||
            !emitExpression(model, node->right, target, &right_pin)) {
            return false;
        }
        return emitBinaryGate(model, node, target, left_pin, right_pin, output_pin);
    }

    return false;
}

bool Synthesizer::emitBinaryGate(const MathModel& model,
                                 const ExpressionNode* node,
                                 circuit::Circuit& target,
                                 circuit::Pin* left_pin,
                                 circuit::Pin* right_pin,
                                 circuit::Pin** output_pin)
{
    (void)model;
    if (output_pin == static_cast<circuit::Pin**>(0) || left_pin == static_cast<circuit::Pin*>(0) ||
        right_pin == static_cast<circuit::Pin*>(0)) {
        return false;
    }

    char pin_name[64];
    (void)std::snprintf(pin_name, sizeof(pin_name), "op_%u", static_cast<unsigned int>(target.gateCount()));
    circuit::Pin* result = target.addPin(pin_name, 0U, 0U);
    if (result == static_cast<circuit::Pin*>(0)) {
        return false;
    }

    const std::uint8_t truth = circuit::Virtual3PinGate::truthByteFor(node->operation);
    return target.addGate(left_pin, right_pin, result, truth, static_cast<circuit::Virtual3PinGate**>(0));
}

} // namespace math
} // namespace semon
