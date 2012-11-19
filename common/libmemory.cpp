/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libmemory.cpp
///  Implements the interface to the system virtual memory manager, along with
///  a number of custom memory allocators that can be used for specific memory
///  allocation patterns.
///
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include "libmemory.hpp"
#include "libdlmalloc.hpp"

#if   CMN_IS_WINDOWS
    #include <windows.h>
#elif CMN_IS_APPLE || CMN_IS_LINUX
    #include <unistd.h>
    #include <execinfo.h>
    #include <sys/mman.h>
#else
    #error No implementation of libmemory.cpp for your platform!
#endif

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/
using memory::allocator_t;
using memory::heap_allocator_t;
using memory::page_allocator_t;
using memory::proxy_allocator_t;
using memory::trace_allocator_t;
using memory::allocation_marker_t;
using memory::allocation_tracker_t;
using memory::decrement_allocator_t;
using memory::increment_allocator_t;

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

/// The leading guard page is filled with this value.
static const uint32_t Guard_Code_Head = 0xABAD1DEA;

/// The trailing guard page is filled with this value.
static const uint32_t Guard_Code_Tail = 0xDEADC0DE;

/*/////////////////////////////////////////////////////////////////////////80*/

static void fill(uint32_t *arr, size_t count, uint32_t const &value)
{
    uint32_t *end = arr  + count;
    switch (count & 0x07)
    {
    case 0:
        while (arr != end)
        {
            *arr = value; ++arr;
            /* fall-through */
    case 7:
            *arr = value; ++arr;
            /* fall-through */
    case 6:
            *arr = value; ++arr;
            /* fall-through */
    case 5:
            *arr = value; ++arr;
            /* fall-through */
    case 4:
            *arr = value; ++arr;
            /* fall-through */
    case 3:
            *arr = value; ++arr;
            /* fall-through */
    case 2:
            *arr = value; ++arr;
            /* fall-through */
    case 1:
            *arr = value; ++arr;
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void init_guard(
    void     *page,
    size_t    page_size,
    size_t    alloc_size,
    uint32_t  value)
{
    // the allocation size gets written to the first size_t bytes of the page.
    size_t    fill_size = page_size   - sizeof(size_t);
    assert  ((fill_size % page_size) == 0);

    uint8_t  *base_data = ((uint8_t *) page) + sizeof(size_t);
    uint32_t *page_data =  (uint32_t*) base_data;
    size_t    fill_num  = fill_size / sizeof(uint32_t);

    // the allocation size gets written to the first size_t bytes of the page.
    *((size_t*)  page)  = alloc_size;

    // the rest of the page is filled with the guard pattern.
    ::fill(page_data, fill_num, value);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool check_guard(void *page, size_t page_size, uint32_t value)
{
    // the allocation size gets written to the first size_t bytes of the page.
    uint8_t  *tail_data = ((uint8_t *) page) + page_size;
    uint8_t  *base_data = ((uint8_t *) page) + sizeof(size_t);
    uint32_t *page_ptr  =  (uint32_t*) base_data;
    uint32_t *page_end  =  (uint32_t*) tail_data;
    while  (page_ptr != page_end && *page_ptr++ == value)
    {
        /* empty */
    }
    return (page_ptr == page_end);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void trace_insert(
    trace_allocator_t *alloc,
    void              *node_mem,
    void              *user_mem,
    size_t             requested_size)
{
    trace_allocator_t::trace_node_t *node = (trace_allocator_t::trace_node_t*) node_mem;

    // initialize the node fields.
    node->address         = user_mem;
    node->next            = NULL;
    node->prev            = NULL;
    node->requested_size  = requested_size;
    memory::capture_callstack(node->stack_frames, trace_allocator_t::MAX_STACK_FRAMES);

    // we want to insert 'node' at the end of the list.
    if (alloc->list_size  > 0)
    {
        // inserting into a non-empty list.
        node->next             = NULL;
        node->prev             = alloc->list_tail;
        alloc->list_tail->next = node;
        alloc->list_tail       = node;
        alloc->list_size      += 1;
    }
    else
    {
        // inserting into an empty list.
        node->next             = NULL;
        node->prev             = NULL;
        alloc->list_head       = node;
        alloc->list_tail       = node;
        alloc->list_size       = 1;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void trace_remove(trace_allocator_t *alloc, void *address)
{
    trace_allocator_t::trace_node_t *iter = alloc->list_head;
    while (iter != NULL)
    {
        if (iter->address != address)
        {
            iter = iter->next;
            continue;
        }
        // found the node we're looking for; remove it.
        if (alloc->list_size  > 1)
        {
            // the list will be non-empty after removal.
            if (iter != alloc->list_head && iter != alloc->list_tail)
            {
                // iter is somewhere in the middle of the list.
                iter->next->prev   = iter->prev;
                iter->prev->next   = iter->next;
                alloc->list_size  -= 1;
            }
            else if (iter == alloc->list_head)
            {
                // iter is the head of the list.
                iter->next->prev   = NULL;
                alloc->list_head   = iter->next;
                alloc->list_size  -= 1;
            }
            else if (iter == alloc->list_tail)
            {
                // iter is the tail of the list.
                iter->prev->next  = NULL;
                alloc->list_tail  = iter->prev;
                alloc->list_size -= 1;
            }
        }
        else
        {
            // the list will be empty after removal.
            alloc->list_head  = NULL;
            alloc->list_tail  = NULL;
            alloc->list_size  = 0;
        }
        break;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

allocator_t::allocator_t(void)
    :
    tracker(NULL),
    name(NULL),
    total_allocation_count(0),
    total_allocation_size(0),
    watermark_allocation_count(0),
    watermark_allocation_size(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

allocator_t::allocator_t(char const *allocator_name)
    :
    tracker(NULL),
    name(allocator_name),
    total_allocation_count(0),
    total_allocation_size(0),
    watermark_allocation_count(0),
    watermark_allocation_size(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

allocator_t::~allocator_t(void)
{
    assert_no_leaks();
}

/*/////////////////////////////////////////////////////////////////////////80*/

page_allocator_t::page_allocator_t(void)
    :
    page_size(0),
    check_on_free(false)
{
    page_size = memory::vmm_page_size();
}

/*/////////////////////////////////////////////////////////////////////////80*/

page_allocator_t::page_allocator_t(char const *allocator_name)
    :
    allocator_t(allocator_name),
    page_size(0),
    check_on_free(false)
{
    page_size = memory::vmm_page_size();
}

/*/////////////////////////////////////////////////////////////////////////80*/

page_allocator_t::~page_allocator_t(void)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool page_allocator_t::check_guard(void *ptr)
{
    if (ptr != NULL)
    {
        size_t   round_size  = *((size_t  *)(((uint8_t*) ptr) - page_size));
        uint8_t *head        = ((uint8_t*)  ptr) - page_size;
        uint8_t *tail        = ((uint8_t*)  ptr) + round_size;
        bool     guard_head  = ::check_guard(head, page_size, Guard_Code_Head);
        bool     guard_tail  = ::check_guard(tail, page_size, Guard_Code_Tail);
        return  (guard_head && guard_tail);
    }
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t page_allocator_t::allocation_size(void *ptr)
{
    return (ptr != NULL) ? *((size_t*)(((uint8_t*) ptr) - page_size)) : 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* page_allocator_t::allocate(size_t size_in_bytes, size_t alignment)
{
    CMN_UNUSED(alignment);
    size_t  round_size   = memory::align_up(size_in_bytes, page_size);
    size_t  guard_size   = page_size  * 2; // for guard pages.
    size_t  total_size   = guard_size + round_size;
    void   *raw_ptr      = memory::vmm_reserve(total_size);
    if (raw_ptr != NULL && memory::vmm_commit (raw_ptr, total_size))
    {
        uint8_t *head    = (uint8_t*) raw_ptr;
        uint8_t *user    =((uint8_t*) raw_ptr)  + page_size;
        uint8_t *tail    =((uint8_t*) raw_ptr)  + page_size + round_size;
        ::init_guard(head, page_size, round_size, Guard_Code_Head);
        ::init_guard(tail, page_size, round_size, Guard_Code_Tail);
        report_allocation(user, size_in_bytes,    total_size);
        return user;
    }
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void page_allocator_t::deallocate(void *ptr)
{
    if (ptr != NULL)
    {
        size_t   round_size = *((size_t  *)(((uint8_t*) ptr) - page_size));
        size_t   guard_size = page_size  * 2;
        size_t   total_size = guard_size + round_size;
        uint8_t *head       = ((uint8_t*)  ptr) - page_size;
        uint8_t *tail       = ((uint8_t*)  ptr) + round_size;
        if (check_on_free)
        {
            bool guard_head = ::check_guard(head, page_size, Guard_Code_Head);
            bool guard_tail = ::check_guard(tail, page_size, Guard_Code_Tail);
            CMN_UNUSED(guard_head);
            CMN_UNUSED(guard_tail);
            assert(guard_head && guard_tail && "Memory overwrite detected!");
        }
        report_deallocation(head, total_size);
        memory::vmm_release(head, total_size);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void page_allocator_t::reset(void)
{
    // @todo: we can't reset this type of allocator without explicitly
    // tracking all page allocations. we would need some buffer of pages
    // to do this. is it worth the effort in this case? probably so; we
    // could allocate say 8 pages which gives us 32KB of tracking space,
    // which is 8192 allocations on a 32-bit system or 4096 on 64-bit.
    // that should be more than enough for this type of allocator, which
    // is only used for very large block allocations.
    assert_no_leaks();
    reset_allocation_counts();
    reset_allocation_watermarks();
}

/*/////////////////////////////////////////////////////////////////////////80*/

heap_allocator_t::heap_allocator_t(void)
    :
    memory_space(NULL),
    memory_block(NULL),
    memory_size(0),
    memory_over(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

heap_allocator_t::heap_allocator_t(char const *allocator_name)
    :
    allocator_t(allocator_name),
    memory_space(NULL),
    memory_block(NULL),
    memory_size(0),
    memory_over(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

heap_allocator_t::heap_allocator_t(void *mem_block, size_t mem_size)
    :
    memory_space(NULL),
    memory_block(NULL),
    memory_size(0),
    memory_over(0)
{
    bool bind_result    = bind(mem_block, mem_size);
    assert(bind_result != false); // assert on failure in debug builds.
    CMN_UNUSED(bind_result);
}

/*/////////////////////////////////////////////////////////////////////////80*/

heap_allocator_t::~heap_allocator_t(void)
{
    unbind();
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool heap_allocator_t::bind(void *mem_block, size_t mem_size)
{
    assert(mem_block    != NULL);
    assert(memory_space == NULL);
    assert(mem_size      > 128 * sizeof(size_t)); // minimum mspace size

    // initialze a new mspace in the provided block:
    mspace space  = create_mspace_with_base(mem_block, mem_size, 0);
    if    (space != NULL)
    {
        total_allocation_size  = 0;
        total_allocation_count = 0;
        memory_space           = space;
        memory_block           = mem_block;
        memory_size            = mem_size;
        memory_over            = 0;
        reset_allocation_counts();
        reset_allocation_watermarks();
        return true;
    }
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* heap_allocator_t::unbind(size_t *out_mem_size /* = NULL */)
{
    if (out_mem_size != NULL) *out_mem_size = memory_size;
    void *mem_block   = memory_block;
    if   (mem_block)
    {
        size_t nfreed = destroy_mspace(memory_space);
        memory_space  = NULL;
        memory_block  = NULL;
        memory_size   = 0;
        memory_over   = 0;
        CMN_UNUSED(nfreed);
        reset_allocation_counts();
        reset_allocation_watermarks();
    }
    return mem_block;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t heap_allocator_t::allocation_size(void *ptr)
{
    return mspace_usable_size(ptr);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* heap_allocator_t::allocate(size_t size_in_bytes, size_t alignment)
{
    void  *ptr = mspace_memalign(memory_space, alignment, size_in_bytes);
    size_t msz = mspace_usable_size(ptr);
    if    (ptr != NULL && msz > 0) report_allocation(ptr, size_in_bytes, msz);
    return ptr;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void heap_allocator_t::deallocate(void *ptr)
{
    size_t msz = mspace_usable_size(ptr);
    mspace_free(memory_space, ptr);
    if (ptr != NULL && msz > 0) report_deallocation(ptr, msz);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void heap_allocator_t::reset(void)
{
    if (memory_space != NULL)
    {
        mspace mem_space  = NULL;
        void  *mem_block  = memory_block;
        size_t mem_size   = memory_size;
        size_t nfreed     = destroy_mspace(memory_space);
        mem_space         = create_mspace_with_base(mem_block, mem_size, 0);
        assert(mem_space != NULL);
        // reset our internal members.
        memory_space      = mem_space;
        memory_over       = 0;
        reset_allocation_counts();
        reset_allocation_watermarks();
        CMN_UNUSED(nfreed);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

decrement_allocator_t::decrement_allocator_t(void)
    :
    memory_block(NULL),
    memory_size(0),
    memory_offset(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

decrement_allocator_t::decrement_allocator_t(char const *allocator_name)
    :
    allocator_t(allocator_name),
    memory_block(NULL),
    memory_size(0),
    memory_offset(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

decrement_allocator_t::decrement_allocator_t(void *block, size_t size_in_bytes)
    :
    memory_block(NULL),
    memory_size(0),
    memory_offset(0)
{
    bool bind_result    = bind(block, size_in_bytes);
    assert(bind_result != false); // assert on failure in debug builds.
    CMN_UNUSED(bind_result);
}

/*/////////////////////////////////////////////////////////////////////////80*/

decrement_allocator_t::~decrement_allocator_t(void)
{
    unbind();
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool decrement_allocator_t::bind(void *block, size_t size_in_bytes)
{
    assert(NULL  == memory_block && 0 == memory_size);
    assert(NULL  !=        block && 0 != size_in_bytes);
    total_allocation_size         = 0;
    total_allocation_count        = 0;
    watermark_allocation_count    = 0;
    watermark_allocation_size     = 0;
    memory_block                  = (uint8_t*) block;
    memory_size                   = size_in_bytes;
    memory_offset                 = size_in_bytes;
    reset_allocation_counts();
    reset_allocation_watermarks();
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* decrement_allocator_t::unbind(size_t *out_size_in_bytes /* = NULL */)
{
    if (out_size_in_bytes != NULL) *out_size_in_bytes = memory_size;
    void  *block  = memory_block;
    memory_block  = NULL;
    memory_size   = 0;
    memory_offset = 0;
    reset_allocation_counts();
    reset_allocation_watermarks();
    return block;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void decrement_allocator_t::marker(allocation_marker_t *out_marker)
{
    out_marker->total_allocation_count = total_allocation_count;
    out_marker->total_allocation_size  = total_allocation_size;
    out_marker->marker_value           = memory_offset;
    out_marker->marker_frame           = 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void decrement_allocator_t::reset_to_marker(allocation_marker_t *mark)
{
    assert(mark->marker_value >= memory_offset);
    assert(mark->marker_value <= memory_size);
    total_allocation_count     = mark->total_allocation_count;
    total_allocation_size      = mark->total_allocation_size;
    memory_offset              = mark->marker_value;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t decrement_allocator_t::allocation_size(void *ptr)
{
    return (ptr != NULL) ? *((size_t*)(((uint8_t*) ptr) - sizeof(size_t))) : 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* decrement_allocator_t::allocate(size_t size_in_bytes, size_t alignment)
{
    size_t    total_size = size_in_bytes + sizeof(size_t);
    size_t    alloc_size = memory::align_up(total_size, alignment);
    ptrdiff_t new_offset = ((ptrdiff_t) memory_offset)  - alloc_size;
    if (new_offset      >= 0)
    {
        uint8_t *mem_ptr = memory_block + new_offset;
        uint8_t *aln_ptr = memory::align_to(mem_ptr, alignment);
        uint8_t *asz_ptr = aln_ptr  - sizeof(alloc_size);
       *(size_t*)asz_ptr = alloc_size;
        memory_offset    = new_offset;
        report_allocation(aln_ptr, size_in_bytes, alloc_size);
        return  aln_ptr;
    }
    else return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void decrement_allocator_t::deallocate(void *ptr)
{
    if (ptr != NULL)
    {
        size_t size = *((size_t*)(((uint8_t*) ptr) - sizeof(size_t)));
        report_deallocation(ptr, size);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void decrement_allocator_t::reset(void)
{
    if (memory_size   > 0)
    {
        memory_offset = memory_size;
        reset_allocation_counts();
        reset_allocation_watermarks();
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

increment_allocator_t::increment_allocator_t(void)
    :
    memory_block(NULL),
    memory_size(0),
    memory_offset(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

increment_allocator_t::increment_allocator_t(char const *allocator_name)
    :
    allocator_t(allocator_name),
    memory_block(NULL),
    memory_size(0),
    memory_offset(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

increment_allocator_t::increment_allocator_t(void *block, size_t size_in_bytes)
    :
    memory_block(NULL),
    memory_size(0),
    memory_offset(0)
{
    bool bind_result    = bind(block, size_in_bytes);
    assert(bind_result != false); // assert on failure in debug builds.
    CMN_UNUSED(bind_result);
}

/*/////////////////////////////////////////////////////////////////////////80*/

increment_allocator_t::~increment_allocator_t(void)
{
    unbind();
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool increment_allocator_t::bind(void *block, size_t size_in_bytes)
{
    assert(NULL  == memory_block && 0 == memory_size);
    assert(NULL  !=        block && 0 != size_in_bytes);
    total_allocation_size  = 0;
    total_allocation_count = 0;
    memory_block           = (uint8_t*) block;
    memory_size            = size_in_bytes;
    memory_offset          = 0;
    reset_allocation_counts();
    reset_allocation_watermarks();
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* increment_allocator_t::unbind(size_t *out_size_in_bytes /* = NULL */)
{
    if (out_size_in_bytes != NULL) *out_size_in_bytes = memory_size;
    void  *block  = memory_block;
    memory_block  = NULL;
    memory_size   = 0;
    memory_offset = 0;
    reset_allocation_counts();
    reset_allocation_watermarks();
    return block;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void increment_allocator_t::marker(allocation_marker_t *out_marker)
{
    out_marker->total_allocation_count = total_allocation_count;
    out_marker->total_allocation_size  = total_allocation_size;
    out_marker->marker_value           = memory_offset;
    out_marker->marker_frame           = 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void increment_allocator_t::reset_to_marker(allocation_marker_t *mark)
{
    assert(mark->marker_value <= memory_offset);
    assert(mark->marker_value <= memory_size);
    total_allocation_count     = mark->total_allocation_count;
    total_allocation_size      = mark->total_allocation_size;
    memory_offset              = mark->marker_value;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t increment_allocator_t::allocation_size(void *ptr)
{
    return ptr != NULL ? *((size_t*)(((uint8_t*) ptr) - sizeof(size_t))) : 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* increment_allocator_t::allocate(size_t size_in_bytes, size_t alignment)
{
    size_t   total_size   = size_in_bytes + sizeof(size_t);
    size_t   alloc_size   = memory::align_up(total_size, alignment);
    if ((memory_offset    + alloc_size) <= memory_size)
    {
        uint8_t *mem_ptr  = memory_block + memory_offset + sizeof(alloc_size);
        uint8_t *aln_ptr  = memory::align_to(mem_ptr, alignment);
        uint8_t *asz_ptr  = aln_ptr  - sizeof(alloc_size);
       *(size_t*)asz_ptr  = alloc_size;
        memory_offset    += alloc_size;
        report_allocation(aln_ptr, size_in_bytes, alloc_size);
        return  aln_ptr;
    }
    else return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void increment_allocator_t::deallocate(void *ptr)
{
    if (ptr != NULL)
    {
        size_t size = *((size_t*)(((uint8_t*) ptr) - sizeof(size_t)));
        report_deallocation(ptr, size);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void increment_allocator_t::reset(void)
{
    memory_offset = 0;
    reset_allocation_counts();
    reset_allocation_watermarks();
}

/*/////////////////////////////////////////////////////////////////////////80*/

proxy_allocator_t::proxy_allocator_t(void)
    :
    base_allocator(NULL)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

proxy_allocator_t::proxy_allocator_t(char const *allocator_name)
    :
    allocator_t(allocator_name),
    base_allocator(NULL)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

proxy_allocator_t::proxy_allocator_t(allocator_t *base_alloc)
    :
    base_allocator(base_alloc)
{
    assert(base_alloc != NULL);
}

/*/////////////////////////////////////////////////////////////////////////80*/

proxy_allocator_t::~proxy_allocator_t()
{
    base_allocator = NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void proxy_allocator_t::bind(allocator_t *base_alloc)
{
    assert(base_alloc != NULL);
    base_allocator = base_alloc;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void proxy_allocator_t::unbind(void)
{
    base_allocator = NULL;
    reset_allocation_counts();
    reset_allocation_watermarks();
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t proxy_allocator_t::allocation_size(void *ptr)
{
    return base_allocator->allocation_size(ptr);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* proxy_allocator_t::allocate(size_t size_in_bytes, size_t alignment)
{
    void *ptr = base_allocator->allocate(size_in_bytes, alignment);
    if (ptr  != NULL)
    {
        size_t alloc_size = base_allocator->allocation_size(ptr);
        report_allocation(ptr, size_in_bytes, alloc_size);
    }
    return ptr;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void proxy_allocator_t::deallocate(void *ptr)
{
    size_t alloc_size = base_allocator->allocation_size(ptr);
    if (alloc_size    > 0) report_deallocation(ptr, alloc_size);
    base_allocator->deallocate(ptr);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void proxy_allocator_t::reset(void)
{
    base_allocator->reset();
    reset_allocation_counts();
    reset_allocation_watermarks();
}

/*/////////////////////////////////////////////////////////////////////////80*/

trace_allocator_t::trace_allocator_t(void)
    :
    base_allocator(NULL),
    list_head(NULL),
    list_tail(NULL),
    list_size(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

trace_allocator_t::trace_allocator_t(char const *allocator_name)
    :
    allocator_t(allocator_name),
    base_allocator(NULL),
    list_head(NULL),
    list_tail(NULL),
    list_size(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

trace_allocator_t::trace_allocator_t(allocator_t *base_alloc)
    :
    base_allocator(base_alloc),
    list_head(NULL),
    list_tail(NULL),
    list_size(0)
{
    assert(base_alloc != NULL);
}

/*/////////////////////////////////////////////////////////////////////////80*/

trace_allocator_t::~trace_allocator_t()
{
    base_allocator = NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void trace_allocator_t::bind(allocator_t *base_alloc)
{
    assert(base_alloc != NULL);
    base_allocator = base_alloc;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void trace_allocator_t::unbind(void)
{
    base_allocator = NULL;
    reset_allocation_counts();
    reset_allocation_watermarks();
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t trace_allocator_t::allocation_size(void *ptr)
{
    return base_allocator->allocation_size(ptr);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* trace_allocator_t::allocate(size_t size_in_bytes, size_t alignment)
{
    // allocate additional memory for our tracking data.
    size_t req = size_in_bytes + sizeof(trace_allocator_t::trace_node_t);
    void  *ptr = base_allocator->allocate(req, alignment);
    if (ptr != NULL)
    {
        size_t   alloc_size = base_allocator->allocation_size(ptr);
        uint8_t *trace_node =((uint8_t*) ptr) + size_in_bytes;
        trace_insert(this, trace_node, ptr, size_in_bytes);
        report_allocation (ptr, size_in_bytes, alloc_size);
    }
    return ptr;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void trace_allocator_t::deallocate(void *ptr)
{
    size_t alloc_size = base_allocator->allocation_size(ptr);
    trace_remove(this, ptr);
    if (alloc_size > 0) report_deallocation(ptr, alloc_size);
    base_allocator->deallocate(ptr);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void trace_allocator_t::reset(void)
{
    base_allocator->reset();
    list_head = NULL;
    list_tail = NULL;
    list_size = 0;
    reset_allocation_counts();
    reset_allocation_watermarks();
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t memory::capture_callstack(void **frames, size_t max_frames)
{
#if   CMN_IS_WINDOWS
    size_t num_frames = CaptureStackBackTrace(0, max_frames, frames, NULL);
    frames[0]         = (void*) num_frames;
    return num_frames;
#elif CMN_IS_APPLE || CMN_IS_LINUX
    size_t num_frames = (size_t) backtrace(frames, (int) max_frames);
    frames[0]         = (void*)  num_frames;
    return num_frames;
#else
    #error No implementation of memory::capture_callstack() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t memory::vmm_page_size(void)
{
#if   CMN_IS_WINDOWS
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return (size_t) sys_info.dwPageSize;
#elif CMN_IS_APPLE || CMN_IS_LINUX
    return (size_t) getpagesize();
#else
    #error No implementation of memory::vmm_page_size() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* memory::vmm_reserve(size_t size_in_bytes)
{
#if   CMN_IS_WINDOWS
    return VirtualAlloc(NULL, size_in_bytes, MEM_RESERVE, PAGE_NOACCESS);
#elif CMN_IS_APPLE || CMN_IS_LINUX
    int    flags       = MAP_PRIVATE | MAP_ANON;
    void  *map_result  = mmap(NULL, size_in_bytes, PROT_NONE, flags, -1, 0);
    return map_result != MAP_FAILED ? map_result : NULL;
#else
    #error No implementation of memory::vmm_reserve() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool memory::vmm_commit(void *addr_start, size_t size_in_bytes)
{
#if   CMN_IS_WINDOWS
    void *ptr = VirtualAlloc(
        addr_start,
        size_in_bytes,
        MEM_COMMIT,
        PAGE_READWRITE);
    return (ptr != NULL);
#elif CMN_IS_APPLE || CMN_IS_LINUX
    return (0 == mprotect(addr_start, size_in_bytes, PROT_READ | PROT_WRITE));
#else
    #error No implementation of memory::vmm_commit() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool memory::vmm_decommit(void *addr_start, size_t size_in_bytes)
{
#if   CMN_IS_WINDOWS
    return (VirtualFree(addr_start, size_in_bytes, MEM_DECOMMIT) != 0);
#elif CMN_IS_APPLE || CMN_IS_LINUX
    // @note: this is OS X 10.6+ specific. older versions of OS X and Linux
    // would use the following snippet, at significant performance cost:
    // int flags = MAP_FIXED | MAP_PRIVATE | MAP_ANON;
    // mmap(addr_start, total_size, PROT_NONE, flags, -1, 0)
    // @todo: should mprotect(addr_start, size_in_bytes, PROT_NONE) after this?
    return (0 == madvise (addr_start, size_in_bytes, MADV_FREE));
#else
    #error No implementation of memory::vmm_decommit() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void memory::vmm_release(void *addr_start, size_t size_in_bytes)
{
#if  CMN_IS_WINDOWS
    CMN_UNUSED(size_in_bytes);
    VirtualFree(addr_start, 0, MEM_RELEASE);
#elif CMN_IS_APPLE || CMN_IS_LINUX
    munmap(addr_start, size_in_bytes);
#else
    #error No implementation of memory::vmm_release() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
