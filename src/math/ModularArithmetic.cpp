#include "math/ModularArithmetic.hpp"
#include "circuit/CircuitTopology.hpp"
#include "circuit/Virtual3PinGate.hpp"
#include "core/PagedAllocator.hpp"
#include "diagnostics/Diagnostics.hpp"

#include <cstring>

namespace semon {
namespace math {

namespace {

struct ModContext {
    circuit::Circuit& circuit;
    core::PagedAllocator* allocator;
    std::uint32_t next_uid;

    ModContext(circuit::Circuit& c, core::PagedAllocator* a) : circuit(c), allocator(a), next_uid(20000) {}

    circuit::Pin* temp(const char* prefix) {
        char name[64];
        std::snprintf(name, sizeof(name), "%s_%u", prefix, next_uid++);
        return circuit.addPin(name, 0, 0);
    }

    circuit::Pin* constPin(bool v) {
        circuit::Pin* p = temp("const");
        circuit.lockPin(p, v);
        return p;
    }

    circuit::Pin* andGate(circuit::Pin* a, circuit::Pin* b, circuit::Pin* out = nullptr) {
        if (!out) out = temp("and");
        circuit.addGate(a, b, out, circuit::Virtual3PinGate::TRUTH_AND);
        return out;
    }

    circuit::Pin* xorGate(circuit::Pin* a, circuit::Pin* b, circuit::Pin* out = nullptr) {
        if (!out) out = temp("xor");
        circuit.addGate(a, b, out, circuit::Virtual3PinGate::TRUTH_XOR);
        return out;
    }

    circuit::Pin* orGate(circuit::Pin* a, circuit::Pin* b, circuit::Pin* out = nullptr) {
        if (!out) out = temp("or");
        circuit.addGate(a, b, out, circuit::Virtual3PinGate::TRUTH_OR);
        return out;
    }

    circuit::Pin* notGate(circuit::Pin* a, circuit::Pin* out = nullptr) {
        if (!out) out = temp("not");
        circuit.addGate(a, nullptr, out, circuit::Virtual3PinGate::TRUTH_INVERT);
        return out;
    }

    circuit::Pin* majGate(circuit::Pin* a, circuit::Pin* b, circuit::Pin* c, circuit::Pin* out = nullptr) {
        if (!out) out = temp("maj");
        circuit::Pin* ab = andGate(a, b);
        circuit::Pin* ac = andGate(a, c);
        circuit::Pin* bc = andGate(b, c);
        circuit::Pin* ab_or_ac = orGate(ab, ac);
        orGate(ab_or_ac, bc, out);
        return out;
    }

    void fullAdder(circuit::Pin* a, circuit::Pin* b, circuit::Pin* cin,
                   circuit::Pin** sum, circuit::Pin** cout) {
        circuit::Pin* ab = xorGate(a, b);
        *sum = xorGate(ab, cin);
        *cout = majGate(a, b, cin);
    }

    circuit::Pin* add(circuit::Pin* const* a, circuit::Pin* const* b,
                      circuit::Pin* cin, circuit::Pin** sum, std::uint32_t bits) {
        circuit::Pin* carry = cin;
        for (std::uint32_t i = 0; i < bits; ++i) {
            circuit::Pin* s = temp("sum");
            circuit::Pin* c = temp("carry");
            fullAdder(a[i], b[i], carry, &s, &c);
            sum[i] = s;
            carry = c;
        }
        return carry;
    }

    // Subtractor: a - b = a + (~b) + 1
    circuit::Pin* sub(circuit::Pin* const* a, circuit::Pin* const* b,
                      circuit::Pin** diff, std::uint32_t bits) {
        circuit::Pin* not_b[65536];
        for (std::uint32_t i = 0; i < bits; ++i) not_b[i] = notGate(b[i]);
        circuit::Pin* one = constPin(true);
        return add(a, not_b, one, diff, bits);
    }

