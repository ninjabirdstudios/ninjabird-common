/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines an abstraction on top of the processor management
/// functionality provided by the operating system, relying on either pthreads
/// or the equivalent Win32 APIs to create and manage threads, thread-local
/// storage and synchronization primitives.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/
#ifndef LIBPROCESSOR_HPP_INCLUDED
#define LIBPROCESSOR_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <cassert>
#include "common.hpp"
#include "common_traits.hpp"

#if   CMN_IS_APPLE
    #include <errno.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <mach/mach_time.h>
#elif CMN_IS_LINUX
    #include <time.h>
    #include <errno.h>
    #include <unistd.h>
    #include <pthread.h>
#elif CMN_IS_WINDOWS
    #define _WIN32_WINNT _WIN32_WINNT_VISTA
    #include <intrin.h>
    #include <process.h>
    #include <windows.h>
    #undef  _WIN32_WINNT
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace processor {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// Define compiler intrinsics for memory fences that prevent the compiler and
/// processor for reordering reads and writes.
#ifndef PROCESSOR_MFENCE_DEFINED
    #ifdef _MSC_VER
        #define PROCESSOR_MFENCE_READ         _ReadBarrier()
        #define PROCESSOR_MFENCE_WRITE        _WriteBarrier()
        #define PROCESSOR_MFENCE_READ_WRITE   _ReadWriteBarrier()
        #define PROCESSOR_MFENCE_DEFINED
    #endif /* defined(_MSC_VER) */
    #ifdef __GNUC__
      #if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
        #define PROCESSOR_MFENCE_READ         __sync_synchronize()
        #define PROCESSOR_MFENCE_WRITE        __sync_synchronize()
        #define PROCESSOR_MFENCE_READ_WRITE   __sync_synchronize()
        #define PROCESSOR_MFENCE_DEFINED
      #elif defined(__ppc__) || defined(__powerpc__) || defined(__PPC__)
        #define PROCESSOR_MFENCE_READ         asm volatile("sync":::"memory")
        #define PROCESSOR_MFENCE_WRITE        asm volatile("sync":::"memory")
        #define PROCESSOR_MFENCE_READ_WRITE   asm volatile("sync":::"memory")
        #define PROCESSOR_MFENCE_DEFINED
      #elif defined(__i386__) || defined(__i486__) || defined(__i586__) || \
            defined(__i686__) || defined(__x86_64__)
        #define PROCESSOR_MFENCE_READ         asm volatile("lfence":::"memory")
        #define PROCESSOR_MFENCE_WRITE        asm volatile("sfence":::"memory")
        #define PROCESSOR_MFENCE_READ_WRITE   asm volatile("mfence":::"memory")
        #define PROCESSOR_MFENCE_DEFINED
      #endif /* GNU C version */
    #endif /* defined(__GNUC__) */
#endif /* !defined(PROCESSOR_MFENCE_DEFINED) */

/// Alias for the PROCESSOR_MFENCE_READ_WRITE macro to force a full memory
/// barrier. See http://en.wikipedia.org/wiki/Memory_barrier
CMN_FORCE_INLINE void full_barrier (void) { PROCESSOR_MFENCE_READ_WRITE; }

/// Alias for the PROCESSOR_MFENCE_READ macro to force a memory barrier that
/// forces reads to complete at the point of the call. For more information,
/// see http://en.wikipedia.org/wiki/Memory_barrier
CMN_FORCE_INLINE void read_barrier (void) { PROCESSOR_MFENCE_READ;       }

/// Alias for the PROCESSOR_MFENCE_WRITE macro to force a memory barrier that
/// forces writes to complete at the point of the call. For more information,
/// see http://en.wikipedia.org/wiki/Memory_barrier
CMN_FORCE_INLINE void write_barrier(void) { PROCESSOR_MFENCE_WRITE;      }

/// Abstracts the system threading library to provide a base class
/// implementation for a thread. Inherit from this base class and override
/// the appropriate methods to implement your specific behavior.
class CMN_PUBLIC thread_t
{
public:
    /// The default stack size for a thread. A value of zero allows the runtime
    /// system to choose the thread stack size.
    static const size_t DEFAULT_STACK_SIZE = 0;

public:
    uint64_t       start_time;    /// absolute time of thread start, in ns.
    size_t         stack_size;    /// thread stack size, specified in bytes.
#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_t      thread_object; /// the pthreads thread identifier.
    pthread_attr_t attributes;    /// attributes assigned to the thread object.
#elif CMN_IS_WINDOWS
    HANDLE         thread_object; /// Win32 thread handle.
#else
    #error No implementation of processor::thread_t for your platform!
#endif

public:
    /// Default constructor. The thread does not start until the
    /// thread_t::start() method is called.
    thread_t(void);

    /// Type destructor. Thread implementations override this destructor to
    /// release implementation resources.
    virtual ~thread_t(void);

public:
    /// Creates and starts the thread.
    ///
    /// @param thread_stack_size The thread stack size, in bytes. Specify zero
    /// or thread_t::DEFAULT_STACK_SIZE to allow the runtime system to select
    /// the thread stack size.
    /// @return true if the thread was created successfully.
    bool  start(size_t thread_stack_size = thread_t::DEFAULT_STACK_SIZE);

    /// Yields the remainder of calling thread's current timeslice.
    void  yield(void);

    /// Puts the calling thread to sleep for at least the specified number of
    /// microseconds.
    ///
    /// @param microseconds The number of microseconds to sleep for.
    void  sleep(uint64_t microseconds);

    /// Blocks the calling thread until the thread terminates.
    ///
    /// @return The return value of the thread_t::run() method.
    void* join(void);

public:
    /// Allows implementations to initialize the values of any thread-local
    /// data. Thread-local storage slots must be registered by the calling
    /// process before threads are created. The default implementation is
    /// empty and returns true. This method executes when the thread is
    /// started, before the thread_t::run() method executes.
    ///
    /// @return true if thread local storage was successfully initialized.
    virtual bool  create_thread_locals(void);

    /// Allows implementations to initialize any local resources, not including
    /// thread-local storage data. This method is called before the run method
    /// executes, after thread_t::create_thread_locals() returns successfully.
    /// The default implementation is empty.
    ///
    /// @return true if thread resources were successfully initialized.
    virtual bool  startup(void);

    /// Allows implementations to release any resources allocated by the
    /// thread, not including thread-local storage data. This method is called
    /// when the thread terminates, before thread_t::delete_thread_locals().
    /// The default implementation is empty.
    virtual void  cleanup(void);

    /// Allows implementations to release any resources assigned to thread-
    /// local storage slots. The default implementation is empty. This method
    /// executes after the thread_t::run() method returns.
    virtual void  delete_thread_locals(void);

    /// Allows implementations to implement the thread's main loop. The default
    /// implementation immediately returns zero/NULL.
    ///
    /// @return The result of thread execution.
    virtual void* run(void);
};

/// Abstracts the system threading library to provide a condition variable
/// primitive. All methods are implemented inline.
class CMN_PUBLIC condition_t
{
public:

#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_mutex_t    mutex;
    pthread_cond_t     condition;
#elif CMN_IS_WINDOWS
    CRITICAL_SECTION   mutex;
    CONDITION_VARIABLE condition;
#else
    #error  No implementation of processor::condition_t for your platform!
#endif

public:
    /// Constructs a new condition instance initialized with the default
    /// attributes (non-recursive mutex.)
    inline condition_t(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&condition, NULL);
#elif CMN_IS_WINDOWS
        InitializeCriticalSection(&mutex);
        InitializeConditionVariable(&condition);
#else
        #error No implementation of condition_t::condition_t() for your platform!
#endif
    }

    /// Destroys the underlying synchronization primitives.
    inline ~condition_t(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_cond_destroy(&condition);
        pthread_mutex_destroy(&mutex);
#elif CMN_IS_WINDOWS
        DeleteCriticalSection(&mutex);
#else
        #error No implementation of condition_t::~condition_t() for your platform!
#endif
    }

public:
    /// Attempts to acquire ownership of the mutex protecting the shared data,
    /// blocking the calling thread indefinitely if the mutex is not available.
    ///
    /// @return true if ownership of the mutex was granted.
    inline bool lock(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        return (0 == pthread_mutex_lock(&mutex));
#elif CMN_IS_WINDOWS
        EnterCriticalSection(&mutex);
        return true;
#else
        #error No implementation of condition_t::lock() for your platform!
#endif
    }

    /// Waits for the condition to become signalled, blocking the calling
    /// thread indefinitely, while atomically releasing the associated mutex.
    /// The mutex must currently be held by the calling thread.
    inline void wait(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_cond_wait(&condition, &mutex);
#elif CMN_IS_WINDOWS
        SleepConditionVariableCS(&condition, &mutex, INFINITE);
#else
        #error No implementation of condition_t::wait() for your platform!
#endif
    }

    /// Signals that the condition was met and wakes up at least one thread
    /// waiting on the condition, if any threads are waiting.
    inline void wake_one(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_cond_signal(&condition);
#elif CMN_IS_WINDOWS
        WakeConditionVariable(&condition);
#else
        #error No implementation of condition_t::wake_one() for your platform!
#endif
    }

    /// Signals all waiting threads that the condition was met.
    inline void wake_all(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_cond_broadcast(&condition);
#elif CMN_IS_WINDOWS
        WakeAllConditionVariable(&condition);
#else
        #error No implementation of condition_t::wake_all() for your platform!
#endif
    }

    /// Relinquishes ownership of the mutex protecting the shared data.
    inline void unlock(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_unlock(&mutex);
#elif CMN_IS_WINDOWS
        LeaveCriticalSection(&mutex);
#else
        #error No implementation of condition_t::unlock() for your platform!
#endif
    }
};

