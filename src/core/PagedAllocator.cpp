#include "core/PagedAllocator.hpp"

#include <cstring>
#include <new>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace semon {
namespace core {

namespace {
constexpr std::uint32_t PAGE_MAGIC = 0x53504147U;

std::size_t nativePageSize()
{
    return SystemSettings::NATIVE_PAGE_SIZE;
}
}

PagedAllocator::PagedAllocator()
    : chains_(),
      pages_(),
      page_data_(),
      chain_count_(0U),
      page_count_(0U),
      free_page_count_(0U),
      active_object_count_(0U),
      released_object_count_(0U)
{
}

bool PagedAllocator::initialize(std::size_t requested_page_count)
{
    if (requested_page_count == 0U || requested_page_count > SystemSettings::MAX_PAGES) {
        return false;
    }

    shutdown();
    page_count_ = requested_page_count;
    free_page_count_ = requested_page_count;

    for (std::size_t index = 0U; index < SystemSettings::MAX_CHAINS; ++index) {
        chains_[index].parent = static_cast<void*>(0);
        chains_[index].type = ObjectType::METADATA;
        chains_[index].head = static_cast<PageHeader*>(0);
        chains_[index].tail = static_cast<PageHeader*>(0);
        chains_[index].page_count = 0U;
        chains_[index].total_objects = 0U;
        chains_[index].active_objects = 0U;
        chains_[index].free_objects = 0U;
    }

    return true;
}

void PagedAllocator::shutdown()
{
    const std::size_t used_pages = usedPageCount();
    for (std::size_t index = 0U; index < used_pages; ++index) {
        if (page_data_[index] != static_cast<void*>(0)) {
            platformFreePage(page_data_[index]);
            page_data_[index] = static_cast<void*>(0);
        }
        pages_[index] = static_cast<PageHeader*>(0);
    }

    chain_count_ = 0U;
    page_count_ = 0U;
    free_page_count_ = 0U;
    active_object_count_ = 0U;
    released_object_count_ = 0U;
}

bool PagedAllocator::compact(ObjectType type, void* parent)
{
    PageChain* chain = findChain(parent, type);
    if (chain == static_cast<PageChain*>(0)) {
        return true;
    }

    for (PageHeader* page = chain->head; page != static_cast<PageHeader*>(0); page = page->next) {
        std::size_t write_slot = 0U;
        for (std::size_t read_slot = 0U; read_slot < page->object_count; ++read_slot) {
            const std::uint64_t mask = UINT64_C(1) << read_slot;
            if ((page->free_mask & mask) != 0U) {
                continue;
            }

            void* source = pageSlot(page, read_slot);
            if (write_slot != read_slot) {
                void* destination = pageSlot(page, write_slot);
                std::memmove(destination, source, page->object_size);
                page->free_mask |= mask;
                page->free_mask &= ~(UINT64_C(1) << write_slot);
            }
            write_slot += 1U;
        }
        updatePageStats(page);
    }

    updateChainStats(chain);
    return true;
}

void PagedAllocator::compactAll()
{
    for (std::size_t index = 0U; index < chain_count_; ++index) {
        (void)compact(chains_[index].type, chains_[index].parent);
    }
}

std::size_t PagedAllocator::pageCount() const
{
    return page_count_;
}

std::size_t PagedAllocator::freePageCount() const
{
    return free_page_count_;
}

std::size_t PagedAllocator::usedPageCount() const
{
    return page_count_ - free_page_count_;
}

std::size_t PagedAllocator::activeObjectCount() const
{
    return active_object_count_;
}

std::size_t PagedAllocator::releasedObjectCount() const
{
    return released_object_count_;
}

std::size_t PagedAllocator::chainCount() const
{
    return chain_count_;
}

const PageChain* PagedAllocator::chainAt(std::size_t index) const
{
    if (index >= chain_count_) {
        return static_cast<const PageChain*>(0);
    }
    return &chains_[index];
}

const PageHeader* PagedAllocator::pageAt(std::size_t index) const
{
    if (index >= usedPageCount()) {
        return static_cast<const PageHeader*>(0);
    }
    return pages_[index];
}

bool PagedAllocator::shouldCompact() const
{
    const std::size_t used = usedPageCount();
    const std::size_t free_pages = freePageCount();
    if (used == 0U) {
        return false;
    }

    const std::size_t scarcity_threshold = (used + SystemSettings::DEFRAGMENTATION_DIVISOR - 1U) /
                                           SystemSettings::DEFRAGMENTATION_DIVISOR;
    if (free_pages < scarcity_threshold) {
        return true;
    }

    return free_pages >= scarcity_threshold;
}

PageChain* PagedAllocator::findChain(void* parent, ObjectType type)
{
    for (std::size_t index = 0U; index < chain_count_; ++index) {
        if (chains_[index].parent == parent && chains_[index].type == type) {
            return &chains_[index];
        }
    }
    return static_cast<PageChain*>(0);
}

PageChain* PagedAllocator::findOrCreateChain(void* parent, ObjectType type, std::size_t object_size)
{
    if (object_size == 0U || object_size > nativePageSize() - sizeof(PageHeader)) {
        diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::ALLOCATOR,
                         "PagedAllocator::findOrCreateChain",
                         static_cast<void*>(0),
                         object_size,
                         "invalid object size");
        return static_cast<PageChain*>(0);
    }

