#ifndef SEMON_DIAGNOSTICS_DIAGNOSTICS_HPP
#define SEMON_DIAGNOSTICS_DIAGNOSTICS_HPP

#include <cstddef>
#include <cstdint>

namespace semon {
namespace diagnostics {

enum class DiagnosticCode : std::uint32_t {
    NONE = 0U,
    CONTRACT = 1U,
    ALLOCATOR = 2U,
    MATH = 3U,
    CIRCUIT = 4U,
    SOLVER = 5U,
    PERSISTENCE = 6U,
    LIMIT = 7U
};

struct DiagnosticRecord {
    const char* component;
    void* page_address;
    std::size_t object_offset;
    std::uint32_t code;
    std::uint64_t cycle_count;
    char label[64];
};

extern bool g_errorFlag;

class Diagnostics {
public:
    Diagnostics() = delete;

    static void clear();
    static void set(DiagnosticCode code,
                    const char* component,
                    void* page_address,
                    std::size_t object_offset,
                    const char* label);
    static DiagnosticRecord lastRecord();
    static std::uint64_t cycleCount();
    static void enterLoop();
    static void enterLoop(std::uint64_t current_cycle);
};

} // namespace diagnostics
} // namespace semon

#define SEMON_CONTRACT(expression) \
    do { \
        if (!(expression)) { \
            semon::diagnostics::Diagnostics::set( \
                semon::diagnostics::DiagnosticCode::CONTRACT, \
                __FILE__, \
                reinterpret_cast<void*>(0), \
                static_cast<std::size_t>(__LINE__), \
                #expression); \
            return false; \
        } \
    } while (false)

#define SEMON_LOOP() \
    do { \
        semon::diagnostics::Diagnostics::enterLoop(); \
    } while (false)

#endif