/// Abstracts the system threading library to provide an unnamed mutex
/// primitive. All methods are implemented inline.
class CMN_PUBLIC mutex_t
{
public:

#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_mutex_t  sync;
#elif CMN_IS_WINDOWS
    CRITICAL_SECTION sync;
#else
    #error No implementation of processor::mutex_t for your platform!
#endif

public:
    /// Constructs a new mutex instance initialized with the default
    /// mutex attributes (non-recursive.)
    inline mutex_t(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_init(&sync, NULL);
#elif CMN_IS_WINDOWS
        InitializeCriticalSection(&sync);
#else
        #error No implementation of mutex_t::mutex_t() for your platform!
#endif
    }

    /// Destroys the underlying synchronization primitive.
    inline ~mutex_t(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_destroy(&sync);
#elif CMN_IS_WINDOWS
        DeleteCriticalSection(&sync);
#else
        #error No implementation of mutex_t::~mutex_t() for your platform!
#endif
    }

public:
    /// Blocks the calling thread indefinitely while waiting to acquire
    /// ownership of the mutex.
    ///
    /// @return true if ownership of the mutex was granted.
    inline bool lock(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        return (0 == pthread_mutex_lock(&sync));
#elif CMN_IS_WINDOWS
        EnterCriticalSection(&sync);
        return true;
#else
        #error No implementation of mutex_t::lock() for your platform!
#endif
    }

