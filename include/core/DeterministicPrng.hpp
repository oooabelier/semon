#ifndef SEMON_CORE_DETERMINISTIC_PRNG_HPP
#define SEMON_CORE_DETERMINISTIC_PRNG_HPP

#include <cstdint>

namespace semon {
namespace core {

class DeterministicPrng {
public:
    DeterministicPrng();

    void seed(std::uint64_t seed_value);
    std::uint64_t nextU64();
    std::uint32_t nextU32();
    std::uint32_t nextRange(std::uint32_t exclusive_upper_bound);
    std::uint64_t state0() const;
    std::uint64_t state1() const;
    std::uint64_t state2() const;
    std::uint64_t state3() const;
    std::uint64_t seedValue() const;
    std::uint64_t counter() const;
    bool identicalTo(const DeterministicPrng& other) const;

private:
    static std::uint64_t splitMix64(std::uint64_t value);
    static std::uint64_t rotl(std::uint64_t value, int shift);

    std::uint64_t state_[4];
    std::uint64_t seed_;
    std::uint64_t counter_;
};

} // namespace core
} // namespace semon

#endif
