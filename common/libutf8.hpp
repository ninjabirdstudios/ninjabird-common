/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libutf8.hpp
///  Defines a replacement for the standard C string library designed for
///  working with NULL-terminated UTF-8 encoded strings, along with functions
///  for converting to and from different string encodings. No memory
///  management is performed by this module.
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIBUTF8_HPP_INCLUDED
#define LIBUTF8_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace utf8 {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef REPLACEMENT_CHAR
#define REPLACEMENT_CHAR     ((uint32_t) 0x0000FFFD)
#endif /* !defined(REPLACEMENT_CHAR) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef MAX_BMP
#define MAX_BMP              ((uint32_t) 0x0000FFFF)
#endif /* !defined(MAX_BMP) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef MAX_UTF16
#define MAX_UTF16            ((uint32_t) 0x0010FFFF)
#endif /* !defined(MAX_UTF16) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef MAX_UTF32
#define MAX_UTF32            ((uint32_t) 0x7FFFFFFF)
#endif /* !defined(MAX_UTF32) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef MAX_UTF32_LEGAL
#define MAX_UTF32_LEGAL      ((uint32_t) 0x0010FFFF)
#endif /* !defined(MAX_UTF32_LEGAL) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef SURROGATE_HS
#define SURROGATE_HS         ((uint32_t) 0x0000D800)
#endif /* !defined(SURROGATE_HS) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef SURROGATE_HE
#define SURROGATE_HE         ((uint32_t) 0x0000DBFF)
#endif /* !defined(SURROGATE_HE) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef SURROGATE_LS
#define SURROGATE_LS         ((uint32_t) 0x0000DC00)
#endif /* !defined(SURROGATE_LS) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef SURROGATE_LE
#define SURROGATE_LE         ((uint32_t) 0x0000DFFF)
#endif /* !defined(SURROGATE_LE) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef HALF_SHIFT
#define HALF_SHIFT           (10)
#endif /* !defined(HALF_SHIFT) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef HALF_BASE
#define HALF_BASE            (0x00010000UL)
#endif /* !defined(HALF_BASE) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef HALF_MASK
#define HALF_MASK            (0x000003FFUL)
#endif /* !defined(HALF_MASK) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef HAS_NULL_BYTE
#define HAS_NULL_BYTE(_x)    (((_x) - 0x01010101) & (~(_x) & 0x80808080))
#endif /* !defined(HAS_NULL_BYTE) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef IS_LEGAL_UTF8_START
#define IS_LEGAL_UTF8_START(ptr)                                              \
    ((ptr[0] & 0x80) == 0     ||                                              \
    ((ptr[0] & 0xFF) >= 0xC2  &&                                              \
     (ptr[0] & 0xFF) <= 0xDF) ||                                              \
     (ptr[0] & 0xF0) == 0xE0  ||                                              \
     (ptr[0] & 0xFF) == 0xF0)
#endif /* !defined(IS_LEGAL_UTF8_START) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef IS_UTF8_SEQUENCE_VALID
#define IS_UTF8_SEQUENCE_VALID(ptr)                                           \
     ((ptr[0] & 0x80) == 0      ? 1 :                                         \
    (((ptr[0] & 0xFF) >= 0xC2   &&                                            \
      (ptr[0] & 0xFF) <= 0xDF   &&                                            \
      (ptr[1] & 0xC0) == 0x80)  ? 2 :                                         \
     ((ptr[0] & 0xF0) == 0xE0   &&                                            \
      (ptr[1] & 0xC0) == 0x80   &&                                            \
      (ptr[2] & 0xC0) == 0x80   ? 3 :                                         \
     ((ptr[0] & 0xFF) == 0xF0   &&                                            \
      (ptr[1] & 0xC0) == 0x80   &&                                            \
      (ptr[2] & 0xC0) == 0x80   &&                                            \
      (ptr[3] & 0xC0) == 0x80   ? 4 : 0))))