    /// Attempts to acquire ownership of the mutex.
    ///
    /// @return true if ownership of the mutex was granted.
    inline bool try_lock(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        return (0 == pthread_mutex_trylock(&sync));
#elif CMN_IS_WINDOWS
        return (TRUE == TryEnterCriticalSection(&sync));
#else
        #error No implementation of mutex_t::try_lock() for your platform!
#endif
    }

    /// Relinquishes ownership of the mutex.
    inline void unlock(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_unlock(&sync);
#elif CMN_IS_WINDOWS
        LeaveCriticalSection(&sync);
#else
        #error No implementation of mutex_t::unlock() for your platform!
#endif
    }
};

/// Abstracts the system threading library to provide a waitable event
/// primitive. All methods are implemented inline.
class CMN_PUBLIC signal_t
{
public:

#if   CMN_IS_APPLE || CMN_IS_LINUX
    pthread_mutex_t mutex;
    pthread_cond_t  signal;
    bool            signal_flag;
#elif CMN_IS_WINDOWS
    HANDLE          signal;      /// Handle used to signal the event status.
#else
    #error No implementation of processor::signal_t for your platform!
#endif

public:
    /// Constructs a new instance with the specified initial state.
    ///
    /// @param initially_signalled true to create the waitable object in a
    /// signalled state.
    inline signal_t(bool initially_signalled = false)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        signal_flag  = initially_signalled;
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&signal, NULL);
#elif CMN_IS_WINDOWS
        BOOL state = initially_signalled ? TRUE : FALSE;
        signal     = CreateEvent(NULL, TRUE, state, NULL);
#else
        #error No implementation of signal_t::signal_t() for your platform!
#endif
    }

    /// Type destructor.
    inline ~signal_t(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_cond_destroy(&signal);
        pthread_mutex_destroy(&mutex);
#elif CMN_IS_WINDOWS
        if (signal != NULL)
        {
            CloseHandle(signal);
            signal  = NULL;
        }
#else
        #error No implementation of signal_t::~signal_t() for your platform!
#endif
    }

public:
    /// Sets the waitable object state to signalled.
    inline void set(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_lock(&mutex);
        signal_flag = true;
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&signal);
#elif CMN_IS_WINDOWS
        SetEvent(signal);
#else
        #error No implementation of signal_t::set() for your platform!
#endif
    }

    /// Sets the waitable object state to unsignalled.
    inline void reset(void)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_lock(&mutex);
        signal_flag = false;
        pthread_mutex_unlock(&mutex);
#elif CMN_IS_WINDOWS
        ResetEvent(signal);
#else
        #error No implementation of signal_t::reset() for your platform!
#endif
    }

    /// Blocks the calling thread until the waitable object state becomes
    /// signalled. This method will block the calling thread indefinitely
    /// until the waitable object becomes signalled. The waitable object
    /// state is reset to unsignalled.
    inline void wait()
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        pthread_mutex_lock(&mutex);
        while (!signal_flag)
        {
            // wait for the signal to be set.
            pthread_cond_wait(&signal, &mutex);
        }
        // reset the signal flag.
        signal_flag = false;
        pthread_mutex_unlock(&mutex);
#elif CMN_IS_WINDOWS
        WaitForSingleObject(signal, INFINITE);
#else
        #error No implementation of signal_t::wait() for your platform!
#endif
    }

    /// Blocks the calling thread until the waitable object state becomes
    /// signalled, or until the specified time interval has elapsed. If
    /// the waitable object was signalled, the waitable object state is reset
    /// to unsignalled.
    ///
    /// @param timeout The timeout value, specified in microseconds.
    /// @return true if the timeout interval has elapsed.
    inline bool timed_wait(uint64_t timeout)
    {
#if   CMN_IS_APPLE || CMN_IS_LINUX
        // @note: 1000000 converts usec to seconds.
        struct timespec    time_out = {0};
        uint64_t           wait_sec = time(NULL) + (timeout / 1000000);
        long               wait_ns  =(timeout % 1000000) * 1000;
        time_out.tv_sec  = wait_sec;
        time_out.tv_nsec = wait_ns;
        pthread_mutex_lock(&mutex);
        while (!signal_flag)
        {
            // wait for the signal to be set.
            int result = pthread_cond_timedwait(&signal, &mutex, &time_out);
            if (result == ETIMEDOUT)
            {
                pthread_mutex_unlock(&mutex);
                return true;
            }
        }
        pthread_mutex_unlock(&mutex);
        return false;
#elif CMN_IS_WINDOWS
        DWORD   millis = (DWORD) (timeout / 1000);
        DWORD   result = WaitForSingleObject(signal, millis);
        switch (result)
        {
            case WAIT_OBJECT_0:  return false;
            case WAIT_TIMEOUT:   return true;
            case WAIT_ABANDONED: return false;
            default:             break;
        }
        return false;
#else
        #error No implementation of signal_t::timed_wait() for your platform!
#endif
    }
};

/// Defines a template for performing lockless atomic operations on integral
/// types. Speciailizations are defined for 32- and 64-bit signed and unsigned
/// integer values.
template <typename T>
class atomic_t
{
private:
    /// The atomic integral value. Do not access this value directly.
    volatile T v;

public:
    /// Default constructor. The value is initially zero.
    atomic_t(void);

    /// Constructs a new instance with the specified initial value.
    /// @param initial_value The initial value of this instance.
    atomic_t(T initial_value);

public:
    /// Atomically reads the value stored in this instance.
    /// @return The value stored in this instance.
    T    load(void) volatile;

    /// Atomically set the value stored in this instance and return the
    /// previous value.
    ///
    /// @param new_value The value to store in this instance.
    /// @return The value previously stored in this instance.
    T    exchange(T new_value) volatile;

