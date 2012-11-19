/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: common_traits.hpp
///  Uses template metaprogramming magic to define type traits for the basic
///  built-in types. This information can be used to determine the alignment
///  of specific types and to optimize various operations such as moving and
///  copying data.
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef CMN_COMMON_TRAITS_HPP_INCLUDED
#define CMN_COMMON_TRAITS_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <assert.h>
#include <string.h>
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace traits {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// A helper type used when determining the native alignment of a type T.
/// The first member is a character, which forces the compiler to add padding
/// and allows us to determine the native alignment of the type.
template <typename T>
struct alignof_helper
{
    char x; /// Force a mis-alignment for type T.
    T    y; /// An instance of type T, used to determine its native alignment.
    /// Default constructor. This must be implemented explicitly for type T
    /// that are complex, that is, that implement their own constructor.
    alignof_helper(void) { /* empty */ }
};

/// A 16-byte value forced to use 16-byte alignment.
CMN_ALIGN_BEGIN(16) struct aligned16_t
{
    uint64_t member[2]; /// A 16-byte value.
} CMN_ALIGN_END(16);

/// Given a desired alignment value, provides a type with the specified
/// alignment. Specialized for alignments 0, 1, 2, 4, 8 and 16-bytes.
template <size_t N>
struct type_with_alignment
{
    /// Invalid alignment will result in an array with size -1,
    /// which is not allowed and will result in an error.
    typedef char ERROR_Invalid_Alignment[N>0?-1:1];
};
template<> struct type_with_alignment<0>  { /* empty */         };
template<> struct type_with_alignment<1>  { uint8_t     member; };
template<> struct type_with_alignment<2>  { uint16_t    member; };
template<> struct type_with_alignment<4>  { uint32_t    member; };
template<> struct type_with_alignment<8>  { uint64_t    member; };
template<> struct type_with_alignment<16> { aligned16_t member; };

/// Can be used to query the native alignment of a particular type. Used like
/// so: traits::alignof<my_type>::value and value will be the alignment of
/// the type my_type.
template <typename T>
struct alignof
{
    enum
    {
        value = offsetof(struct traits::alignof_helper<T>, y)
    };
};

/// Select one type or another based on a compile-time boolean flag.
/// Result evaluates to T if flag is true; F if flag is false.
template <bool flag, typename T, typename F>
struct select
{
    typedef T result;
};
template <typename T, typename F>
struct select<false, T, F>
{
    typedef F result;
};
template <typename T, typename F>
struct select<true,  T, F>
{
    typedef T result;
};

/// Converts an integer value into an enumerated type.
template <int Value>
struct int_to_type
{
    enum
    {
        value = Value
    };
};

/// Converts one type to another type.
template <typename T>
struct type_to_type
{
    typedef T original_type;
};

/// Can be used to query whether or not a type is a built-in type.
/// The default is false; specializations override for built-ins.
/// Used like so: traits::is_integral<my_type>::value and value will
/// be either true or false.
template <typename T>
struct is_integral
{
    enum { value = false };
};
template<> struct is_integral<int8_t>   { enum { value = true }; };
template<> struct is_integral<uint8_t>  { enum { value = true }; };
template<> struct is_integral<int16_t>  { enum { value = true }; };
template<> struct is_integral<uint16_t> { enum { value = true }; };
template<> struct is_integral<int32_t>  { enum { value = true }; };
template<> struct is_integral<uint32_t> { enum { value = true }; };
template<> struct is_integral<int64_t>  { enum { value = true }; };
template<> struct is_integral<uint64_t> { enum { value = true }; };
template<> struct is_integral<wchar_t>  { enum { value = true }; };
template<> struct is_integral<bool>     { enum { value = true }; };

/// Can be used to query whether or not a type is a floating-point type.
/// The default is false; specializations override for built-ins.
/// Used like so: traits::is_float<my_type>::value and value will be
/// either true or false.
template <typename T>
struct is_float
{
    enum { value = false };
};
template<> struct is_float<float>       { enum { value = true }; };
template<> struct is_float<double>      { enum { value = true }; };

/// Can be used to query whether or not a type is a blittable type, that is,
/// whether or not it can be memcpy'd. The default is false; specializations
/// override for built-ins. Used like so: traits::is_pod<my_type>::value and
/// value will be either true or false. Provide additional specializations for
/// custom types to optimize move/copy operations.
template <typename T>
struct is_pod
{
    enum { value = false };
};
template<> struct is_pod<int8_t>        { enum { value = true }; };
template<> struct is_pod<uint8_t>       { enum { value = true }; };
template<> struct is_pod<int16_t>       { enum { value = true }; };
template<> struct is_pod<uint16_t>      { enum { value = true }; };
template<> struct is_pod<int32_t>       { enum { value = true }; };
template<> struct is_pod<uint32_t>      { enum { value = true }; };
template<> struct is_pod<int64_t>       { enum { value = true }; };
template<> struct is_pod<uint64_t>      { enum { value = true }; };
template<> struct is_pod<float>         { enum { value = true }; };
template<> struct is_pod<double>        { enum { value = true }; };
template<> struct is_pod<wchar_t>       { enum { value = true }; };
template<> struct is_pod<bool>          { enum { value = true }; };

/// A helper structure that can be used to query basic information about a
/// particular type on the current runtime platform.
template <typename T>
struct type_traits
{
private:
    /// typedefs for use with sizeof:
    typedef char (&yes)[1];
    typedef char (&no) [2];

private:
    /// no implementation because sizeof does not require one.

    /// is reference type:
    template <typename U>
    static yes is_ref(type_to_type<U&>);
    static no  is_ref(...);

    /// is pointer type:
    template <typename U>
    static yes is_ptr(type_to_type<U*>);
    static no  is_ptr(...);

    /// is const-pointer type:
    template <typename U>
    static yes is_cptr(type_to_type<U const*>);
    static no  is_cptr(...);

    /// is volatile-pointer type:
    template <typename U>
    static yes is_vptr(type_to_type<U volatile *>);
    static no  is_vptr(...);

    /// is const volatile-pointer type:
    template <typename U>
    static yes is_cvptr(type_to_type<U const volatile *>);
    static no  is_cvptr(...);

    /// is class member pointer type:
    template <typename U, typename V>
    static yes is_mptr(type_to_type<U V::*>);
    static no  is_mptr(...);

    /// is const type:
    template <typename U>
    static yes is_ctype(type_to_type<const U>);
    static no  is_ctype(...);

    /// is volatile type:
    template <typename U>
    static yes is_vtype(type_to_type<volatile U>);
    static no  is_vtype(...);

public:
    enum { is_reference = (sizeof(is_ref  (type_to_type<T>()))==sizeof(yes)) };
    enum { is_pointer   = (sizeof(is_ptr  (type_to_type<T>()))==sizeof(yes)) ||
                          (sizeof(is_cptr (type_to_type<T>()))==sizeof(yes)) ||
                          (sizeof(is_vptr (type_to_type<T>()))==sizeof(yes)) ||
                          (sizeof(is_cvptr(type_to_type<T>()))==sizeof(yes)) };
    enum { is_member_ptr= (sizeof(is_mptr (type_to_type<T>()))==sizeof(yes)) };
    enum { is_const     = (sizeof(is_ctype(type_to_type<T>()))==sizeof(yes)) };
    enum { is_volatile  = (sizeof(is_vtype(type_to_type<T>()))==sizeof(yes)) };

public:
    /// used to select type T if T is a reference type; otherwise T&:
    typedef typename select<is_reference, T, T&>::result  reference_type;
    /// used to select type T if T is a const type; otherwise T const:
    typedef typename select<is_const, T, T const>::result const_type;
};

/// Can be used to query whether or not a type is a fundamental (built-in)
/// type. Use like so: traits::is_fundamental<my_type>::value and value will
/// be either true or false.
template <typename T>
struct is_fundamental
{
    enum
    {
        value = is_integral<T>::value || is_float<T>::value
    };
};

/// Can be used to query whether or not a type has a trivial constructor
/// (basically no constructor at all.) Use like so:
/// traits::has_trivial_ctor<my_type>::value and value will be either true
/// or false. Specialize for custom types.
template <typename T>
struct has_trivial_ctor
{
    enum
    {
        value =
            is_fundamental<T>::value ||
            is_pod<T>::value         ||
            type_traits<T>::is_pointer
    };
};

/// Can be used to query whether or not a type has a trivial destructor
/// (basically no destructor at all.) Use like so:
/// traits::has_trivial_dtor<my_type>::value and value will be either true
/// or false. Specialize for custom types.
template <typename T>
struct has_trivial_dtor
{
    enum
    {
        value =
            is_fundamental<T>::value ||
            is_pod<T>::value         ||
            type_traits<T>::is_pointer
    };
};

/// Can be used to query whether or not a type has a trivial copy method,
/// and can be memcpy'd. Use like so:
/// traits::has_trivial_copy<my_type>::value and value will be either true
/// or false. Specialize for custom types.
template <typename T>
struct has_trivial_copy
{
    enum
    {
        value =
            is_fundamental<T>::value ||
            is_pod<T>::value         ||
            type_traits<T>::is_pointer
    };
};

/// Can be used to query whether or not a type has a trivial assignment method,
/// and can be memcpy'd. Use like so:
/// traits::has_trivial_assign<my_type>::value and value will be either true
/// or false. Specialize for custom types.
template <typename T>
struct has_trivial_assign
{
    enum
    {
        value =
            is_fundamental<T>::value ||
            is_pod<T>::value         ||
            type_traits<T>::is_pointer
    };
};

/// Uses placement new to invoke the default constructor for a single instance
/// of an object. Specialized for non-blittable types with constructors.
///
/// @param mem Pointer to the address of the object to initialize.
template <typename T>
inline void construct(T *mem, int_to_type<false>)
{
    new (mem) T();
}

/// Invokes the constructor for a single instance of an object. Specialized
/// for blittable types, and translates to a no-op.
///
/// @param mem Pointer to the address of the object to initialize.
template <typename T>
inline void construct(T*, int_to_type<true>)
{
    /* empty */
}

/// Invokes the constructor for a single instance of a type using the most
/// efficient method available based on type T.
///
/// @param mem The instance to construct.
template <typename T>
inline void construct(T *mem)
{
    construct(mem, int_to_type<has_trivial_ctor<T>::value>());
}

/// Uses placement new to invoke the default constructor for each item in an
/// array. Specialized for non-blittable types.
///
/// @param arr The array of items to initialize.
/// @param count The number of items in the array.
template <typename T>
inline void construct(T *arr, size_t count, int_to_type<false>)
{
    CMN_UNUSED(arr); // @note: MSVC  complains otherwise...
    for (size_t i = 0; i < count; ++i)
    {
        new (arr + i) T();
    }
}

/// Invokes the default constructor for an array of items. Specialized for
/// blittable types and amounts to a no-op (as there is no constructor.)
///
/// @param arr The array of items to initialize.
/// @param count The number of items in the array.
template <typename T>
inline void construct(T*, size_t, int_to_type<true>)
{
    /* empty */
}

/// Invokes the constructor for each instance in an array using the most
/// efficient method available based on type T.
///
/// @param arr The array to initialize.
/// @param count The number of items in the array.
template <typename T>
inline void construct(T *arr, size_t count)
{
    construct(arr, count, int_to_type<has_trivial_ctor<T>::value>());
}

/// Uses placement new to invoke the copy constructor for a single instance
/// of an object. Specialized for non-blittable types.
///
/// @param mem Pointer to the address of the object to initialize.
/// @param orig The item to copy.
template <typename T>
inline void copy_construct(T *mem, T const &orig, int_to_type<false>)
{
    new (mem) T(orig);
}

/// Invokes the constructor for a single instance of an object. Specialized
/// for blittable types, and uses the assignment operator (operator =).
///
/// @param mem Pointer to the address of the object to initialize.
/// @param orig The item to copy.
template <typename T>
inline void copy_construct(T *mem, T const &orig, int_to_type<true>)
{
    mem[0] = orig;
}

/// Invokes the copy constructor for a type using the most efficient method
/// available based on the type T.
///
/// @param mem A pointer to the destination object.
/// @param orig The source object.
template <typename T>
inline void copy_construct(T *mem, T const &orig)
{
    copy_construct(mem, orig, int_to_type<has_trivial_copy<T>::value>());
}

/// Uses placement new to invoke the copy constructor to copy items from one
/// location to another. Specialized for non-blittable types.
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void copy_construct(
    T const *src,
    size_t   count,
    T       *dst,
    int_to_type<false>)
{
    for (size_t i = 0; i < count; ++i)
    {
        new (dst + i) T(src[i]);
    }
}

/// Uses placement new to invoke the copy constructor to copy items from one
/// location to another. Specialized for blittable types, which amounts to a
/// memcpy operation.
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void copy_construct(
    T const *src,
    size_t   count,
    T       *dst,
    int_to_type<true>)
{
    memcpy(dst, src, count * sizeof(T));
}

/// Invokes the copy constructor for each item in an array using the most
/// efficient method available based on the type T.
///
/// @param src The source array.
/// @param count The number of elements to copy.
/// @param dst The destination array.
template <typename T>
inline void copy_construct(T *src, size_t count, T *dst)
{
    copy_construct(src, count, dst, int_to_type<has_trivial_copy<T>::value>());
}

/// Invokes the type destructor for a single instance. Specialized for
/// non-blittable types.
///
/// @param mem Pointer to the item to delete.
template <typename T>
inline void destruct(T *mem, int_to_type<false>)
{
    CMN_UNUSED(mem); // @note: MSVC complains otherwise...
    mem->~T();
}

/// Invokes the type destructor for a single instance. Specialized for
/// blittable types, and translates to a no-op (as there is no destructor.)
///
/// @param mem Pointer to the item to delete.
template <typename T>
inline void destruct(T*, int_to_type<true>)
{
    /* empty */
}

/// Invokes the destructor for a single instance of a type using the most
/// efficient method available based on type T.
///
/// @param mem The instance to destroy.
template <typename T>
inline void destruct(T *mem)
{
    destruct(mem, int_to_type<has_trivial_dtor<T>::value>());
}

/// Invokes the type destructor for each element in an array. Specialized
/// for non-blittable types.
///
/// @param arr The array of items to destroy.
/// @param count The number of items in the array.
template <typename T>
inline void destruct(T *arr, size_t count, int_to_type<false>)
{
    CMN_UNUSED(arr); // @note: MSVC complains otherwise...
    for (size_t i = 0; i < count; ++i)
    {
        (arr + i)->~T();
    }
}

/// Invokes the type destructor for each element in an array. Specialized
/// for blittable types, and translates to a no-op (as there is no destructor.)
///
/// @param arr The array of items to destroy.
/// @param count The number of items in the array.
template <typename T>
inline void destruct(T*, size_t, int_to_type<true>)
{
    /* empty */
}

/// Invokes the destructor for each instance in an array using the most
/// efficient method available based on type T.
///
/// @param arr The array to destroy.
/// @param count The number of items in the array.
template <typename T>
inline void destruct(T *arr, size_t count)
{
    destruct(arr, count, int_to_type<has_trivial_dtor<T>::value>());
}

/// Copies items from one location to another, specialized for non-blittable
/// types with loop unrolling using Duff's Device.
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void copy(
    T const * CMN_RESTRICT src,
    size_t                 count,
    T       * CMN_RESTRICT dst, int_to_type<false>)
{
    T const *end  = src + count;
    switch (count & 0x03)
    {
    case 0:
        while (src != end)
        {
            *dst++ = *src++;
            /* fall-through */
    case 3:
            *dst++ = *src++;
            /* fall-through */
    case 2:
            *dst++ = *src++;
            /* fall-through */
    case 1:
            *dst++ = *src++;
        }
    }
}

/// Copies items from one location to another, specialized for blittable
/// types; uses the standard C library function memcpy.
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void copy(
    T const * CMN_RESTRICT src,
    size_t                 count,
    T       * CMN_RESTRICT dst, int_to_type<true>)
{
    memcpy(dst, src, count * sizeof(T));
}

/// Copies items from one location to another, using the most efficient means
/// available for a given type T.
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void copy(
    T const * CMN_RESTRICT src, 
    size_t                 count, 
    T       * CMN_RESTRICT dst)
{
    copy(src, count, dst, int_to_type<has_trivial_copy<T>::value>());
}

/// Copies items from one location to another, specialized for non-blittable
/// types to use the assignment operator (operator =).
///
/// @param src The starting point of the copy operation in the source array.
/// @param end The terminal point of the copy operation in the source array.
/// @param dst The destination array.
template <typename T>
inline void copy(
    T const * CMN_RESTRICT src,
    T const * CMN_RESTRICT end,
    T       * CMN_RESTRICT dst, int_to_type<false>)
{
    while (src != end)
    {
        *dst++ = *src++;
    }
}

/// Copies items from one location to another, specialized for blittable
/// types to use the standard C library function memcpy.
///
/// @param src The starting point of the copy operation in the source array.
/// @param end The terminal point of the copy operation in the source array.
/// @param dst The destination array.
template <typename T>
inline void copy(
    T const * CMN_RESTRICT src,
    T const * CMN_RESTRICT end,
    T       * CMN_RESTRICT dst, int_to_type<true>)
{
    /* copy items starting at 'src', ending at 'end' to 'dst' */
    /* specialized for blittable types (calls memcpy)         */
    /* NOTE: assert no overlap between src & dst              */
    size_t n =
        reinterpret_cast<uint8_t const*>(end) -
        reinterpret_cast<uint8_t const*>(src);
    memcpy(dst, src, n);
}

/// Copies items from one location to another, using the most efficient method
/// available for a given type T.
///
/// @param src The starting point of the copy operation in the source array.
/// @param end The terminal point of the copy operation in the source array.
/// @param dst The destination array.
template <typename T>
inline void copy(
    T const * CMN_RESTRICT src,
    T const * CMN_RESTRICT end,
    T       * CMN_RESTRICT dst)
{
    copy(src, end, dst, int_to_type<has_trivial_copy<T>::value>());
}

/// Moves items from one location to another, specialized for non-blittable
/// types to use the assignment operator (operator =).
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void move(T const *src, size_t count, T *dst, int_to_type<false>)
{
    for (ptrdiff_t i = count - 1; i >= 0; --i)
    {
        dst[i] = src[i];
    }
}

/// Moves items from one location to another, specialized for blittable
/// types; uses the standard C library function memmove. The source and
/// destination may overlap.
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void move(T const *src, size_t count, T *dst, int_to_type<true>)
{
    memmove(dst, src, count * sizeof(T));
}

/// Moves items from one location to another, using the most efficient means
/// available for a given type T. If the source and destination overlap, a
/// move operation is performed; otherwise, a copy operation is performed.
///
/// @param src The source array.
/// @param count The number of items to copy.
/// @param dst The destination array.
template <typename T>
inline void move(T const *src, size_t count, T *dst)
{
    if (dst > src && dst < src + count)
    {
        move(src, count, dst, int_to_type<has_trivial_copy<T>::value>());
    }
    else
    {
        copy(src, count, dst, int_to_type<has_trivial_copy<T>::value>());
    }
}

/// Moves items from one location to another, specialized for non-blittable
/// types to use the assignment operator (operator =).
///
/// @param src The starting point of the copy operation in the source array.
/// @param end The terminal point of the copy operation in the source array.
/// @param dst The destination array.
template <typename T>
inline void move(T const *src, T const *end, T *dst, int_to_type<false>)
{
    // copy items starting at 'src', ending at 'end' to 'dst'
    while (--end >= src)
    {
        *dst++ = *end;
    }
}

/// Moves items from one location to another, specialized for blittable types
/// to use the standard C library function memmove. The source and destination
/// may overlap.
///
/// @param src The starting point of the copy operation in the source array.
/// @param end The terminal point of the copy operation in the source array.
/// @param dst The destination array.
template <typename T>
inline void move(T const *src, T const *end, T *dst, int_to_type<true>)
{
    size_t n =
        reinterpret_cast<uint8_t const*>(end) -
        reinterpret_cast<uint8_t const*>(src);
    memmove(dst, src, n);
}

/// Moves items from one location to another, using the most efficient means
/// available for a given type T. If the source and destination overlap, a
/// move operation is performed; otherwise, a copy operation is performed.
///
/// @param src The starting point of the copy operation in the source array.
/// @param end The terminal point of the copy operation in the source array.
/// @param dst The destination array.
template <typename T>
inline void move(T const *src, T const *end, T *dst)
{
    if (dst > src && dst < end)
    {
        move(src, end, dst, int_to_type<has_trivial_copy<T>::value>());
    }
    else
    {
        copy(src, end, dst, int_to_type<has_trivial_copy<T>::value>());
    }
}

/// Sets all items in an array to a particular value. The fill loop is unrolled
/// using Duff's device.
///
/// @param arr The array to fill.
/// @param count The number of items in the array to set to @a value.
/// @param value The value to which all items in the array will be set.
template <typename T>
inline void fill(T *arr, size_t count, T const &value)
{
    T *end = arr  + count;
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

/// Determines whether an integer value is a power-of-two.
///
/// @param value The value to check.
/// @return true if @a value is a power-of-two.
template <typename int_type>
inline bool power_of_two(int_type const &value)
{
    return ((value & (value - 1)) == 0);
}

/// Determines the next power of two greater than or equal to a specified
/// value. If the value is already a power of two, it is unchanged. If the
/// value is less than some minimum value, the minimum value is returned.
///
/// @param value The value to inspect and possibly adjust.
/// @param minimum The minimum value to return. This must be a power of two.
/// @return The next power of two greater than or equal to @a value.
template <typename int_type>
inline int_type power_of_two_greater_or_equal(int_type value, int_type minimum)
{
    assert(traits::power_of_two(minimum));
    if (value < minimum)
    {
        // clamp to the minimum value.
        return minimum;
    }
    if (traits::power_of_two(value))
    {
        // return the input value, unchanged.
        return value;
    }
    // find the next power-of-two greater than value, starting from
    // a known power-of-two (the minimum value).
    int_type i = minimum;
    while (i < value) i <<= 1;
    return i;
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace traits */

#endif /* CMN_COMMON_TRAITS_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
