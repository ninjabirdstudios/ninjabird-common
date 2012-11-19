/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines an interface to the system virtual memory manager, along
/// with a number of custom memory allocator implementations that can be used
/// for specific memory allocation pattersn. Unlike most core libraries, this
/// library has a default dependency on libdlmalloc (Doug Lea's memory allocator
/// implementation, which is used as the default heap implementation because it
/// is cross-platform and behaves consistently and well across platforms.)
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/
#ifndef LIBMEMORY_HPP_INCLUDED
#define LIBMEMORY_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <new>
#include <cassert>
#include "common.hpp"
#include "common_traits.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace memory {

/*//////////////
//   Macros   //
//////////////*/

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/
class allocator_t;

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// Represents a snapshot of a memory allocator at a specific point in time.
/// This information is only reported by some types of allocators, which allow
/// an appplication to request a marker and then free back to a specific point.
struct allocation_marker_t
{
    size_t     total_allocation_count; /// Snapshot of total_allocation_count
    size_t     total_allocation_size;  /// Snapshot of total_allocation_size
    size_t     marker_value;           /// Allocator-specific value
    size_t     marker_frame;           /// Allocator-specific value
};

/// Defines the interface to an object that can capture and report stack trace
/// and allocation size information for a given memory allocator. Typically,
/// there would be one instance of an allocation_tracker_t implementation for
/// each thread.
class CMN_PUBLIC allocation_tracker_t
{
public:
    /// Default constructor. For the base class, this is a no-op.
    allocation_tracker_t(void);

    /// Type destructor. For the base class, this is a no-op.
    virtual ~allocation_tracker_t(void);

public:
    /// Reports a memory allocation through a given allocator instance.
    ///
    /// @param allocator The memory allocator reporting the event.
    /// @param address The address of the pointer returned to the caller.
    /// @param requested_size The number of bytes requested by the caller.
    /// @param allocated_size The number of bytes actually allocated.
    virtual void report_allocation(
        allocator_t *allocator,
        void        *address,
        size_t       requested_size,
        size_t       allocated_size);

    /// Reports memory being released back to the system through a given
    /// allocator instance.
    ///
    /// @param allocator The memory allocator reporting the event.
    /// @param address The address of the pointer returned to the caller.
    /// @param allocated_size The size of the allocation, in bytes.
    virtual void report_deallocation(
        allocator_t *allocator,
        void        *address,
        size_t       allocated_size);

    /// Reports a new high watermark being reached for a given allocator.
    ///
    /// @param allocator The memory allocator reporting the event.
    /// @param allocation_count The number of allocations made through the
    /// memory allocator @a allocator.
    /// @param total_size The new high watermark value for @a allocator
    /// specified in bytes.
    virtual void report_watermark(
        allocator_t *allocator,
        size_t       allocation_count,
        size_t       total_size);
};

/// Defines the interface to a memory allocator. All memory allocators in the
/// runtime system are required to implement this interface. Allocators are not
/// thread safe.
class CMN_PUBLIC allocator_t
{
public:
    /// Pointer to an optional allocation tracker implementation that can be
    /// used to gather real-time allocation data from this allocator.
    allocation_tracker_t *tracker;

    /// An optional name assigned to the allocator. This value should point to
    /// a string literal.
    char const           *name;

    /// The total number of outstanding allocations that have been made
    /// through this allocator instance.
    size_t                total_allocation_count;

    /// The total size, in bytes, of all outstanding allocations that have
    /// been made through this allocator instance. This value includes padding,
    /// that is, represents the total number of bytes actually allocated, not
    /// the the total number of bytes requested.
    size_t                total_allocation_size;

    /// A high watermark value indicating the maximum number of outstanding
    /// allocations that have ever been active at one time through this
    /// allocator instance.
    size_t                watermark_allocation_count;

    /// A high watermark value indicating the maximum number of bytes that have
    /// ever been wired at once through this allocator instance.
    size_t                watermark_allocation_size;

public:
    /// Default constructor.
    allocator_t(void);

    /// Constructs a new instance with the specified name.
    ///
    /// @param allocator_name Pointer to a NULL-terminated ASCII or UTF-8
    /// string representing a name for the allocator instance. This should be
    /// a pointer to a string literal; the string is not copied and must exist
    /// for the lifetime of the allocator.
    allocator_t(char const *allocator_name);