    /// Atomically reads the value, adds @a count to it, and stores the new
    /// value back in this instance.
    ///
    /// @param count The amount to add to the current value.
    /// @return The previous value.
    T    fetch_add(T count) volatile;

    /// Atomically reads the value, subtracts @a count from it, and stores the
    /// new value back in this instance.
    ///
    /// @param count The amount to subtract from the current value.
    /// @return The previous value.
    T    fetch_sub(T count) volatile;

    /// Atomically reads the value, bitwise-AND's @a value with it, and stores
    /// the new  value back in this instance.
    ///
    /// @param value The value to AND with the current value.
    /// @return The previous value.
    T    fetch_and(T value) volatile;

    /// Atomically reads the value, XOR's @a value with it, and stores the new
    /// value back in this instance.
    ///
    /// @param value The value to XOR with the current value.
    /// @return The previous value.
    T    fetch_xor(T value) volatile;

    /// Atomically reads the value, bitwise-OR's @a value with it, and stores
    /// the new  value back in this instance.
    ///
    /// @param value The value to OR with the current value.
    /// @return The previous value.
    T    fetch_or (T value) volatile;

    /// Atomically stores @a new_value into this instance.
    /// @param new_value The new value stored in this instance.
    void store(T new_value) volatile;

    /// Atomically compares the current value with @a expected, and if they
    /// are equal, stores @a new_value.
    ///
    /// @param expected The value this instance is expected to have.
    /// @param new_value The desired value to be stored into this instance.
    /// @return true if the current value matched @a expected and was updated
    /// with @a new_value.
    bool compare_exchange(T &expected, T new_value) volatile;

public:
    /// Atomically reads the value stored in this instance, so you can do:
    /// atomic_int32_t my_value(5);
    /// int32_t copy_of = my_value;
    operator T() volatile;

    /// Atomically stores @a new_value in this instance and returns it.
    ///
    /// @param new_value The value to store in this instance.
    /// @return @a new_value.
    CMN_FORCE_INLINE T operator = (T new_value) volatile
    {
        store (new_value);
        return new_value;
    }

    /// Atomically perform a post-increment operation.
    /// @return The value stored in this instance after the increment.
    CMN_FORCE_INLINE T operator ++(void) volatile
    {
        return fetch_add(1) + 1;
    }

    /// Atomically perform a pre-increment operation.
    /// @return The value stored in this instance before the increment.
    CMN_FORCE_INLINE T operator ++(int)  volatile
    {
        return fetch_add(1);
    }

    /// Atomically perform a pre-decrement operation.
    /// @return The value stored in this instance after the decrement.
    CMN_FORCE_INLINE T operator --(void) volatile
    {
        return fetch_sub(1) - 1;
    }

    /// Atomically perform a post-decrement operation.
    /// @return The return value is undefined.
    CMN_FORCE_INLINE T operator --(int)  volatile
    {
        return fetch_sub(1);
    }

    /// Performs an atomic increment and assign.
    ///
    /// @param value The amount to increment by.
    /// @return The new value.
    CMN_FORCE_INLINE T operator +=(T value) volatile
    {
        return fetch_add(value) + value;
    }

    /// Performs an atomic decrement and assign.
    ///
    /// @param value The amount to decrement by.
    /// @return The new value.
    CMN_FORCE_INLINE T operator -=(T value) volatile
    {
        return fetch_sub(value) - value;
    }

    /// Performs an atomic bitwise-AND and assign.
    ///
    /// @param value The value to AND with.
    /// @return The new value.
    CMN_FORCE_INLINE T operator &=(T value) volatile
    {
        return fetch_and(value) & value;
    }

    /// Performs an atomic bitwise-OR and assign.
    ///
    /// @param value The value to OR with.
    /// @return The new value.
    CMN_FORCE_INLINE T operator |=(T value) volatile
    {
        return fetch_or (value) | value;
    }

    /// Performs an atomic bitwise-XOR and assign.
    ///
    /// @param value The value to XOR with.
    /// @return The new value.
    CMN_FORCE_INLINE T operator ^=(T value) volatile
    {
        return fetch_xor(value) ^ value;
    }

private:
    atomic_t(processor::atomic_t<T> const &);
    processor::atomic_t<T>& operator = (processor::atomic_t<T> const &);
};

/// Implement atomic_t for GNU C++ compilers. The implementation is fairly
/// straightforward and does not require explicit specialization.
#ifdef __GNUC__
template <typename T>
CMN_FORCE_INLINE atomic_t<T>::atomic_t(void)
{
    // @note: __sync_lock_test_and_set is not a full barrier
    // so we issue a __sync_synchronize (full barrier) afterward.
    __sync_lock_test_and_set(&v, 0);
    __sync_synchronize();
}

template <typename T>
CMN_FORCE_INLINE atomic_t<T>::atomic_t(T initial_value)
{
    // @note: __sync_lock_test_and_set is not a full barrier
    // so we issue a __sync_synchronize (full barrier) afterward.
    __sync_lock_test_and_set(&v, initial_value);
    __sync_synchronize();
}

template <typename T>
CMN_FORCE_INLINE T atomic_t<T>::load(void) volatile
{
    return __sync_fetch_and_add((volatile T*)&v, 0);
}

template <typename T>
CMN_FORCE_INLINE T atomic_t<T>::exchange(T new_value) volatile
{
    T old_value = __sync_lock_test_and_set(&v, new_value);
    __sync_synchronize();
    return  old_value;
}

template <typename T>
CMN_FORCE_INLINE T atomic_t<T>::fetch_add(T count) volatile
{
    return __sync_fetch_and_add(&v, count);
}