#endif /* !defined(IS_UTF8_SEQUENCE_VALID) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef UTF8_SEQUENCE_LENGTH
#define UTF8_SEQUENCE_LENGTH(ptr)                                             \
     ((ptr[0] & 0x80) == 0      ? 1 :                                         \
    (((ptr[0] & 0xFF) >= 0xC2   &&                                            \
      (ptr[0] & 0xFF) <= 0xDF)  ? 2 :                                         \
     ((ptr[0] & 0xF0) == 0xE0   ? 3 :                                         \
     ((ptr[0] & 0xFF) == 0xF0   ? 4 : 0))))
#endif /* !defined(UTF8_SEQUENCE_LENGTH) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef UTF8_CHARACTER_LENGTH
#define UTF8_CHARACTER_LENGTH(ch)                                             \
    ((ch >= 0x00000 && ch <= 0x0007F) ? 1 :                                   \
     (ch >= 0x00080 && ch <= 0x007FF) ? 2 :                                   \
     (ch >= 0x00800 && ch <= 0x0FFFF) ? 3 :                                   \
     (ch >= 0x10000 && ch <= 0x3FFFF) ? 4 : 0)
#endif /* !defined(UTF8_CHARACTER_LENGTH) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef UTF8_GET_CHARACTER
#define UTF8_GET_CHARACTER(ptr)                                               \
     ((ptr[0] & 0x80) == 0      ?   ptr[0] :                                  \
    (((ptr[0] & 0xFF) >= 0xC2   &&                                            \
      (ptr[0] & 0xFF) <= 0xDF   &&                                            \
      (ptr[1] & 0xC0) == 0x80)  ?                                             \
     ((ptr[0] & 0x1F) << 6)     |  (ptr[1] & 0x3F) :                          \
     ((ptr[0] & 0xF0) == 0xE0   &&                                            \
      (ptr[1] & 0xC0) == 0x80   &&                                            \
      (ptr[2] & 0xC0) == 0x80   ?                                             \
     ((ptr[0] & 0x0F) << 12)    | ((ptr[1] & 0x3F) << 6) | (ptr[2] & 0x3F)  : \
     ((ptr[0] & 0xFF) == 0xF0   &&                                            \
      (ptr[1] & 0xC0) == 0x80   &&                                            \
      (ptr[2] & 0xC0) == 0x80   &&                                            \
      (ptr[3] & 0xC0) == 0x80   ?                                             \
     ((ptr[1] & 0x3F) << 12)    | ((ptr[2] & 0x3F) << 6) | (ptr[3] & 0x3F) :  \
     (uint32_t) -1))))
#endif /* !defined(UTF8_GET_CHARACTER) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef UTF8_PUT_CHARACTER
#define UTF8_PUT_CHARACTER(ptr, ch)                                           \
    ((ch >= 0x00000 && ch <= 0x0007F)                 ?                       \
      (ptr[0] = (char) ch)                            :                       \
     (ch >= 0x00080 && ch <= 0x007FF)                 ?                       \
     ((ptr[0] = (char) ((ch >>    6) & 0x1F) | 0xC0)  ,                       \
      (ptr[1] = (char)  (ch  & 0x3F)         | 0x80)) :                       \
     (ch >= 0x00800 && ch <= 0x0FFFF)          ?                              \
     ((ptr[0] = (char) ((ch >>   12) & 0x0F) | 0xE0)  ,                       \
      (ptr[1] = (char) ((ch <<    6) & 0x3F) | 0x80)  ,                       \
      (ptr[2] = (char)  (ch  & 0x3F)         | 0x80)) :                       \
     (ch >= 0x10000 && ch <= 0x3FFFF)          ?                              \
     ((ptr[0] = (char) ((uint8_t)      0xF0))  ,                              \
      (ptr[1] = (char) ((ch >>   12) & 0x3F) | 0x80)  ,                       \
      (ptr[2] = (char) ((ch >>    6) & 0x3F) | 0x80)  ,                       \
      (ptr[3] = (char)  (ch  & 0x3F)         | 0x80)) : 0)