    PageChain* existing = findChain(parent, type);
    if (existing != static_cast<PageChain*>(0)) {
        return existing;
    }

    if (chain_count_ >= SystemSettings::MAX_CHAINS) {
        diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::ALLOCATOR,
                         "PagedAllocator::findOrCreateChain",
                         static_cast<void*>(0),
                         chain_count_,
                         "MAX_CHAINS");
        return static_cast<PageChain*>(0);
    }

    PageChain* chain = &chains_[chain_count_];
    chain_count_ += 1U;
    chain->parent = parent;
    chain->type = type;
    chain->head = static_cast<PageHeader*>(0);
    chain->tail = static_cast<PageHeader*>(0);
    chain->page_count = 0U;
    chain->total_objects = 0U;
    chain->active_objects = 0U;
    chain->free_objects = 0U;
    return chain;
}

PageHeader* PagedAllocator::allocatePage(void* parent, ObjectType type, std::size_t object_size)
{
    if (free_page_count_ == 0U) {
        diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::ALLOCATOR,
                         "PagedAllocator::allocatePage",
                         static_cast<void*>(0),
                         free_page_count_,
                         "no native pages");
        return static_cast<PageHeader*>(0);
    }

    void* raw_page = platformAllocatePage();
    if (raw_page == static_cast<void*>(0)) {
        diagnostics::Diagnostics::set(diagnostics::DiagnosticCode::ALLOCATOR,
                         "PagedAllocator::allocatePage",
                         static_cast<void*>(0),
                         0U,
                         "platform allocation failed");
        return static_cast<PageHeader*>(0);
    }

    std::memset(raw_page, 0, nativePageSize());
    const std::size_t used_index = usedPageCount();
    PageHeader* header = reinterpret_cast<PageHeader*>(raw_page);
    const std::size_t usable_bytes = nativePageSize() - sizeof(PageHeader);
    std::size_t object_count = usable_bytes / object_size;
    if (object_count > 64U) {
        object_count = 64U;
    }

    header->prev = static_cast<PageHeader*>(0);
    header->next = static_cast<PageHeader*>(0);
    header->parent = parent;
    header->type = type;
    header->object_size = object_size;
    header->object_count = object_count;
    header->active_count = 0U;
    header->dead_count = 0U;
    header->bytes_allocated = 0U;
    header->bytes_unused = usable_bytes;
    header->free_mask = 0ULL;
    for (std::size_t index = 0U; index < object_count; ++index) {
        header->free_mask |= (UINT64_C(1) << index);
    }
    header->magic = PAGE_MAGIC;

    pages_[used_index] = header;
    page_data_[used_index] = raw_page;
    free_page_count_ -= 1U;
    updatePageStats(header);
    return header;
}