template <typename T>
CMN_FORCE_INLINE T atomic_t<T>::fetch_sub(T count) volatile
{
    return __sync_fetch_and_sub(&v, count);
}

template <typename T>
CMN_FORCE_INLINE T atomic_t<T>::fetch_and(T value) volatile
{
    return __sync_fetch_and_and(&v, value);
}

template <typename T>
CMN_FORCE_INLINE T atomic_t<T>::fetch_xor(T value) volatile
{
    return __sync_fetch_and_xor(&v, value);
}

template <typename T>
CMN_FORCE_INLINE T atomic_t<T>::fetch_or(T value)  volatile
{
    return __sync_fetch_and_or (&v, value);
}

template <typename T>
CMN_FORCE_INLINE void atomic_t<T>::store(T new_value) volatile
{
    // @note: __sync_lock_test_and_set is not a full barrier
    // so we issue a __sync_synchronize (full barrier) afterward.
    __sync_lock_test_and_set(&v, new_value);
    __sync_synchronize();
}

template <typename T>
CMN_FORCE_INLINE bool atomic_t<T>::compare_exchange(
    T &expected,
    T  new_value)
    volatile
{
    T    actual  = __sync_val_compare_and_swap(&v, expected, new_value);
    bool success =(actual == expected);
    expected     = actual;
    return success;
}

template <typename T>
CMN_FORCE_INLINE atomic_t<T>::operator T() volatile
{
    return __sync_fetch_and_add((volatile T*)&v, 0);
}
#endif /* defined(__GNUC__) */

/// Implement atomic_t for Microsoft Visual C++ compilers. We must explicitly
/// specialize for each type we need to expose.
#ifdef _MSC_VER
template <> class atomic_t<int32_t>
{
private:
    /// The atomic integral value. Do not access this value directly.
    volatile int32_t v;

public:
    CMN_FORCE_INLINE atomic_t(void)
    {
        _InterlockedExchange(
            reinterpret_cast<volatile long*>(&v), 0);
    }
    CMN_FORCE_INLINE atomic_t(int32_t initial_value)
    {
        _InterlockedExchange(
            reinterpret_cast<volatile long*>(&v),
            initial_value);
    }

public:
    CMN_FORCE_INLINE int32_t load(void) volatile
    {
        return _InterlockedExchangeAdd(
            reinterpret_cast<volatile long*>(&v), 0);
    }
    CMN_FORCE_INLINE int32_t exchange(int32_t new_value) volatile
    {
        return _InterlockedExchange(
            reinterpret_cast<volatile long*>(&v), new_value);
    }
    CMN_FORCE_INLINE int32_t fetch_add(int32_t count) volatile
    {
        return _InterlockedExchangeAdd(
            reinterpret_cast<volatile long*>(&v),  count);
    }
    CMN_FORCE_INLINE int32_t fetch_sub(int32_t count) volatile
    {
        return _InterlockedExchangeAdd(
            reinterpret_cast<volatile long*>(&v), -count);
    }
    CMN_FORCE_INLINE int32_t fetch_and(int32_t value) volatile
    {
        return _InterlockedAnd(
            reinterpret_cast<volatile long*>(&v), value);
    }
    CMN_FORCE_INLINE int32_t fetch_xor(int32_t value) volatile
    {
        return _InterlockedXor(
            reinterpret_cast<volatile long*>(&v), value);
    }
    CMN_FORCE_INLINE int32_t fetch_or (int32_t value) volatile
    {
        return _InterlockedOr(
            reinterpret_cast<volatile long*>(&v), value);
    }
    CMN_FORCE_INLINE void store(int32_t new_value) volatile
    {
        _InterlockedExchange(
            reinterpret_cast<volatile long*>(&v), new_value);
    }
    CMN_FORCE_INLINE bool compare_exchange(int32_t &expected, int32_t new_value) volatile
    {
        int32_t actual  = _InterlockedCompareExchange(
            reinterpret_cast<volatile long*>(&v),
            new_value,
            expected);
        bool    success =(actual == expected);
        expected        = actual;
        return success;
    }
    CMN_FORCE_INLINE   operator int32_t() volatile
    {
        return _InterlockedExchangeAdd(
            reinterpret_cast<volatile long*>(&v), 0);
    }
};