#endif /* !defined(UTF8_PUT_CHARACTER) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef UTF8_TO_UPPER
#define UTF8_TO_UPPER(ch)                                                     \
    ((ch >= 'a' && ch <= 'z') ? (ch - 'a' + 'A') : ch)
#endif /* !defined(UTF8_TO_UPPER) */

/*/////////////////////////////////////////////////////////////////////////80*/

/// Computes the number of bytes required to store an ASCII string
/// in a UTF-8 encoding.
///
/// @param ascii Pointer to the start of the NULL-terminated ASCII
/// source string.
/// @param out_length If this value is non-NULL, on return it is updated
/// with the length of the source string (in characters.)
/// @return The number of bytes required to store @a ascii when using a
/// UTF-8 encoding, including the terminating NULL.
CMN_PUBLIC size_t size_for_ascii(char const *ascii, size_t *out_length);

/// Computes the number of bytes required to store a UTF-16 encoded string
/// in a UTF-8 encoding.
///
/// @param utf16 Pointer to the start of the NULL-terminated UTF-16
/// source string.
/// @param out_length If this value is non-NULL, on return it is updated
/// with the length of the source string (in characters.)
/// @return The number of bytes required to store @a utf16 when using a
/// UTF-8 encoding, including the terminating NULL.
CMN_PUBLIC size_t size_for_utf16(uint16_t const *utf16, size_t *out_length);

/// Computes the number of bytes required to store an UTF-32 encoded string
/// in a UTF-8 encoding.
///
/// @param utf32 Pointer to the start of the NULL-terminated UTF-32
/// source string.
/// @param out_length If this value is non-NULL, on return it is updated
/// with the length of the source string (in characters.)
/// @return The number of bytes required to store @a ascii when using a
/// UTF-8 encoding, including the terminating NULL.
CMN_PUBLIC size_t size_for_utf32(uint32_t const *utf32, size_t *out_length);

/// Converts an ASCII-encoded string to use a UTF-8 encoding.
///
/// @param src_start Pointer to the start of the source buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param src_end Pointer to one element past the last codepoint to be
/// converted (usually the NULL-terminator.)
/// @param tgt_start Pointer to the start of the target buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param tgt_end Pointer to one element past the last codepoint to be
/// converted.
/// @return true if the conversion was performed without error.
CMN_PUBLIC bool from_ascii(
    uint8_t const **src_start,
    uint8_t const  *src_end,
    uint8_t       **tgt_start,
    uint8_t        *tgt_end);

/// Converts a UTF-16 encoded string to use a UTF-8 encoding.
///
/// @param src_start Pointer to the start of the source buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param src_end Pointer to one element past the last codepoint to be
/// converted (usually the NULL-terminator.)
/// @param tgt_start Pointer to the start of the target buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param tgt_end Pointer to one element past the last codepoint to be
/// converted.
/// @return true if the conversion was performed without error.
CMN_PUBLIC bool from_utf16(
    uint16_t const **src_start,
    uint16_t const  *src_end,
    uint8_t        **tgt_start,
    uint8_t         *tgt_end);

/// Converts a UTF-32 encoded string to use a UTF-8 encoding.
///
/// @param src_start Pointer to the start of the source buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param src_end Pointer to one element past the last codepoint to be
/// converted (usually the NULL-terminator.)
/// @param tgt_start Pointer to the start of the target buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param tgt_end Pointer to one element past the last codepoint to be
/// converted.
/// @return true if the conversion was performed without error.
CMN_PUBLIC bool from_utf32(
    uint32_t const **src_start,
    uint32_t const  *src_end,
    uint8_t        **tgt_start,
    uint8_t         *tgt_end);

/// Converts a UTF-8 encoded string to use a UTF-16 encoding.
///
/// @param src_start Pointer to the start of the source buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param src_end Pointer to one element past the last codepoint to be
/// converted (usually the NULL-terminator.)
/// @param tgt_start Pointer to the start of the target buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param tgt_end Pointer to one element past the last codepoint to be
/// converted.
/// @return true if the conversion was performed without error.
CMN_PUBLIC bool to_utf16(
    uint8_t const  **src_start,
    uint8_t const   *src_end,
    uint16_t       **tgt_start,
    uint16_t        *tgt_end);

