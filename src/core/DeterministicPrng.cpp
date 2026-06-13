#include "core/DeterministicPrng.hpp"

namespace semon {
namespace core {

DeterministicPrng::DeterministicPrng()
    : state_{0ULL, 0ULL, 0ULL, 0ULL},
      seed_(0ULL),
      counter_(0ULL)
{
}

void DeterministicPrng::seed(std::uint64_t seed_value)
{
    seed_ = seed_value;
    counter_ = 0ULL;
    std::uint64_t value = seed_value;
    for (std::uint32_t index = 0U; index < 4U; ++index) {
        value = splitMix64(value);
        state_[index] = value;
    }
    if ((state_[0] | state_[1] | state_[2] | state_[3]) == 0ULL) {
        state_[0] = 0x9E3779B97F4A7C15ULL;
    }
}

std::uint64_t DeterministicPrng::nextU64()
{
    const std::uint64_t result = rotl(state_[1] * 5ULL, 7) * 9ULL;
    const std::uint64_t t = state_[1] << 17;

    state_[2] ^= state_[0];
    state_[3] ^= state_[1];
    state_[1] ^= state_[2];
    state_[0] ^= state_[3];
    state_[2] ^= t;
    state_[3] = rotl(state_[3], 45);

    counter_ += 1ULL;
    return result;
}

std::uint32_t DeterministicPrng::nextU32()
{
    return static_cast<std::uint32_t>(nextU64() >> 32U);
}

std::uint32_t DeterministicPrng::nextRange(std::uint32_t exclusive_upper_bound)
{
    if (exclusive_upper_bound == 0U) {
        return 0U;
    }
    return static_cast<std::uint32_t>(nextU64() % exclusive_upper_bound);
}

std::uint64_t DeterministicPrng::state0() const
{
    return state_[0];
}

std::uint64_t DeterministicPrng::state1() const
{
    return state_[1];
}

std::uint64_t DeterministicPrng::state2() const
{
    return state_[2];
}

std::uint64_t DeterministicPrng::state3() const
{
    return state_[3];
}

std::uint64_t DeterministicPrng::seedValue() const
{
    return seed_;
}

std::uint64_t DeterministicPrng::counter() const
{
    return counter_;
}

bool DeterministicPrng::identicalTo(const DeterministicPrng& other) const
{
    return (state_[0] == other.state_[0]) &&
           (state_[1] == other.state_[1]) &&
           (state_[2] == other.state_[2]) &&
           (state_[3] == other.state_[3]) &&
           (seed_ == other.seed_) &&
           (counter_ == other.counter_);
}

std::uint64_t DeterministicPrng::splitMix64(std::uint64_t value)
{
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

std::uint64_t DeterministicPrng::rotl(std::uint64_t value, int shift)
{
    return (value << shift) | (value >> (64 - shift));
}

} // namespace core
} // namespace semon