#if defined(_M_X64) || defined(_M_IA64)
template <> /// Specialization for 64-bit signed integer values.
class atomic_t<int64_t>
{
private:
    /// The atomic integral value. Do not access this value directly.
    volatile int64_t v;

public:
    CMN_FORCE_INLINE atomic_t(void)
    {
        _InterlockedExchange64(
            reinterpret_cast<volatile int64_t*>(&v), 0);
    }
    CMN_FORCE_INLINE atomic_t(int64_t initial_value)
    {
        _InterlockedExchange64(
            reinterpret_cast<volatile int64_t*>(&v), initial_value);
    }

public:
    CMN_FORCE_INLINE int64_t load(void) volatile
    {
        return _InterlockedExchangeAdd64(
            reinterpret_cast<volatile int64_t*>(&v), 0);
    }
    CMN_FORCE_INLINE int64_t exchange(int64_t new_value) volatile
    {
        return _InterlockedExchange64(
            reinterpret_cast<volatile int64_t*>(&v), new_value);
    }
    CMN_FORCE_INLINE int64_t fetch_add(int64_t count) volatile
    {
        return _InterlockedExchangeAdd64(
            reinterpret_cast<volatile int64_t*>(&v),  count);
    }
    CMN_FORCE_INLINE int64_t fetch_sub(int64_t count) volatile
    {
        return _InterlockedExchangeAdd64(
            reinterpret_cast<volatile int64_t*>(&v), -count);
    }
    CMN_FORCE_INLINE int64_t fetch_and(int64_t value) volatile
    {
        return _InterlockedAnd64(
            reinterpret_cast<volatile int64_t*>(&v), value);
    }
    CMN_FORCE_INLINE int64_t fetch_xor(int64_t value) volatile
    {
        return _InterlockedXor64(
            reinterpret_cast<volatile int64_t*>(&v), value);
    }
    CMN_FORCE_INLINE int64_t fetch_or (int64_t value) volatile
    {
        return _InterlockedOr64(
            reinterpret_cast<volatile int64_t*>(&v), value);
    }
    CMN_FORCE_INLINE void store(int64_t new_value) volatile
    {
        _InterlockedExchange64(
            reinterpret_cast<volatile int64_t*>(&v), new_value);
    }
    CMN_FORCE_INLINE bool compare_exchange(int64_t &expected, int64_t new_value) volatile
    {
        int64_t actual  =_InterlockedCompareExchange64(
            reinterpret_cast<volatile int64_t*>(&v),
            new_value,
            expected);
        bool    success =(actual == expected);
        expected        = actual;
        return success;
    }
    CMN_FORCE_INLINE   operator int64_t() volatile
    {
        return _InterlockedExchangeAdd64(
            reinterpret_cast<volatile int64_t*>(&v), 0);
    }
};
#endif /* defined(_M_X64) || defined(_M_IA64) */
#endif /* defined(_MSC_VER) */

typedef atomic_t<int32_t>  atomic_int32_t;  /// Specialized for 32-bit signed

#if defined(__GNUC__) &&  defined(__x86_64__)
typedef atomic_t<int64_t>  atomic_int64_t;  /// Specialized for 32-bit signed
#endif /* defined(__GNUC__) &&  defined(__x86_64__) */
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IA64))
typedef atomic_t<int64_t>  atomic_int64_t;  /// Specialized for 64-bit signed
#endif /* defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IA64)) */

/// Defines a lock-free ring buffer implementation that is safe for concurrent
/// access by a single reader and a single writer. The implementation also
/// supports atomically writing messages defined by separate header and
/// payload structures, which makes the implementation suitable for passing
/// messages between threads.
class CMN_PUBLIC channel_t
{
public:
    uint8_t                                               *storage;
    uint32_t                                               size;
    uint32_t                                               mask;
    CMN_ALIGN_BEGIN(4) volatile uint32_t CMN_ALIGN_END(4)  offset_r;
    CMN_ALIGN_BEGIN(4) volatile uint32_t CMN_ALIGN_END(4)  offset_w;

public:
    /// Default constructor. Call channel_t::bind() before attempting
    /// to access the buffer.
    channel_t(void);

    /// Constructs a new instance with at least the specified capacity. This
    /// constructor is equivalent to calling the default constructor followed
    /// by channel_t::bind().
    ///
    /// @param buffer Pointer to the externally-managed memory block that will
    /// be used to store data written to the channel.
    /// @param size_in_bytes The size of @a buffer, in bytes. This value must
    /// be a power-of-two.
    channel_t(void *buffer, size_t size_in_bytes);

    /// Type destructor. Frees storage and address space reserved for this
    /// buffer instance.
    ~channel_t(void);

public:
    /// Gets the number of bytes of address space that have been reserved and
    /// committed for this channel.
    ///
    /// @return The number of bytes of address space committed to this channel.
    /// This value will always be an even multiple of the system page size.
    inline size_t bytes_committed(void) const
    {
        return size;
    }

    /// Gets the number of bytes available on the channel for reading at the
    /// current instant.
    ///
    /// @return The number of bytes available to be read.
    inline size_t bytes_available(void) const
    {
        return (offset_w - offset_r);
    }

    /// Manually advances the read pointer by the specified amount. This
    /// function should be called after directly accessing the buffer for
    /// a read operation, typically using channel_t::describe_read().
    /// Do not call this function when using standard read calls. This method
    /// is useful when copying between event buffer instances. This method
    /// does not enforce boundary conditions.
    ///
    /// @param amount The number of bytes of input consumed by the caller.
    inline void consume(size_t amount)
    {
        // @note: no memory barrier necessary here.
        offset_r += (uint32_t) amount;
    }

    /// Manually advances the write pointer by the specified amount This
    /// function should be called after directly accessing the buffer for a
    /// write operation, typically using channel_t::describe_write().
    /// Do not call this function when using standard write calls. This method
    /// does not enforce boundary conditions.
    ///
    /// @param amount The number of bytes of output produced by the caller.
    inline void produce(size_t amount)
    {
        // force all pending reads or writes to complete.
        processor::read_barrier();
        offset_w += (uint32_t) amount;
    }

public:
    /// Binds the channel instance to an externally-managed memory block.
    ///
    /// @param buffer Pointer to the externally-managed memory block that will
    /// be used to store data written to the channel.
    /// @param size_in_bytes The size of @a buffer, in bytes. This value must
    /// be a power-of-two.
    /// @return true if @a buffer and @a size_in_bytes meet all of the
    /// necessary alignment and size criteria.
    bool  bind(void *buffer, size_t size_in_bytes);

    /// Unbinds this allocator instance from the current externally-managed
    /// memory block. Do not attempt to read from or write to the channel until
    /// the channel_t::bind() method is called again.
    ///
    /// @param out_size_in_bytes If non-NULL, this location is updated with
    /// the size of the externally-managed memory block, specified in bytes.
    /// @return A pointer to the externally-managed memory block.
    void*  unbind(size_t *out_size_in_bytes = NULL);

public:
    /// Attempts to read the specified amount of data from the channel. The
    /// operation succeeds only if the requested amount of data can be read.
    ///
    /// @param output The output buffer to which data will be copied.
    /// @param amount The number of bytes to read.
    /// @return true if the specified amount of data was copied into @a output.
    bool read(void *output, size_t amount);

