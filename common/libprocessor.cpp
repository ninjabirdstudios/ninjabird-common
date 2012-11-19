/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements an abstraction on top of the processor management
/// functionality provided by the operating system, relying on either pthreads
/// or the equivalent Win32 APIs to create and manage threads, thread-local
/// storage and synchronization primitives.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include "libprocessor.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/
using processor::thread_t;
using processor::channel_t;

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

#if   CMN_IS_APPLE
    static mach_timebase_info_data_t Time_Scale = {0};
#elif CMN_IS_WINDOWS
    static LARGE_INTEGER             Time_Scale = {0};
#endif

/*/////////////////////////////////////////////////////////////////////////80*/

#ifdef _MSC_VER
    #define STD_CALL                 __stdcall
#else
    #define STD_CALL
#endif

/*/////////////////////////////////////////////////////////////////////////80*/

static void thread_shutdown(void *arg)
{
    processor::thread_t *thread = (processor::thread_t*) arg;
    if (thread != NULL)
    {
        thread->cleanup();
        thread->delete_thread_locals();
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

#if CMN_IS_APPLE || CMN_IS_LINUX
static void* thread_startup(void *arg)
{
    // initialize the thread, and enter its main loop. when
    // the thread's main loop exits, cleanup and return.
    processor::thread_t *thread = (processor::thread_t*) arg;
    int            cleanup      = 0;
    void          *return_value = NULL;
    thread->start_time          = processor::current_time();
    thread->thread_object       = pthread_self();
    pthread_attr_getstacksize(&thread->attributes, &thread->stack_size);
    pthread_cleanup_push(thread_shutdown, thread);
    if (thread->create_thread_locals())
    {
        // we need to run the cleanup method.
        cleanup = 1;
        // allocate any thread-local resources.
        if (thread->startup())
        {
            // allow the thread to execute.
            return_value = thread->run();
        }
    }
    pthread_cleanup_pop(cleanup);
    pthread_exit(return_value);
    return return_value;
}
#endif

/*/////////////////////////////////////////////////////////////////////////80*/

#if CMN_IS_WINDOWS
static unsigned int __stdcall thread_startup(void *arg)
{
    processor::thread_t *thread = (processor::thread_t*) arg;
    void          *return_value = NULL;
    __try
    {
        thread->start_time = processor::current_time();
        if (thread->create_thread_locals())
        {
            if (thread->startup())
            {
                return_value = thread->run();
            }
        }
    }
    __finally
    {
        thread->cleanup();
        thread->delete_thread_locals();
        _endthreadex((unsigned int) return_value);
    }
    return (unsigned int) return_value;
}
#endif

/*/////////////////////////////////////////////////////////////////////////80*/

processor::thread_t::thread_t(void)
    :
    start_time(0),
    stack_size(processor::thread_t::DEFAULT_STACK_SIZE)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_attr_init(&attributes);
#elif CMN_IS_WINDOWS
    thread_object = NULL;
#else
    #error No implementation of thread_t::thread_t() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

processor::thread_t::~thread_t(void)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_attr_destroy(&attributes);
#elif CMN_IS_WINDOWS
    if (thread_object != NULL)
    {
        CloseHandle(thread_object);
        thread_object  = NULL;
    }
#else
    #error No implementation of thread_t::~thread_t() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool processor::thread_t::start(size_t thread_stack_size)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    // explicitly create joinable threads. some platforms may not
    // create joinable threads by default, so we do it explicitly.
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    if (thread_stack_size > 0)
    {
        // set the stack size to the caller-supplied value.
        pthread_attr_setstacksize(&attributes, thread_stack_size);
    }
    // create the thread. the thread is created in the started state.
    if (pthread_create(&thread_object, &attributes, thread_startup, this))
    {
        // thread creation failed. this is a serious error.
        return false;
    }
    return true;
#elif CMN_IS_WINDOWS
    unsigned id   = 0;
    thread_object = (HANDLE) _beginthreadex(
        NULL,
        thread_stack_size,
        thread_startup,
        this,
        0, /* running */
        &id);
    if (NULL == thread_object)
    {
        // cannot create or start the thread.
        return false;
    }
    // @todo: not sure how to query thread stack size under Windows.
    stack_size = thread_stack_size;
    return true;
#else
    #error No implementation of thread_t::start() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void processor::thread_t::yield(void)
{
#if   CMN_IS_APPLE
    pthread_yield_np();
#elif CMN_IS_LINUX
    sched_yield();
#elif CMN_IS_WINDOWS
    // @note: if we can detect a SMP (hyperthreading) system we can use the
    // more efficient PAUSE instruction instead of SwitchToThread, as the
    // SwitchToThread function causes a kernel-mode switch.
    SwitchToThread();
#else
    #error No implementation of thread_t::yield() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void processor::thread_t::sleep(uint64_t microseconds)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    usleep(microseconds);
#elif CMN_IS_WINDOWS
    Sleep((DWORD) (microseconds / 1000));
#else
    #error No implementation of thread_t::sleep() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* processor::thread_t::join(void)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    void  *return_value = NULL;
    pthread_join(thread_object, &return_value);
    return return_value;
#elif CMN_IS_WINDOWS
    DWORD  exit_code = 0;
    WaitForSingleObject(thread_object, INFINITE);
    GetExitCodeThread(thread_object, &exit_code);
    return (void*) exit_code;
#else
    #error No implementation of thread_t::join() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool processor::thread_t::create_thread_locals(void)
{
    // @note: implementation-defined.
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool processor::thread_t::startup(void)
{
    // @note: implementation-defined.
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void processor::thread_t::cleanup(void)
{
    // @note: implementation-defined.
}

/*/////////////////////////////////////////////////////////////////////////80*/

void processor::thread_t::delete_thread_locals(void)
{
    // @note: implementation-defined.
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* processor::thread_t::run(void)
{
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint64_t processor::current_time(void)
{
#if   CMN_IS_APPLE
    if (0 == Time_Scale.denom)
    {
        (void) mach_timebase_info(&Time_Scale);
    }
    return mach_absolute_time() * Time_Scale.numer / Time_Scale.denom;
#elif CMN_IS_LINUX
    struct timespect ts   = {0};
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t(ts.tv_sec) * 1000000000 + ts.tv_nsec);
#elif CMN_IS_WINDOWS
    if (0 == Time_Scale.QuadPart)
    {
        (void) QueryPerformanceFrequency (&Time_Scale);
        // convert 1 second (expressed as nanoseconds) to time units.
        Time_Scale.QuadPart = 1000000000 / Time_Scale.QuadPart;
    }
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    return (qpc.QuadPart * Time_Scale.QuadPart);
#else
    #error No implementation of processor::current_time() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool processor::create_tls_slot(size_t *out_id)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_key_t key;
    if (0 == pthread_key_create(&key, NULL))
    {
        *out_id = (size_t) key;
        return true;
    }
    return false;
#elif CMN_IS_WINDOWS
    DWORD key;
    if ((key = TlsAlloc()) != TLS_OUT_OF_INDEXES)
    {
        *out_id = key;
        return true;
    }
    return false;
#else
    #error No implementation of processor::create_tls_slot() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void processor::delete_tls_slot(size_t tls_id)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_key_delete((pthread_key_t) tls_id);
#elif CMN_IS_WINDOWS
    TlsFree((DWORD) tls_id);
#else
    #error No implementation of processor::delete_tls_slot() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* processor::threadlocal_get_direct(size_t tls_id)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    return pthread_getspecific((pthread_key_t) tls_id);
#elif CMN_IS_WINDOWS
    return TlsGetValue((DWORD) tls_id);
#else
    #error No implementation of processor::threadlocal_get_direct() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool processor::threadlocal_set_direct(size_t tls_id, void const *value)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_key_t key = (pthread_key_t) tls_id;
    return (0 == pthread_setspecific(key, value));
#elif CMN_IS_WINDOWS
    return (TlsSetValue((DWORD) tls_id, (LPVOID) value) != 0);
#else
    #error No implementation of processor::threadlocal_set_direct() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

channel_t::channel_t(void)
    :
    storage(NULL),
    size(0),
    mask(0),
    offset_r(0),
    offset_w(0)
{
    /* empty */
}

/*/////////////////////////////////////////////////////////////////////////80*/

channel_t::channel_t(void *buffer, size_t size_in_bytes)
    :
    storage(NULL),
    size(0),
    mask(0),
    offset_r(0),
    offset_w(0)
{
    bind(buffer, size_in_bytes);
}

/*/////////////////////////////////////////////////////////////////////////80*/

channel_t::~channel_t(void)
{
    unbind();
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::bind(void *buffer, size_t size_in_bytes)
{
    if (NULL == buffer || size_in_bytes <= 1)
    {
        // invalid buffer or size supplied.
        return false;
    }
    if ((size_in_bytes & (size_in_bytes  - 1)) != 0)
    {
        // the size is not a power-of-two.
        assert((size_in_bytes & (size_in_bytes - 1)) == 0 && "Must be pow2");
        return false;
    }

    // @note: the platform page size will be a power-of-two, and our
    // buffer size is rounded up to a multiple of the page size.
    // this allows us to use bitwise AND instead of modulus.
    storage  = (uint8_t*) buffer;
    size     = (uint32_t) size_in_bytes;
    mask     = (uint32_t)(size_in_bytes - 1);
    offset_r = 0;
    offset_w = 0;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void* channel_t::unbind(size_t *out_size_in_bytes /* = NULL */)
{
    void   *buffer = storage;
    size_t  nbytes = size;
    storage        = NULL;
    size           = 0;
    mask           = 0;
    offset_r       = 0;
    offset_w       = 0;
    if (out_size_in_bytes) *out_size_in_bytes = nbytes;
    return buffer;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::read(void *output, size_t amount)
{
    if (0 == amount) return true;

    // store the read and write offsets in local variables. this ensures that
    // they remain consistent for the duration of the function and prevents
    // cache misses due to false sharing. when we determine how much data is
    // available to be read, we might miss some data that is available due to
    // a concurrent write operation, but that's ok - it will be picked up on
    // the next read. 'available' might wrap around, but this is okay.
    uint32_t       rd_ofs     = offset_r;
    uint32_t       wr_ofs     = offset_w;
    uint32_t       available  = wr_ofs - rd_ofs;
    uint32_t       actual_rdo = 0;
    uint32_t       bytes_left = 0;
    uint32_t       tail_bytes = 0;
    uint8_t const *src        = storage;
    uint8_t       *dst        = NULL;

    if (amount > available)
    {
        // not enough available to complete the read.
        return false;
    }

    // prevent the compiler from re-ordering reads and writes.
    processor::full_barrier();

    // copy data from the tail, and then from the head.
    // this will never cross the buffer write offset.
    dst        = (uint8_t*) output;
    actual_rdo =  rd_ofs & mask;
    bytes_left = (uint32_t) amount;
    tail_bytes =  CMN_MIN(bytes_left, size - actual_rdo);
    traits::copy(src + actual_rdo, tail_bytes, dst);
    bytes_left-=  tail_bytes;
    if (bytes_left)
    {
        // copy data from the front of the buffer.
        traits::copy(src, bytes_left, dst + tail_bytes);
    }

    // advance the read offset. this is a read-modify-store operation,
    // but since there is only a single reader, only the store must be
    // atomic. writes of 32-bit aligned values are atomic on all modern CPUs.
    rd_ofs  += (uint32_t) amount;
    offset_r =  rd_ofs;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::write(void const *source, size_t amount)
{
    if (0 == amount) return true;

    // store the read and write offsets in local variables. this ensures that
    // they remain consistent for the duration of the function and prevents
    // cache misses due to false sharing.
    uint32_t       rd_ofs     = offset_r;
    uint32_t       wr_ofs     = offset_w;
    uint32_t       available  = size - (wr_ofs - rd_ofs);
    uint32_t       actual_wro = 0;
    uint32_t       bytes_left = 0;
    uint32_t       tail_bytes = 0;
    uint8_t       *dst        = storage;
    uint8_t const *src        = NULL;

    if (amount > available)
    {
        // not enough buffer space available to complete the write.
        return false;
    }

    // copy data into the buffer, first at the tail and then at the head.
    src        = (uint8_t const*) source;
    actual_wro =  wr_ofs & mask;
    bytes_left = (uint32_t) amount;
    tail_bytes =  CMN_MIN(bytes_left, size - actual_wro);
    traits::copy(src, tail_bytes, dst + actual_wro);
    bytes_left-=  tail_bytes;
    if (bytes_left)
    {
        // copy data to the front of the buffer.
        traits::copy(src + tail_bytes, bytes_left, dst);
    }

    // advance the write offset. issue a memory fence to make sure that
    // all data gets written to the buffer before the offset is updated.
    processor::full_barrier();

    wr_ofs  += (uint32_t) amount;
    offset_w =  wr_ofs;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::write_message(
    void const *header,
    size_t      header_size,
    void const *payload,
    size_t      payload_size)
{
    if (NULL == header  || 0 == header_size)
    {
        // invalid message header specified.
        return false;
    }
    if (NULL == payload || 0 == payload_size)
    {
        // write consists of only header data; a standard write will suffice.
        return write(header, header_size);
    }

    // store the read and write offsets in local variables. this ensures that
    // they remain consistent for the duration of the function and prevents
    // cache misses due to false sharing.
    uint32_t       rd_ofs     = offset_r;
    uint32_t       wr_ofs     = offset_w;
    uint32_t       available  = size - (wr_ofs - rd_ofs);
    uint32_t       actual_wro = 0;
    uint32_t       bytes_left = 0;
    uint32_t       tail_bytes = 0;
    uint8_t       *dst        = storage;
    uint8_t const *src        = NULL;

    if (header_size + payload_size > available)
    {
        // not enough buffer space available to complete the write.
        return false;
    }

    // copy header data into the buffer, first at the tail and then the head.
    src        = (uint8_t const*) header;
    actual_wro =  wr_ofs & mask;
    bytes_left = (uint32_t) header_size;
    tail_bytes =  CMN_MIN(bytes_left, size - actual_wro);
    traits::copy(src, tail_bytes, dst + actual_wro);
    bytes_left-=  tail_bytes;
    if (bytes_left)
    {
        // copy data to the front of the buffer.
        traits::copy(src + tail_bytes, bytes_left, dst);
    }

    // update our local write offset:
    wr_ofs    += (uint32_t) header_size;

    // re-compute values based on the new local write offset:
    src        = (uint8_t const*) payload;
    available  = size - (wr_ofs - rd_ofs);
    actual_wro = wr_ofs & mask;
    bytes_left = (uint32_t) payload_size;
    tail_bytes =  CMN_MIN(bytes_left, size - actual_wro);
    traits::copy(src, tail_bytes, dst + actual_wro);
    bytes_left-= tail_bytes;
    if (bytes_left)
    {
        // copy data to the front of the buffer.
        traits::copy(src + tail_bytes, bytes_left, dst);
    }

    // advance the write offset. issue a memory fence to make sure that
    // all data gets written to the buffer before the offset is updated.
    processor::full_barrier();

    wr_ofs  += (uint32_t) payload_size;
    offset_w =  wr_ofs;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::move_all(processor::channel_t *target)
{
    return move_data(offset_w - offset_r, target);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::move_data(
    size_t                size_in_bytes,
    processor::channel_t *target)
{
    assert(this != target);
    if (0 == size_in_bytes)
    {
        // nothing to move.
        return true;
    }

    uint8_t *sd1  = NULL;
    uint8_t *sd2  = NULL;
    uint8_t *td1  = NULL;
    uint8_t *td2  = NULL;
    size_t   ss1  = 0;
    size_t   ss2  = 0;
    size_t   ts1  = 0;
    size_t   ts2  = 0;
    size_t   size = size_in_bytes;
    bool     rres =   this->describe_read (size, &sd1, &ss1, &sd2, &ss2);
    bool     wres = target->describe_write(size, &td1, &ts1, &td2, &ts2);

    if (!rres || !wres)
    {
        // not enough space available in source or target buffers.
        return false;
    }

    // case 1: one region in source, one region in target:
    if ((ss1 && !ss2) && (ts1 && !ts2))
    {
        traits::copy(sd1, ss1, td1);
    }
    // case 2: two regions in source, one region in target:
    if ((ss1 &&  ss2) && (ts1 && !ts2))
    {
        traits::copy(sd1, ss1, td1);
        traits::copy(sd2, ss2, td1 + ss1);
    }
    // case 3: one region in source, two regions in target:
    if ((ss1 && !ss2) && (ts1 &&  ts2))
    {
        traits::copy(sd1,       ts1, td1);
        traits::copy(sd1 + ts1, ts2, td2);
    }
    // case 4: two regions in source, two regions in target:
    if ((ss1 &&  ss2) && (ts1 &&  ts2))
    {
        if (ss1 > ts1)
        {
            // copy source region 1:
            traits::copy(sd1,       ts1, td1);
            traits::copy(sd1 + ts1, ss1 - ts1, td2);
            // copy source region 2:
            traits::copy(sd2,  ss2, td2 + (ss1 - ts1));
        }
        else
        {
            // copy source region 1:
            traits::copy(sd1, ss1, td1);
            // copy source region 2:
            traits::copy(sd2,   ts1 - ss1,  td1 + ss1);
            traits::copy(sd2 + (ts1 - ss1), ts2,  td2);
        }
    }

    // update read and write pointers:
      this->consume(size);
    target->produce(size);
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::describe_read(
    size_t    amount,
    uint8_t **out_region1_data,
    size_t   *out_region1_size,
    uint8_t **out_region2_data,
    size_t   *out_region2_size)
{
    uint32_t  rd_ofs      = offset_r;
    uint32_t  wr_ofs      = offset_w;
    uint32_t  available   = wr_ofs - rd_ofs;
    uint32_t  actual_rdo  = 0;
    uint32_t  bytes_left  = 0;
    uint32_t  tail_bytes  = 0;
    uint8_t  *src         = storage;

    if (amount > available)
    {
        // not enough data available to read.
        *out_region1_data = NULL;
        *out_region1_size = 0;
        *out_region2_data = NULL;
        *out_region2_size = 0;
        return false;
    }

    // prevent the compiler/processor from reordering reads and writes.
    processor::full_barrier();

    // compute the regions and sizes and return them.
    actual_rdo = rd_ofs & mask;
    bytes_left = (uint32_t) amount;
    tail_bytes = CMN_MIN(bytes_left, size - actual_rdo);
    if (bytes_left - tail_bytes > 0)
    {
        // the read is split across two regions.
        *out_region1_data = src + actual_rdo;
        *out_region1_size = tail_bytes;
        *out_region2_data = src;
        *out_region2_size = bytes_left - tail_bytes;
    }
    else
    {
        // the read is contained within one region.
        *out_region1_data = src + actual_rdo;
        *out_region1_size = tail_bytes;
        *out_region2_data = NULL;
        *out_region2_size = 0;
    }
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool channel_t::describe_write(
    size_t    amount,
    uint8_t **out_region1_data,
    size_t   *out_region1_size,
    uint8_t **out_region2_data,
    size_t   *out_region2_size)
{
    uint32_t  rd_ofs      = offset_r;
    uint32_t  wr_ofs      = offset_w;
    uint32_t  available   = size - (wr_ofs - rd_ofs);
    uint32_t  actual_wro  = 0;
    uint32_t  bytes_left  = 0;
    uint32_t  tail_bytes  = 0;
    uint8_t  *dst         = storage;

    if (amount > available)
    {
        // not enough space available in the buffer.
        *out_region1_data = NULL;
        *out_region1_size = 0;
        *out_region2_data = NULL;
        *out_region2_size = 0;
        return false;
    }

    // compute regions and sizes and return them. no memory barrier necessary.
    actual_wro = wr_ofs & mask;
    bytes_left = (uint32_t) amount;
    tail_bytes = CMN_MIN(bytes_left, size - actual_wro);
    if (bytes_left - tail_bytes > 0)
    {
        // the write is split across two regions.
        *out_region1_data = dst + actual_wro;
        *out_region1_size = tail_bytes;
        *out_region2_data = dst;
        *out_region2_size = bytes_left - tail_bytes;
    }
    else
    {
        // the write is contained within one region.
        *out_region1_data = dst + actual_wro;
        *out_region1_size = tail_bytes;
        *out_region2_data = NULL;
        *out_region2_size = 0;
    }
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