void PagedAllocator::appendPage(PageChain* chain, PageHeader* page)
{
    if (chain == static_cast<PageChain*>(0) || page == static_cast<PageHeader*>(0)) {
        return;
    }

    page->prev = chain->tail;
    page->next = static_cast<PageHeader*>(0);
    if (chain->tail != static_cast<PageHeader*>(0)) {
        chain->tail->next = page;
    } else {
        chain->head = page;
    }
    chain->tail = page;
    chain->page_count += 1U;
    updateChainStats(chain);
}

PageHeader* PagedAllocator::findPageWithFreeSlot(const PageChain* chain) const
{
    if (chain == static_cast<const PageChain*>(0)) {
        return static_cast<PageHeader*>(0);
    }

    for (PageHeader* page = chain->head; page != static_cast<PageHeader*>(0); page = page->next) {
        if (page->free_mask != 0ULL) {
            return page;
        }
    }
    return static_cast<PageHeader*>(0);
}

std::size_t PagedAllocator::firstFreeSlot(const PageHeader* page) const
{
    for (std::size_t index = 0U; index < page->object_count; ++index) {
        if ((page->free_mask & (UINT64_C(1) << index)) != 0ULL) {
            return index;
        }
    }
    return page->object_count;
}

void* PagedAllocator::pageSlot(PageHeader* page, std::size_t slot_index) const
{
    char* data = reinterpret_cast<char*>(page);
    return data + sizeof(PageHeader) + (slot_index * page->object_size);
}

PageHeader* PagedAllocator::pageForAddress(const void* object) const
{
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(object);
    const std::size_t used_pages = usedPageCount();
    for (std::size_t page_index = 0U; page_index < used_pages; ++page_index) {
        PageHeader* page = pages_[page_index];
        const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(page) + sizeof(PageHeader);
        const std::uintptr_t end = reinterpret_cast<std::uintptr_t>(page) + nativePageSize();
        if (address >= begin && address < end) {
            return page;
        }
    }
    return static_cast<PageHeader*>(0);
}

std::size_t PagedAllocator::slotIndexForAddress(const PageHeader* page, const void* object) const
{
    const std::uintptr_t object_address = reinterpret_cast<std::uintptr_t>(object);
    const std::uintptr_t data_address = reinterpret_cast<std::uintptr_t>(page) + sizeof(PageHeader);
    return (object_address - data_address) / page->object_size;
}

void PagedAllocator::updateChainStats(PageChain* chain)
{
    if (chain == static_cast<PageChain*>(0)) {
        return;
    }

    chain->total_objects = 0U;
    chain->active_objects = 0U;
    chain->free_objects = 0U;
    for (PageHeader* page = chain->head; page != static_cast<PageHeader*>(0); page = page->next) {
        chain->total_objects += page->object_count;
        chain->active_objects += page->active_count;
        chain->free_objects += page->object_count - page->active_count - page->dead_count;
    }
}

void PagedAllocator::updatePageStats(PageHeader* page)
{
    if (page == static_cast<PageHeader*>(0)) {
        return;
    }

    std::size_t free_slots = 0U;
    for (std::size_t index = 0U; index < page->object_count; ++index) {
        if ((page->free_mask & (UINT64_C(1) << index)) != 0ULL) {
            free_slots += 1U;
        }
    }

    page->bytes_allocated = page->active_count * page->object_size;
    page->bytes_unused = (free_slots * page->object_size) +
                         (nativePageSize() - sizeof(PageHeader) - (page->object_count * page->object_size));
}

void* PagedAllocator::platformAllocatePage()
{
#if defined(_WIN32)
    return VirtualAlloc(static_cast<LPVOID>(0), nativePageSize(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    void* page = mmap(static_cast<void*>(0),
                      nativePageSize(),
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1,
                      0);
    if (page == MAP_FAILED) {
        return static_cast<void*>(0);
    }
    return page;
#endif
}

void PagedAllocator::platformFreePage(void* page)
{
#if defined(_WIN32)
    (void)VirtualFree(page, 0U, MEM_RELEASE);
#else
    (void)munmap(page, nativePageSize());
#endif
}

} // namespace core
} // namespace semon