/// Converts a UTF-8 encoded string to use a UTF-32 encoding.
///
/// @param src_start Pointer to the start of the source buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param src_end Pointer to one element past the last codepoint to be
/// converted (usually the NULL-terminator.)
/// @param tgt_start Pointer to the start of the target buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param tgt_end Pointer to one element past the last codepoint to be
/// converted.
/// @return true if the conversion was performed without error.
CMN_PUBLIC bool to_utf32(
    uint8_t const  **src_start,
    uint8_t const   *src_end,
    uint32_t       **tgt_start,
    uint32_t        *tgt_end);

/// Converts a UTF-16 encoded string to use a UTF-32 encoding.
///
/// @param src_start Pointer to the start of the source buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param src_end Pointer to one element past the last codepoint to be
/// converted (usually the NULL-terminator.)
/// @param tgt_start Pointer to the start of the target buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param tgt_end Pointer to one element past the last codepoint to be
/// converted.
/// @return true if the conversion was performed without error.
CMN_PUBLIC bool utf16_to_utf32(
    uint16_t const **src_start,
    uint16_t const  *src_end,
    uint32_t       **tgt_start,
    uint32_t        *tgt_end);

/// Converts a UTF-32 encoded string to use a UTF-16 encoding.
///
/// @param src_start Pointer to the start of the source buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param src_end Pointer to one element past the last codepoint to be
/// converted (usually the NULL-terminator.)
/// @param tgt_start Pointer to the start of the target buffer. On return
/// this value is updated with the address of the next codepoint to be
/// converted.
/// @param tgt_end Pointer to one element past the last codepoint to be
/// converted.
/// @return true if the conversion was performed without error.
CMN_PUBLIC bool utf32_to_utf16(
    uint32_t const **src_start,
    uint32_t const  *src_end,
    uint16_t       **tgt_start,
    uint16_t        *tgt_end);

/// Converts a NULL-terminated ASCII string to a NULL-terminated UTF-8
/// encoded string.
///
/// @param ascii_str Pointer to the start of the NULL-terminated ASCII
/// string.
/// @param utf8_buf Pointer to the output buffer to which UTF-8 codepoints
/// will be written. If this value is NULL, the @a out_size parameter is
/// updated with the required size of the buffer, in bytes, and the
/// function returns NULL.
/// @param buf_size The maximum number of bytes that can be written to the
/// UTF-8 output buffer.
/// @param out_size If this value is non-NULL, on return it is updated with
/// the length of the string, in bytes, including the terminating NULL.
/// @return A pointer to the start of the UTF-8 buffer @a utf8_buf if
/// there was enough space in the output buffer to store the entire string.
/// Returns NULL if @a utf8_buf was not large enough to store the string.
CMN_PUBLIC char* from_ascii(
    char const *ascii_str,
    char       *utf8_buf,
    size_t      buf_size,
    size_t     *out_size);

/// Converts a NULL-terminated UTF-16 or UTF-32 string to a NULL-terminated
/// UTF-8 encoded string.
///
/// @param wide_str Pointer to the start of the NULL-terminated wide
/// character string.
/// @param utf8_buf Pointer to the output buffer to which UTF-8 codepoints
/// will be written. If this value is NULL, the @a out_size parameter is
/// updated with the required size of the buffer, in bytes, and the
/// function returns NULL.
/// @param buf_size The maximum number of bytes that can be written to the
/// UTF-8 output buffer.
/// @param out_size If this value is non-NULL, on return it is updated with
/// the length of the string, in bytes, including the terminating NULL.
/// @return A pointer to the start of the UTF-8 buffer @a utf8_buf if
/// there was enough space in the output buffer to store the entire string.
/// Returns NULL if @a utf8_buf was not large enough to store the string.
CMN_PUBLIC char* from_wide(
    wchar_t const *wide_str,
    char          *utf8_buf,
    size_t         buf_size,
    size_t        *out_size);

