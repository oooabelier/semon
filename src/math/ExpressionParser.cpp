#include "math/ExpressionParser.hpp"

#include <cstring>

namespace semon {
namespace math {

ExpressionParser::ExpressionParser(MathModel& model)
    : model_(&model),
      text_(static_cast<const char*>(0)),
      position_(0U)
{
}

bool ExpressionParser::parse(const char* text, ExpressionNode** root, char* error, std::size_t error_size)
{
    if (root == static_cast<ExpressionNode**>(0)) {
        return setError(error, error_size, "null root");
    }

    text_ = text;
    position_ = 0U;
    *root = static_cast<ExpressionNode*>(0);

    if (!parseExpression(root, error, error_size)) {
        return false;
    }

    skipWhitespace();
    if (text_ == static_cast<const char*>(0) || text_[position_] != '\0') {
        return setError(error, error_size, "trailing input");
    }

    return *root != static_cast<ExpressionNode*>(0);
}

bool ExpressionParser::parseExpression(ExpressionNode** node, char* error, std::size_t error_size)
{
    return parseConcat(node, error, error_size);
}

bool ExpressionParser::parseConcat(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseOr(node, error, error_size)) {
        return false;
    }

    while (consume("||")) {
        ExpressionNode* right = static_cast<ExpressionNode*>(0);
        if (!parseOr(&right, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(MathOperation::CONCAT, *node, right);
        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseOr(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseXor(node, error, error_size)) {
        return false;
    }

    while (true) {
        skipWhitespace();
        if (text_[position_] == '|' && text_[position_ + 1U] == '|') {
            break;
        }
        if (!consume('|')) {
            break;
        }
        ExpressionNode* right = static_cast<ExpressionNode*>(0);
        if (!parseXor(&right, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(MathOperation::OR, *node, right);
        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseXor(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseAnd(node, error, error_size)) {
        return false;
    }

    while (consume('^')) {
        ExpressionNode* right = static_cast<ExpressionNode*>(0);
        if (!parseAnd(&right, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(MathOperation::XOR, *node, right);
        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseAnd(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseEquality(node, error, error_size)) {
        return false;
    }

    while (true) {
        skipWhitespace();
        if (text_[position_] == '&' && text_[position_ + 1U] == '&') {
            position_ += 2U;
        } else if (!consume('&')) {
            break;
        }
        ExpressionNode* right = static_cast<ExpressionNode*>(0);
        if (!parseEquality(&right, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(MathOperation::AND, *node, right);
        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseEquality(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseShift(node, error, error_size)) {
        return false;
    }

    MathOperation relation = MathOperation::NONE;
    if (consume("==")) {
        relation = MathOperation::EQ;
    } else if (consume("!=")) {
        relation = MathOperation::NE;
    } else if (consume("<=")) {
        relation = MathOperation::LE;
    } else if (consume(">=")) {
        relation = MathOperation::GE;
    } else if (consume('<')) {
        relation = MathOperation::LT;
    } else if (consume('>')) {
        relation = MathOperation::GT;
    }

    if (relation != MathOperation::NONE) {
        ExpressionNode* right = static_cast<ExpressionNode*>(0);
        if (!parseShift(&right, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(relation, *node, right);
        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseShift(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseTerm(node, error, error_size)) {
        return false;
    }

    while (true) {
        if (consume("<<")) {
            ExpressionNode* right = static_cast<ExpressionNode*>(0);
            if (!parseTerm(&right, error, error_size)) {
                return false;
            }
            *node = model_->makeOperationNode(MathOperation::SHL, *node, right);
        } else if (consume(">>")) {
            ExpressionNode* right = static_cast<ExpressionNode*>(0);
            if (!parseTerm(&right, error, error_size)) {
                return false;
            }
            *node = model_->makeOperationNode(MathOperation::SHR, *node, right);
        } else {
            break;
        }

        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseTerm(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseFactor(node, error, error_size)) {
        return false;
    }

    while (true) {
        MathOperation operation = MathOperation::NONE;
        if (consume('+')) {
            operation = MathOperation::ADD;
        } else if (consume('-')) {
            operation = MathOperation::SUB;
        } else {
            break;
        }

        ExpressionNode* right = static_cast<ExpressionNode*>(0);
        if (!parseFactor(&right, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(operation, *node, right);
        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseFactor(ExpressionNode** node, char* error, std::size_t error_size)
{
    if (!parseUnary(node, error, error_size)) {
        return false;
    }

    while (true) {
        MathOperation operation = MathOperation::NONE;
        if (consume('*')) {
            operation = MathOperation::MUL;
        } else if (consume('/')) {
            operation = MathOperation::DIV;
        } else if (consume('%')) {
            operation = MathOperation::MOD;
        } else {
            break;
        }

        ExpressionNode* right = static_cast<ExpressionNode*>(0);
        if (!parseUnary(&right, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(operation, *node, right);
        if (*node == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }
    }
    return true;
}

bool ExpressionParser::parseUnary(ExpressionNode** node, char* error, std::size_t error_size)
{
    skipWhitespace();
    if (consume('!') || consume('~')) {
        if (!parseUnary(node, error, error_size)) {
            return false;
        }
        *node = model_->makeOperationNode(MathOperation::NOT, *node, static_cast<ExpressionNode*>(0));
        return *node != static_cast<ExpressionNode*>(0);
    }

    if (consume('-')) {
        if (!parseUnary(node, error, error_size)) {
            return false;
        }
        ExpressionNode* zero = model_->makeConstantNode(0ULL, 1U);
        *node = model_->makeOperationNode(MathOperation::SUB, zero, *node);
        return *node != static_cast<ExpressionNode*>(0);
    }

    if (consume('+')) {
        return parseUnary(node, error, error_size);
    }

    return parsePrimary(node, error, error_size);
}

bool ExpressionParser::parsePrimary(ExpressionNode** node, char* error, std::size_t error_size)
{
    skipWhitespace();
    *node = static_cast<ExpressionNode*>(0);

    if (consume('(')) {
        if (!parseExpression(node, error, error_size)) {
            return false;
        }
        return consume(')');
    }

    std::uint64_t number = 0ULL;
    if (readNumber(&number)) {
        *node = model_->makeConstantNode(number, 64U);
        return *node != static_cast<ExpressionNode*>(0);
    }

    char identifier[64];
    if (readIdentifier(identifier, sizeof(identifier))) {
        const MathVariable* variable = model_->variableByName(identifier);
        std::uint32_t variable_uid = 0U;
        if (variable == static_cast<const MathVariable*>(0)) {
            if (!model_->addVariable(identifier, 1U, &variable_uid)) {
                return setError(error, error_size, "variable capacity exceeded");
            }
        } else {
            variable_uid = variable->uid;
        }

        ExpressionNode* base = model_->makeVariableNode(variable_uid);
        if (base == static_cast<ExpressionNode*>(0)) {
            return setError(error, error_size, "allocation failed");
        }

        skipWhitespace();
        if (consume('(')) {
            std::int32_t depth = 1;
            while (text_[position_] != '\0' && depth > 0) {
                if (text_[position_] == '(') {
                    depth += 1;
                } else if (text_[position_] == ')') {
                    depth -= 1;
                }
                if (depth > 0) {
                    position_ += 1U;
                }
            }
            if (!consume(')')) {
                return setError(error, error_size, "unterminated call");
            }
            *node = base;
            (*node)->operation = MathOperation::FUNCTION_CALL;
            std::size_t name_index = 0U;
            while ((name_index + 1U) < sizeof((*node)->name) && identifier[name_index] != '\0') {
                (*node)->name[name_index] = identifier[name_index];
                name_index += 1U;
            }
            (*node)->name[name_index] = '\0';
            return true;
        }

        if (consume('[')) {
            std::uint64_t start = 0ULL;
            std::uint64_t end = 0ULL;
            if (!readNumber(&start) || !consume(':') || !readNumber(&end)) {
                return setError(error, error_size, "bad slice");
            }
            if (!consume(']')) {
                return setError(error, error_size, "unterminated slice");
            }
            ExpressionNode* end_node = model_->makeConstantNode(end, 32U);
            *node = model_->makeOperationNode(MathOperation::SLICE, base, end_node);
            if (*node == static_cast<ExpressionNode*>(0)) {
                return setError(error, error_size, "allocation failed");
            }
            (*node)->bit_start = static_cast<std::uint32_t>(start);
            (*node)->bit_end = static_cast<std::uint32_t>(end);
            return true;
        }

        *node = base;
        return true;
    }

    return setError(error, error_size, "expected expression");
}

void ExpressionParser::skipWhitespace()
{
    while (text_[position_] == ' ' || text_[position_] == '\t' || text_[position_] == '\n' || text_[position_] == '\r') {
        position_ += 1U;
    }
}

bool ExpressionParser::consume(char expected)
{
    skipWhitespace();
    if (text_[position_] != expected) {
        return false;
    }
    position_ += 1U;
    return true;
}

bool ExpressionParser::consume(const char* token)
{
    skipWhitespace();
    const std::size_t length = std::strlen(token);
    if (std::strncmp(text_ + position_, token, length) != 0) {
        return false;
    }
    position_ += length;
    return true;
}

bool ExpressionParser::isIdentifierStart(char value) const
{
    return (value >= 'A' && value <= 'Z') ||
           (value >= 'a' && value <= 'z') ||
           value == '_';
}

bool ExpressionParser::isIdentifierContinue(char value) const
{
    return isIdentifierStart(value) || (value >= '0' && value <= '9');
}

bool ExpressionParser::readIdentifier(char* destination, std::size_t destination_size)
{
    skipWhitespace();
    if (destination_size == 0U || !isIdentifierStart(text_[position_])) {
        return false;
    }

    std::size_t index = 0U;
    while (isIdentifierContinue(text_[position_]) && (index + 1U) < destination_size) {
        destination[index] = text_[position_];
        position_ += 1U;
        index += 1U;
    }
    destination[index] = '\0';
    return true;
}

bool ExpressionParser::readNumber(std::uint64_t* value)
{
    skipWhitespace();
    if (value == static_cast<std::uint64_t*>(0)) {
        return false;
    }

    std::uint64_t result = 0ULL;
    std::uint32_t base = 10U;
    if (text_[position_] == '0' && (text_[position_ + 1U] == 'x' || text_[position_ + 1U] == 'X')) {
        base = 16U;
        position_ += 2U;
    }

    bool found = false;
    while (true) {
        const char current = text_[position_];
        std::uint32_t digit = 0U;
        if (current >= '0' && current <= '9') {
            digit = static_cast<std::uint32_t>(current - '0');
        } else if (base == 16U && current >= 'a' && current <= 'f') {
            digit = static_cast<std::uint32_t>(10U + current - 'a');
        } else if (base == 16U && current >= 'A' && current <= 'F') {
            digit = static_cast<std::uint32_t>(10U + current - 'A');
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }
        result = (result * base) + digit;
        position_ += 1U;
        found = true;
    }

    if (!found) {
        return false;
    }

    *value = result;
    return true;
}

bool ExpressionParser::setError(char* error, std::size_t error_size, const char* message) const
{
    if (error != static_cast<char*>(0) && error_size > 0U) {
        std::size_t index = 0U;
        while ((index + 1U) < error_size && message != static_cast<const char*>(0) && message[index] != '\0') {
            error[index] = message[index];
            index += 1U;
        }
        error[index] = '\0';
    }
    return false;
}

} // namespace math
} // namespace semon
