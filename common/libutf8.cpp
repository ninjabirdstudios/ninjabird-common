/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libutf8.cpp
///  Implements a replacement for the standard C string library designed for
///  working with NULL-terminated UTF-8 encoded strings, along with functions
///  for converting to and from different string encodings. No memory
///  management is performed by this module.
///
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "libutf8.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

// index using the first byte of a UTF-8 sequence to get the number of
// bytes that follow in the codepoint - legal UTF-8 can't have 4/5 bytes.
static const char Trail_UTF8[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*/////////////////////////////////////////////////////////////////////////80*/

// magic values used during UTF-8 conversion.
static const uint32_t Offset_UTF8[6] =
{
    0x00000000UL,
    0x00003080UL,
    0x000E2080UL,
    0x03C82080UL,
    0xFA082080UL,
    0x82082080UL
};

/*/////////////////////////////////////////////////////////////////////////80*/

static const uint8_t First_Byte_Mark_UTF8[7] =
{
    0x00,
    0x00,
    0xC0,
    0xE0,
    0xF0,
    0xF8,
    0xFC
};

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::size_for_ascii(char const *ascii, size_t *out_length)
{
    uint8_t const *src_str = (uint8_t const*) ascii;
    uint8_t const *src_end = (uint8_t const*) ascii;
    uint8_t        ch      = 0;
    size_t         src_len = 0;
    size_t         dst_len = 0;

    if (NULL == src_str)
    {
        // 1 byte required for NULL-terminator.
        return 1;
    }

    while ((ch = *src_end++) != 0)
    {
        if (ch < 0x80)
        {
            // 1 byte required for chars  <= 127.
            dst_len += 1;
        }
        else
        {
            // 2 bytes required for chars >  127.
            dst_len += 2;
        }
        ++src_len;
    }


    // add a byte for the NULL-terminator.
    dst_len += 1;

    // save the source length for the caller, if they want it.
    if (out_length != NULL) *out_length = src_len;
    return dst_len;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::size_for_utf16(uint16_t const *utf16, size_t *out_length)
{
    uint16_t const *src_end = utf16;
    uint16_t const *source  = utf16;
    size_t          src_len = 0;
    size_t          dst_len = 0;

    if (NULL == src_end)
    {
        // 1 byte required for NULL-terminator.
        return 1;
    }

    // find the end of the source string:
    while (*src_end++ != 0) ++src_len;

    // at this point, src_end points at the NULL terminator.
    for (size_t i = 0; i < src_len; ++i)
    {
        unsigned short  bytes_to_write = 0;
        uint32_t        ch             = *source++;

        // if we have a surrogate pair, convert to UTF-32 first.
        if (ch >= SURROGATE_HS && ch <= SURROGATE_HE)
        {
            // we have a surrogate pair in ch and c2.
            if (source < src_end)
            {
                uint32_t c2 = *source;

                // if it's a low surrogate, convert to UTF-32.
                if (c2 >= SURROGATE_LS && c2 <= SURROGATE_LE)
                {
                    ch = ((ch - SURROGATE_HS) << HALF_SHIFT) +
                          (c2 - SURROGATE_LS)  + HALF_BASE;
                    ++source;
                }
            }
        }

        // determine the number of bytes needed for UTF-8 encoding.
        if      (ch < 0x80UL)    bytes_to_write = 1;
        else if (ch < 0x800UL)   bytes_to_write = 2;
        else if (ch < 0x10000UL) bytes_to_write = 3;
        else if (ch < 0x110000)  bytes_to_write = 4;
        else                     bytes_to_write = 3;
        dst_len += bytes_to_write;
    }

    // add a byte for the NULL-terminator.
    dst_len += 1;

    // save the source length for the caller, if they want it.
    if (out_length != NULL) *out_length = src_len;
    return dst_len;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::size_for_utf32(uint32_t const *utf32, size_t *out_length)
{
    uint32_t const *src_end = utf32;
    uint32_t const *source  = utf32;
    size_t          src_len = 0;
    size_t          dst_len = 0;

    if (NULL == src_end)
    {
        // 1 byte required for NULL-terminator.
        return 1;
    }

    // find the end of the source string:
    while (*src_end++ != 0)  ++src_len;

    // at this point, srcStr points at the NULL terminator.
    while (source < src_end)
    {
        unsigned short  bytes_to_write = 0;
        uint32_t        ch             = *source++;

        // determine the number of bytes needed for UTF-8 encoding.
        if (ch < 0x80UL)                bytes_to_write = 1;
        else if (ch < 0x800UL)          bytes_to_write = 2;
        else if (ch < 0x10000UL)        bytes_to_write = 3;
        else if (ch <= MAX_UTF32_LEGAL) bytes_to_write = 4;
        else                            bytes_to_write = 3;
        dst_len += bytes_to_write;
    }

    // add a byte for the NULL-terminator.
    dst_len += 1;

    // save the source length for the caller, if the want it.
    if (out_length != NULL) *out_length = src_len;
    return dst_len;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool utf8::from_ascii(
    uint8_t const **src_start,
    uint8_t const  *src_end,
    uint8_t       **tgt_start,
    uint8_t        *tgt_end)
{
    bool           result  =  true;
    uint8_t const  *source = *src_start;
    uint8_t        *target = *tgt_start;

    while (source < src_end)
    {
        uint8_t ch = *source++;

        if (ch < 0x80)
        {
            if (target >= tgt_end)
            {
                --source;
                result = false;
                break;
            }
            // UTF-8 represents characters [0, 127] with no change.
            *target++ = ch;
        }
        else
        {
            if (target + 1 >= tgt_end)
            {
                --source;
                result = false;
                break;
            }
            // characters in [128, 255] require two bytes.
            *target++ = (ch >>    6) | 0xC0;
            *target++ = (ch  & 0x3F) | 0x80;
        }
    }
    *src_start = source;
    *tgt_start = target;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool utf8::from_utf16(
    uint16_t const **src_start,
    uint16_t const  *src_end,
    uint8_t        **tgt_start,
    uint8_t         *tgt_end)
{
    bool             result =  true;
    uint16_t const  *source = *src_start;
    uint8_t         *target = *tgt_start;

    while (source < src_end)
    {
        unsigned short  bytes_to_write = 0;
        uint16_t const *ss             = source;
        uint32_t const  byte_mask      = 0xBF;
        uint32_t const  byte_mark      = 0x80;
        uint32_t        ch             = *source++;

        // if we have a surrogate pair, convert to UTF-32 first.
        if (ch >= SURROGATE_HS && ch <= SURROGATE_HE)
        {
            // we have a surrogate pair in ch and c2.
            if (source < src_end)
            {
                uint32_t c2 = *source;

                // if it's a low surrogate, convert to UTF-32.
                if (c2 >= SURROGATE_LS && c2 <= SURROGATE_LE)
                {
                    ch = ((ch - SURROGATE_HS) << HALF_SHIFT) +
                          (c2 - SURROGATE_LS)  + HALF_BASE;
                    ++source;
                }
                // else, ch is the correct UTF-32 character.
            }
            else
            {
                // we're missing 16 bits following the high surrogate.
                --source;
                result = false;
                break;
            }
        }

        // determine the number of bytes needed for UTF-8 encoding.
        if      (ch < 0x80UL)    bytes_to_write = 1;
        else if (ch < 0x800UL)   bytes_to_write = 2;
        else if (ch < 0x10000UL) bytes_to_write = 3;
        else if (ch < 0x110000)  bytes_to_write = 4;
        else
        {
            bytes_to_write = 3;
            ch             = (uint32_t) REPLACEMENT_CHAR;
        }

        // make sure there is enough room in the target buffer.
        if (target + bytes_to_write > tgt_end)
        {
            source = ss;
            result = false;
            break;
        }

        //  write the UTF-8 codepoint.
        // @note: fallthrough intentional in switch.
        target += bytes_to_write;
        switch (bytes_to_write)
        {
        case 4:
            *--target = (uint8_t) ((ch | byte_mark) & byte_mask);
            ch >>= 6;
        case 3:
            *--target = (uint8_t) ((ch | byte_mark) & byte_mask);
            ch >>= 6;
        case 2:
            *--target = (uint8_t) ((ch | byte_mark) & byte_mask);
            ch >>= 6;
        case 1:
            *--target = (uint8_t) ((ch | First_Byte_Mark_UTF8[bytes_to_write]));
        }
        target += bytes_to_write;
    }

    *src_start = source;
    *tgt_start = target;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool utf8::from_utf32(
    uint32_t const **src_start,
    uint32_t const  *src_end,
    uint8_t        **tgt_start,
    uint8_t         *tgt_end)
{
    bool            result =  0;
    uint32_t const  *source = *src_start;
    uint8_t         *target = *tgt_start;

    while (source < src_end)
    {
        unsigned short  bytes_to_write = 0;
        uint32_t const  byte_mask      = 0xBF;
        uint32_t const  byte_mark      = 0x80;
        uint32_t        ch             = *source++;

        // determine the number of bytes needed for UTF-8 encoding.
        if      (ch < 0x80UL)           bytes_to_write = 1;
        else if (ch < 0x800UL)          bytes_to_write = 2;
        else if (ch < 0x10000UL)        bytes_to_write = 3;
        else if (ch <= MAX_UTF32_LEGAL) bytes_to_write = 4;
        else
        {
            bytes_to_write = 3;
            ch             = (uint32_t) REPLACEMENT_CHAR;
        }

        // make sure there is enough room in the target buffer.
        if (target + bytes_to_write > tgt_end)
        {
            --source;
            result = false;
            break;
        }

        //  write the UTF-8 codepoint
        // @NOTE: fallthrough intentional in switch.
        target += bytes_to_write;
        switch (bytes_to_write)
        {
        case 4:
            *--target = (uint8_t) ((ch | byte_mark) & byte_mask);
            ch >>= 6;
        case 3:
            *--target = (uint8_t) ((ch | byte_mark) & byte_mask);
            ch >>= 6;
        case 2:
            *--target = (uint8_t) ((ch | byte_mark) & byte_mask);
            ch >>= 6;
        case 1:
            *--target = (uint8_t) ((ch | First_Byte_Mark_UTF8[bytes_to_write]));
        }
        target += bytes_to_write;
    }
    *src_start = source;
    *tgt_start = target;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool utf8::to_utf16(
    uint8_t const  **src_start,
    uint8_t const   *src_end,
    uint16_t       **tgt_start,
    uint16_t        *tgt_end)
{
    bool            result =  0;
    uint8_t const  *source = *src_start;
    uint16_t       *target = *tgt_start;

    while (source < src_end)
    {
        unsigned short  extra_bytes = Trail_UTF8[*source];
        uint32_t        ch          = 0;

        if (source + extra_bytes >= src_end)
        {
            result = false;
            break;
        }

        if (!utf8::valid_codepoint(source, extra_bytes + 1))
        {
            result = false;
            break;
        }

        // @note: fall-through is intentional.
        switch (extra_bytes)
        {
        case 5: /* @note: invalid UTF-8 */
            ch  += *source++;
            ch <<= 6;
        case 4: /* @note: invalid UTF-8 */
            ch  += *source++;
            ch <<= 6;
        case 3:
            ch  += *source++;
            ch <<= 6;
        case 2:
            ch  += *source++;
            ch <<= 6;
        case 1:
            ch  += *source++;
            ch <<= 6;
        case 0:
            ch  += *source++;
        }

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:6385) /* code analysis warning */
#endif
        ch -= Offset_UTF8[extra_bytes];
#ifdef _MSC_VER
#pragma warning (pop)
#endif

        // make sure there is enough room in the target buffer.
        if (target >= tgt_end)
        {
            source -= (extra_bytes + 1);
            result  = false;
            break;
        }

        if (ch <= MAX_BMP)
        {
            if (ch >= SURROGATE_HS && ch <= SURROGATE_LE)
            {
                *target++ = (uint16_t) REPLACEMENT_CHAR;
            }
            else
            {
                /* normal case */
                *target++ = (uint16_t) ch;
            }
        }
        else if (ch > MAX_UTF16)
        {
            *target++ = (uint16_t) REPLACEMENT_CHAR;
        }
        else
        {
            // target is a character in the range 0xFFFF - 10FFFF.
            if (target + 1 >= tgt_end)
            {
                source -= (extra_bytes + 1);
                result  = false;
                break;
            }
            ch        -= HALF_BASE;
            *target++  = (uint16_t) ((ch >> HALF_SHIFT) + SURROGATE_HS);
            *target++  = (uint16_t) ((ch  & HALF_MASK)  + SURROGATE_LS);
        }
    }
    *src_start = source;
    *tgt_start = target;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool utf8::to_utf32(
    uint8_t const  **src_start,
    uint8_t const   *src_end,
    uint32_t       **tgt_start,
    uint32_t        *tgt_end)
{
    bool           result =  0;
    uint8_t const  *source = *src_start;
    uint32_t       *target = *tgt_start;

    while (source < src_end)
    {
        unsigned short  extra_bytes = Trail_UTF8[*source];
        uint32_t        ch          = 0;

        if (source + extra_bytes >= src_end)
        {
            result = false;
            break;
        }

        if (!utf8::valid_codepoint(source, extra_bytes + 1))
        {
            result = false;
            break;
        }

        // @note: fall-through is intentional.
        switch (extra_bytes)
        {
        case 5: /* @note: invalid UTF-8 */
            ch  += *source++;
            ch <<= 6;
        case 4: /* @note: invalid UTF-8 */
            ch  += *source++;
            ch <<= 6;
        case 3:
            ch  += *source++;
            ch <<= 6;
        case 2:
            ch  += *source++;
            ch <<= 6;
        case 1:
            ch  += *source++;
            ch <<= 6;
        case 0:
            ch  += *source++;
        }

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:6385) /* code analysis warning */
#endif
        ch -= Offset_UTF8[extra_bytes];
#ifdef _MSC_VER
#pragma warning (pop)
#endif

        // make sure there is enough room in the target buffer.
        if (target >= tgt_end)
        {
            source -= (extra_bytes + 1);
            result  = false;
            break;
        }

        if (ch <= MAX_UTF32_LEGAL)
        {
            if (ch >= SURROGATE_HS && ch <= SURROGATE_LE)
            {
                *target++ = (uint32_t) REPLACEMENT_CHAR;
            }
            else
            {
                // normal case.
                *target++ = ch;
            }
        }
        else
        {
            *target++ = (uint32_t) REPLACEMENT_CHAR;
        }
    }
    *src_start = source;
    *tgt_start = target;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool utf8::utf16_to_utf32(
    uint16_t const **src_start,
    uint16_t const  *src_end,
    uint32_t       **tgt_start,
    uint32_t        *tgt_end)
{
    bool             result =  true;
    uint16_t const  *source = *src_start;
    uint32_t        *target = *tgt_start;

    while (source < src_end)
    {
        uint32_t ch = *source++;
        uint32_t c2 =  0;

        // ensure we don't run off the end of the target buffer.
        if (target >= tgt_end)
        {
            --source;
            result = false;
            break;
        }

        if (ch >= SURROGATE_HS && ch <= SURROGATE_HE)
        {
            // we have a surrogate pair in ch and c2.
            if (source < src_end)
            {
                c2 = *source;

                // if it's a low surrogate, convert to UTF-32.
                if (c2 >= SURROGATE_LS && c2 <= SURROGATE_LE)
                {
                    ch = ((ch - SURROGATE_HS) << HALF_SHIFT) +
                          (c2 - SURROGATE_LS)  + HALF_BASE;
                    ++source;
                }
                // else, ch is the correct UTF-32 character.
            }
            else
            {
                // we're missing 16 bits following the high surrogate.
                --source;
                result = false;
                break;
            }
        }
        // else, ch is the correct UTF-32 character.
        *target++ = ch;
    }
    *src_start = source;
    *tgt_start = target;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool utf8::utf32_to_utf16(
    uint32_t const **src_start,
    uint32_t const  *src_end,
    uint16_t       **tgt_start,
    uint16_t        *tgt_end)
{
    bool           result =  true;
    uint32_t const *source = *src_start;
    uint16_t       *target = *tgt_start;

    while (source < src_end)
    {
        uint32_t ch = *source++;

        // ensure we don't run off the end of the target buffer.
        if (target >= tgt_end)
        {
            --source;
            result = false;
            break;
        }

        if (ch <= MAX_BMP)
        {
            if (ch >= SURROGATE_HS && ch <= SURROGATE_LE)
            {
                // UTF-16 surrogate values are illegal in UTF-32.
                // use the replacement character.
                *target++ = (uint16_t) REPLACEMENT_CHAR;
            }
            else
            {
                // normal case.
                *target++ = (uint16_t) ch;
            }
        }
        else if (ch > MAX_UTF32_LEGAL)
        {
            // this is an illegal UTF-32 character.
            // use the replacement character.
            *target++ = (uint16_t) REPLACEMENT_CHAR;
        }
        else
        {
            if (target + 1 < tgt_end)
            {
                // a valid UTF-32 character, and there's enough buffer.
                ch        -= HALF_BASE;
                *target++  = (uint16_t) ((ch >> HALF_SHIFT) + SURROGATE_HS);
                *target++  = (uint16_t) ((ch &  HALF_MASK)  + SURROGATE_LS);
            }
            else
            {
                // out of space in the target buffer, but more to convert.
                --source;
                result = false;
                break;
            }
        }
    }
    *src_start = source;
    *tgt_start = target;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::from_ascii(
    char const *ascii_str,
    char       *utf8_buf,
    size_t      buf_size,
    size_t     *out_size)
{
    size_t         src_len = 0;
    size_t         dst_len = 0;
    uint8_t       *dst_ptr = (uint8_t*)       utf8_buf;
    uint8_t       *dst_str = (uint8_t*)       utf8_buf;
    uint8_t const *src_str = (uint8_t const*) ascii_str;

    // compute the source and destination sizes.
    dst_len = utf8::size_for_ascii(ascii_str, &src_len);
    if (out_size != NULL) *out_size = dst_len;

    // convert the string, if there's enough space.
    if (dst_len <= buf_size)
    {
        utf8::from_ascii(
            &src_str,
             src_str + src_len,
            &dst_ptr,
             dst_ptr + dst_len);
        dst_str[dst_len - 1] = 0;
        return (char*) dst_str;
    }
    // not enough space in the output buffer.
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::from_wide(
    wchar_t const *wide_str,
    char          *utf8_buf,
    size_t         buf_size,
    size_t        *out_size)
{
#if CMN_IS_WINDOWS

    size_t          src_len = 0;
    size_t          dst_len = 0;
    uint8_t        *dst_ptr = (uint8_t*)        utf8_buf;
    uint8_t        *dst_str = (uint8_t*)        utf8_buf;
    uint16_t const *src_str = (uint16_t const*) wide_str;

    // compute the source and destination sizes.
    dst_len = utf8::size_for_utf16(src_str, &src_len);
    if (out_size != NULL) *out_size = dst_len;

    // convert the string, if there's enough space.
    if (dst_len <= buf_size)
    {
        utf8::from_utf16(
            &src_str,
             src_str + src_len,
            &dst_ptr,
             dst_ptr + dst_len);
        dst_str[dst_len - 1] = 0;
        return (char*) dst_str;
    }
    // not enough space in the output buffer.
    return NULL;

#else

    size_t          src_len = 0;
    size_t          dst_len = 0;
    uint8_t        *dst_ptr = (uint8_t*)        utf8_buf;
    uint8_t        *dst_str = (uint8_t*)        utf8_buf;
    uint32_t const *src_str = (uint32_t const*) wide_str;

    // compute the source and destination sizes.
    dst_len = utf8::size_for_utf32(src_str, &src_len);
    if (out_size != NULL) *out_size = dst_len;

    // convert the string, if there's enough space.
    if (dst_len <= buf_size)
    {
        utf8::from_utf32(
            &src_str,
             src_str + src_len,
            &dst_ptr,
             dst_ptr + dst_len);
        dst_str[dst_len - 1] = 0;
        return (char*) dst_str;
    }
    // not enough space in the output buffer.
    return NULL;

#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

wchar_t* utf8::to_wide(
    char const    *utf8_str,
    wchar_t       *wide_buf,
    size_t         buf_size,
    size_t        *out_size)
{
    size_t         slb   = utf8_str ? utf8::string_length_bytes(utf8_str) : 0;
    size_t         len   = utf8::string_length_chars(utf8_str);
    uint8_t const *src   = (uint8_t const*)   utf8_str;
    uint16_t      *dst16 = NULL;
    uint32_t      *dst32 = NULL;

    if (out_size != NULL)
    {
        // store the number of chars.
        *out_size = len + 1;
    }
    if (NULL == wide_buf || buf_size < (len + 1))
    {
        // not enough space in output.
        return NULL;
    }

#if CMN_IS_WINDOWS
    dst32 = NULL;
    dst16 = (uint16_t*) wide_buf;
    if (len > 0)
    {
        if (!utf8::to_utf16(&src, src + slb, &dst16, dst16 + len))
        {
            // conversion failed.
            return NULL;
        }
    }
    *dst16 = 0;
#else
    dst16 = NULL;
    dst32 = (uint32_t*) wide_buf;
    if (len > 0)
    {
        if (!utf8::to_utf32(&src, src + slb, &dst32, dst32 + len))
        {
            // conversion failed.
            return NULL;
        }
    }
    *dst32 = 0;
#endif
    return wide_buf;
}

bool utf8::valid_codepoint(uint8_t const *src, size_t len)
{
    uint8_t const *src_ptr = src + len;
    uint8_t        a       = 0;

    // @NOTE: outer switch fallthrough is intentional.
    switch (len)
    {
    default: return false;
    case 4:  if ((a = (*--src_ptr)) < 0x80 || a > 0xBF) {return false;}
    case 3:  if ((a = (*--src_ptr)) < 0x80 || a > 0xBF) {return false;}
    case 2:
        {
            if ((a = (*--src_ptr))  > 0xBF)
            {
                return false;
            }
            switch (*src)
            {
            case 0xE0: if (a < 0xA0) {return false;} break;
            case 0xED: if (a > 0x9F) {return false;} break;
            case 0xF0: if (a < 0x90) {return false;} break;
            case 0xF4: if (a > 0x8F) {return false;} break;
            default:   if (a < 0x80) {return false;}
            }
        }
    case 1:  if (*src >= 0x80 && *src < 0xC2) {return false;}
    }
    if (*src > 0xF4) {return false;}
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void utf8::string_length(
    char const *str,
    size_t     *out_chars,
    size_t     *out_bytes)
{
    size_t sequence_len = 0;
    size_t chars_len    = 0;
    size_t bytes_len    = 0;

    if (str != NULL)
    {
        while (*str)
        {
            sequence_len   = UTF8_SEQUENCE_LENGTH(str);
            if (sequence_len)
            {
                chars_len += 1;
                bytes_len += sequence_len;
                str       += sequence_len;
            }
            else
            {
                bytes_len += 1;
                str       += 1;
            }
        }
    }
    if (out_chars != NULL) *out_chars = chars_len;
    if (out_bytes != NULL) *out_bytes = bytes_len + 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::string_length_bytes(char const *str)
{
    return (str != NULL) ? ::strlen(str) + 1 : 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::string_length_chars(char const *str)
{
    size_t sequence_len = 0;
    size_t total_len    = 0;

    if (str != NULL)
    {
        while (*str)
        {
            sequence_len = UTF8_SEQUENCE_LENGTH(str);
            if (sequence_len)
            {
                ++total_len;
                str += sequence_len;
            }
            else
            {
                ++str;
            }
        }
        return total_len;
    }
    else
    {
        // empty/NULL string - no characters.
        return 0;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::next_codepoint(char const *str, uint32_t *out_codepoint)
{
    if (str != NULL)
    {
        uint32_t ch  = UTF8_GET_CHARACTER(str);
        str         += UTF8_CHARACTER_LENGTH(ch);
        if (out_codepoint != NULL) *out_codepoint = ch;
    }
    else
    {
        // NULL source string.
        if (out_codepoint != NULL) *out_codepoint = 0;
    }
    return (char*) str;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::codepoint_at(char const *str, size_t index)
{
    if (str != NULL)
    {
        while (index > 0)
        {
            uint32_t ch = UTF8_GET_CHARACTER(str);
            if (0 == ch)
            {
                // hit the end of the string.
                return NULL;
            }
            str += UTF8_CHARACTER_LENGTH(ch);
            --index;
        }
        return (char*) str;
    }
    else
    {
        // invalid source string.
        return NULL;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void utf8::to_upper(char *str)
{
    if (NULL == str)
    {
        // invalid string buffer.
        return;
    }

    // perform the string conversion.
    while (*str)
    {
        uint32_t ch = UTF8_GET_CHARACTER(str);
        if (ch     != 0xFFFFFFFFUL)
        {
            if (ch != 0)
            {
                size_t cl = UTF8_CHARACTER_LENGTH(ch);
                ch        = UTF8_TO_UPPER(ch);
                UTF8_PUT_CHARACTER(str, ch);
                str      += cl;
            }
            else
            {
                // NULL-terminator character.
                break;
            }
        }
        else
        {
            // invalid source character - skip it.
            ++str;
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::copy_string(char *dst, size_t dst_size, char const *src)
{
    size_t n = 0;

    if (NULL == dst || 0 == dst_size)
    {
        // invalid destination buffer/size.
        return 0;
    }
    if (NULL == src)
    {
        // NULL source string - NULL-terminate dst and bail.
        *dst = 0;
        return 0;
    }

    // perform the string copy operation.
    while (*src)
    {
        uint32_t ch = UTF8_GET_CHARACTER(src);
        if (ch     != 0xFFFFFFFFUL)
        {
            if (ch != 0)
            {
                size_t cl     = UTF8_CHARACTER_LENGTH(ch);
                if (dst_size  > cl)
                {
                    UTF8_PUT_CHARACTER(dst, ch);
                    src      += cl;
                    dst      += cl;
                    dst_size -= cl;
                    ++n;
                }
                else
                {
                    // out of space in the destination buffer.
                    break;
                }
            }
            else
            {
                // NULL-terminator character.
                break;
            }
        }
        else
        {
            // invalid source character - skip it.
            ++src;
        }
    }
    // make sure to add the NULL-terminator, even on error.
    if (dst_size >= 1) *dst = 0;
    return n;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::copy_string(
    char       *dst,
    size_t      dst_size,
    char const *src,
    size_t      count)
{
    size_t n = 0;

    if (NULL == dst || 0 == dst_size)
    {
        // invalid destination buffer/size.
        return 0;
    }
    if (NULL == src)
    {
        // NULL source string - NULL-terminate dst and bail.
        *dst = 0;
        return 0;
    }

    // perform the string copy operation.
    while (*src && n < count)
    {
        uint32_t ch = UTF8_GET_CHARACTER(src);
        if (ch     != 0xFFFFFFFFUL)
        {
            if (ch != 0)
            {
                size_t    cl  = UTF8_CHARACTER_LENGTH(ch);
                if (dst_size  > cl)
                {
                    UTF8_PUT_CHARACTER(dst, ch);
                    src      += cl;
                    dst      += cl;
                    dst_size -= cl;
                    ++n;
                }
                else
                {
                    // out of space in the destination buffer.
                    break;
                }
            }
            else
            {
                // NULL-terminator character.
                break;
            }
        }
        else
        {
            // invalid source character - skip it.
            ++src;
        }
    }
    // make sure to add the NULL-terminator, even on error.
    if (dst_size >= 1) *dst = 0;
    return n;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::substring(
    char       *dst,
    size_t      dst_size,
    char const *src_start,
    char const *src_end)
{
    size_t n = 0;

    if (NULL == dst || 0 == dst_size)
    {
        // invalid destination buffer/size.
        return 0;
    }
    if (NULL == src_start)
    {
        // NULL source string - NULL-terminate dst and bail.
        *dst = 0;
        return 0;
    }

    // perform the string copy operation.
    while (*src_start && src_start != src_end)
    {
        uint32_t ch = UTF8_GET_CHARACTER(src_start);
        if (ch     != 0xFFFFFFFFUL)
        {
            if (ch != 0)
            {
                size_t cl      = UTF8_CHARACTER_LENGTH(ch);
                if (dst_size   > cl)
                {
                    UTF8_PUT_CHARACTER(dst, ch);
                    src_start += cl;
                    dst       += cl;
                    dst_size  -= cl;
                    ++n;
                }
                else
                {
                    // out of space in the destination buffer.
                    break;
                }
            }
            else
            {
                // NULL-terminator character.
                break;
            }
        }
        else
        {
            // invalid source character - skip it.
            ++src_start;
        }
    }
    // make sure to add the NULL-terminator, even on error.
    if (dst_size >= 1) *dst = 0;
    return n;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::append_string(char *dst, size_t dst_size, char const *src)
{
    size_t      len = 0;
    char const *str = dst;
    if (NULL == dst || 0 == dst_size)
    {
        // invalid destination buffer/size.
        return 0;
    }
    while (*str++ != 0)
    {
        ++len;
    }
    if (len >= dst_size)
    {
        // invalid destination size.
        return 0;
    }
    return utf8::copy_string(&dst[len], dst_size - len, src);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t utf8::append_string(
    char       *dst,
    size_t      dst_size,
    char const *src,
    size_t      count)
{
    size_t      len = 0;
    char const *str = dst;
    if (NULL == dst || 0 == dst_size)
    {
        // invalid destination buffer/size.
        return 0;
    }
    while (*str++ != 0)
    {
        ++len;
    }
    if (len >= dst_size)
    {
        // invalid destination size.
        return 0;
    }
    return utf8::copy_string(&dst[len], dst_size - len, src, count);
}

/*/////////////////////////////////////////////////////////////////////////80*/

signed utf8::compare_strings(char const *a, char const *b)
{
    uint32_t ch1;
    uint32_t ch2;
    uint32_t len;

    if (NULL == a && NULL == b)
    {
        return  0;
    }
    if (NULL == a && NULL != b)
    {
        return -1;
    }
    if (NULL != a && NULL == b)
    {
        return +1;
    }

    // both strings are non-NULL.
    while (*a && *b)
    {
        ch1  = UTF8_GET_CHARACTER(a);
        ch2  = UTF8_GET_CHARACTER(b);
        if (ch1 != ch2)
        {
            return ch1 - ch2;
        }
        len  = UTF8_CHARACTER_LENGTH(ch1);
        a   += len;
        b   += len;
    }
    ch1 = UTF8_GET_CHARACTER(a);
    ch2 = UTF8_GET_CHARACTER(b);
    return ch1 - ch2;
}

/*/////////////////////////////////////////////////////////////////////////80*/

signed utf8::compare_strings(char const *a, char const *b, size_t count)
{
    uint32_t ch1;
    uint32_t ch2;
    uint32_t len;
    size_t   n = 0;

    if (NULL == a && NULL == b)
    {
        return  0;
    }
    if (NULL == a && NULL != b)
    {
        return -1;
    }
    if (NULL != a && NULL == b)
    {
        return +1;
    }

    // both strings are non-NULL.
    while (*a && *b && n < count)
    {
        ch1  = UTF8_GET_CHARACTER(a);
        ch2  = UTF8_GET_CHARACTER(b);
        if (ch1 != ch2)
        {
            return ch1 - ch2;
        }
        len  = UTF8_CHARACTER_LENGTH(ch1);
        a   += len;
        b   += len;
        ++n;
    }
    ch1 = UTF8_GET_CHARACTER(a);
    ch2 = UTF8_GET_CHARACTER(b);
    return ch1 - ch2;
}

/*/////////////////////////////////////////////////////////////////////////80*/

signed utf8::compare_strings_normalized(char const *a, char const *b)
{
    uint32_t ch1;
    uint32_t ch2;
    uint32_t len;

    if (NULL == a && NULL == b)
    {
        return  0;
    }
    if (NULL == a && NULL != b)
    {
        return -1;
    }
    if (NULL != a && NULL == b)
    {
        return +1;
    }

    // both strings are non-NULL.
    while (*a && *b)
    {
        ch1  = UTF8_GET_CHARACTER(a);
        ch2  = UTF8_GET_CHARACTER(b);
        ch1  = UTF8_TO_UPPER(ch1);
        ch2  = UTF8_TO_UPPER(ch2);
        if (ch1 != ch2)
        {
            return ch1 - ch2;
        }
        len  = UTF8_CHARACTER_LENGTH(ch1);
        a   += len;
        b   += len;
    }
    ch1 = UTF8_GET_CHARACTER(a);
    ch2 = UTF8_GET_CHARACTER(b);
    ch1 = UTF8_TO_UPPER(ch1);
    ch2 = UTF8_TO_UPPER(ch2);
    return ch1 - ch2;
}

/*/////////////////////////////////////////////////////////////////////////80*/

signed utf8::compare_strings_normalized(
    char const *a,
    char const *b,
    size_t      count)
{
    uint32_t ch1;
    uint32_t ch2;
    uint32_t len;
    size_t   n = 0;

    if (NULL == a && NULL == b)
    {
        return  0;
    }
    if (NULL == a && NULL != b)
    {
        return -1;
    }
    if (NULL != a && NULL == b)
    {
        return +1;
    }

    // both strings are non-NULL.
    while (*a && *b && n < count)
    {
        ch1  = UTF8_GET_CHARACTER(a);
        ch2  = UTF8_GET_CHARACTER(b);
        ch1  = UTF8_TO_UPPER(ch1);
        ch2  = UTF8_TO_UPPER(ch2);
        if (ch1 != ch2)
        {
            return ch1 - ch2;
        }
        len  = UTF8_CHARACTER_LENGTH(ch1);
        a   += len;
        b   += len;
        ++n;
    }
    ch1 = UTF8_GET_CHARACTER(a);
    ch2 = UTF8_GET_CHARACTER(b);
    ch1 = UTF8_TO_UPPER(ch1);
    ch2 = UTF8_TO_UPPER(ch2);
    return ch1 - ch2;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::first(char const *str, uint32_t ch)
{
    if (str != NULL)
    {
        while (*str)
        {
            uint32_t c   = UTF8_GET_CHARACTER(str);
            size_t   len = UTF8_CHARACTER_LENGTH(ch);
            if (ch == c)
            {
                // found the first occurrence.
                return (char*) str;
            }
            str += len;
        }
        return NULL;
    }
    else
    {
        // the codepoint will not be found.
        return NULL;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::first_of(char const *str, char const *chars)
{
    uint32_t ch1 = 0;
    size_t   len = 0;

    if (NULL == str)
    {
        return NULL;
    }
    if (NULL == chars)
    {
        // return a pointer to the start of the string.
        return (char*) str;
    }

    ch1 = UTF8_GET_CHARACTER(str);
    while (ch1)
    {
        if (utf8::first(chars, ch1) != NULL)
        {
            return (char*) str;
        }
        len  = UTF8_CHARACTER_LENGTH(ch1);
        str += len;
        ch1  = UTF8_GET_CHARACTER(str);
    }
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::first_not_of(char const *str, char const *chars)
{
    uint32_t ch1 = 0;
    size_t   len = 0;

    if (NULL == str)
    {
        return NULL;
    }
    if (NULL == chars)
    {
        // return a pointer to the start of the string.
        return (char*) str;
    }

    ch1 = UTF8_GET_CHARACTER(str);
    while (ch1)
    {
        if (utf8::first(chars, ch1) == NULL)
        {
            return (char*) str;
        }
        len  = UTF8_CHARACTER_LENGTH(ch1);
        str += len;
        ch1  = UTF8_GET_CHARACTER(str);
    }
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::last(char const *str, uint32_t ch)
{
    if (str != NULL)
    {
        char *last = NULL;
        while (*str)
        {
            uint32_t c   = UTF8_GET_CHARACTER(str);
            size_t   len = UTF8_CHARACTER_LENGTH(ch);
            if (ch == c)
            {
                last = (char*) str;
            }
            str += len;
        }
        return last;
    }
    else
    {
        // the codepoint will not be found.
        return NULL;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::find(char const *search, char const *find)
{
    uint32_t ch1 = 0;
    size_t   len = 0;

    if (NULL == search)
    {
        return NULL;
    }
    if (NULL == find)
    {
        // return the end of the string.
        while (*search++ != 0)
        {
            ++len;
        }
        return (char*) search;
    }

    ch1 = UTF8_GET_CHARACTER(search);
    len = utf8::string_length_chars(find);
    while (ch1)
    {
        if (0 == utf8::compare_strings(search, find, len))
        {
            return (char*) search;
        }
        search += UTF8_CHARACTER_LENGTH(ch1);
        ch1     = UTF8_GET_CHARACTER(search);
    }
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::find_normalized(char const *search, char const *find)
{
    uint32_t ch1 = 0;
    size_t   len = 0;

    if (NULL == search)
    {
        return NULL;
    }
    if (NULL == find)
    {
        // return the end of the string.
        while (*search++ != 0)
        {
            ++len;
        }
        return (char*) search;
    }

    ch1 = UTF8_GET_CHARACTER(search);
    len = utf8::string_length_chars(find);
    while (ch1)
    {
        if (0 == utf8::compare_strings_normalized(search, find, len))
        {
            return (char*) search;
        }
        search += UTF8_CHARACTER_LENGTH(ch1);
        ch1     = UTF8_GET_CHARACTER(search);
    }
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::delimit(char **str, char const *delimiters)
{
    char     *s   = NULL;
    char     *r   = NULL;
    size_t    len = 0;
    uint32_t  ch1 = 0;

    if (NULL == str || NULL == *str || NULL == delimiters)
    {
        return NULL;
    }

    s   = *str;
    r   =  s;
    ch1 = UTF8_GET_CHARACTER(s);
    while (ch1)
    {
        len = UTF8_CHARACTER_LENGTH(ch1);
        if (utf8::first(delimiters, ch1) != NULL)
        {
            *s    = 0;   // overwrite the delimiter with a NULL
             s   += len;
            *str  = s;   // *str points to start of next token
            return r;    // return start of current token
        }
        s   += len;
        ch1  = UTF8_GET_CHARACTER(s);
    }
    *str = 0;
    return r;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::token(char *str, char const *delimiters, char **out_token)
{
    char     *s   = NULL;
    uint32_t  ch1 = 0;

    if (NULL == str)
    {
        if (NULL == out_token)
        {
            return NULL;
        }
        else
        {
            str = *out_token;
        }
    }
    if (NULL == str)
    {
        return NULL;
    }

    ch1 = UTF8_GET_CHARACTER(str);
    while (ch1)
    {
        char *r   = utf8::first(delimiters, ch1);
        if (NULL == s && NULL == r)
        {
            s = str;
        }
        if (NULL != s && NULL != r)
        {
            break;
        }
        str += UTF8_CHARACTER_LENGTH(ch1);
        ch1  = UTF8_GET_CHARACTER(str);
    }

    if (s == str)
    {
        s = NULL;
    }
    if (ch1)
    {
        *str = 0;
        if (out_token)
        {
            str        += UTF8_CHARACTER_LENGTH(ch1);
            *out_token  = str;
        }
    }
    else if (out_token)
    {
        *out_token = NULL;
    }
    return s;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* utf8::format(
    char const *format_string,
    char       *output_buffer,
    size_t      max_length,
    size_t     *out_length, ...)
{
    va_list  varargs;
    va_start(varargs, out_length);
#if CMN_IS_WINDOWS
    // on Windows, vsnprintf will return -1 if truncation would occur, so
    // we need to use the _vscprintf function first to figure out how many
    // characters would be generated (not including the trailing NULL.)
    // vsnprintf_s will always NULL-terminate the output buffer.
    int length = _vscprintf(format_string, varargs);
    vsnprintf_s(output_buffer, max_length, _TRUNCATE, format_string, varargs);
#else
    // on OSX and *nix systems, vsnprintf will return the number of characters
    // that would have been written to the output buffer, even if fewer
    // characters were written because the buffer is not long enough. the
    // vsnprintf function will always NULL-terminate the output buffer.
    int length = vsnprintf(output_buffer, max_length, format_string, varargs);
#endif
    if (out_length != NULL)
    {
        // store the output length for the caller.
        *out_length = (size_t) length;
    }
    va_end(varargs);
    return output_buffer;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
