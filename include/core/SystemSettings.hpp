#ifndef SEMON_CORE_SYSTEM_SETTINGS_HPP
#define SEMON_CORE_SYSTEM_SETTINGS_HPP

#include <cstddef>
#include <cstdint>

namespace semon {
namespace core {

class SystemSettings {
public:
    SystemSettings() = delete;

    static constexpr std::size_t NATIVE_PAGE_SIZE = 4096U;
    static constexpr std::uint32_t DEFRAGMENTATION_DIVISOR = 4U;
    static constexpr std::uint32_t MAXIMUM_BIT_WIDTH = 65536U;
    static constexpr std::uint64_t MAX_LOOP_CYCLES = 10000000ULL;
    static constexpr std::size_t MAX_NAME_LENGTH = 63U;
    static constexpr std::size_t MAX_REGISTRY_ITEMS = 256U;
    static constexpr std::size_t MAX_VARIABLES_PER_MODEL = 8U;
    static constexpr std::size_t MAX_CONSTANTS_PER_MODEL = 4U;
    static constexpr std::size_t MAX_EQUATIONS_PER_MODEL = 4U;
    static constexpr std::size_t MAX_INEQUALITIES_PER_MODEL = 4U;
    static constexpr std::size_t MAX_FUNCTIONS_PER_MODEL = 1U;
    static constexpr std::size_t MAX_BUS_PINS = 4096U;
    static constexpr std::size_t MAX_CHAINS = 256U;
    static constexpr std::size_t MAX_PAGES = 4096U;
};

} // namespace core
} // namespace semon

#endif