/// Converts a NULL-terminated UTF-8 string to a NULL-terminated UTF-16 or
/// UTF-32 encoded string.
///
/// @param utf8_str Pointer to the start of the NULL-terminated UTF-8
/// character string.
/// @param wide_buf Pointer to the output buffer to which UTF-16 or UTF-32
/// codepoints will be written. If this value is NULL, the @a out_size
/// parameter is updated with the required size of the buffer, in
/// characters, and the function returns NULL.
/// @param buf_size The maximum number of characters that can be written to
/// the wide character output buffer.
/// @param out_size If this value is non-NULL, on return it is updated with
/// the length of the string, in characters, including the terminating
/// NULL.
/// @return A pointer to the start of the wide character buffer @a wide_buf
/// if there was enough space in the output buffer to store the entire
/// string. Returns NULL if @a wide_buf was not large enough.
CMN_PUBLIC wchar_t* to_wide(
    char const    *utf8_str,
    wchar_t       *wide_buf,
    size_t         buf_size,
    size_t        *out_size);

/// Determines whether a UTF-8 codepoint is valid.
///
/// @param src Pointer to the start of a UTF-8 codepoint.
/// @param len The length of the codepoint, in bytes.
/// @return true if the codepoint is valid.
CMN_PUBLIC bool valid_codepoint(uint8_t const *src, size_t len);

/// Determines the length of a UTF-8 encoded string in both bytes and
/// characters.
///
/// @param str Pointer to the start of a NULL-terminated, UTF-8 encoded buffer.
/// @param out_chars If non-NULL, on return this value is updated with the
/// length of the string in characters, not including the trailing NULL.
/// @param out_bytes If non-NULL, on return this value is updated with the
/// length of the string in bytes, including the trailing NULL.
CMN_PUBLIC void string_length(
  char const *str,
  size_t     *out_chars,
  size_t     *out_bytes);

/// Determines the length of a UTF-8 encoded string in bytes.
///
/// @param str Pointer to the start of a NULL-terminated, UTF-8 encoded buffer.
/// @return The length of the string in bytes, including the trailing NULL.
CMN_PUBLIC size_t string_length_bytes(char const *str);

/// Determines the length of a UTF-8 encoded string in characters.
///
/// @param str Pointer to the start of a NULL-terminated, UTF-8 encoded buffer.
/// @return The length of the string in characters, not including the trailing
/// NULL byte.
CMN_PUBLIC size_t string_length_chars(char const *str);

/// Decodes a single UTF-8 codepoint and returns a pointer to the start of the
/// next UTF-8 codepoint in the string.
///
/// @param str Pointer to the start of a UTF-8 codepoint.
/// @param out_codepoint If non-NULL, on return this value is updated with the
/// value of the UTF-8 codepoint pointed to by @a str.
/// @return A pointer to the start of the next UTF-8 codepoint in @a str.
CMN_PUBLIC char* next_codepoint(char const *str, uint32_t *out_codepoint);

/// Retrieves the i-th UTF-8 codepoint in a string. This operation has O(N)
/// time complexity (N = length of string.)
///
/// @param str Pointer to the start of a NULL-terminated, UTF-8 encoded buffer.
/// @param index The zero-based index of the codepoint to retrieve.
/// @return A pointer to the start of the specified codepoint, or NULL.
CMN_PUBLIC char* codepoint_at(char const *str, size_t index);

/// Converts all characters in a string to their upper-case equivalents. Only
/// ASCII characters in [0, 127] are modified.
///
/// @param str Pointer to the start of a NULL-terminated, UTF-8 encoded buffer.
CMN_PUBLIC void to_upper(char *str);

