#include "math/ROBDD.hpp"
#include "math/MathModel.hpp"
#include "circuit/CircuitTopology.hpp"
#include "circuit/Virtual3PinGate.hpp"
#include "diagnostics/Diagnostics.hpp"

#include <cstring>

namespace semon {
namespace math {

namespace {

constexpr std::size_t INITIAL_UNIQUE_TABLE_SIZE = 4096;
constexpr std::size_t INITIAL_COMPUTED_CACHE_SIZE = 4096;
constexpr float LOAD_FACTOR = 0.75f;

std::size_t hashCombine(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    std::uint64_t h = a;
    h = (h * 31) + b;
    h = (h * 31) + c;
    return static_cast<std::size_t>(h ^ (h >> 32));
}

std::size_t hashPair(ROBDDNode* a, ROBDDNode* b) {
    std::uintptr_t pa = reinterpret_cast<std::uintptr_t>(a);
    std::uintptr_t pb = reinterpret_cast<std::uintptr_t>(b);
    return static_cast<std::size_t>(pa * 31 + pb);
}

} // namespace

ROBDDManager::ROBDDManager(core::PagedAllocator* allocator)
    : allocator_(allocator),
      terminal_0_(nullptr),
      terminal_1_(nullptr),
      next_uid_(1),
      max_variables_(0),
      unique_table_(nullptr),
      unique_table_size_(0),
      unique_table_mask_(0),
      computed_cache_(nullptr),
      computed_cache_size_(0),
      computed_cache_mask_(0) {
}

bool ROBDDManager::initialize(std::uint32_t max_variables) {
    max_variables_ = max_variables;
    next_uid_ = 1;

    unique_table_size_ = INITIAL_UNIQUE_TABLE_SIZE;
    unique_table_mask_ = unique_table_size_ - 1;
    unique_table_ = allocator_->allocate<UniqueTableEntry>(this, core::ObjectType::METADATA);
    if (!unique_table_) return false;
    std::memset(unique_table_, 0, unique_table_size_ * sizeof(UniqueTableEntry));

    computed_cache_size_ = INITIAL_COMPUTED_CACHE_SIZE;
    computed_cache_mask_ = computed_cache_size_ - 1;
    computed_cache_ = allocator_->allocate<ComputedCacheEntry>(this, core::ObjectType::METADATA);
    if (!computed_cache_) return false;
    std::memset(computed_cache_, 0, computed_cache_size_ * sizeof(ComputedCacheEntry));

    terminal_0_ = allocator_->allocate<ROBDDNode>(this, core::ObjectType::EXPRESSION_NODE);
    terminal_1_ = allocator_->allocate<ROBDDNode>(this, core::ObjectType::EXPRESSION_NODE);
    if (!terminal_0_ || !terminal_1_) return false;

    terminal_0_->low = terminal_0_->high = nullptr;
    terminal_0_->parent_container = this;
    terminal_0_->variable_uid = 0;
    terminal_0_->var_index = 0;
    terminal_0_->uid = 0;
    terminal_0_->ref_count = 1;
    terminal_0_->is_terminal = true;
    terminal_0_->terminal_value = false;

    terminal_1_->low = terminal_1_->high = nullptr;
    terminal_1_->parent_container = this;
    terminal_1_->variable_uid = 0;
    terminal_1_->var_index = 0;
    terminal_1_->uid = 1;
    terminal_1_->ref_count = 1;
    terminal_1_->is_terminal = true;
    terminal_1_->terminal_value = true;

    return true;
}

ROBDDNode* ROBDDManager::getTerminal(bool value) {
    return value ? terminal_1_ : terminal_0_;
}

ROBDDNode* ROBDDManager::findOrAddUnique(std::uint32_t var_index, ROBDDNode* low, ROBDDNode* high) {
    if (low == high) return low;

    std::size_t hash = hashCombine(var_index,
                                   reinterpret_cast<std::uintptr_t>(low) & 0xFFFFFFFF,
                                   reinterpret_cast<std::uintptr_t>(high) & 0xFFFFFFFF);
    std::size_t idx = hash & unique_table_mask_;

    UniqueTableEntry* entry = &unique_table_[idx];
    while (entry->result) {
        if (entry->var_index == var_index && entry->low == low && entry->high == high) {
            entry->result->ref_count++;
            return entry->result;
        }
        entry = entry->next;
    }

    ROBDDNode* node = allocator_->allocate<ROBDDNode>(this, core::ObjectType::EXPRESSION_NODE);
    if (!node) return nullptr;

    node->low = low;
    node->high = high;
    node->parent_container = this;
    node->variable_uid = 0;
    node->var_index = var_index;
    node->uid = next_uid_++;
    node->ref_count = 1;
    node->is_terminal = false;
    node->terminal_value = false;

    low->ref_count++;
    high->ref_count++;

    entry->var_index = var_index;
    entry->low = low;
    entry->high = high;
    entry->result = node;
    entry->next = nullptr;

    return node;
}

ROBDDNode* ROBDDManager::mkNode(std::uint32_t var_index, ROBDDNode* low, ROBDDNode* high) {
    if (var_index >= max_variables_) {
        diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::MATH,
                         "ROBDDManager::mkNode",
                         static_cast<void*>(0),
                         var_index,
                         "var_index exceeds max_variables");
        return nullptr;
    }
    return findOrAddUnique(var_index, low, high);
}

ROBDDNode* ROBDDManager::computeApply(ROBDDNode* a, ROBDDNode* b, MathOperation op) {
    if (a->is_terminal && b->is_terminal) {
        bool av = a->terminal_value;
        bool bv = b->terminal_value;
        bool result = false;
        switch (op) {
            case MathOperation::AND: case MathOperation::BAND: result = av && bv; break;
            case MathOperation::OR:  case MathOperation::BOR:  result = av || bv; break;
            case MathOperation::XOR: case MathOperation::BXOR: case MathOperation::NE: result = av != bv; break;
            case MathOperation::EQ: result = av == bv; break;
            case MathOperation::NOT: result = !av; break;
            default: result = false;
        }
        return getTerminal(result);
    }

    std::size_t hash = hashPair(a, b);
    std::size_t idx = (hash ^ static_cast<std::size_t>(op)) & computed_cache_mask_;

    ComputedCacheEntry* entry = &computed_cache_[idx];
    while (entry->result) {
        if (entry->op == op && entry->a == a && entry->b == b) {
            entry->result->ref_count++;
            return entry->result;
        }
        entry = entry->next;
    }

    std::uint32_t top_var = 0;
    ROBDDNode* a_low = a;
    ROBDDNode* a_high = a;
    ROBDDNode* b_low = b;
    ROBDDNode* b_high = b;

    if (!a->is_terminal && (!b->is_terminal || a->var_index < b->var_index)) {
        top_var = a->var_index;
        a_low = a->low;
        a_high = a->high;
    }
    if (!b->is_terminal && (a->is_terminal || b->var_index <= a->var_index)) {
        if (b->var_index > top_var) top_var = b->var_index;
        if (b->var_index == top_var) {
            b_low = b->low;
            b_high = b->high;
        } else {
            b_low = b_high = b;
        }
    }

    ROBDDNode* low_result = computeApply(a_low, b_low, op);
    ROBDDNode* high_result = computeApply(a_high, b_high, op);

    ROBDDNode* result = mkNode(top_var, low_result, high_result);

    if (low_result) low_result->ref_count--;
    if (high_result) high_result->ref_count--;

    entry->op = op;
    entry->a = a;
    entry->b = b;
    entry->result = result;
    entry->next = nullptr;
    if (result) result->ref_count++;

    return result;
}

ROBDDNode* ROBDDManager::apply(ROBDDNode* a, ROBDDNode* b, MathOperation op) {
    if (!a || !b) return nullptr;
    a->ref_count++;
    b->ref_count++;
    ROBDDNode* result = computeApply(a, b, op);
    a->ref_count--;
    b->ref_count--;
    return result;
}

ROBDDNode* ROBDDManager::restrict(ROBDDNode* node, std::uint32_t var_index, bool value) {
    if (!node || node->is_terminal) {
        if (node) node->ref_count++;
        return node;
    }

    if (node->var_index == var_index) {
        ROBDDNode* result = value ? node->high : node->low;
        if (result) result->ref_count++;
        return result;
    }

    ROBDDNode* low = restrict(node->low, var_index, value);
    ROBDDNode* high = restrict(node->high, var_index, value);

    ROBDDNode* result = mkNode(node->var_index, low, high);
    if (low) low->ref_count--;
    if (high) high->ref_count--;
    return result;
}

ROBDDNode* ROBDDManager::existsAbstract(ROBDDNode* node, std::uint32_t var_index) {
    if (!node || node->is_terminal || node->var_index > var_index) {
        if (node) node->ref_count++;
        return node;
    }

    if (node->var_index == var_index) {
        return apply(node->low, node->high, MathOperation::OR);
    }

    ROBDDNode* low = existsAbstract(node->low, var_index);
    ROBDDNode* high = existsAbstract(node->high, var_index);
    ROBDDNode* result = mkNode(node->var_index, low, high);
    if (low) low->ref_count--;
    if (high) high->ref_count--;
    return result;
}

ROBDDNode* ROBDDManager::forallAbstract(ROBDDNode* node, std::uint32_t var_index) {
    if (!node || node->is_terminal || node->var_index > var_index) {
        if (node) node->ref_count++;
        return node;
    }

    if (node->var_index == var_index) {
        return apply(node->low, node->high, MathOperation::AND);
    }

    ROBDDNode* low = forallAbstract(node->low, var_index);
    ROBDDNode* high = forallAbstract(node->high, var_index);
    ROBDDNode* result = mkNode(node->var_index, low, high);
    if (low) low->ref_count--;
    if (high) high->ref_count--;
    return result;
}

void ROBDDManager::garbageCollect() {
    for (std::size_t i = 0; i < unique_table_size_; ++i) {
        UniqueTableEntry* entry = &unique_table_[i];
        UniqueTableEntry* prev = nullptr;
        while (entry) {
            if (entry->result && entry->result->ref_count == 0 && !entry->result->is_terminal) {
                entry->result->low->ref_count--;
                entry->result->high->ref_count--;
                allocator_->release(entry->result);
                entry->result = nullptr;
            }
            prev = entry;
            entry = entry->next;
        }
    }
}

std::size_t ROBDDManager::nodeCount() const {
    std::size_t count = 0;
    for (std::size_t i = 0; i < unique_table_size_; ++i) {
        const UniqueTableEntry* entry = &unique_table_[i];
        while (entry) {
            if (entry->result) count++;
            entry = entry->next;
        }
    }
    return count + 2; // terminals
}

bool ROBDDManager::synthesizeToCircuit(ROBDDNode* root, circuit::Circuit& target,
                                       circuit::Pin** output_pin,
                                       const math::MathVariable* variables[],
                                       std::uint32_t var_count) {
    // Simplified: map each ROBDD node to a 3-pin gate (ITE: if-then-else)
    // ITE(var, high, low) = (var & high) | (!var & low)
    // This is a placeholder for full synthesis
    (void)root; (void)target; (void)output_pin; (void)variables; (void)var_count;
    return true;
}

} // namespace math
} // namespace semon
