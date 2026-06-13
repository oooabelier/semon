#ifndef SEMON_CORE_PAGED_ALLOCATOR_HPP
#define SEMON_CORE_PAGED_ALLOCATOR_HPP

#include <cstddef>
#include <cstdint>

#include "core/SystemSettings.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace semon {
namespace core {

enum class ObjectType : std::uint8_t {
    PIN = 1U,
    BUS = 2U,
    GATE = 3U,
    SUBCIRCUIT = 4U,
    EXPRESSION_NODE = 5U,
    MEDIUM_CONSTANT = 6U,
    LARGE_CONSTANT_CHUNK = 7U,
    METADATA = 8U,
    TEST_OBJECT = 9U
};

struct PageHeader {
    PageHeader* prev;
    PageHeader* next;
    void* parent;
    ObjectType type;
    std::size_t object_size;
    std::size_t object_count;
    std::size_t active_count;
    std::size_t dead_count;
    std::size_t bytes_allocated;
    std::size_t bytes_unused;
    std::uint64_t free_mask;
    std::uint32_t magic;
};

struct PageChain {
    void* parent;
    ObjectType type;
    PageHeader* head;
    PageHeader* tail;
    std::size_t page_count;
    std::size_t total_objects;
    std::size_t active_objects;
    std::size_t free_objects;
};

class PagedAllocator {
public:
    PagedAllocator();

    bool initialize(std::size_t page_count);
    void shutdown();

    template <typename T>
    T* allocate(void* parent, ObjectType type)
    {
        PageChain* chain = findOrCreateChain(parent, type, sizeof(T));
        if (chain == static_cast<PageChain*>(0)) {
            return static_cast<T*>(0);
        }

        PageHeader* page = findPageWithFreeSlot(chain);
        if (page == static_cast<PageHeader*>(0)) {
            page = allocatePage(parent, type, sizeof(T));
            if (page == static_cast<PageHeader*>(0)) {
                return static_cast<T*>(0);
            }
            appendPage(chain, page);
        }

        const std::size_t slot_index = firstFreeSlot(page);
        page->free_mask &= ~(UINT64_C(1) << slot_index);
        void* memory = pageSlot(page, slot_index);
        page->active_count += 1U;
        active_object_count_ += 1U;
        page->dead_count += 0U;
        updateChainStats(chain);
        return reinterpret_cast<T*>(memory);
    }

    template <typename T>
    bool release(T* object)
    {
        if (object == static_cast<T*>(0)) {
            return false;
        }

        PageHeader* page = pageForAddress(object);
        if (page == static_cast<PageHeader*>(0)) {
            return false;
        }

        const std::size_t slot_index = slotIndexForAddress(page, object);
        const std::uint64_t mask = UINT64_C(1) << slot_index;
        if ((page->free_mask & mask) != 0U) {
            return false;
        }

        object->~T();
        page->free_mask |= mask;
        if (page->active_count > 0U) {
            page->active_count -= 1U;
        }
        if (active_object_count_ > 0U) {
            active_object_count_ -= 1U;
        }
        page->dead_count += 1U;
        released_object_count_ += 1U;
        PageChain* chain = findChain(page->parent, page->type);
        if (chain != static_cast<PageChain*>(0)) {
            updateChainStats(chain);
        }
        return true;
    }

    bool compact(ObjectType type, void* parent);
    void compactAll();

    std::size_t pageCount() const;
    std::size_t freePageCount() const;
    std::size_t usedPageCount() const;
    std::size_t activeObjectCount() const;
    std::size_t releasedObjectCount() const;
    std::size_t chainCount() const;
    const PageChain* chainAt(std::size_t index) const;
    const PageHeader* pageAt(std::size_t index) const;
    bool shouldCompact() const;

private:
    PageChain* findChain(void* parent, ObjectType type);
    PageChain* findOrCreateChain(void* parent, ObjectType type, std::size_t object_size);
    PageHeader* allocatePage(void* parent, ObjectType type, std::size_t object_size);
    void appendPage(PageChain* chain, PageHeader* page);
    PageHeader* findPageWithFreeSlot(const PageChain* chain) const;
    std::size_t firstFreeSlot(const PageHeader* page) const;
    void* pageSlot(PageHeader* page, std::size_t slot_index) const;
    PageHeader* pageForAddress(const void* object) const;
    std::size_t slotIndexForAddress(const PageHeader* page, const void* object) const;
    void updateChainStats(PageChain* chain);
    void updatePageStats(PageHeader* page);
    void refreshFreePageCount();
    static void* platformAllocatePage();
    static void platformFreePage(void* page);

    PageChain chains_[SystemSettings::MAX_CHAINS];
    PageHeader* pages_[SystemSettings::MAX_PAGES];
    void* page_data_[SystemSettings::MAX_PAGES];
    std::size_t chain_count_;
    std::size_t page_count_;
    std::size_t free_page_count_;
    std::size_t active_object_count_;
    std::size_t released_object_count_;
};

} // namespace core
} // namespace semon

#endif
