#ifndef SEMON_CORE_REGISTRY_HPP
#define SEMON_CORE_REGISTRY_HPP

#include <cstddef>
#include <cstdint>

#include "core/SystemSettings.hpp"

namespace semon {
namespace core {

struct RegistryItem {
    std::uint32_t uid;
    char name[64];
    bool active;

    RegistryItem();
};

class ModelRegistry {
public:
    ModelRegistry();

    bool add(std::uint32_t uid, const char* name);
    bool remove(std::uint32_t uid);
    const RegistryItem* find(std::uint32_t uid) const;
    const RegistryItem* at(std::size_t index) const;
    std::size_t count() const;
    void clear();

private:
    RegistryItem items_[SystemSettings::MAX_REGISTRY_ITEMS];
    std::size_t count_;
};

class CircuitRegistry {
public:
    CircuitRegistry();

    bool add(std::uint32_t uid, const char* name);
    bool remove(std::uint32_t uid);
    const RegistryItem* find(std::uint32_t uid) const;
    const RegistryItem* at(std::size_t index) const;
    std::size_t count() const;
    void clear();

private:
    RegistryItem items_[SystemSettings::MAX_REGISTRY_ITEMS];
    std::size_t count_;
};

extern ModelRegistry g_modelRegistry;
extern CircuitRegistry g_circuitRegistry;

} // namespace core
} // namespace semon

#endif
