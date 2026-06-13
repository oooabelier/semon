#include "math/ExpressionParser.hpp"
#include "math/MathModel.hpp"

#include <cstring>

namespace semon {
namespace math {

MathModel::MathModel(core::PagedAllocator* allocator)
    : allocator_(allocator),
      uid_(0U),
      name_(),
      variables_(),
      constants_(),
      equations_(),
      inequalities_(),
      functions_(),
      variable_count_(0U),
      constant_count_(0U),
      equation_count_(0U),
      inequality_count_(0U),
      function_count_(0U),
      next_variable_uid_(1U),
      next_constant_uid_(1U),
      next_equation_uid_(1U),
      next_inequality_uid_(1U),
      next_function_uid_(1U)
{
}

bool MathModel::initialize(std::uint32_t uid, const char* name)
{
    if (!copyName(name_, sizeof(name_), name)) {
        return false;
    }
    uid_ = uid;
    variable_count_ = 0U;
    constant_count_ = 0U;
    equation_count_ = 0U;
    inequality_count_ = 0U;
    function_count_ = 0U;
    next_variable_uid_ = 1U;
    next_constant_uid_ = 1U;
    next_equation_uid_ = 1U;
    next_inequality_uid_ = 1U;
    next_function_uid_ = 1U;
    return true;
}

std::uint32_t MathModel::uid() const
{
    return uid_;
}

const char* MathModel::name() const
{
    return name_;
}

core::PagedAllocator* MathModel::allocator() const
{
    return allocator_;
}

bool MathModel::addVariable(const char* name, std::uint32_t bit_width, std::uint32_t* uid_out)
{
    if (!ensureVariableCapacity() || bit_width == 0U || bit_width > core::SystemSettings::MAXIMUM_BIT_WIDTH) {
        return false;
    }

    MathVariable* variable = &variables_[variable_count_];
    variable->uid = next_variable_uid_;
    (void)copyName(variable->name, sizeof(variable->name), name);
    variable->bit_width = bit_width;
    variable->released = false;
    if (uid_out != static_cast<std::uint32_t*>(0)) {
        *uid_out = variable->uid;
    }
    variable_count_ += 1U;
    next_variable_uid_ += 1U;
    return true;
}

bool MathModel::addConstant(const char* name, std::uint64_t value, std::uint32_t bit_width, std::uint32_t* uid_out)
{
    if (!ensureConstantCapacity() || bit_width == 0U || bit_width > core::SystemSettings::MAXIMUM_BIT_WIDTH) {
        return false;
    }

    MathConstant* constant = &constants_[constant_count_];
    constant->uid = next_constant_uid_;
    (void)copyName(constant->name, sizeof(constant->name), name);
    constant->bit_width = bit_width;
    constant->small_value = static_cast<std::uintptr_t>(value);
    constant->data = static_cast<void*>(0);
    constant->is_medium = false;
    constant->is_large = false;
    if (uid_out != static_cast<std::uint32_t*>(0)) {
        *uid_out = constant->uid;
    }
    constant_count_ += 1U;
    next_constant_uid_ += 1U;
    return true;
}

bool MathModel::addEquation(const char* name, ExpressionNode* lhs, ExpressionNode* rhs, std::uint32_t* uid_out)
{
    if (!ensureEquationCapacity() || lhs == static_cast<ExpressionNode*>(0) || rhs == static_cast<ExpressionNode*>(0)) {
        return false;
    }

    MathEquation* equation = &equations_[equation_count_];
    equation->uid = next_equation_uid_;
    (void)copyName(equation->name, sizeof(equation->name), name);
    equation->lhs = lhs;
    equation->rhs = rhs;
    equation->released = false;
    if (uid_out != static_cast<std::uint32_t*>(0)) {
        *uid_out = equation->uid;
    }
    equation_count_ += 1U;
    next_equation_uid_ += 1U;
    return true;
}

bool MathModel::addInequality(const char* name,
                              ExpressionNode* lhs,
                              ExpressionNode* rhs,
                              MathOperation relation,
                              std::uint32_t* uid_out)
{
    if (!ensureInequalityCapacity() || lhs == static_cast<ExpressionNode*>(0) || rhs == static_cast<ExpressionNode*>(0)) {
        return false;
    }

    MathInequality* inequality = &inequalities_[inequality_count_];
    inequality->uid = next_inequality_uid_;
    (void)copyName(inequality->name, sizeof(inequality->name), name);
    inequality->lhs = lhs;
    inequality->rhs = rhs;
    inequality->relation = relation;
    inequality->released = false;
    if (uid_out != static_cast<std::uint32_t*>(0)) {
        *uid_out = inequality->uid;
    }
    inequality_count_ += 1U;
    next_inequality_uid_ += 1U;
    return true;
}

bool MathModel::addFunction(const char* name,
                            const char* const* arguments,
                            std::uint32_t argument_count,
                            ExpressionNode* body,
                            std::uint32_t* uid_out)
{
    if (!ensureFunctionCapacity() || body == static_cast<ExpressionNode*>(0) || argument_count > 8U) {
        return false;
    }

    MathFunction* function = &functions_[function_count_];
    function->uid = next_function_uid_;
    (void)copyName(function->name, sizeof(function->name), name);
    function->argument_count = argument_count;
    function->body = body;
    function->released = false;
    for (std::uint32_t index = 0U; index < argument_count; ++index) {
        (void)copyName(function->argument_names[index], sizeof(function->argument_names[index]), arguments[index]);
    }
    if (uid_out != static_cast<std::uint32_t*>(0)) {
        *uid_out = function->uid;
    }
    function_count_ += 1U;
    next_function_uid_ += 1U;
    return true;
}

bool MathModel::parseAndAddEquation(const char* text, std::uint32_t* uid_out, char* error, std::size_t error_size)
{
    ExpressionParser parser(*this);
    ExpressionNode* root = static_cast<ExpressionNode*>(0);
    if (!parser.parse(text, &root, error, error_size) || root == static_cast<ExpressionNode*>(0)) {
        return false;
    }

    ExpressionNode* lhs = root;
    ExpressionNode* rhs = root;
    if (root->node_type == MathNodeType::OPERATION) {
        if (root->operation == MathOperation::EQ ||
            root->operation == MathOperation::NE ||
            root->operation == MathOperation::LT ||
            root->operation == MathOperation::GT ||
            root->operation == MathOperation::LE ||
            root->operation == MathOperation::GE) {
            lhs = root->left;
            rhs = root->right;
        } else {
            rhs = makeConstantNode(1ULL, 1U);
        }
    } else {
        rhs = makeConstantNode(1ULL, 1U);
    }

    return addEquation(root->node_type == MathNodeType::OPERATION &&
                           (root->operation == MathOperation::EQ ||
                            root->operation == MathOperation::NE ||
                            root->operation == MathOperation::LT ||
                            root->operation == MathOperation::GT ||
                            root->operation == MathOperation::LE ||
                            root->operation == MathOperation::GE)
                           ? root->name
                           : text,
                       lhs,
                       rhs,
                       uid_out);
}

ExpressionNode* MathModel::makeNode(MathNodeType node_type, MathOperation operation)
{
    if (allocator_ == static_cast<core::PagedAllocator*>(0)) {
        return static_cast<ExpressionNode*>(0);
    }

    ExpressionNode* node = allocator_->allocate<ExpressionNode>(this, core::ObjectType::EXPRESSION_NODE);
    if (node == static_cast<ExpressionNode*>(0)) {
        return static_cast<ExpressionNode*>(0);
    }

    node->left = static_cast<ExpressionNode*>(0);
    node->right = static_cast<ExpressionNode*>(0);
    node->parent_container = this;
    node->variable_uid = 0U;
    node->node_type = node_type;
    node->flags = 0U;
    node->operation = operation;
    node->value = 0ULL;
    node->bit_start = 0U;
    node->bit_end = 0U;
    node->name[0] = '\0';
    return node;
}

ExpressionNode* MathModel::makeVariableNode(std::uint32_t variable_uid)
{
    ExpressionNode* node = makeNode(MathNodeType::VARIABLE, MathOperation::NONE);
    if (node == static_cast<ExpressionNode*>(0)) {
        return static_cast<ExpressionNode*>(0);
    }

    node->variable_uid = variable_uid;
    const MathVariable* variable = variableByUid(variable_uid);
    if (variable != static_cast<const MathVariable*>(0)) {
        (void)copyName(node->name, sizeof(node->name), variable->name);
    }
    return node;
}

ExpressionNode* MathModel::makeConstantNode(std::uint64_t value, std::uint32_t bit_width)
{
    ExpressionNode* node = makeNode(MathNodeType::CONSTANT, MathOperation::NONE);
    if (node == static_cast<ExpressionNode*>(0)) {
        return static_cast<ExpressionNode*>(0);
    }

    node->value = value;
    node->bit_end = bit_width;
    return node;
}

ExpressionNode* MathModel::makeOperationNode(MathOperation operation, ExpressionNode* left, ExpressionNode* right)
{
    ExpressionNode* node = makeNode(MathNodeType::OPERATION, operation);
    if (node == static_cast<ExpressionNode*>(0)) {
        return static_cast<ExpressionNode*>(0);
    }

    node->left = left;
    node->right = right;
    return node;
}

const MathVariable* MathModel::variableByUid(std::uint32_t uid) const
{
    for (std::uint32_t index = 0U; index < variable_count_; ++index) {
        if (variables_[index].uid == uid) {
            return &variables_[index];
        }
    }
    return static_cast<const MathVariable*>(0);
}

const MathVariable* MathModel::variableByName(const char* name) const
{
    for (std::uint32_t index = 0U; index < variable_count_; ++index) {
        if (std::strcmp(variables_[index].name, name) == 0) {
            return &variables_[index];
        }
    }
    return static_cast<const MathVariable*>(0);
}

const MathConstant* MathModel::constantByUid(std::uint32_t uid) const
{
    for (std::uint32_t index = 0U; index < constant_count_; ++index) {
        if (constants_[index].uid == uid) {
            return &constants_[index];
        }
    }
    return static_cast<const MathConstant*>(0);
}

const MathEquation* MathModel::equationByUid(std::uint32_t uid) const
{
    for (std::uint32_t index = 0U; index < equation_count_; ++index) {
        if (equations_[index].uid == uid) {
            return &equations_[index];
        }
    }
    return static_cast<const MathEquation*>(0);
}

const MathFunction* MathModel::functionByName(const char* name) const
{
    for (std::uint32_t index = 0U; index < function_count_; ++index) {
        if (std::strcmp(functions_[index].name, name) == 0) {
            return &functions_[index];
        }
    }
    return static_cast<const MathFunction*>(0);
}

std::uint32_t MathModel::variableCount() const
{
    return variable_count_;
}

std::uint32_t MathModel::constantCount() const
{
    return constant_count_;
}

std::uint32_t MathModel::equationCount() const
{
    return equation_count_;
}

std::uint32_t MathModel::inequalityCount() const
{
    return inequality_count_;
}

std::uint32_t MathModel::functionCount() const
{
    return function_count_;
}

bool MathModel::copyName(char* destination, std::size_t destination_size, const char* source) const
{
    if (destination == static_cast<char*>(0) || destination_size == 0U) {
        return false;
    }

    std::size_t index = 0U;
    while ((index + 1U) < destination_size && source != static_cast<const char*>(0) && source[index] != '\0') {
        destination[index] = source[index];
        index += 1U;
    }
    destination[index] = '\0';
    return true;
}

bool MathModel::ensureVariableCapacity() const
{
    return variable_count_ < core::SystemSettings::MAX_VARIABLES_PER_MODEL;
}

bool MathModel::ensureConstantCapacity() const
{
    return constant_count_ < core::SystemSettings::MAX_CONSTANTS_PER_MODEL;
}

bool MathModel::ensureEquationCapacity() const
{
    return equation_count_ < core::SystemSettings::MAX_EQUATIONS_PER_MODEL;
}

bool MathModel::ensureInequalityCapacity() const
{
    return inequality_count_ < core::SystemSettings::MAX_INEQUALITIES_PER_MODEL;
}

bool MathModel::ensureFunctionCapacity() const
{
    return function_count_ < core::SystemSettings::MAX_FUNCTIONS_PER_MODEL;
}

} // namespace math
} // namespace semon
