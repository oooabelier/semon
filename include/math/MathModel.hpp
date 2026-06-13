#ifndef SEMON_MATH_MATH_MODEL_HPP
#define SEMON_MATH_MATH_MODEL_HPP

#include <cstddef>
#include <cstdint>

#include "core/PagedAllocator.hpp"
#include "core/SystemSettings.hpp"
#include "math/MathTypes.hpp"

namespace semon {
namespace math {

class MathModel {
public:
    explicit MathModel(core::PagedAllocator* allocator);

    bool initialize(std::uint32_t uid, const char* name);
    std::uint32_t uid() const;
    const char* name() const;
    core::PagedAllocator* allocator() const;

    bool addVariable(const char* name, std::uint32_t bit_width, std::uint32_t* uid_out);
    bool addConstant(const char* name, std::uint64_t value, std::uint32_t bit_width, std::uint32_t* uid_out);
    bool addEquation(const char* name, ExpressionNode* lhs, ExpressionNode* rhs, std::uint32_t* uid_out);
    bool addInequality(const char* name, ExpressionNode* lhs, ExpressionNode* rhs, MathOperation relation, std::uint32_t* uid_out);
    bool addFunction(const char* name, const char* const* arguments, std::uint32_t argument_count, ExpressionNode* body, std::uint32_t* uid_out);
    bool parseAndAddEquation(const char* text, std::uint32_t* uid_out, char* error, std::size_t error_size);

    ExpressionNode* makeNode(MathNodeType node_type, MathOperation operation);
    ExpressionNode* makeVariableNode(std::uint32_t variable_uid);
    ExpressionNode* makeConstantNode(std::uint64_t value, std::uint32_t bit_width);
    ExpressionNode* makeOperationNode(MathOperation operation, ExpressionNode* left, ExpressionNode* right);

    const MathVariable* variableByUid(std::uint32_t uid) const;
    const MathVariable* variableByName(const char* name) const;
    const MathConstant* constantByUid(std::uint32_t uid) const;
    const MathEquation* equationByUid(std::uint32_t uid) const;
    const MathFunction* functionByName(const char* name) const;

    std::uint32_t variableCount() const;
    std::uint32_t constantCount() const;
    std::uint32_t equationCount() const;
    std::uint32_t inequalityCount() const;
    std::uint32_t functionCount() const;

private:
    bool copyName(char* destination, std::size_t destination_size, const char* source) const;
    bool ensureVariableCapacity() const;
    bool ensureConstantCapacity() const;
    bool ensureEquationCapacity() const;
    bool ensureInequalityCapacity() const;
    bool ensureFunctionCapacity() const;

    core::PagedAllocator* allocator_;
    std::uint32_t uid_;
    char name_[64];
    MathVariable variables_[core::SystemSettings::MAX_VARIABLES_PER_MODEL];
    MathConstant constants_[core::SystemSettings::MAX_CONSTANTS_PER_MODEL];
    MathEquation equations_[core::SystemSettings::MAX_EQUATIONS_PER_MODEL];
    MathInequality inequalities_[core::SystemSettings::MAX_INEQUALITIES_PER_MODEL];
    MathFunction functions_[core::SystemSettings::MAX_FUNCTIONS_PER_MODEL];
    std::uint32_t variable_count_;
    std::uint32_t constant_count_;
    std::uint32_t equation_count_;
    std::uint32_t inequality_count_;
    std::uint32_t function_count_;
    std::uint32_t next_variable_uid_;
    std::uint32_t next_constant_uid_;
    std::uint32_t next_equation_uid_;
    std::uint32_t next_inequality_uid_;
    std::uint32_t next_function_uid_;
};

} // namespace math
} // namespace semon

#endif