/// Copies a UTF-8 string from one buffer to another, ensuring that the
/// destination buffer is NULL-terminated in all cases.
///
/// @param dst Pointer to the destination buffer.
/// @param dst_size The maximum number of bytes that can be written to @a dst.
/// @param src Pointer to the source buffer.
/// @return The number of characters copied.
CMN_PUBLIC size_t copy_string(char *dst, size_t dst_size, char const *src);

/// Copies a portion of a UTF-8 string from one buffer to another, ensuring
/// that the destination buffer is NULL-terminated in all cases.
///
/// @param dst Pointer to the destination buffer.
/// @param dst_size The maximum number of bytes that can be written to @a dst.
/// @param src Pointer to the source buffer.
/// @param count The number of characters to copy from src.
/// @return The number of characters copied.
CMN_PUBLIC size_t copy_string(
    char       *dst,
    size_t      dst_size,
    char const *src,
    size_t      count);

/// Copies a portion of a UTF-8 string from one buffer to another, ensuring
/// that the destination buffer is NULL-terminated in all cases.
///
/// @param dst Pointer to the destination buffer.
/// @param dst_size The maximum number of bytes that can be written to @a dst.
/// @param src_start Pointer to the first codepoint in the source buffer to
/// be copied.
/// @param src_end Pointer to the last codepoint in the source buffer to be
/// copied.
/// @return The number of characters copied.
CMN_PUBLIC size_t substring(
    char       *dst,
    size_t      dst_size,
    char const *src_start,
    char const *src_end);

/// Copies the contents of one string onto the end of another.
///
/// @param dst Pointer to the destination buffer.
/// @param dst_size The maximum number of bytes that can be written to @a dst.
/// @param src Pointer to the source buffer.
/// @return The number of characters copied.
CMN_PUBLIC size_t append_string(char *dst, size_t dst_size, char const *src);

/// Copies a portion of one string onto the end of another.
///
/// @param dst Pointer to the destination buffer.
/// @param dst_size The maximum number of bytes that can be written to @a dst.
/// @param src Pointer to the source buffer.
/// @param count The number of characters to copy from src.
/// @return The number of characters copied.
CMN_PUBLIC size_t append_string(
    char       *dst,
    size_t      dst_size,
    char const *src,
    size_t      count);

/// Performs a case-sensitive comparison of two UTF-8 strings.
///
/// @param a Pointer to the first string.
/// @param b Pointer to the second string.
/// @return Zero if the strings are the same, less than zero if @a a is less
/// than @a b or greater than zero if @a a is greater than @a b.
CMN_PUBLIC signed compare_strings(char const *a, char const *b);

/// Performs a case-sensitive comparison of two UTF-8 strings.
///
/// @param a Pointer to the first string.
/// @param b Pointer to the second string.
/// @param count The number of characters to compare.
/// @return Zero if the strings are the same, less than zero if @a a is less
/// than @a b or greater than zero if @a a is greater than @a b.
CMN_PUBLIC signed compare_strings(
    char const *a,
    char const *b,
    size_t      count);

/// Performs a case-insensitive comparison of two UTF-8 strings.
///
/// @param a Pointer to the first string.
/// @param b Pointer to the second string.
/// @return Zero if the strings are the same, less than zero if @a a is less
/// than @a b or greater than zero if @a a is greater than @a b.
CMN_PUBLIC signed compare_strings_normalized(char const *a, char const *b);

/// Performs a case-insensitive comparison of two UTF-8 strings.
///
/// @param a Pointer to the first string.
/// @param b Pointer to the second string.
/// @param count The number of characters to compare.
/// @return Zero if the strings are the same, less than zero if @a a is less
/// than @a b or greater than zero if @a a is greater than @a b.
CMN_PUBLIC signed compare_strings_normalized(
    char const *a,
    char const *b,
    size_t      count);

/// Searches for the first occurrence of a specific character within a string.
///
/// @param str Pointer to the NULL-terminated search string.
/// @param ch The codepoint to search for.
/// @return A pointer to the first occurrence of the specified character, or
/// NULL if the search string does not contain the specified character.
CMN_PUBLIC char* first(char const *str, uint32_t ch);