    /// Type destructor. Verifies that there are no memory leaks, that is,
    /// verifies that total_allocation_count and total_allocation_size are 0.
    virtual ~allocator_t(void);

public:
    /// Determines the number of bytes allocated for a particular memory
    /// allocation request from this memory allocator instance. All allocators
    /// are required to return allocation sizes.
    ///
    /// @param ptr A pointer returned by a previous call to this allocator
    /// instance's allocator_t::allocate() method.
    /// @return The number of bytes allocated to @a ptr, or zero if the
    /// allocation is unknown.
    virtual size_t allocation_size(void *ptr) = 0;

    /// Requests memory from the allocator.
    ///
    /// @param size_in_bytes The minimum number of bytes being requested. More
    /// memory than requested may be returned.
    /// @param alignment The required alignment of the returned memory address.
    /// @return A pointer to the allocated memory, or NULL if the request
    /// could not be satisfied.
    virtual void*  allocate(size_t size_in_bytes, size_t alignment) = 0;

    /// Returns memory to the allocator.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    virtual void   deallocate(void *ptr) = 0;

    /// Resets the allocator to its initial state, freeing all existing memory
    /// allocations in a single operation. Any attempt to access previously
    /// allocated memory results in undefined behavior or an access violation.
    virtual void   reset(void) = 0;

public:
    /// Allocates memory for a single instance of a specific type and calls
    /// the default (parameterless) constructor on the returned memory.
    ///
    /// @return A pointer to the new instance of the specified type.
    template <typename T>
    inline T* new_instance(void)
    {
        T *mem = (T*) allocate(sizeof(T), traits::alignof<T>::value);
        traits::construct(mem);
        return mem;
    }

    /// Allocates memory for a single instance of a specific type and calls
    /// a constructor accepting a single parameter of type P1 on the returned
    /// memory.
    ///
    /// @param p1 The parameter value to pass to the constructor.
    /// @return A pointer to the new instance of the specified type.
    template <typename T, typename P1>
    inline T* new_instance(P1 const& p1)
    {
        T* mem = (T*) allocate(sizeof(T), traits::alignof<T>::value);
        return new (mem) T(p1);
    }

    /// Allocates memory for multiple instances of a specific type and calls
    /// the default (parameterless) constructor on the returned memory. The
    /// constructor is called once for each instance.
    ///
    /// @param count The number of instances to allocate storage for.
    /// @return A pointer to the array of instances.
    template <typename T>
    inline T* new_array(size_t count)
    {
        T *mem = (T*) allocate(sizeof(T) * count, traits::alignof<T>::value);
        traits::construct(mem, count);
        return mem;
    }

    /// Releases memory previously allocated by this instance, calling the
    /// destructor for the type before releasing the memory.
    ///
    /// @param ptr A pointer returned by this allocator instance.
    template <typename T>
    inline void delete_instance(T *ptr)
    {
        if (ptr != NULL)
        {
            traits::destruct(ptr);
            deallocate(ptr);
        }
    }

    /// Releases memory previously allocated by this instance for an array of
    /// objects, calling the destructor for the type for each instance in the
    /// array before releasing the memory.
    ///
    /// @param count The number of instances in the array. This corresponds to
    /// the number of items for which the type destructor will be invoked.
    /// Passing zero will not cause a crash, but no destructors will be invoked
    /// in this case.
    /// @param ptr A pointer returned by this allocator instance.
    template <typename T>
    inline void delete_array(size_t count, T *ptr)
    {
        if (ptr != NULL)
        {
            traits::destruct(ptr, count);
            deallocate(ptr);
        }
    }

    /// Used by allocator implementations to report a memory allocation.
    ///
    /// @param ptr The address of the returned memory block.
    /// @param requested_size The requested allocation size, in bytes.
    /// @param allocated_size The number of bytes actually allocated.
    inline void report_allocation(
        void   *ptr,
        size_t  requested_size,
        size_t  allocated_size)
    {
        CMN_UNUSED(ptr);
        CMN_UNUSED(requested_size);
        total_allocation_count     += 1;
        total_allocation_size      += allocated_size;
        watermark_allocation_count  = CMN_MAX(total_allocation_count, watermark_allocation_count);
        watermark_allocation_size   = CMN_MAX(total_allocation_size,  watermark_allocation_size);
        if (tracker) tracker->report_allocation(this, ptr, requested_size, allocated_size);
    }

    /// Used by allocator implementation to report when an application releases
    /// some memory.
    ///
    /// @param ptr The addresss of the memory block being freed by the
    /// application.
    /// @param size The size of the memory block, in bytes.
    inline void report_deallocation(void *ptr, size_t size)
    {
        CMN_UNUSED(ptr);
        total_allocation_count -= 1;
        total_allocation_size  -= size;
        if (tracker) tracker->report_deallocation(this, ptr, size);
    }

