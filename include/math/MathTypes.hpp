#ifndef SEMON_MATH_MATHTYPES_HPP
#define SEMON_MATH_MATHTYPES_HPP

#include <cstddef>
#include <cstdint>

namespace semon {
namespace math {

enum class MathNodeType : std::uint8_t {
    VARIABLE = 1U,
    CONSTANT = 2U,
    OPERATION = 3U,
    BDD_CHOICE = 4U
};

enum class MathOperation : std::uint8_t {
    NONE = 0U,
    ADD = 1U,
    SUB = 2U,
    MUL = 3U,
    DIV = 4U,
    MOD = 5U,
    SHL = 6U,
    SHR = 7U,
    BAND = 8U,
    BOR = 9U,
    BXOR = 10U,
    AND = 11U,
    OR = 12U,
    XOR = 13U,
    NOT = 14U,
    EQ = 15U,
    NE = 16U,
    LT = 17U,
    GT = 18U,
    LE = 19U,
    GE = 20U,
    CONCAT = 21U,
    SLICE = 22U,
    FUNCTION_CALL = 23U
};

struct ExpressionNode {
    ExpressionNode* left;
    ExpressionNode* right;
    void* parent_container;
    std::uint32_t variable_uid;
    MathNodeType node_type;
    std::uint8_t flags;

    MathOperation operation;
    std::uint64_t value;
    std::uint32_t bit_start;
    std::uint32_t bit_end;
    char name[64];
};

struct MathVariable {
    std::uint32_t uid;
    char name[64];
    std::uint32_t bit_width;
    bool released;
};

struct MathConstant {
    std::uint32_t uid;
    char name[64];
    std::uint32_t bit_width;
    std::uintptr_t small_value;
    void* data;
    bool is_medium;
    bool is_large;
};

struct MathEquation {
    std::uint32_t uid;
    char name[64];
    ExpressionNode* lhs;
    ExpressionNode* rhs;
    bool released;
};

struct MathInequality {
    std::uint32_t uid;
    char name[64];
    ExpressionNode* lhs;
    ExpressionNode* rhs;
    MathOperation relation;
    bool released;
};

struct MathFunction {
    std::uint32_t uid;
    char name[64];
    char argument_names[8][64];
    std::uint32_t argument_count;
    ExpressionNode* body;
    bool released;
};

} // namespace math
} // namespace semon

#endif
