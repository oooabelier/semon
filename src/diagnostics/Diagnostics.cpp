#include "diagnostics/Diagnostics.hpp"

#include "core/SystemSettings.hpp"

namespace semon {
namespace diagnostics {

bool g_errorFlag = false;

namespace {
DiagnosticRecord g_last_record = {
    static_cast<const char*>(0),
    static_cast<void*>(0),
    0U,
    static_cast<std::uint32_t>(DiagnosticCode::NONE),
    0ULL,
    {0}
};
std::uint64_t g_cycle_count = 0ULL;
}

void Diagnostics::clear()
{
    g_errorFlag = false;
    g_last_record.component = static_cast<const char*>(0);
    g_last_record.page_address = static_cast<void*>(0);
    g_last_record.object_offset = 0U;
    g_last_record.code = static_cast<std::uint32_t>(DiagnosticCode::NONE);
    g_last_record.cycle_count = 0ULL;
    g_last_record.label[0] = '\0';
    g_cycle_count = 0ULL;
}

void Diagnostics::set(DiagnosticCode code,
                      const char* component,
                      void* page_address,
                      std::size_t object_offset,
                      const char* label)
{
    g_errorFlag = true;
    g_last_record.component = component;
    g_last_record.page_address = page_address;
    g_last_record.object_offset = object_offset;
    g_last_record.code = static_cast<std::uint32_t>(code);
    g_last_record.cycle_count = g_cycle_count;
    if (label != static_cast<const char*>(0)) {
        std::size_t index = 0U;
        while ((index + 1U) < sizeof(g_last_record.label) && label[index] != '\0') {
            g_last_record.label[index] = label[index];
            index += 1U;
        }
        g_last_record.label[index] = '\0';
    } else {
        g_last_record.label[0] = '\0';
    }
}

DiagnosticRecord Diagnostics::lastRecord()
{
    return g_last_record;
}

std::uint64_t Diagnostics::cycleCount()
{
    return g_cycle_count;
}

void Diagnostics::enterLoop()
{
    g_cycle_count += 1ULL;
    if (g_cycle_count > core::SystemSettings::MAX_LOOP_CYCLES) {
        Diagnostics::set(DiagnosticCode::LIMIT,
                         "loop-watchdog",
                         static_cast<void*>(0),
                         0U,
                         "MAX_LOOP_CYCLES");
    }
}

void Diagnostics::enterLoop(std::uint64_t current_cycle)
{
    if (current_cycle > g_cycle_count) {
        g_cycle_count = current_cycle;
    } else {
        g_cycle_count += 1ULL;
    }

    if (g_cycle_count > core::SystemSettings::MAX_LOOP_CYCLES) {
        Diagnostics::set(DiagnosticCode::LIMIT,
                         "loop-watchdog",
                         static_cast<void*>(0),
                         0U,
                         "MAX_LOOP_CYCLES");
    }
}

} // namespace diagnostics
} // namespace semon