    /// Resets the memory allocation statistics for the allocator instance.
    inline void reset_allocation_counts(void)
    {
        total_allocation_count = 0;
        total_allocation_size  = 0;
    }

    /// Resets the memory allocation watermarks for the allocator instance.
    inline void reset_allocation_watermarks(void)
    {
        watermark_allocation_count = 0;
        watermark_allocation_size  = 0;
    }

    /// Uses the C assert macro to make sure there are no memory leaks.
    /// This function is called from the destructor of all allocators.
    inline void assert_no_leaks(void)
    {
        assert(0 == total_allocation_count && 0 == total_allocation_size);
    }

private:
    allocator_t(allocator_t const &other);             /* noimp */
    allocator_t& operator =(allocator_t const &other); /* noimp */
};

/// Defines the page allocator implementation. This allocator reserves and
/// commits pages directly using the virtual memory management functionality
/// provided by the operating system. Typically there is only one page
/// allocator instance in any given application. Each allocation carries at
/// least a two-page overhead, plus any padding overhead. Allocations are
/// aligned on a page boundary and are padded to the nearest multiple of the
/// system page size. The two additional pages act as guard pages and are used
/// to detect memory overwrites. On typical systems this amounts to
/// approximately 8KB of overhead per-allocation.
class CMN_PUBLIC page_allocator_t : public allocator_t
{
public:
    size_t page_size;     /// Operating system page size, in bytes.
    bool   check_on_free; /// Should verify guard pages during deallocate()?

public:
    /// Default constructor. Requests the operating system page size.
    page_allocator_t(void);

    /// Constructs a new instance with the specified allocator name.
    ///
    /// @param allocator_name The name of the memory allocator instance. This
    /// should be a string literal and not a dynamically allocated string. The
    /// string is not copied.
    page_allocator_t(char const *allocator_name);

    /// Type destructor.
    ~page_allocator_t(void);

public:
    /// Explicitly checks the validity of the guard pages for a given
    /// memory allocation to ensure that no memory has been overwritten.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    /// @return true if guard page validation was successful, or false if a
    /// memory overwrite was detected.
    bool   check_guard(void *ptr);

public:
    /// Determines the number of bytes allocated for a particular memory
    /// allocation request from this memory allocator instance. All allocators
    /// are required to return allocation sizes.
    ///
    /// @param ptr A pointer returned by a previous call to this allocator
    /// instance's allocator_t::allocate() method.
    /// @return The number of bytes allocated to @a ptr, or zero if the
    /// allocation is unknown.
    size_t allocation_size(void *ptr);

    /// Requests memory from the allocator.
    ///
    /// @param size_in_bytes The minimum number of bytes being requested. More
    /// memory than requested may be returned.
    /// @param alignment The required alignment of the returned memory address.
    /// @return A pointer to the allocated memory, or NULL if the request
    /// could not be satisfied.
    void*  allocate(size_t size_in_bytes, size_t alignment);

    /// Returns memory to the allocator.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    void   deallocate(void *ptr);

    /// Resets the allocator to its initial state, freeing all existing memory
    /// allocations in a single operation. Any attempt to access previously
    /// allocated memory results in undefined behavior or an access violation.
    void   reset(void);

private:
    page_allocator_t(page_allocator_t const &other);
    page_allocator_t& operator =(page_allocator_t const &other);
};

/// Defines the heap allocator implementation. All calls are redirected to the
/// mspace_* memory management functions from Doug Lea's memory allocator.
class CMN_PUBLIC heap_allocator_t : public allocator_t
{
public:
    void   *memory_space;  /// The dlmalloc mspace, defined within memory_block.
    void   *memory_block;  /// The memory block from which this allocator draws.
    size_t  memory_size;   /// The size of memory_block, in bytes.
    size_t  memory_over;   /// The number of bytes allocated from the system.

public:
    /// Default constructor. Call heap_allocator_t::bind() before attempting
    /// to use the allocator.
    heap_allocator_t(void);

    /// Constructs a new instance with the specified allocator name. Call
    /// the heap_allocator_t::bind() method before attempting to perform any
    /// operations on the allocator.
    ///
    /// @param allocator_name The name of the memory allocator instance. This
    /// should be a string literal and not a dynamically allocated string. The
    /// string is not copied.
    heap_allocator_t(char const *allocator_name);