    /// Attempts to write the specified amount of data to the channel. The
    /// operation succeeds only if the requested amount of data can be written.
    ///
    /// @param source The buffer from which data will be copied.
    /// @param amount The number of bytes to copy from source into the buffer.
    /// @return true if the specified amount of data was copied to the buffer.
    bool write(void const *source, size_t amount);

    /// Attempts to write a message (consisting of a header and a payload) to
    /// the buffer. The operation succeeds only if the specified amount of data
    /// can be written. The message is written atomically, so that the reader
    /// will never see a partial message. The reader can read the message with
    /// multiple read operations.
    ///
    /// @param header Pointer to the message header structure. This value
    /// cannot be null.
    /// @param header_size The size of the message header, in bytes. This value
    /// must be greater than zero.
    /// @param payload Pointer to the message payload data. This value is
    /// optional and may be null.
    /// @param payload_size The size of the message payload data, in bytes.
    bool write_message(
        void const *header,
        size_t      header_size,
        void const *payload,
        size_t      payload_size);

    /// Reads the entire block of available data from this buffer, and writes
    /// it to the end of another buffer. The read operation consumes all
    /// available bytes from this buffer. The write operation appends this
    /// data to the target buffer. This method is useful when producing
    /// composite event buffers. The operation succeeds only if the entire
    /// buffer contents can be written.
    ///
    /// @param target Pointer to the target buffer. This cannot be the same as
    /// the source buffer.
    /// @return true if the buffer contents were moved successfully.
    bool move_all(processor::channel_t *target);

    /// Reads a block of available data from this channel, and appends it to
    /// another channel. The read operation consumes available bytes from this
    /// channel. The write operation appends this data to the target channel.
    /// This method is useful when producing composite channels. The operation
    /// succeeds only if the entire requested region can be written.
    ///
    /// @param size_in_bytes The amount of data to move from this buffer into
    /// the target buffer.
    /// @param target Pointer to the target channel. This cannot be the same as
    /// the source buffer.
    /// @return true if the buffer contents were moved successfully.
    bool move_data(size_t size_in_bytes, processor::channel_t *target);

    /// Describes the region(s) of data that would be accessed during a read
    /// operation. Reads of up to two separate memory regions may be required.
    /// This function does not actually read any data, nor does it advance the
    /// read pointer. It is typically used when copying data from one channel
    /// to another, where the caller would call describe_read passing the
    /// source channel_t::bytes_available() for @a amount, copy the memory
    /// using memcpy, and finally call channel_t::consume().
    ///
    /// @param amount The amount of data that would be read, in bytes.
    /// @param out_region1_data On return, this location is updated to point to
    /// the start of the first region containing the data that would be read.
    /// This parameter cannot be null. If @a amount bytes are not available to
    /// read, this value is set to null.
    /// @param out_region1_size On return, this location is updated with the
    /// size of the first data region, in bytes. This parameter cannot be null.
    /// If @a amount bytes are not available to read, this value is set to 0.
    /// @param out_region2_data On return, this location is updated to point to
    /// the start of the second region containing the data that would be
    /// read, if the data is split across buffer boundaries. This parameter
    /// cannot be null. If @a amount bytes are not available to read, or the
    /// read would not cross buffer boundaries, then this value is set to null.
    /// @param out_region2_size On return this location is updated with the
    /// size of the second data region, in bytes, if the data is split across
    /// buffer boundaries. This parameter cannot be null. If @a amount bytes
    /// are not available to read, or the read would not cross buffer
    /// boundaries, this value is set to 0.
    /// @return true if @a amount bytes are available to read.
    bool describe_read(
        size_t    amount,
        uint8_t **out_region1_data,
        size_t   *out_region1_size,
        uint8_t **out_region2_data,
        size_t   *out_region2_size);

    /// Describes the region(s) of data that would be accessed during a write
    /// operation. Writes to up to two separate memory regions may be required.
    /// This function does not actually write any data, nor does it advance the
    /// write pointer. It is typically used when copying data from one channel
    /// to another, where the caller would call describe_write passing the
    /// source channel_t::bytes_available() for @a amount, copy the memory
    /// using memcpy, and finally call channel_t::produce().
    ///
    /// @param amount The amount of data that would be written, in bytes.
    /// @param out_region1_data On return, this location is updated to point to
    /// the start of the first region containing the data that would be
    /// written. This parameter cannot be null. If @a amount bytes are not
    /// available, this value is set to null.
    /// @param out_region1_size On return, this location is updated with the
    /// size of the first data region, in bytes. This parameter cannot be null.
    /// If @a amount bytes are not available, this value is set to 0.
    /// @param out_region2_data On return, this location is updated to point to
    /// the start of the second region containing the data that would be
    /// written, if the data is split across buffer boundaries. This parameter
    /// cannot be null. If @a amount bytes are not available, or the write
    /// would not cross buffer boundaries, then this value is set to null.
    /// @param out_region2_size On return this location is updated with the
    /// size of the second data region, in bytes, if the data is split across
    /// buffer boundaries. This parameter cannot be null. If @a amount bytes
    /// are not available, or the write would not cross buffer boundaries, this
    /// value is set to 0.
    /// @return true if @a amount bytes are available in the buffer.
    bool describe_write(
        size_t    amount,
        uint8_t **out_region1_data,
        size_t   *out_region1_size,
        uint8_t **out_region2_data,
        size_t   *out_region2_size);

public:
    /// Attempts to write a message (consisting of a header only) to the
    /// buffer. The operation succeeds only if the specified amount of data
    /// can be written. The message is written atomically, so that the reader
    /// will never see a partial message. The reader can read the message with
    /// multiple read operations.
    ///
    /// @param header Pointer to the message header structure. This value
    /// cannot be null.
    template <typename Header>
    inline bool write_message(Header const *header)
    {
        return write_message(header, sizeof(Header), NULL, 0);
    }

