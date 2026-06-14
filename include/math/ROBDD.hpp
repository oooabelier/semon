#ifndef SEMON_MATH_ROBDD_HPP
#define SEMON_MATH_ROBDD_HPP

#include <cstddef>
#include <cstdint>

#include "core/PagedAllocator.hpp"
#include "core/SystemSettings.hpp"
#include "math/MathTypes.hpp"

namespace semon {
namespace math {

class ROBDDManager;

struct ROBDDNode {
    ROBDDNode* low;
    ROBDDNode* high;
    void* parent_container;
    std::uint32_t variable_uid;
    std::uint32_t var_index;
    std::uint32_t uid;
    std::uint32_t ref_count;
    bool is_terminal;
    bool terminal_value;
};

class ROBDDManager {
public:
    explicit ROBDDManager(core::PagedAllocator* allocator);

    bool initialize(std::uint32_t max_variables);

    ROBDDNode* getTerminal(bool value);
    ROBDDNode* mkNode(std::uint32_t var_index, ROBDDNode* low, ROBDDNode* high);
    ROBDDNode* apply(ROBDDNode* a, ROBDDNode* b, MathOperation op);
    ROBDDNode* restrict(ROBDDNode* node, std::uint32_t var_index, bool value);
    ROBDDNode* existsAbstract(ROBDDNode* node, std::uint32_t var_index);
    ROBDDNode* forallAbstract(ROBDDNode* node, std::uint32_t var_index);

    bool synthesizeToCircuit(ROBDDNode* root, circuit::Circuit& target,
                             circuit::Pin** output_pin,
                             const math::MathVariable* variables[],
                             std::uint32_t var_count);

    void garbageCollect();
    std::size_t nodeCount() const;

private:
    struct UniqueTableEntry {
        std::uint32_t var_index;
        ROBDDNode* low;
        ROBDDNode* high;
        ROBDDNode* result;
        UniqueTableEntry* next;
    };

    struct ComputedCacheEntry {
        MathOperation op;
        ROBDDNode* a;
        ROBDDNode* b;
        ROBDDNode* result;
        ComputedCacheEntry* next;
    };

    core::PagedAllocator* allocator_;
    ROBDDNode* terminal_0_;
    ROBDDNode* terminal_1_;
    std::uint32_t next_uid_;
    std::uint32_t max_variables_;

    UniqueTableEntry* unique_table_;
    std::size_t unique_table_size_;
    std::size_t unique_table_mask_;

    ComputedCacheEntry* computed_cache_;
    std::size_t computed_cache_size_;
    std::size_t computed_cache_mask_;

    ROBDDNode* findOrAddUnique(std::uint32_t var_index, ROBDDNode* low, ROBDDNode* high);
    ROBDDNode* computeApply(ROBDDNode* a, ROBDDNode* b, MathOperation op);
    void resizeUniqueTable();
    void resizeComputedCache();
};

} // namespace math
} // namespace semon

#endif