    /// Constructs a new allocator instance around the specified memory block.
    /// The allocator will sub-allocate from this block. This constructor is
    /// equivalent to calling the default constructor followed by a call to
    /// heap_allocator_t::bind(mem_block, mem_size).
    ///
    /// @param mem_block A pointer to an externally-managed memory block, from
    /// which this allocator will sub-allocate.
    /// @param mem_size The size of @a mem_block, specified in bytes.
    heap_allocator_t(void *mem_block, size_t mem_size);

    /// Type destructor. This does not free the memory_block, it simply calls
    /// heap_allocator_t::unbind(). Any memory allocated from the system
    /// outside of memory_block is freed.
    ~heap_allocator_t(void);

public:
    /// Binds the allocator to an externally-managed memory block.
    ///
    /// @param mem_block A pointer to an externally-managed memory block, from
    /// which this allocator will sub-allocate.
    /// @param mem_size The size of @a mem_block, specified in bytes.
    /// @return true if the binding is successful and memory can be requested
    /// from this heap allocator instance.
    bool  bind(void *mem_block, size_t mem_size);

    /// Detaches this allocator instance from an externally-managed memory
    /// block. This does not free the memory block, but does free any
    /// additional memory allocated from the system.
    ///
    /// @param out_mem_size If this value is non-null, on return it is updated
    /// with the size of the externally-managed memory block, in bytes.
    /// @return A pointer to the externally-managed memory block.
    void *unbind(size_t *out_mem_size = NULL);

public:
    /// Determines the number of bytes allocated for a particular memory
    /// allocation request from this memory allocator instance. All allocators
    /// are required to return allocation sizes.
    ///
    /// @param ptr A pointer returned by a previous call to this allocator
    /// instance's allocator_t::allocate() method.
    /// @return The number of bytes allocated to @a ptr, or zero if the
    /// allocation is unknown.
    size_t allocation_size(void *ptr);

    /// Requests memory from the allocator.
    ///
    /// @param size_in_bytes The minimum number of bytes being requested. More
    /// memory than requested may be returned.
    /// @param alignment The required alignment of the returned memory address.
    /// @return A pointer to the allocated memory, or NULL if the request
    /// could not be satisfied.
    void*  allocate(size_t size_in_bytes, size_t alignment);

    /// Returns memory to the allocator.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    void   deallocate(void *ptr);

    /// Resets the allocator to its initial state, freeing all existing memory
    /// allocations in a single operation. Any attempt to access previously
    /// allocated memory results in undefined behavior or an access violation.
    void   reset(void);

private:
    heap_allocator_t(heap_allocator_t const &other);             /* noimp */
    heap_allocator_t& operator =(heap_allocator_t const &other); /* noimp */
};

/// Defines a linear address decrement allocator implementation. The owner
/// provides an externally-managed memory block from which this allocator
/// sub-allocates. The owner is responsible for freeing the master memory
/// block.
class CMN_PUBLIC decrement_allocator_t : public allocator_t
{
public:
    uint8_t *memory_block;   /// Pointer to externally-managed memory block.
    size_t   memory_size;    /// Size of memory_block, specified in bytes.
    size_t   memory_offset;  /// Allocation byte offset into memory_block.

public:
    /// Default constructor. Call the decrement_allocator_t::bind() method
    /// before attempting to perform any operations on the allocator.
    decrement_allocator_t(void);

    /// Constructs a new instance with the specified allocator name. Call
    /// the decrement_allocator_t::bind() method before attempting to perform
    /// any operations on the allocator.
    ///
    /// @param allocator_name The name of the memory allocator instance. This
    /// should be a string literal and not a dynamically allocated string. The
    /// string is not copied.
    decrement_allocator_t(char const *allocator_name);

    /// Constructs a new instance that will sub-allocate from an existing,
    /// caller-managed memory block. This is equivalent to using the default
    /// constructor followed by a call to decrement_allocator_t::bind().
    ///
    /// @param block A pointer to a caller-managed memory block. This value
    /// cannot be NULL.
    /// @param size_in_bytes The maximum number of bytes from @a block from
    /// which this allocator may sub-allocate.
    decrement_allocator_t(void *block, size_t size_in_bytes);

    /// Type destructor. This does not free the memory_block, it simply calls
    /// decrement_allocator_t::unbind().
    ~decrement_allocator_t(void);

public:
    /// Binds the allocator instance to an externally-managed memory block.
    /// The allocator should not have any outstanding memory allocations.
    ///
    /// @param block A pointer to the caller-managed memory block. This value
    /// cannot be NULL.
    /// @param size_in_bytes The maximum number of bytes from @a block from
    /// which this allocator may sub-allocate.
    /// @return true if the binding is successful and memory can be requested
    /// from this heap allocator instance.
    bool  bind(void *block, size_t size_in_bytes);