/// Searches for the first occurrence of any of a set of characters.
///
/// @param str Pointer to the NULL-terminated search string.
/// @param chars Pointer to a NULL-terminated set of characters to search for.
/// @return A pointer to the first occurrence of any of the characters in
/// @a chars found within @a str.
CMN_PUBLIC char* first_of(char const *str, char const *chars);

/// Searches for the first occurrence of any character that is not part of a
/// given set of characters.
///
/// @param str Pointer to the NULL-terminated search string.
/// @param chars Pointer to a NULL-terminated set of characters that cause the
/// search to continue.
/// @return A pointer to the first occurrence of any of the character not in
/// @a chars found within @a str.
CMN_PUBLIC char* first_not_of(char const *str, char const *chars);

/// Searches for the last occurrence of a specific character within a string.
///
/// @param str Pointer to the NULL-terminated search string.
/// @param ch The codepoint to search for.
/// @return A pointer to the last occurrence of the specified character, or
/// NULL if the search string does not contain the specified character.
CMN_PUBLIC char* last(char const *str, uint32_t ch);

/// Searches for the first occurrence of one string within another.
///
/// @param search Pointer to the NULL-terminated string to search.
/// @param find Pointer to the NULL-terminated string to find.
/// @return A pointer to the first occurrence of @a find in @a search, or
/// NULL if the search string does not contain the specified substring.
CMN_PUBLIC char* find(char const *search, char const *find);

/// Searches for the first occurrence of one string within another, ignoring
/// character case differences.
///
/// @param search Pointer to the NULL-terminated string to search.
/// @param find Pointer to the NULL-terminated string to find.
/// @return A pointer to the first occurrence of @a find in @a search, or
/// NULL if the search string does not contain the specified substring.
CMN_PUBLIC char* find_normalized(char const *search, char const *find);

/// Replaces any occurrence of a delimiter character with a NULL.
///
/// @param in_out_str Pointer to the NULL-terminated string to search. On
/// return, this value will be updated to point to the start of the next token.
/// @param delimiters Pointer to a NULL-terminated array of characters
/// representing delimiter characters.
/// @return A pointer to the start of the token (such that the return value
/// points to the start of the token, and *str points to the start of the next
/// token); or NULL if an empty or NULL string is supplied.
CMN_PUBLIC char* delimit(char **in_out_str, char const *delimiters);

/// Reads a token from a string.
///
/// @param str Pointer to the NULL-terminated string to search. The contents
/// of this string may be modified.
/// @param delimiters Pointer to a NULL-terminated array of characters
/// representing delimiter characters.
/// @param out_token On return, this value is updated to point to the start of
/// the current token.
/// @return A pointer to the start of the next token, or NULL of no more
/// tokens are available.
CMN_PUBLIC char* token(char *str, char const *delimiters, char **out_token);

/// Performs printf-style formatting to a user-managed buffer. This function
/// uses the standard library vsnprintf function.
///
/// @param format_string Pointer to a NULL-terminated string following the
/// format specification for the standard C library printf function.
/// @param output_buffer Pointer to the start of a buffer to which character
/// data will be written. This buffer will always be NULL-terminated.
/// @param max_length The maximum number of bytes that can be written to
/// @a output_buffer.
/// @param out_length On return, this location is updated with the number of
/// characters written to @a output_buffer, not including the trailing NULL.
/// @return A pointer to the formatted string (@a output_buffer).
CMN_PUBLIC char* format(
    char const *format_string,
    char       *output_buffer,
    size_t      max_length,
    size_t     *out_length, ...);

/// Checks a character value to determine whether it represents a standard
/// decimal digit [0, 9].
///
/// @param ch The character to check.
/// @return true if the specified character is a decimal digit.
inline bool is_digit(char ch)
{
    return (ch >= '0' && ch <= '9');
}

