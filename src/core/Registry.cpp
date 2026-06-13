#include "core/Registry.hpp"

#include <cstring>

namespace semon {
namespace core {

namespace {
void copySmallName(char* destination, const char* source)
{
    std::size_t index = 0U;
    while ((index + 1U) < sizeof(RegistryItem::name) && source != static_cast<const char*>(0) && source[index] != '\0') {
        destination[index] = source[index];
        index += 1U;
    }
    destination[index] = '\0';
}
}

RegistryItem::RegistryItem()
    : uid(0U),
      name{0},
      active(false)
{
}

ModelRegistry::ModelRegistry()
    : items_(),
      count_(0U)
{
}

bool ModelRegistry::add(std::uint32_t uid, const char* name)
{
    if (count_ >= SystemSettings::MAX_REGISTRY_ITEMS) {
        return false;
    }
    for (std::size_t index = 0U; index < count_; ++index) {
        if (items_[index].uid == uid) {
            copySmallName(items_[index].name, name);
            items_[index].active = true;
            return true;
        }
    }
    items_[count_].uid = uid;
    copySmallName(items_[count_].name, name);
    items_[count_].active = true;
    count_ += 1U;
    return true;
}

bool ModelRegistry::remove(std::uint32_t uid)
{
    for (std::size_t index = 0U; index < count_; ++index) {
        if (items_[index].uid == uid) {
            items_[index].active = false;
            return true;
        }
    }
    return false;
}

const RegistryItem* ModelRegistry::find(std::uint32_t uid) const
{
    for (std::size_t index = 0U; index < count_; ++index) {
        if (items_[index].uid == uid && items_[index].active) {
            return &items_[index];
        }
    }
    return static_cast<const RegistryItem*>(0);
}

const RegistryItem* ModelRegistry::at(std::size_t index) const
{
    if (index >= count_) {
        return static_cast<const RegistryItem*>(0);
    }
    return &items_[index];
}

std::size_t ModelRegistry::count() const
{
    return count_;
}

void ModelRegistry::clear()
{
    count_ = 0U;
}

CircuitRegistry::CircuitRegistry()
    : items_(),
      count_(0U)
{
}

bool CircuitRegistry::add(std::uint32_t uid, const char* name)
{
    if (count_ >= SystemSettings::MAX_REGISTRY_ITEMS) {
        return false;
    }
    for (std::size_t index = 0U; index < count_; ++index) {
        if (items_[index].uid == uid) {
            copySmallName(items_[index].name, name);
            items_[index].active = true;
            return true;
        }
    }
    items_[count_].uid = uid;
    copySmallName(items_[count_].name, name);
    items_[count_].active = true;
    count_ += 1U;
    return true;
}

bool CircuitRegistry::remove(std::uint32_t uid)
{
    for (std::size_t index = 0U; index < count_; ++index) {
        if (items_[index].uid == uid) {
            items_[index].active = false;
            return true;
        }
    }
    return false;
}

const RegistryItem* CircuitRegistry::find(std::uint32_t uid) const
{
    for (std::size_t index = 0U; index < count_; ++index) {
        if (items_[index].uid == uid && items_[index].active) {
            return &items_[index];
        }
    }
    return static_cast<const RegistryItem*>(0);
}

const RegistryItem* CircuitRegistry::at(std::size_t index) const
{
    if (index >= count_) {
        return static_cast<const RegistryItem*>(0);
    }
    return &items_[index];
}

std::size_t CircuitRegistry::count() const
{
    return count_;
}

void CircuitRegistry::clear()
{
    count_ = 0U;
}

ModelRegistry g_modelRegistry;
CircuitRegistry g_circuitRegistry;

} // namespace core
} // namespace semon