    /// Unbinds this allocator instance from the current externally-managed
    /// memory block. When this method is called, the allocator should not
    /// have any outstanding memory allocations. Do not attempt to allocate
    /// from or otherwise query the memory allocator instance until the
    /// decrement_allocator_t::bind() method is called again.
    ///
    /// @param out_size_in_bytes If non-NULL, this location is updated with
    /// the size of the externally-managed memory block, specified in bytes.
    /// @return A pointer to the externally-managed memory block.
    void*  unbind(size_t *out_size_in_bytes = NULL);

    /// Obtains a marker that can later be used to perform a partial reset of
    /// the memory allocator back to the current allocation position.
    ///
    /// @param out_marker Pointer to a marker structure that can later be
    /// passed to decrement_allocator_t::reset_to_marker().
    void   marker(allocation_marker_t *out_marker);

    /// Performs a partial or complete reset of the allocator back to a
    /// previously marked position returned by decrement_allocator_t::marker().
    ///
    /// @param mark A marker returned by decrement_allocator_t::marker().
    void   reset_to_marker(allocation_marker_t *mark);

public:
    /// Determines the number of bytes allocated for a particular memory
    /// allocation request from this memory allocator instance. All allocators
    /// are required to return allocation sizes.
    ///
    /// @param ptr A pointer returned by a previous call to this allocator
    /// instance's allocator_t::allocate() method.
    /// @return The number of bytes allocated to @a ptr, or zero if the
    /// allocation is unknown.
    size_t allocation_size(void *ptr);

    /// Requests memory from the allocator.
    ///
    /// @param size_in_bytes The minimum number of bytes being requested. More
    /// memory than requested may be returned.
    /// @param alignment The required alignment of the returned memory address.
    /// @return A pointer to the allocated memory, or NULL if the request
    /// could not be satisfied.
    void*  allocate(size_t size_in_bytes, size_t alignment);

    /// Returns memory to the allocator.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    void   deallocate(void *ptr);

    /// Resets the allocator to its initial state, freeing all existing memory
    /// allocations in a single operation. Any attempt to access previously
    /// allocated memory results in undefined behavior or an access violation.
    void   reset(void);

private:
    decrement_allocator_t(decrement_allocator_t const &other);
    decrement_allocator_t& operator =(decrement_allocator_t const &other);
};

/// Defines a linear address-increment allocator implementation. The owner
/// provides an externally-managed memory block from which this allocator
/// sub-allocates. The owner is responsible for freeing the master memory
/// block.
class CMN_PUBLIC increment_allocator_t : public allocator_t
{
public:
    uint8_t *memory_block;   /// Pointer to externally-managed memory block.
    size_t   memory_size;    /// Size of memory_block, specified in bytes.
    size_t   memory_offset;  /// Allocation byte offset into memory_block.

public:
    /// Default constructor. Call the increment_allocator_t::bind() method
    /// before attempting to perform any operations on the allocator.
    increment_allocator_t(void);

    /// Constructs a new instance with the specified allocator name. Call
    /// the increment_allocator_t::bind() method before attempting to perform
    /// any operations on the allocator.
    ///
    /// @param allocator_name The name of the memory allocator instance. This
    /// should be a string literal and not a dynamically allocated string. The
    /// string is not copied.
    increment_allocator_t(char const *allocator_name);

    /// Constructs a new instance that will sub-allocate from an existing,
    /// caller-managed memory block. This is equivalent to using the default
    /// constructor followed by a call to increment_allocator_t::bind().
    ///
    /// @param block A pointer to a caller-managed memory block. This value
    /// cannot be NULL.
    /// @param size_in_bytes The maximum number of bytes from @a block from
    /// which this allocator may sub-allocate.
    increment_allocator_t(void *block, size_t size_in_bytes);

    /// Type destructor. This does not free the memory_block, it simply calls
    /// increment_allocator_t::unbind().
    ~increment_allocator_t(void);

public:
    /// Binds the allocator instance to an externally-managed memory block.
    /// The allocator should not have any outstanding memory allocations.
    ///
    /// @param block A pointer to the caller-managed memory block. This value
    /// cannot be NULL.
    /// @param size_in_bytes The maximum number of bytes from @a block from
    /// which this allocator may sub-allocate.
    /// @return true if the binding is successful and memory can be requested
    /// from this heap allocator instance.
    bool  bind(void *block, size_t size_in_bytes);

