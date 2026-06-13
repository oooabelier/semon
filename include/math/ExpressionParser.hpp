#ifndef SEMON_MATH_EXPRESSION_PARSER_HPP
#define SEMON_MATH_EXPRESSION_PARSER_HPP

#include <cstddef>

#include "math/MathModel.hpp"
#include "math/MathTypes.hpp"

namespace semon {
namespace math {

class ExpressionParser {
public:
    explicit ExpressionParser(MathModel& model);

    bool parse(const char* text, ExpressionNode** root, char* error, std::size_t error_size);

private:
    bool parseExpression(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseConcat(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseOr(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseXor(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseAnd(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseEquality(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseShift(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseTerm(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseFactor(ExpressionNode** node, char* error, std::size_t error_size);
    bool parseUnary(ExpressionNode** node, char* error, std::size_t error_size);
    bool parsePrimary(ExpressionNode** node, char* error, std::size_t error_size);

    void skipWhitespace();
    bool consume(char expected);
    bool consume(const char* token);
    bool isIdentifierStart(char value) const;
    bool isIdentifierContinue(char value) const;
    bool readIdentifier(char* destination, std::size_t destination_size);
    bool readNumber(std::uint64_t* value);
    bool setError(char* error, std::size_t error_size, const char* message) const;

    MathModel* model_;
    const char* text_;
    std::size_t position_;
};

} // namespace math
} // namespace semon

#endif
