#ifndef SEMON_MATH_MODULAR_ARITHMETIC_HPP
#define SEMON_MATH_MODULAR_ARITHMETIC_HPP

#include <cstddef>
#include <cstdint>

#include "core/PagedAllocator.hpp"
#include "circuit/CircuitTopology.hpp"

namespace semon {
namespace math {

class ModularArithmetic {
public:
    // Build modular adder: out = (a + b) mod m
    static bool buildModAdd(circuit::Circuit& target, core::PagedAllocator* alloc,
                            circuit::Pin* const* a, circuit::Pin* const* b,
                            circuit::Pin* const* m, circuit::Pin** out,
                            std::uint32_t bits);

    // Build modular subtractor: out = (a - b) mod m
    static bool buildModSub(circuit::Circuit& target, core::PagedAllocator* alloc,
                            circuit::Pin* const* a, circuit::Pin* const* b,
                            circuit::Pin* const* m, circuit::Pin** out,
                            std::uint32_t bits);

    // Build modular multiplier: out = (a * b) mod m (Montgomery)
    static bool buildModMul(circuit::Circuit& target, core::PagedAllocator* alloc,
                            circuit::Pin* const* a, circuit::Pin* const* b,
                            circuit::Pin* const* m, circuit::Pin** out,
                            std::uint32_t bits);

    // Build modular inverter: out = a^-1 mod m (Extended Euclidean)
    static bool buildModInv(circuit::Circuit& target, core::PagedAllocator* alloc,
                            circuit::Pin* const* a, circuit::Pin* const* m,
                            circuit::Pin** out, std::uint32_t bits);

    // Build modular square: out = a^2 mod m
    static bool buildModSquare(circuit::Circuit& target, core::PagedAllocator* alloc,
                               circuit::Pin* const* a, circuit::Pin* const* m,
                               circuit::Pin** out, std::uint32_t bits);

    // Build Montgomery multiplier (internal)
    static bool buildMontMul(circuit::Circuit& target, core::PagedAllocator* alloc,
                             circuit::Pin* const* a, circuit::Pin* const* b,
                             circuit::Pin* const* m, circuit::Pin* const* m_prime,
                             circuit::Pin** out, std::uint32_t bits);

    // Build REDC (Montgomery reduction)
    static bool buildREDC(circuit::Circuit& target, core::PagedAllocator* alloc,
                          circuit::Pin* const* t, circuit::Pin* const* m,
                          circuit::Pin* const* m_prime, circuit::Pin** out,
                          std::uint32_t bits);

    // Build comparator: out = (a < b) for modular values
    static bool buildModCompare(circuit::Circuit& target, core::PagedAllocator* alloc,
                                circuit::Pin* const* a, circuit::Pin* const* b,
                                circuit::Pin** out, std::uint32_t bits);

    // Build constant-time select: out = sel ? a : b
    static bool buildSelect(circuit::Circuit& target, core::PagedAllocator* alloc,
                            circuit::Pin* sel, circuit::Pin* const* a,
                            circuit::Pin* const* b, circuit::Pin** out,
                            std::uint32_t bits);
};

} // namespace math
} // namespace semon

#endif