    /// Unbinds this allocator instance from the current externally-managed
    /// memory block. When this method is called, the allocator should not
    /// have any outstanding memory allocations. Do not attempt to allocate
    /// from or otherwise query the memory allocator instance until the
    /// increment_allocator_t::bind() method is called again.
    ///
    /// @param out_size_in_bytes If non-NULL, this location is updated with
    /// the size of the externally-managed memory block, specified in bytes.
    /// @return A pointer to the externally-managed memory block.
    void*  unbind(size_t *out_size_in_bytes = NULL);

    /// Obtains a marker that can later be used to perform a partial reset of
    /// the memory allocator back to the current allocation position.
    ///
    /// @param out_marker Pointer to a marker structure that can later be
    /// passed to increment_allocator_t::reset_to_marker().
    void   marker(allocation_marker_t *out_marker);

    /// Performs a partial or complete reset of the allocator back to a
    /// previously marked position returned by increment_allocator_t::marker().
    ///
    /// @param mark A marker returned by increment_allocator_t::marker().
    void   reset_to_marker(allocation_marker_t *mark);

public:
    /// Determines the number of bytes allocated for a particular memory
    /// allocation request from this memory allocator instance. All allocators
    /// are required to return allocation sizes.
    ///
    /// @param ptr A pointer returned by a previous call to this allocator
    /// instance's allocator_t::allocate() method.
    /// @return The number of bytes allocated to @a ptr, or zero if the
    /// allocation is unknown.
    size_t allocation_size(void *ptr);

    /// Requests memory from the allocator.
    ///
    /// @param size_in_bytes The minimum number of bytes being requested. More
    /// memory than requested may be returned.
    /// @param alignment The required alignment of the returned memory address.
    /// @return A pointer to the allocated memory, or NULL if the request
    /// could not be satisfied.
    void*  allocate(size_t size_in_bytes, size_t alignment);

    /// Returns memory to the allocator.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    void   deallocate(void *ptr);

    /// Resets the allocator to its initial state, freeing all existing memory
    /// allocations in a single operation. Any attempt to access previously
    /// allocated memory results in undefined behavior or an access violation.
    void   reset(void);

private:
    increment_allocator_t(increment_allocator_t const &other);
    increment_allocator_t& operator =(increment_allocator_t const &other);
};

/// Defines a proxy allocator, which simply forwards allocations on to a base
/// allocator. Proxy allocators are named, and are typically used as the top-
/// level allocator for an individual runtime system.
class CMN_PUBLIC proxy_allocator_t : public allocator_t
{
public:
    allocator_t *base_allocator;

public:
    /// Default constructor. Call proxy_allocator_t::bind() before attempting
    /// to perform any operations on the allocator.
    proxy_allocator_t(void);

    /// Constructs a new instance with the specified allocator name. Call
    /// the proxy_allocator_t::bind() method before attempting to perform
    /// any operations on the allocator.
    ///
    /// @param allocator_name The name of the memory allocator instance. This
    /// should be a string literal and not a dynamically allocated string. The
    /// string is not copied.
    proxy_allocator_t(char const *allocator_name);

    /// Constructs a new instance with the specified base allocator. This is
    /// equivalent to using the default allocator followed by bind().
    ///
    /// @param base_alloc The allocator implementation to which allocation
    /// requests will be forwarded. This value must not be NULL.
    proxy_allocator_t(allocator_t *base_alloc);

    /// Type destructor.
    ~proxy_allocator_t();

public:
    /// Binds this proxy allocator to another memory allocator which performs
    /// the actual allocations.
    ///
    /// @param base_alloc The allocator implementation to which allocation
    /// requests will be forwarded. This value must not be NULL.
    void bind(allocator_t *base_alloc);

    /// Unbinds this proxy allocator from its base allocator. You must call
    /// proxy_allocator_t::bind() before attempting to perform any operations.
    void unbind(void);

public:
    /// Determines the number of bytes allocated for a particular memory
    /// allocation request from this memory allocator instance. All allocators
    /// are required to return allocation sizes.
    ///
    /// @param ptr A pointer returned by a previous call to this allocator
    /// instance's allocator_t::allocate() method.
    /// @return The number of bytes allocated to @a ptr, or zero if the
    /// allocation is unknown.
    size_t allocation_size(void *ptr);

    /// Requests memory from the allocator.
    ///
    /// @param size_in_bytes The minimum number of bytes being requested. More
    /// memory than requested may be returned.
    /// @param alignment The required alignment of the returned memory address.
    /// @return A pointer to the allocated memory, or NULL if the request
    /// could not be satisfied.
    void*  allocate(size_t size_in_bytes, size_t alignment);

