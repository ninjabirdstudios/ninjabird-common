/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines a default set of hashing ad CRC calculation functions with
/// various performnce characteristics. All functions operate on raw memory
/// buffers.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/
#ifndef LIBHASH_HPP_INCLUDED
#define LIBHASH_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace hash {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// Computes a 32-bit hash value for a NULL-terminated UTF-8 (or ASCII)
/// encoded string.
///
/// @param str Pointer to the start of a NULL-terminated ASCII or UTF-8
/// encoded string to hash.
/// @return The 32-bit hash of the specified string.
CMN_PUBLIC uint32_t hash32_string(char const *str);

/// Computes a 32-bit hash value for a UTF-8 (or ASCII) encoded string or
/// substring, as defined by the specified start and end points.
///
/// @param start Pointer to the start of the string. This character is included
/// in the hash value, unless @a start and @a end point to the same character.
/// @param end Pointer to the end of the string. This character is not included
/// in the hash value.
/// @return The 32-bit hash of the specified string.
CMN_PUBLIC uint32_t hash32_string(char const *start, char const *end);

/// Computes a 32-bit hash value for a UTF-8 (or ASCII) encoded string,
/// including only the specified number of bytes in the hash.
///
/// @param str Pointer to the start of the string to hash.
/// @param length_bytes The number of bytes in @a str to read and hash.
/// @return The 32-bit hash of the specified string.
CMN_PUBLIC uint32_t hash32_string(char const *str, size_t length_bytes);

/// Uses MurmurHash3 to compute a 32-bit hash value of a block of data. See:
/// http://code.google.com/p/smhasher/ for more information.
///
/// @param data Pointer to the start of the data to hash.
/// @param length The number of bytes of data to hash.
/// @param seed A starting seed for the hash value.
/// @return The 32-bit hash of the specified data.
CMN_PUBLIC uint32_t hash32(
    void const *data,
    size_t      length,
    uint32_t    seed);

/// Uses MurmurHash3 to compute a 128-bit hash value of a block of data. This
/// implementaton is optimized for 32-bit platforms. For more information, see:
/// http://code.google.com/p/smhasher/
///
/// @param data Pointer to the start of the data to hash.
/// @param length The number of bytes of data to hash.
/// @param seed A starting seed for the hash value.
/// @param out_hash A pointer to an block of data, at least 16 bytes in length,
/// to which the resulting hash value will be written.
CMN_PUBLIC void     hash128_32(
    void const *data,
    size_t      length,
    uint32_t    seed,
    void       *out_hash);

/// Uses MurmurHash3 to compute a 128-bit hash value of a block of data. This
/// implementaton is optimized for 64-bit platforms. For more information, see:
/// http://code.google.com/p/smhasher/
///
/// @param data Pointer to the start of the data to hash.
/// @param length The number of bytes of data to hash.
/// @param seed A starting seed for the hash value.
/// @param out_hash A pointer to an block of data, at least 16 bytes in length,
/// to which the resulting hash value will be written.
CMN_PUBLIC void     hash128_64(
    void const *data,
    size_t      length,
    uint32_t    seed,
    void       *out_hash);

/// Computes a 32-bit CRC value for a block of data using CRC-32.
///
/// @param data Pointer to the start of the block of data.
/// @param offset The byte offset into @a data at which to start reading.
/// @param length The number of bytes to read from @a data.
/// @param seed The starting seed value.
/// @return The CRC-32 of the data.
CMN_PUBLIC uint32_t crc32(
    void const *data,
    ptrdiff_t   offset,
    size_t      length,
    uint32_t    seed);

/// Generates a 32-bit unsigned integer name for a given string name.
///
/// @param name_str Pointer to the start of a NULL-terminated UTF-8
/// (or ASCII) string representing the name.
/// @param out_name_int On return, this location is updated with the integer
/// identifier equivalent of @a name_str. This value cannot be NULL.
/// @return true if the name was generated successfully, or false if one or
/// more parameters are invalid.
CMN_PUBLIC bool generate_name(char const *name_str, uint32_t *out_name_int);

/// Generates a 32-bit unsigned integer name for a given string name.
///
/// @param name_str_beg Pointer to the start of a UTF-8 (or ASCII) string
/// representing the name.
/// @param name_str_end Pointer to the one byte after the last byte of the
/// name string (for example, where the NULL-terminator would be.)
/// @param out_name_int On return, this location is updated with the integer
/// identifier equivalent of @a name_str. This value cannot be NULL.
/// @return true if the name was generated successfully, or false if one or
/// more parameters are invalid.
CMN_PUBLIC bool generate_name(
    char const *name_str_beg,
    char const *name_str_end,
    uint32_t   *out_name_int);

/// Generates zero or more 32-bit unsigned integer names from given string
/// identifiers, checking for hash collisions.
///
/// @param name_strs An array of NULL-terminated UTF-8 (or ASCII) strings
/// representing the names.
/// @param name_ints Pointer to an array uint32_t values that will be updated
/// with integer identifier equivalents of the values in @name_strs, such that
/// the following holds:
/// @a name_ints[i + @a name_offset] <== @a name_strs[i].
/// @param name_count The number of name identifiers listed in @a name_strs.
/// @param name_offset The zero-based offset into @a name_ints at which to
/// begin writing integer names. This value defaults to 0. Note that when
/// checking for name collisions, the check always starts at index 0.
/// @return The number of integer names written to @a name_ints. If this value
/// is less than @a name_count, an error occurred.
CMN_PUBLIC size_t generate_names(
    char const **name_strs,
    uint32_t    *name_ints,
    size_t       name_count,
    size_t       name_offset = 0);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace hash */

#endif /* LIBHASH_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