/// Attempts to convert a decimal string value back into its corresponding
/// integer equivalent. Both signed and unsigned values are supported.
///
/// @param first A pointer to the first numeric character in the string to
/// be parsed.
/// @param last A pointer to the last potential numeric character in the string
/// to be parsed. The number value may end prior to this character.
/// @param out_int A pointer to an integer value that will be updated with the
/// parsed value.
/// @return A pointer to the last character of the integer value, or @a first
/// if the string cannot be parsed into an integer value.
template <typename int_type>
inline char* parse_decimal(
  char     *first,
  char     *last,
  int_type *out_int)
{
    signed   sign   = 1;
    int_type result = 0;

    if (first != last)
    {
        if ('-' == *first)
        {
            sign = -1;
            ++first;
        }
        else if ('+' == *first)
        {
            sign = +1;
            ++first;
        }
    }
    for (; first != last && utf8::is_digit(*first); ++first)
    {
        result = 10 * result + (*first - '0');
    }
    *out_int = result * sign;
    return first;
}

/// Attempts to convert a hexadecimal string value back into its corresponding
/// integer equivalent. The int_type is typically an unsigned value.
///
/// @param first A pointer to the first numeric character in the string to
/// be parsed.
/// @param last A pointer to the last potential numeric character in the string
/// to be parsed. The number value may end prior to this character.
/// @param out_int A pointer to an integer value that will be updated with the
/// parsed value.
/// @return A pointer to the last character of the integer value, or @a first
/// if the string cannot be parsed into an integer value.
template <typename int_type>
inline char* parse_hexadecimal(
    char     *first,
    char     *last,
    int_type *out_int)
{
    int_type result = 0;
    for (; first != last; ++first)
    {
        unsigned int digit;
        if (utf8::is_digit(*first))
        {
            digit = *first - '0';
        }
        else if (*first >= 'a' && *first <= 'f')
        {
            digit = *first - 'a' + 10;
        }
        else if (*first >= 'A' && *first <= 'F')
        {
            digit = *first - 'A' + 10;
        }
        else break;
        result = 16 * result + digit;
    }
    *out_int = result;
    return first;
}

/// Attempts to convert a floating-point string value back into its
/// corresponding binary equivalent. The float_type should be either float or
/// double.
///
/// @param first A pointer to the first numeric character in the string to
/// be parsed.
/// @param last A pointer to the last potential numeric character in the string
/// to be parsed. The number value may end prior to this character.
/// @param out_int A pointer to an integer value that will be updated with the
/// parsed value.
/// @return A pointer to the last character of the value, or @a first if the
/// string cannot be parsed into a floating-point value.
template <typename float_type>
inline char* parse_number(
    char       *first,
    char       *last,
    float_type *out_num)
{
    float_type sign     = 1.0;
    float_type result   = 0.0;
    bool       exp_neg  = false;
    int        exponent = 0;

    if (first != last)
    {
        if ('-' == *first)
        {
            sign = -1.0;
            ++first;
        }
        else if ('+' == *first)
        {
            sign = +1.0;
            ++first;
        }
    }
    for (; first != last && utf8::is_digit(*first); ++first)
    {
        result = 10 * result + (*first - '0');
    }
    if (first != last && '.' == *first)
    {
        float_type inv_base = 0.1;
        ++first;
        for (; first != last && utf8::is_digit(*first); ++first)
        {
            result   += (*first - '0') * inv_base;
            inv_base *= 0.1;
        }
    }
    result *= sign;
    if (first != last && ('e' == *first || 'E' == *first))
    {
        ++first;
        if ('-' == *first)
        {
            exp_neg = true;
            ++first;
        }
        else if ('+' == *first)
        {
            exp_neg = false;
            ++first;
        }
        for (; first != last && utf8::is_digit(*first); ++first)
        {
            exponent = 10 * exponent + (*first - '0');
        }
    }
    if (exponent != 0)
    {
        float_type power_of_ten = 10;
        for (; exponent > 1; exponent--)
        {
            power_of_ten *= 10;
        }
        if (exp_neg) result /= power_of_ten;
        else         result *= power_of_ten;
    }
    *out_num = result;
    return first;
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace utf8 */

#endif /* LIBUTF8_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