    /// Returns memory to the allocator.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    void   deallocate(void *ptr);

    /// Resets the allocator to its initial state, freeing all existing memory
    /// allocations in a single operation. Any attempt to access previously
    /// allocated memory results in undefined behavior or an access violation.
    void   reset(void);

private:
    proxy_allocator_t(proxy_allocator_t const &other);             /* noimp */
    proxy_allocator_t& operator =(proxy_allocator_t const &other); /* noimp */
};

/// Defines a trace allocator, which captures and stores stack trace data for
/// each allocation. This comes at a performance and memory cost, but is useful
/// for tracking down memory leaks within a specific subsystem.
class CMN_PUBLIC trace_allocator_t : public allocator_t
{
public:
    /// The maximum number of stack frames we will capture for each allocation.
    static const size_t      MAX_STACK_FRAMES = 60;

public:
    /// A single node in the doubly-linked list of nodes representing
    /// outstanding memory allocations. Instances of this type are allocated
    /// at the end of user allocation requests.
    struct trace_node_t
    {
        void                *address;
        struct trace_node_t *next;
        struct trace_node_t *prev;
        size_t               requested_size;
        void                *stack_frames[MAX_STACK_FRAMES];
    };

public:
    /// The allocator we are sitting in front of, and which does the actual
    /// work of managing memory allocations and releases.
    allocator_t  *base_allocator;

    /// Pointer to the node at the beginning of the list of outstanding
    /// memory allocations.
    trace_node_t *list_head;

    /// Pointer to the last node in the list of outstanding memory allocations.
    trace_node_t *list_tail;

    /// The number of outstanding memory allocations.
    size_t        list_size;

public:
    /// Default constructor. Call trace_allocator_t::bind() before attempting
    /// to perform any operations on the allocator.
    trace_allocator_t(void);

    /// Constructs a new instance with the specified allocator name. Call
    /// the trace_allocator_t::bind() method before attempting to perform
    /// any operations on the allocator.
    ///
    /// @param allocator_name The name of the memory allocator instance. This
    /// should be a string literal and not a dynamically allocated string. The
    /// string is not copied.
    trace_allocator_t(char const *allocator_name);

    /// Constructs a new instance with the specified base allocator. This is
    /// equivalent to using the default allocator followed by bind().
    ///
    /// @param base_alloc The allocator implementation to which allocation
    /// requests will be forwarded. This value must not be NULL.
    trace_allocator_t(allocator_t *base_alloc);

    /// Type destructor.
    ~trace_allocator_t();

public:
    /// Binds this proxy allocator to another memory allocator which performs
    /// the actual allocations.
    ///
    /// @param base_alloc The allocator implementation to which allocation
    /// requests will be forwarded. This value must not be NULL.
    void bind(allocator_t *base_alloc);

    /// Unbinds this proxy allocator from its base allocator. You must call
    /// trace_allocator_t::bind() before attempting to perform any operations.
    void unbind(void);

public:
    /// Determines the number of bytes allocated for a particular memory
    /// allocation request from this memory allocator instance. All allocators
    /// are required to return allocation sizes.
    ///
    /// @param ptr A pointer returned by a previous call to this allocator
    /// instance's allocator_t::allocate() method.
    /// @return The number of bytes allocated to @a ptr, or zero if the
    /// allocation is unknown.
    size_t allocation_size(void *ptr);

    /// Requests memory from the allocator.
    ///
    /// @param size_in_bytes The minimum number of bytes being requested. More
    /// memory than requested may be returned.
    /// @param alignment The required alignment of the returned memory address.
    /// @return A pointer to the allocated memory, or NULL if the request
    /// could not be satisfied.
    void*  allocate(size_t size_in_bytes, size_t alignment);

    /// Returns memory to the allocator.
    ///
    /// @param ptr A pointer to a memory block previously allocated by this
    /// allocator instance.
    void   deallocate(void *ptr);

    /// Resets the allocator to its initial state, freeing all existing memory
    /// allocations in a single operation. Any attempt to access previously
    /// allocated memory results in undefined behavior or an access violation.
    void   reset(void);

private:
    trace_allocator_t(trace_allocator_t const &other);             /* noimp */
    trace_allocator_t& operator =(trace_allocator_t const &other); /* noimp */
};

