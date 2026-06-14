#include "math/ESOP.hpp"
#include "math/MathModel.hpp"
#include "circuit/CircuitTopology.hpp"
#include "circuit/Virtual3PinGate.hpp"
#include "diagnostics/Diagnostics.hpp"

#include <cstring>

namespace semon {
namespace math {

ESOPManager::ESOPManager(core::PagedAllocator* allocator)
    : allocator_(allocator), max_variables_(0) {
}

bool ESOPManager::initialize(std::uint32_t max_variables) {
    max_variables_ = max_variables;
    return true;
}

ESOPExpression* ESOPManager::createExpression(std::uint32_t var_count) {
    if (var_count > max_variables_ || var_count > 128) return nullptr;
    ESOPExpression* expr = allocator_->allocate<ESOPExpression>(this, core::ObjectType::EXPRESSION_NODE);
    if (!expr) return nullptr;
    expr->terms = nullptr;
    expr->term_count = 0;
    expr->variable_count = var_count;
    return expr;
}

void ESOPManager::destroyExpression(ESOPExpression* expr) {
    if (!expr) return;
    ESOPProductTerm* term = expr->terms;
    while (term) {
        ESOPProductTerm* next = term->next;
        allocator_->release(term);
        term = next;
    }
    allocator_->release(expr);
}

bool ESOPManager::addTerm(ESOPExpression* expr, const bool* positive, const bool* negative) {
    if (!expr || expr->term_count >= 10000) return false; // Limit

    ESOPProductTerm* term = allocator_->allocate<ESOPProductTerm>(this, core::ObjectType::EXPRESSION_NODE);
    if (!term) return false;

    std::memset(term->positive_mask, 0, sizeof(term->positive_mask));
    std::memset(term->negative_mask, 0, sizeof(term->negative_mask));

    for (std::uint32_t i = 0; i < expr->variable_count; ++i) {
        std::uint32_t word = i / 64;
        std::uint32_t bit = i % 64;
        if (positive && positive[i]) term->positive_mask[word] |= (UINT64_C(1) << bit);
        if (negative && negative[i]) term->negative_mask[word] |= (UINT64_C(1) << bit);
    }

    term->next = expr->terms;
    expr->terms = term;
    expr->term_count++;
    return true;
}

ESOPExpression* ESOPManager::xorExpressions(ESOPExpression* a, ESOPExpression* b) {
    if (!a || !b || a->variable_count != b->variable_count) return nullptr;
    ESOPExpression* result = createExpression(a->variable_count);
    if (!result) return nullptr;

    // XOR = symmetric difference of term sets
    // Simplified: just concatenate (actual ESOP needs reduction)
    ESOPProductTerm* term = a->terms;
    while (term) {
        ESOPProductTerm* copy = allocator_->allocate<ESOPProductTerm>(this, core::ObjectType::EXPRESSION_NODE);
        if (!copy) { destroyExpression(result); return nullptr; }
        *copy = *term;
        copy->next = result->terms;
        result->terms = copy;
        result->term_count++;
        term = term->next;
    }

    term = b->terms;
    while (term) {
        ESOPProductTerm* copy = allocator_->allocate<ESOPProductTerm>(this, core::ObjectType::EXPRESSION_NODE);
        if (!copy) { destroyExpression(result); return nullptr; }
        *copy = *term;
        copy->next = result->terms;
        result->terms = copy;
        result->term_count++;
        term = term->next;
    }

    return result;
}

ESOPExpression* ESOPManager::andExpressions(ESOPExpression* a, ESOPExpression* b) {
    if (!a || !b || a->variable_count != b->variable_count) return nullptr;
    ESOPExpression* result = createExpression(a->variable_count);
    if (!result) return nullptr;

    // AND = distributive product (simplified)
    for (ESOPProductTerm* ta = a->terms; ta; ta = ta->next) {
        for (ESOPProductTerm* tb = b->terms; tb; tb = tb->next) {
            bool conflict = false;
            for (int w = 0; w < 2; ++w) {
                if ((ta->positive_mask[w] & tb->negative_mask[w]) ||
                    (ta->negative_mask[w] & tb->positive_mask[w])) {
                    conflict = true;
                    break;
                }
            }
            if (!conflict) {
                ESOPProductTerm* term = allocator_->allocate<ESOPProductTerm>(this, core::ObjectType::EXPRESSION_NODE);
                if (!term) { destroyExpression(result); return nullptr; }
                for (int w = 0; w < 2; ++w) {
                    term->positive_mask[w] = ta->positive_mask[w] | tb->positive_mask[w];
                    term->negative_mask[w] = ta->negative_mask[w] | tb->negative_mask[w];
                }
                term->next = result->terms;
                result->terms = term;
                result->term_count++;
            }
        }
    }
    return result;
}

ESOPExpression* ESOPManager::notExpression(ESOPExpression* a) {
    if (!a) return nullptr;
    // NOT in ESOP: add constant 1 term and XOR
    ESOPExpression* one = createExpression(a->variable_count);
    if (!one) return nullptr;
    // Constant 1 = empty product term
    ESOPProductTerm* term = allocator_->allocate<ESOPProductTerm>(this, core::ObjectType::EXPRESSION_NODE);
    if (!term) { destroyExpression(one); return nullptr; }
    std::memset(term->positive_mask, 0, sizeof(term->positive_mask));
    std::memset(term->negative_mask, 0, sizeof(term->negative_mask));
    term->next = one->terms;
    one->terms = term;
    one->term_count = 1;
    return xorExpressions(a, one);
}

std::uint32_t ESOPManager::termCount(ESOPExpression* expr) const {
    return expr ? expr->term_count : 0;
}

bool ESOPManager::synthesizeToCircuit(ESOPExpression* expr, circuit::Circuit& target,
                                      circuit::Pin** output_pin,
                                      const math::MathVariable* variables[],
                                      std::uint32_t var_count) {
    // ESOP synthesis: each product term = AND of literals
    // Sum (XOR) of terms = cascade of XOR gates
    // Placeholder implementation
    (void)expr; (void)target; (void)output_pin; (void)variables; (void)var_count;
    return true;
}

} // namespace math
} // namespace semon