    /// Attempts to write a message (consisting of a header and a payload) to
    /// the buffer. The operation succeeds only if the specified amount of data
    /// can be written. The message is written atomically, so that the reader
    /// will never see a partial message. The reader can read the message with
    /// multiple read operations.
    ///
    /// @param header Pointer to the message header structure. This value
    /// cannot be null.
    /// @param payload Pointer to the message payload data. This value is
    /// optional and may be null.
    template <typename Header, typename Payload>
    inline bool write_message(Header const *header, Payload const *payload)
    {
        return write_message(header, sizeof(Header), payload, sizeof(Payload));
    }

private:
    channel_t(channel_t const &other);
    channel_t& operator =(channel_t const &other);
};

/// Gets the current system timestamp, specified in nanoseconds.
///
/// @return The current absolute system timestamp, in nanoseconds.
CMN_PUBLIC uint64_t current_time(void);

/// Creates a new Thread-Local-Storage slot identifier. This identifier is
/// used by all threads in the system to access the thread-specific value
/// associated with the identifier. This function should be called at startup
/// time to create storage identifiers before any threads are created.
///
/// @param out_id On return, this location is updated with the unique
/// identifier that can be used to access the storage location on a per-thread
/// basis.
/// @return true if the slot identifier was allocated successfully.
CMN_PUBLIC bool  create_tls_slot(size_t *out_id);

/// Deletes an existing Thread-Local-Storage slot identifier. This function
/// should be called at process exist time to delete storage identifiers after
/// all threads have exited.
///
/// @param tls_id The slot identifier, as returned by the function
/// processor::create_tls_slot(), to be deleted.
CMN_PUBLIC void  delete_tls_slot(size_t tls_id);

/// Retrieves the pointer-sized value associated with a Thread-Local-Storage
/// slot identifier on the calling thread.
///
/// @param tls_id The slot identifier, as returned by the function
/// processor::create_tls_slot().
/// @return The pointer-sized value stored at the specific slot index. A value
/// of NULL may be returned, but this does not necessarily indicate an error.
CMN_PUBLIC void* threadlocal_get_direct(size_t tls_id);

/// Stores a pointer-sized value in the Thread-Local-Storage slot identifier
/// for the calling thread.
///
/// @param tls_id The slot identifier, as returned by the function
/// processor::create_tls_slot().
/// @param value The pointer-sized value to associate with the slot for the
/// calling thread. This value is not visible to other threads.
/// @return true if the value was set successfully.
CMN_PUBLIC bool  threadlocal_set_direct(size_t tls_id, void const *value);

/// Retrieves the value associated with a Thread-Local-Storage slot identifier
/// for the calling thread.
///
/// @param tls_id The slot identifier, as returned by the function
/// processor::create_tls_slot().
/// @return The value stored at the specific slot index.
template <typename T>
inline T threadlocal_get(size_t tls_id)
{
    return reinterpret_cast<T>(threadlocal_get_direct(tls_id));
}

/// Stores a value in the Thread-Local-Storage slot identifier for the
/// calling thread. The value to be stored must be no larger than the size of
/// a pointer.
///
/// @param tls_id The slot identifier, as returned by the function
/// processor::create_tls_slot().
/// @param value The pointer-sized value to associate with the slot for the
/// calling thread. This value is not visible to other threads.
/// @return true if the value was set successfully.
template <typename T>
inline bool threadlocal_set(size_t tls_id, T const &value)
{
    assert(sizeof(T) <= sizeof(void*));
    return threadlocal_set_direct(tls_id, reinterpret_cast<void*>(value));
}

/// Converts an absolute time value specified in seconds to the corresponding
/// time value specified in microseconds.
///
/// @param seconds The time value, in seconds.
/// @return The corresponding time value, in microseconds.
inline uint64_t seconds_to_microseconds(double seconds)
{
    // one million microseconds in one second.
    return ((uint64_t)(seconds * 1000000.0));
}

/// Converts an absolute time value specified in microseconds to the
/// corresponding time value specified in seconds.
///
/// @param microseconds The time value, in microseconds.
/// @return The corresponding time value, in seconds.
inline double microseconds_to_seconds(uint64_t microseconds)
{
    // one million microseconds in one second.
    return microseconds * 0.000001;
}

/// Converts an absolute time value specified in seconds to the corresponding
/// time value specified in nanoseconds.
///
/// @param seconds The time value, in seconds.
/// @return The corresponding time value, in nanoseconds.
inline uint64_t seconds_to_nanoseconds(double seconds)
{
    // one billion nanoseconds in one second.
    return ((uint64_t)(seconds * 1000000000.0));
}

/// Converts an absolute time value specified in nanoseconds to the
/// corresponding time value specified in seconds.
///
/// @param nanoseconds The time value, in nanoseconds.
/// @return The corresponding time value, in seconds.
inline double nanoseconds_to_seconds(uint64_t nanoseconds)
{
    // one billion nanoseconds in one second.
    return nanoseconds * 0.000000001;
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace processor */

#endif /* LIBPROCESSOR_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
