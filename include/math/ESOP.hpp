#ifndef SEMON_MATH_ESOP_HPP
#define SEMON_MATH_ESOP_HPP

#include <cstddef>
#include <cstdint>

#include "core/PagedAllocator.hpp"
#include "core/SystemSettings.hpp"
#include "math/MathTypes.hpp"

namespace semon {
namespace math {

struct ESOPProductTerm {
    std::uint64_t positive_mask[2];  // Up to 128 variables
    std::uint64_t negative_mask[2];
    ESOPProductTerm* next;
};

struct ESOPExpression {
    ESOPProductTerm* terms;
    std::uint32_t term_count;
    std::uint32_t variable_count;
};

class ESOPManager {
public:
    explicit ESOPManager(core::PagedAllocator* allocator);

    bool initialize(std::uint32_t max_variables);

    ESOPExpression* createExpression(std::uint32_t var_count);
    void destroyExpression(ESOPExpression* expr);

    bool addTerm(ESOPExpression* expr, const bool* positive, const bool* negative);
    ESOPExpression* xorExpressions(ESOPExpression* a, ESOPExpression* b);
    ESOPExpression* andExpressions(ESOPExpression* a, ESOPExpression* b);
    ESOPExpression* notExpression(ESOPExpression* a);

    bool synthesizeToCircuit(ESOPExpression* expr, circuit::Circuit& target,
                             circuit::Pin** output_pin,
                             const math::MathVariable* variables[],
                             std::uint32_t var_count);

    std::uint32_t termCount(ESOPExpression* expr) const;

private:
    core::PagedAllocator* allocator_;
    std::uint32_t max_variables_;
};

} // namespace math
} // namespace semon

#endif