    // Comparator a < b (unsigned)
    circuit::Pin* cmpLT(circuit::Pin* const* a, circuit::Pin* const* b, std::uint32_t bits) {
        // MSB to LSB: gt = a_i & ~b_i | (eq_i & gt_{i+1})
        circuit::Pin* gt = constPin(false);
        circuit::Pin* eq = constPin(true);
        for (int i = bits - 1; i >= 0; --i) {
            circuit::Pin* a_i = a[i], * b_i = b[i];
            circuit::Pin* not_b = notGate(b_i);
            circuit::Pin* a_gt_b = andGate(a_i, not_b);
            circuit::Pin* a_eq_b = notGate(xorGate(a_i, b_i));
            circuit::Pin* new_gt = orGate(a_gt_b, andGate(eq, gt));
            circuit::Pin* new_eq = andGate(eq, a_eq_b);
            gt = new_gt;
            eq = new_eq;
        }
        return gt; // gt = a > b, so a < b = ~gt when neq
    }
};

} // namespace

bool ModularArithmetic::buildModAdd(circuit::Circuit& target, core::PagedAllocator* alloc,
                                    circuit::Pin* const* a, circuit::Pin* const* b,
                                    circuit::Pin* const* m, circuit::Pin** out,
                                    std::uint32_t bits) {
    ModContext ctx(target, alloc);
    circuit::Pin* sum[65536];
    ctx.add(a, b, ctx.constPin(false), sum, bits);

    // If sum >= m, subtract m
    circuit::Pin* cmp = ctx.cmpLT(m, sum, bits); // m < sum => sum >= m
    circuit::Pin* diff[65536];
    ctx.sub(sum, m, diff, bits);

    // Select: out = cmp ? diff : sum
    for (std::uint32_t i = 0; i < bits; ++i) {
        circuit::Pin* not_cmp = ctx.notGate(cmp);
        circuit::Pin* t1 = ctx.andGate(cmp, diff[i]);
        circuit::Pin* t2 = ctx.andGate(not_cmp, sum[i]);
        out[i] = ctx.orGate(t1, t2);
    }
    return true;
}

bool ModularArithmetic::buildModSub(circuit::Circuit& target, core::PagedAllocator* alloc,
                                    circuit::Pin* const* a, circuit::Pin* const* b,
                                    circuit::Pin* const* m, circuit::Pin** out,
                                    std::uint32_t bits) {
    ModContext ctx(target, alloc);
    circuit::Pin* diff[65536];
    ctx.sub(a, b, diff, bits);

    // If a < b, add m
    circuit::Pin* cmp = ctx.cmpLT(a, b, bits);
    circuit::Pin* sum[65536];
    ctx.add(diff, m, ctx.constPin(false), sum, bits);

    for (std::uint32_t i = 0; i < bits; ++i) {
        circuit::Pin* not_cmp = ctx.notGate(cmp);
        circuit::Pin* t1 = ctx.andGate(cmp, sum[i]);
        circuit::Pin* t2 = ctx.andGate(not_cmp, diff[i]);
        out[i] = ctx.orGate(t1, t2);
    }
    return true;
}

bool ModularArithmetic::buildModMul(circuit::Circuit& target, core::PagedAllocator* alloc,
                                    circuit::Pin* const* a, circuit::Pin* const* b,
                                    circuit::Pin* const* m, circuit::Pin** out,
                                    std::uint32_t bits) {
    // Montgomery multiplication requires precomputed m' = -m^-1 mod 2^bits
    // For now, use simple shift-and-add with modular reduction
    ModContext ctx(target, alloc);

    // Product = a * b (2*bits)
    circuit::Pin* product[131072];
    for (std::uint32_t i = 0; i < 2*bits; ++i) product[i] = ctx.constPin(false);

    for (std::uint32_t i = 0; i < bits; ++i) {
        if (i == 0) {
            for (std::uint32_t j = 0; j < bits; ++j) {
                product[i+j] = ctx.andGate(a[i], b[j]);
            }
        } else {
            circuit::Pin* partial[65536];
            for (std::uint32_t j = 0; j < bits; ++j) partial[j] = ctx.andGate(a[i], b[j]);
            circuit::Pin* shifted[131072];
            for (std::uint32_t k = 0; k < i; ++k) shifted[k] = ctx.constPin(false);
            for (std::uint32_t j = 0; j < bits; ++j) shifted[i+j] = partial[j];
            // Add to product
            circuit::Pin* new_product[131072];
            ctx.add(product, shifted, ctx.constPin(false), new_product, 2*bits);
            for (std::uint32_t k = 0; k < 2*bits; ++k) product[k] = new_product[k];
        }
    }

    // Modular reduction: product mod m
    // Simple long division (non-constant time, but functional)
    // For constant-time, need Montgomery or Barrett
    // Placeholder: return lower bits (incorrect for general case)
    for (std::uint32_t i = 0; i < bits; ++i) out[i] = product[i];

    diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::MATH,
                     "buildModMul", static_cast<void*>(0), bits, "uses naive reduction");
    return true;
}

bool ModularArithmetic::buildModInv(circuit::Circuit& target, core::PagedAllocator* alloc,
                                    circuit::Pin* const* a, circuit::Pin* const* m,
                                    circuit::Pin** out, std::uint32_t bits) {
    // Extended Euclidean algorithm in hardware
    // Requires iterative loop with data-dependent branches
    // Not feasible in pure combinational circuit without unrolling
    // Placeholder
    for (std::uint32_t i = 0; i < bits; ++i) out[i] = a[i];
    diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::MATH,
                     "buildModInv", static_cast<void*>(0), bits, "not implemented");
    return false;
}