/// Captures the callstack for the current thread without performing any symbol
/// lookup to resolve addresses into symbol names.
///
/// @param frames Pointer to an array of pointers which will be populated with
/// stack frame data. The first entry is overwritten to specify the number of
/// frames present. Typically the first entry would contain the stack frame
/// indicating the memory::capture_callstack() function.
/// @param max_frames The maximum number of stack frames that can be written
/// to @a frames.
/// @return The number of stack frames written to @a frames.
CMN_PUBLIC size_t capture_callstack(void **frames, size_t max_frames);

/// Queries the operating system to determine the size of a single virtual
/// memory page, in bytes. The system page size is also typically the
/// granularity of the system heap. This function always queries the host
/// system for the page size.
///
/// @return The number of bytes in a single page managed by the virtual
/// memory system of the host operating system.
CMN_PUBLIC size_t vmm_page_size(void);

/// Reserves a block of contiguous address space within the memory map of the
/// calling process. Do not attempt to read from or write to the returned
/// address block until the pages are committed using memory::vmm_commit().
///
/// @param size_in_bytes The number of bytes to reserve. This value may be
/// rounded up to the next largest page boundary.
/// @return A pointer to the start of the reserved address space, or NULL.
CMN_PUBLIC void* vmm_reserve(size_t size_in_bytes);

/// Commits a block of contiguous address space, backing it with physical
/// memory or space in the system page file. Committed pages are both readable
/// and writable.
///
/// @param addr_start The start of the address range to commit. This may be a
/// subset of the address space reserved using memory::vmm_reserve(). This
/// address must be evenly divisible by the operating system page size.
/// @param size_in_bytes The number of bytes to commit, starting from
/// @a addr_start.
/// @return true if the address space is successfully committed.
CMN_PUBLIC bool vmm_commit(void *addr_start, size_t size_in_bytes);

/// De-commits a block of contiguous address space, unwiring the pages from
/// physical memory or the system page file and discarding their contents.
/// Decommitted pages are not readable or writable, but their address space
/// remains reserved.
///
/// @param addr_start The start of the address range to de-commit. This may
/// be a subset of the address space reserved using memory::vmm_reserve().
/// This address must be evenly divisible by the operating system page size.
/// @param size_in_bytes The number of bytes to de-commit, starting from
/// @a addr_start.
/// @return true if the address space is successfully de-committed.
CMN_PUBLIC bool vmm_decommit(void *addr_start, size_t size_in_bytes);

/// Releases an entire block of contiguous address space previously reserved
/// with memory::vmm_reserve() or committed with memory::vmm_commit().
/// The address space may contain a mixture of reserved and committed blocks.
///
/// @param addr_start The address returned by memory::vmm_reserve(). The
/// entire range of address space will be released.
/// @param size_in_bytes The number of bytes requested by the original call to
/// memory::vmm_reserve().
CMN_PUBLIC void vmm_release(void *addr_start, size_t size_in_bytes);

/// Adjusts an address value such that it is aligned to a particular value. If
/// the address is already an even multiple of the specified value, it is not
/// modified.
///
/// @param address The address to adjust.
/// @param pow2 The desired alignment value. This value must be a power of two.
/// @return The adjusted size value.
template <typename T>
inline T* align_to(T *address, size_t pow2)
{
    assert((pow2 & (pow2-1)) == 0); // is a power of two
    return (T*) ((((size_t)address) + (pow2-1)) & ~(pow2-1));
}

/// Adjusts a size value up such that it is an even multiple of a particular
/// value; the returned value is always greater than zero. If the size is
/// already an even multiple of the specified value, it is not modified.
///
/// @param size The size to adjust.
/// @param pow2 The desired alignment value. This value must be a power of two.
/// @return The adjusted size value.
inline size_t align_up(size_t size, size_t pow2)
{
    assert((pow2 & (pow2-1)) == 0); // is a power of two
    return (size == 0) ? pow2 : ((size + (pow2-1)) & ~(pow2-1));
}

/// Determines whether a size value is aligned to a specified value.
///
/// @param size The size value to check.
/// @param pow2 The alignment value. This value must be a power of two.
/// @return true if @a size is aligned to @a pow2.
inline bool aligned_to(size_t size, size_t pow2)
{
    assert ((pow2 & (pow2-1)) == 0); // is a power of two
    return ((size & (pow2-1)) == 0);
}

/// Determines whether a memory address is aligned to a specified value.
///
/// @param address The memory address to check.
/// @param pow2 The alignment value. This value must be a power of two.
/// @return true if @a address is aligned to @a pow2.
inline bool aligned_to(void *address, size_t pow2)
{
    assert((pow2 & (pow2-1)) == 0); // is a power of two
    return ((((size_t)address) & (pow2-1)) == 0);
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace memory */

#endif /* LIBMEMORY_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