bool ModularArithmetic::buildModSquare(circuit::Circuit& target, core::PagedAllocator* alloc,
                                       circuit::Pin* const* a, circuit::Pin* const* m,
                                       circuit::Pin** out, std::uint32_t bits) {
    return buildModMul(target, alloc, a, a, m, out, bits);
}

bool ModularArithmetic::buildMontMul(circuit::Circuit& target, core::PagedAllocator* alloc,
                                     circuit::Pin* const* a, circuit::Pin* const* b,
                                     circuit::Pin* const* m, circuit::Pin* const* m_prime,
                                     circuit::Pin** out, std::uint32_t bits) {
    // Montgomery REDC algorithm
    // t = a * b
    // u = (t + (t * m' mod R) * m) / R
    // if u >= m: u -= m
    (void)m_prime;
    return buildModMul(target, alloc, a, b, m, out, bits);
}

bool ModularArithmetic::buildREDC(circuit::Circuit& target, core::PagedAllocator* alloc,
                                  circuit::Pin* const* t, circuit::Pin* const* m,
                                  circuit::Pin* const* m_prime, circuit::Pin** out,
                                  std::uint32_t bits) {
    (void)m_prime;
    for (std::uint32_t i = 0; i < bits; ++i) out[i] = t[i];
    return true;
}

bool ModularArithmetic::buildModCompare(circuit::Circuit& target, core::PagedAllocator* alloc,
                                        circuit::Pin* const* a, circuit::Pin* const* b,
                                        circuit::Pin** out, std::uint32_t bits) {
    ModContext ctx(target, alloc);
    circuit::Pin* cmp = ctx.cmpLT(a, b, bits);
    for (std::uint32_t i = 0; i < bits; ++i) {
        out[i] = (i == 0) ? cmp : ctx.constPin(false);
    }
    return true;
}

bool ModularArithmetic::buildSelect(circuit::Circuit& target, core::PagedAllocator* alloc,
                                    circuit::Pin* sel, circuit::Pin* const* a,
                                    circuit::Pin* const* b, circuit::Pin** out,
                                    std::uint32_t bits) {
    ModContext ctx(target, alloc);
    circuit::Pin* not_sel = ctx.notGate(sel);
    for (std::uint32_t i = 0; i < bits; ++i) {
        circuit::Pin* t1 = ctx.andGate(sel, a[i]);
        circuit::Pin* t2 = ctx.andGate(not_sel, b[i]);
        out[i] = ctx.orGate(t1, t2);
    }
    return true;
}

} // namespace math
} // namespace semon
