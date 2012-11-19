/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libblob.cpp
///  Implements the interfaces used to manipulate blobs, which are used for
///  storing open-ended data structures, encapsulated as raw blocks of memory.
///  Blobs can be easily passed around at runtime, serialized to and from disk,
///  or sent across a network. Blobs are designed for runtime efficiency, not
///  necessarily size efficiency.
///
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <limits>
#include "libblob.hpp"
#include "common_traits.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

static const char        Base64_Chars[]   =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*/////////////////////////////////////////////////////////////////////////80*/

static const signed char Base64_Indices[] =
{

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 62, -1, -1, -1, 63,  /* ... , '+', ... '/' */
    52, 53, 54, 55, 56, 57, 58, 59,  /* '0' - '7'          */
    60, 61, -1, -1, -1, -1, -1, -1,  /* '8', '9', ...      */

    -1, 0,  1,  2,  3,  4,  5,  6,   /* ..., 'A' - 'G'     */
     7, 8,  9,  10, 11, 12, 13, 14,  /* 'H' - 'O'          */
    15, 16, 17, 18, 19, 20, 21, 22,  /* 'P' - 'W'          */
    23, 24, 25, -1, -1, -1, -1, -1,  /* 'X', 'Y', 'Z', ... */

    -1, 26, 27, 28, 29, 30, 31, 32,  /* ..., 'a' - 'g'     */
    33, 34, 35, 36, 37, 38, 39, 40,  /* 'h' - 'o'          */
    41, 42, 43, 44, 45, 46, 47, 48,  /* 'p' - 'w'          */
    49, 50, 51, -1, -1, -1, -1, -1,  /* 'x', 'y', 'z', ... */

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1
};

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::determine_text_encoding(uint8_t BOM[4], size_t *out_size)
{
    size_t  bom_size = 0;
    int32_t text_enc = blob::TEXT_ENCODING_UNSURE;

    if (0 == BOM[0])
    {
        if (0 == BOM[1] && 0xFE == BOM[2] && 0xFF == BOM[3])
        {
            // UTF32 big-endian.
            bom_size = 4;
            text_enc = blob::TEXT_ENCODING_UTF32_BE;
        }
        else
        {
            // no BOM (or unrecognized).
            bom_size = 0;
            text_enc = blob::TEXT_ENCODING_UNSURE;
        }
    }
    else if (0xFF == BOM[0])
    {
        if (0xFE == BOM[1])
        {
            if (0 == BOM[2] && 0 == BOM[3])
            {
                // UTF32 little-endian.
                bom_size = 4;
                text_enc = blob::TEXT_ENCODING_UTF32_LE;
            }
            else
            {
                // UTF16 little-endian.
                bom_size = 2;
                text_enc = blob::TEXT_ENCODING_UTF16_LE;
            }
        }
        else
        {
            // no BOM (or unrecognized).
            bom_size = 0;
            text_enc = blob::TEXT_ENCODING_UNSURE;
        }
    }
    else if (0xFE == BOM[0] && 0xFF == BOM[1])
    {
        // UTF16 big-endian.
        bom_size = 2;
        text_enc = blob::TEXT_ENCODING_UTF16_BE;
    }
    else if (0xEF == BOM[0] && 0xBB == BOM[1] && 0xBF == BOM[2])
    {
        // UTF-8.
        bom_size = 3;
        text_enc = blob::TEXT_ENCODING_UTF8;
    }
    else
    {
        // no BOM (or unrecognized).
        bom_size = 0;
        text_enc = blob::TEXT_ENCODING_UNSURE;
    }

    if (out_size != NULL)
    {
        // store the BOM size in bytes.
        *out_size = bom_size;
    }
    return text_enc;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::bom(int32_t encoding, uint8_t out_BOM[4])
{
    size_t  bom_size = 0;
    switch (encoding)
    {
        case blob::TEXT_ENCODING_UTF8:
            {
                bom_size   = 3;
                out_BOM[0] = 0xEF;
                out_BOM[1] = 0xBB;
                out_BOM[2] = 0xBF;
                out_BOM[3] = 0x00;
            }
            break;

        case blob::TEXT_ENCODING_UTF16_BE:
            {
                bom_size   = 2;
                out_BOM[0] = 0xFE;
                out_BOM[1] = 0xFF;
                out_BOM[2] = 0x00;
                out_BOM[3] = 0x00;
            }
            break;

        case blob::TEXT_ENCODING_UTF16_LE:
            {
                bom_size   = 2;
                out_BOM[0] = 0xFF;
                out_BOM[1] = 0xFE;
                out_BOM[2] = 0x00;
                out_BOM[3] = 0x00;
            }
            break;

        case blob::TEXT_ENCODING_UTF32_BE:
            {
                bom_size   = 4;
                out_BOM[0] = 0x00;
                out_BOM[1] = 0x00;
                out_BOM[2] = 0xFE;
                out_BOM[3] = 0xFF;
            }
            break;

        case blob::TEXT_ENCODING_UTF32_LE:
            {
                bom_size   = 4;
                out_BOM[0] = 0xFF;
                out_BOM[1] = 0xFE;
                out_BOM[2] = 0x00;
                out_BOM[3] = 0x00;
            }
            break;

        default:
            {
                // no byte order marker.
                bom_size   = 0;
                out_BOM[0] = 0;
                out_BOM[1] = 0;
                out_BOM[2] = 0;
                out_BOM[3] = 0;
            }
            break;
    }
    return bom_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::base64_size(size_t binary_size, size_t *out_pad_size)
{
    // base64 transforms 3 input bytes into 4 output bytes.
    // pad the binary size so it is evenly divisible by 3.
    size_t rem = binary_size % 3;
    size_t adj = (rem != 0)  ? 3 - rem : 0;
    if (out_pad_size  != 0)  *out_pad_size = adj;
    return ((binary_size + adj) / 3) * 4 + 1; // +1 for NULL
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::binary_size(size_t base64_size, size_t pad_size)
{
    return (((3 * base64_size) / 4) - pad_size);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::binary_size(char const *base64_source, size_t base64_length)
{
    if (NULL == base64_source || 0 == base64_length)
    {
        // zero-length input - zero-length output.
        return 0;
    }

    // end points at the last character in the base64 source data.
    char const *end = base64_source + base64_length - 1;
    size_t      pad = 0;
    if (base64_length >= 1 && '=' == *end--)  ++pad;
    if (base64_length >= 2 && '=' == *end--)  ++pad;
    return blob::binary_size(base64_length, pad);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::base64_encode(
    void const *input,
    size_t      input_size,
    char       *output,
    size_t      output_size)
{
    size_t         pad    = 0;
    size_t         req    = blob::base64_size(input_size, &pad);
    size_t         ins    = input_size;
    uint8_t const *inp    = (uint8_t const*) input;
    char          *outp   = output;
    uint8_t        buf[4] = {0};

    if (output_size < req)
    {
        // insufficient space in buffer.
        return 0;
    }

    // process input three bytes at a time.
    while (ins >= 3)
    {
        // buf[0] = left  6 bits of inp[0].
        // buf[1] = right 2 bits of inp[0], left 4 bits of inp[1].
        // buf[2] = right 4 bits of inp[1], left 2 bits of inp[2].
        // buf[3] = right 6 bits of inp[2].
        buf[0]  = (uint8_t)  ((inp[0] & 0xFC) >> 2);
        buf[1]  = (uint8_t) (((inp[0] & 0x03) << 4) + ((inp[1] & 0xF0) >> 4));
        buf[2]  = (uint8_t) (((inp[1] & 0x0F) << 2) + ((inp[2] & 0xC0) >> 6));
        buf[3]  = (uint8_t)   (inp[2] & 0x3F);
        // produce four bytes of output from three bytes of input.
        *outp++ = Base64_Chars[buf[0]];
        *outp++ = Base64_Chars[buf[1]];
        *outp++ = Base64_Chars[buf[2]];
        *outp++ = Base64_Chars[buf[3]];
        // we've consumed and processed three bytes of input.
        inp    += 3;
        ins    -= 3;
    }
    // pad any remaining input (either 1 or 2 bytes) up to three bytes; encode.
    if (ins > 0)
    {
        uint8_t src[3];
        size_t  i  = 0;

        // copy remaining real bytes from input; pad with nulls.
        for (i = 0; i  < ins; ++i) src[i] = *inp++;
        for (     ; i != 3;   ++i) src[i] = 0;
        // buf[0] = left  6 bits of inp[0].
        // buf[1] = right 2 bits of inp[0], left 4 bits of inp[1].
        // buf[2] = right 4 bits of inp[1], left 2 bits of inp[2].
        // buf[3] = right 6 bits of inp[2].
        buf[0]  = (uint8_t)  ((src[0] & 0xFC) >> 2);
        buf[1]  = (uint8_t) (((src[0] & 0x03) << 4) + ((src[1] & 0xF0) >> 4));
        buf[2]  = (uint8_t) (((src[1] & 0x0F) << 2) + ((src[2] & 0xC0) >> 6));
        buf[3]  = (uint8_t)   (src[2] & 0x3F);
        // produce four bytes of output from three bytes of input.
        *(outp+0) = Base64_Chars[buf[0]];
        *(outp+1) = Base64_Chars[buf[1]];
        *(outp+2) = Base64_Chars[buf[2]];
        *(outp+3) = Base64_Chars[buf[3]];
        // overwrite the junk characters with '=' characters.
        for (outp += 1 + ins; ins++ != 3;)  *outp++ = '=';
    }
    // always append the trailing null.
    *outp++ = '\0';
    // return the number of bytes written.
    return ((size_t)(outp - output));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::base64_decode(
    char const *input,
    size_t      input_size,
    void       *output,
    size_t      output_size)
{
    char const *inp    = input;
    char const *end    = input + input_size;
    signed char idx[4] = {0};
    uint8_t    *outp   = (uint8_t*) output;
    size_t      curr   =  0;
    size_t      pad    =  0;
    size_t      req    = blob::binary_size(input_size, 0);

    if (output_size < (req - 2))
    {
        // insufficient space in buffer.
        return 0;
    }

    while (inp != end)
    {
        char ch = *inp++;
        if (ch != '=')
        {
            signed char chi = Base64_Indices[(unsigned char)ch];
            if (chi != -1)
            {
                // valid character, buffer it.
                idx[curr++] = chi;
                pad         = 0;
            }
            else
            {
                // unknown character - skip it.
                continue;
            }
        }
        else
        {
            // this is a padding character.
            idx[curr++] = 0;
            ++pad;
        }

        if (4 == curr)
        {
            // we've read three bytes of data; generate output.
            curr     = 0;
            *outp++  = (uint8_t) ((idx[0] << 2) + ((idx[1] & 0x30) >> 4));
            if (pad != 2)
            {
                *outp++  = (uint8_t) (((idx[1] & 0xF) << 4) + ((idx[2] & 0x3C) >> 2));
                if (pad != 1)
                {
                    *outp++ = (uint8_t) (((idx[2] & 0x3) << 6) + idx[3]);
                }
            }
            if (pad != 0) break;
        }
    }
    // return the number of bytes written.
    return ((size_t)(outp - ((uint8_t*)output)));
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::field_type_minimum_integer(
    int64_t max_value,
    bool    support_signed)
{
    if (max_value < 0 || support_signed)
    {
        // check signed types only.
        if (max_value >= std::numeric_limits<int8_t>::min()  &&
            max_value <= std::numeric_limits<int8_t>::max())
        {
            return blob::FIELD_TYPE_SINT8;
        }
        if (max_value >= std::numeric_limits<int16_t>::min() &&
            max_value <= std::numeric_limits<int16_t>::max())
        {
            return blob::FIELD_TYPE_SINT16;
        }
        if (max_value >= std::numeric_limits<int32_t>::min() &&
            max_value <= std::numeric_limits<int32_t>::max())
        {
            return blob::FIELD_TYPE_SINT32;
        }
        return blob::FIELD_TYPE_SINT64;
    }
    else
    {
        // check unsigned types only.
        uint64_t uns_value = (uint64_t) max_value;
        if (uns_value >= std::numeric_limits<uint8_t>::min()  &&
            uns_value <= std::numeric_limits<uint8_t>::max())
        {
            return blob::FIELD_TYPE_UINT8;
        }
        if (uns_value >= std::numeric_limits<uint16_t>::min() &&
            uns_value <= std::numeric_limits<uint16_t>::max())
        {
            return blob::FIELD_TYPE_UINT16;
        }
        if (uns_value >= std::numeric_limits<uint32_t>::min() &&
            uns_value <= std::numeric_limits<uint32_t>::max())
        {
            return blob::FIELD_TYPE_UINT32;
        }
        return blob::FIELD_TYPE_UINT64;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool blob::field_type_is_signed(int32_t field_type)
{
    switch (field_type)
    {
        case blob::FIELD_TYPE_NONE:        return false;
        case blob::FIELD_TYPE_NULL:        return false;
        case blob::FIELD_TYPE_BOOLEAN:     return false;
        case blob::FIELD_TYPE_SINT8:       return true;
        case blob::FIELD_TYPE_UINT8:       return false;
        case blob::FIELD_TYPE_CHAR:        return true;
        case blob::FIELD_TYPE_SINT16:      return true;
        case blob::FIELD_TYPE_UINT16:      return false;
        case blob::FIELD_TYPE_SINT32:      return true;
        case blob::FIELD_TYPE_UINT32:      return false;
        case blob::FIELD_TYPE_SINT64:      return true;
        case blob::FIELD_TYPE_UINT64:      return false;
        case blob::FIELD_TYPE_FLOAT32:     return true;
        case blob::FIELD_TYPE_FLOAT64:     return true;
        case blob::FIELD_TYPE_VECTOR_2F:   return true;
        case blob::FIELD_TYPE_VECTOR_3F:   return true;
        case blob::FIELD_TYPE_VECTOR_4F:   return true;
        case blob::FIELD_TYPE_MATRIX_2X2F: return true;
        case blob::FIELD_TYPE_MATRIX_3X3F: return true;
        case blob::FIELD_TYPE_MATRIX_3X4F: return true;
        case blob::FIELD_TYPE_MATRIX_4X4F: return true;
        default:                           break;
    }
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool blob::field_type_is_unsigned(int32_t field_type)
{
    switch (field_type)
    {
        case blob::FIELD_TYPE_NONE:        return false;
        case blob::FIELD_TYPE_NULL:        return false;
        case blob::FIELD_TYPE_BOOLEAN:     return true;
        case blob::FIELD_TYPE_SINT8:       return false;
        case blob::FIELD_TYPE_UINT8:       return true;
        case blob::FIELD_TYPE_CHAR:        return false;
        case blob::FIELD_TYPE_SINT16:      return false;
        case blob::FIELD_TYPE_UINT16:      return true;
        case blob::FIELD_TYPE_SINT32:      return false;
        case blob::FIELD_TYPE_UINT32:      return true;
        case blob::FIELD_TYPE_SINT64:      return false;
        case blob::FIELD_TYPE_UINT64:      return true;
        case blob::FIELD_TYPE_FLOAT32:     return false;
        case blob::FIELD_TYPE_FLOAT64:     return false;
        case blob::FIELD_TYPE_VECTOR_2F:   return false;
        case blob::FIELD_TYPE_VECTOR_3F:   return false;
        case blob::FIELD_TYPE_VECTOR_4F:   return false;
        case blob::FIELD_TYPE_MATRIX_2X2F: return false;
        case blob::FIELD_TYPE_MATRIX_3X3F: return false;
        case blob::FIELD_TYPE_MATRIX_3X4F: return false;
        case blob::FIELD_TYPE_MATRIX_4X4F: return false;
        default:                           break;
    }
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::field_size_for_type(int32_t field_type)
{
    switch (field_type)
    {
        case blob::FIELD_TYPE_NONE:        return 0;
        case blob::FIELD_TYPE_NULL:        return 0;
        case blob::FIELD_TYPE_BOOLEAN:     return sizeof(int8_t);
        case blob::FIELD_TYPE_SINT8:       return sizeof(int8_t);
        case blob::FIELD_TYPE_UINT8:       return sizeof(uint8_t);
        case blob::FIELD_TYPE_CHAR:        return sizeof(int8_t);
        case blob::FIELD_TYPE_SINT16:      return sizeof(int16_t);
        case blob::FIELD_TYPE_UINT16:      return sizeof(uint16_t);
        case blob::FIELD_TYPE_SINT32:      return sizeof(int32_t);
        case blob::FIELD_TYPE_UINT32:      return sizeof(uint32_t);
        case blob::FIELD_TYPE_SINT64:      return sizeof(int64_t);
        case blob::FIELD_TYPE_UINT64:      return sizeof(uint64_t);
        case blob::FIELD_TYPE_FLOAT32:     return sizeof(float);
        case blob::FIELD_TYPE_FLOAT64:     return sizeof(double);
        case blob::FIELD_TYPE_VECTOR_2F:   return sizeof(float) * 2;
        case blob::FIELD_TYPE_VECTOR_3F:   return sizeof(float) * 3;
        case blob::FIELD_TYPE_VECTOR_4F:   return sizeof(float) * 4;
        case blob::FIELD_TYPE_MATRIX_2X2F: return sizeof(float) * 4;
        case blob::FIELD_TYPE_MATRIX_3X3F: return sizeof(float) * 9;
        case blob::FIELD_TYPE_MATRIX_3X4F: return sizeof(float) * 12;
        case blob::FIELD_TYPE_MATRIX_4X4F: return sizeof(float) * 16;
        default:                           break;
    }
    return BLOB_FIELD_SIZE_VARIABLE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::total_size_for_type(int32_t field_type)
{
    size_t base_size = blob::field_size_for_type(field_type);
    return base_size + sizeof(int32_t);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::optimize(
    void * CMN_RESTRICT blob_dst,
    void * CMN_RESTRICT blob_src,
    size_t              blob_size)
{
    uint8_t *dst_ptr = (uint8_t*) blob_dst;
    uint8_t *src_ptr = (uint8_t*) blob_src;
    uint8_t *src_end =((uint8_t*) blob_src) + blob_size;
    size_t   src_ofs = 0;
    while (src_ptr  != src_end)
    {
        blob::field_t  field  = {0};
        switch (blob::field_at(blob_src, src_ofs, &field))
        {
            case    blob::FIELD_TYPE_GN_OBJECT:
                {
                    // this is an optimizable object. the generic and the optimized
                    // object representations have the same size, but the data is
                    // organized differently in memory.
                    size_t bytes_written = blob::optimize_object_field(
                        blob_dst, src_ofs,
                        blob_src, src_ofs);
                    dst_ptr += bytes_written;
                    src_ptr += bytes_written;
                    src_ofs += bytes_written;
                }
                break;

            case    blob::FIELD_TYPE_ARRAY:
                {
                    size_t bytes_written = blob::optimize_array_field(
                        blob_dst, src_ofs,
                        blob_src, src_ofs);
                    dst_ptr += bytes_written;
                    src_ptr += bytes_written;
                    src_ofs += bytes_written;
                }
                break;

            default:
                {
                    // no need to optimize; we can just copy this data directly.
                    traits::copy(src_ptr, field.total_size, dst_ptr);
                    dst_ptr += field.total_size;
                    src_ptr += field.total_size;
                    src_ofs += field.total_size;
                }
                break;
        }
    }
    return blob_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::optimize_object_field(
    void * CMN_RESTRICT blob_dst,
    ptrdiff_t           blob_dst_offset,
    void * CMN_RESTRICT blob_src,
    ptrdiff_t           blob_src_offset)
{
    uint8_t   *dst_ptr        = (uint8_t*) blob_dst;
    uint8_t   *src_ptr        = (uint8_t*) blob_src;
    ptrdiff_t  src_type_ofs   = blob_src_offset + 0;
    ptrdiff_t  src_count_ofs  = src_type_ofs    + sizeof(int32_t);
    ptrdiff_t  src_size_ofs   = src_count_ofs   + sizeof(uint32_t);
    ptrdiff_t  src_data_ofs   = src_size_ofs    + sizeof(uint32_t);
    uint32_t   field_count    = blob::read_u32(src_ptr, src_count_ofs);
    size_t     dst_name_sz    = field_count     * sizeof(uint32_t);
    size_t     dst_offset_sz  = field_count     * sizeof(uint32_t);
    ptrdiff_t  dst_type_ofs   = blob_dst_offset + 0;
    ptrdiff_t  dst_count_ofs  = dst_type_ofs    + sizeof(int32_t);
    ptrdiff_t  dst_name_ofs   = dst_count_ofs   + sizeof(uint32_t);
    ptrdiff_t  dst_offset_ofs = dst_name_ofs    + dst_name_sz;
    ptrdiff_t  dst_data_ofs   = dst_offset_ofs  + dst_offset_sz;
    uint32_t  *dst_names      = blob::data_at<uint32_t>(dst_ptr, dst_name_ofs);
    uint32_t  *dst_offsets    = blob::data_at<uint32_t>(dst_ptr, dst_offset_ofs);
    uint8_t   *dst_fields     = blob::data_at<uint8_t> (dst_ptr, dst_data_ofs);
    int32_t    type           = blob::FIELD_TYPE_RT_OBJECT;
    uint32_t   val_ofs        = 0;
    size_t     total_size     = 0;

    // write the field type:
    blob::write_s32(blob_dst, dst_type_ofs, type);
    total_size  += sizeof(type);

    // write the sub-field count:
    blob::write_u32(blob_dst, dst_count_ofs, field_count);
    total_size  += sizeof(field_count);

    // iterate over all fields to generate the names, offsets and data.
    for (size_t i= 0; i < field_count; ++i)
    {
        blob::generic_object_field_t sf = {0};
        blob::generic_object_field_at(blob_src, src_data_ofs, &sf);

        // write the field name:
        blob::write_u32(&dst_names[i], 0, sf.field_name);
        total_size += sizeof(uint32_t);

        // write the field offset:
        blob::write_u32(&dst_offsets[i], 0, val_ofs);
        total_size += sizeof(uint32_t);

        // if this is a generic object, we must optimize it.
        if (blob::FIELD_TYPE_GN_OBJECT == sf.field_type)
        {
            // recursively optimize the object.
            size_t obj_size = blob::optimize_object_field(
                blob_dst,
                dst_data_ofs+ val_ofs,
                blob_src,
                src_data_ofs);
            total_size     += obj_size;
            dst_fields     += obj_size;
            val_ofs        += obj_size;
            src_data_ofs   += sf.total_size;
            continue;
        }

        // this could be an array of generic objects, which
        // again require special handling and optimization.
        if (blob::FIELD_TYPE_ARRAY == sf.field_type)
        {
            // optimize the array.
            size_t arr_size = blob::optimize_array_field(
                blob_dst,
                dst_data_ofs+ val_ofs,
                blob_src,
                src_data_ofs);
            total_size     += arr_size;
            dst_fields     += arr_size;
            val_ofs        += arr_size;
            src_data_ofs   += sf.total_size;
            continue;
        }

        // this is a plain old field and nothing special is required.
        // first, copy over the field value type to the destination.
        blob::write_s32(dst_fields, 0, sf.field_type);
        total_size += sizeof(int32_t);
        dst_fields += sizeof(int32_t);
        val_ofs    += sizeof(int32_t);

        // write the field value data:
        traits::copy((uint8_t*) sf.field_data, sf.field_size, dst_fields);
        total_size += sf.field_size;
        dst_fields += sf.field_size;
        val_ofs    += sf.field_size;

        // update our source offset to the start of the next field.
        src_data_ofs   += sf.total_size;
    }
    return total_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::optimize_array_field(
    void * CMN_RESTRICT blob_dst,
    ptrdiff_t           blob_dst_offset,
    void * CMN_RESTRICT blob_src,
    ptrdiff_t           blob_src_offset)
{
    size_t total_size         =  0;
    blob::array_field_t array = {0};
    blob::array_field_at(blob_src, blob_src_offset, &array);

    // write the array header.
    size_t type_size   = blob::write_field_array(blob_dst, blob_dst_offset);
    blob_dst_offset   += type_size;
    total_size        += type_size;

    // this could be an array of generic objects, in which case we need
    // to optimize them into runtime objects using blob_optimize_object().
    if (blob::FIELD_TYPE_GN_OBJECT == array.item_type)
    {
        // write the array header, but use BLOB_FIELD_TYPE_RT_OBJECT.
        size_t obj_ofs   = 0;
        size_t hdr_size  = blob::write_field_array_info(
            blob_dst,
            blob_dst_offset,
            blob::FIELD_TYPE_RT_OBJECT,
            array.item_count);
        blob_dst_offset += hdr_size;
        total_size      += hdr_size;

        // for each source generic object, optimize into runtime form.
        for (size_t i = 0; i < array.item_count; ++i)
        {
            size_t sub_size  = blob::optimize_object_field(
                blob_dst,
                blob_dst_offset,
                array.item_data,
                obj_ofs);
            blob_dst_offset += sub_size;
            total_size      += sub_size;
            obj_ofs         += sub_size;
        }
        return total_size;
    }
    // this could be an array of arrays; optimize recursively.
    if (blob::FIELD_TYPE_ARRAY == array.item_type)
    {
        // write the array header unchanged.
        size_t arr_ofs   = 0;
        size_t hdr_size  = blob::write_field_array_info(
            blob_dst,
            blob_dst_offset,
            array.item_type,
            array.item_count);
        blob_dst_offset += hdr_size;
        total_size      += hdr_size;

        // recursively write each array.
        for (size_t i = 0; i < array.item_count; ++i)
        {
            size_t sub_size  = blob::optimize_array_field(
                blob_dst,
                blob_dst_offset,
                array.item_data,
                arr_ofs);
            blob_dst_offset += sub_size;
            total_size      += sub_size;
            arr_ofs         += sub_size;
        }
        return total_size;
    }

    // this is an array of directly-copyable values.
    size_t header_size = blob::write_field_array_info(
        blob_dst,
        blob_dst_offset,
        array.item_type,
        array.item_count);
    blob_dst_offset   += header_size;
    total_size        += header_size;

    // now copy over the array data directly.
    size_t values_size = blob::write_field_array_data(
        blob_dst,
        blob_dst_offset,
        array.item_data,
        array.array_size - header_size - type_size);
    blob_dst_offset   += values_size;
    total_size        += values_size;
    return total_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::field_data_size(void *data, ptrdiff_t byte_offset)
{
    int32_t  field_type = blob::read_s32(data, byte_offset);
    size_t   field_size = blob::field_size_for_type(field_type);
    size_t   type_size  = sizeof(int32_t);
    if (field_size != BLOB_FIELD_SIZE_VARIABLE)
    {
        // this is a fixed-length field, no further computation necessary.
        return field_size;
    }
    else if (blob::FIELD_TYPE_ARRAY == field_type)
    {
        // this is a variable-length array possibly containing variable-length
        // data for individual entries. compute the total size. the array info
        // is stored after the field_type, thus the sizeof(int32_t) offset.
        return blob::array_total_size(data, byte_offset + type_size);
    }
    else if (blob::FIELD_TYPE_GN_OBJECT == field_type)
    {
        // this is a variable-length object possibly containing variable-length
        // fields. compute the total size. the object info is stored after the
        // field_type, and thus the sizeof(int32_t) offset.
        return blob::generic_object_total_size(data, byte_offset + type_size);
    }
    else if (blob::FIELD_TYPE_RT_OBJECT == field_type)
    {
        // this is a variable-length object possibly containing variable-length
        // fields. compute the total size. the object info is stored after the
        // field_type, and thus the sizeof(int32_t) offset.
        return blob::runtime_object_total_size(data, byte_offset + type_size);
    }
    else if (blob::FIELD_TYPE_PROTOTYPE == field_type)
    {
        // this is a variable-length prototype object. compute the total size.
        // the prototype info is stored after the field type, and thus the
        // sizeof(int32_t) offset.
        return blob::prototype_total_size(data, byte_offset + type_size);
    }
    else
    {
        // unknown field type or NULL data block.
        return 0;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::field_total_size(void *data, ptrdiff_t byte_offset)
{
    return (blob::field_data_size(data, byte_offset) + sizeof(int32_t));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::array_data_size(void *data, ptrdiff_t byte_offset)
{
    uint32_t item_count  = blob::read_u32(data, byte_offset);
    int32_t  item_type   = blob::read_s32(data, byte_offset + sizeof(uint32_t));
    size_t   item_size   = blob::field_size_for_type(item_type);
    size_t   base_size   = sizeof(uint32_t) + sizeof(int32_t);
    size_t   base_offset = byte_offset + base_size;
    size_t   total_size  = 0;

    if (item_size != BLOB_FIELD_SIZE_VARIABLE)
    {
        // this is an array of fixed-length types. compute the size directly.
        return (item_count * item_size);
    }
    else if (blob::FIELD_TYPE_ARRAY == item_type)
    {
        // this is an array of arrays. compute the total size (inc. metadata.)
        for (size_t i = 0; i < item_count; ++i)
        {
            // for each sub-array, compute the total size.
            ptrdiff_t offset = base_offset  + total_size;
            size_t  sub_size = blob::array_total_size(data, offset);
            total_size      += sub_size;
        }
        return total_size;
    }
    else if (blob::FIELD_TYPE_GN_OBJECT == item_type)
    {
        // this is an array of objects. compute the total size (inc. metadata.)
        for (size_t i = 0; i < item_count; ++i)
        {
            // for each sub-object, compute the total size.
            ptrdiff_t offset = base_offset  + total_size;
            size_t  sub_size = blob::generic_object_total_size(data, offset);
            total_size      += sub_size;
        }
        return total_size;
    }
    else if (blob::FIELD_TYPE_RT_OBJECT == item_type)
    {
        // this is an array of objects. compute the total size (inc. metadata.)
        for (size_t i = 0; i < item_count; ++i)
        {
            // for each sub-object, compute the total size.
            ptrdiff_t offset = base_offset  + total_size;
            size_t  sub_size = blob::runtime_object_total_size(data, offset);
            total_size      += sub_size;
        }
        return total_size;
    }
    else if (blob::FIELD_TYPE_PROTOTYPE == item_type)
    {
        // this is an array of prototypes. compute the total size (+ metadata.)
        for (size_t i = 0; i < item_count; ++i)
        {
            // for each sub-prototype, compute the total size.
            ptrdiff_t offset = base_offset  + total_size;
            size_t  sub_size = blob::prototype_total_size(data, offset);
            total_size      += sub_size;
        }
        return total_size;
    }
    else
    {
        // unknown field type or NULL data block.
        return 0;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::array_total_size(void *data, ptrdiff_t byte_offset)
{
    size_t  base_size = sizeof(uint32_t) + sizeof(int32_t);
    return (base_size + blob::array_data_size(data, byte_offset));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::generic_object_data_size(void *data, ptrdiff_t byte_offset)
{
    uint32_t field_size  = blob::read_u32(data, sizeof(uint32_t) + byte_offset);
    return   field_size  + (sizeof(uint32_t) * 2);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::generic_object_total_size(void *data, ptrdiff_t byte_offset)
{
    return blob::generic_object_data_size(data, byte_offset) + sizeof(int32_t);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::runtime_object_data_size(void *data, ptrdiff_t byte_offset)
{
    uint32_t  field_count = blob::read_u32(data, byte_offset);
    size_t    names_size  = field_count  * sizeof(uint32_t);
    size_t    offset_size = field_count  * sizeof(uint32_t);
    ptrdiff_t offset_ofs  = byte_offset  + sizeof(uint32_t) + names_size;
    ptrdiff_t base_offset = offset_ofs   + offset_size;
    size_t    total_size  = 0;

    // sum the total size of all fields in the object.
    for (size_t i = 0; i  < field_count; ++i)
    {
        ptrdiff_t offset     = base_offset + total_size;
        size_t    field_size = blob::field_total_size(data, offset);
        total_size          += field_size;
    }
    return total_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::runtime_object_total_size(void *data, ptrdiff_t byte_offset)
{
    uint32_t  field_count = blob::read_u32(data, byte_offset);
    size_t    names_size  = field_count * sizeof(uint32_t);
    size_t    offset_size = field_count * sizeof(uint32_t);
    ptrdiff_t offset_ofs  = byte_offset + sizeof(uint32_t) + names_size;
    ptrdiff_t base_offset = offset_ofs  + offset_size;
    size_t    total_size  = 0;

    // sum the total size of all fields in the object.
    for (size_t i = 0; i  < field_count; ++i)
    {
        ptrdiff_t offset     = base_offset + total_size;
        size_t    field_size = blob::field_total_size(data, offset);
        total_size          += field_size;
    }
    return (sizeof(uint32_t) + names_size + offset_size + total_size);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::prototype_data_size(void *data, ptrdiff_t byte_offset)
{
    uint32_t  field_count = blob::read_u32(data, byte_offset);
    size_t    names_size  = field_count * sizeof(uint32_t);
    size_t    types_size  = field_count * sizeof(int32_t);
    return   (names_size  + types_size);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::prototype_total_size(void *data, ptrdiff_t byte_offset)
{
    return blob::prototype_data_size(data, byte_offset) + sizeof(uint32_t);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::field_at(
    void          *data,
    ptrdiff_t      byte_offset,
    blob::field_t *out_field_info)
{
    ptrdiff_t  type_offset      = byte_offset + 0;
    ptrdiff_t  data_offset      = byte_offset + sizeof(int32_t);
    int32_t    field_type       = blob::read_s32(data, type_offset);
    size_t     total_size       = blob::field_total_size(data, byte_offset);
    size_t     field_size       = total_size  - sizeof(int32_t);
    out_field_info->total_size  = total_size;
    out_field_info->field_size  = field_size;
    out_field_info->field_type  = field_type;
    out_field_info->field_data  = ((uint8_t*) data) + data_offset;
    return field_type;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void blob::array_field_at(
    void                *data,
    ptrdiff_t            byte_offset,
    blob::array_field_t *out_field_info)
{
    size_t   count_size        = sizeof(uint32_t);
    size_t   type_size         = sizeof(int32_t);
    out_field_info->item_count = blob::read_u32(data, byte_offset);
    out_field_info->item_type  = blob::read_s32(data, byte_offset + count_size);
    out_field_info->array_size = blob::array_total_size(data, byte_offset);
    out_field_info->item_data  = ((uint8_t*) data) + byte_offset + count_size + type_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void blob::generic_object_at(
    void                   *data,
    ptrdiff_t               byte_offset,
    blob::generic_object_t *out_field_info)
{
    ptrdiff_t count_offset       = byte_offset + 0;
    ptrdiff_t size_offset        = byte_offset + sizeof(uint32_t);
    ptrdiff_t data_offset        = byte_offset + sizeof(uint32_t) * 2;
    uint32_t  field_count        = blob::read_u32(data, count_offset);
    uint32_t  field_size         = blob::read_u32(data, size_offset);
    size_t    object_size        = blob::generic_object_total_size(data, byte_offset);
    out_field_info->object_size  = object_size;
    out_field_info->field_size   = field_size;
    out_field_info->field_count  = field_count;
    out_field_info->field_values = ((uint8_t*) data) + data_offset;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::generic_object_field_at(
    void                         *data,
    ptrdiff_t                     byte_offset,
    blob::generic_object_field_t *out_field_info)
{
    ptrdiff_t name_offset      = byte_offset + 0;
    ptrdiff_t type_offset      = byte_offset + sizeof(uint32_t);
    ptrdiff_t size_offset      = byte_offset + sizeof(uint32_t) + sizeof(int32_t);
    ptrdiff_t data_offset      = byte_offset +(sizeof(uint32_t) * 2) + sizeof(int32_t);
    uint32_t  field_name       = blob::read_u32(data, name_offset);
    int32_t   field_type       = blob::read_s32(data, type_offset);
    uint32_t  field_size       = blob::read_u32(data, size_offset);
    out_field_info->total_size = field_size  +(sizeof(uint32_t) * 2) +(sizeof(int32_t) * 2);
    out_field_info->field_name = field_name;
    out_field_info->field_type = field_type;
    out_field_info->field_size = field_size;
    out_field_info->field_data = ((uint8_t*) data) + data_offset;
    return field_type;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void blob::runtime_object_at(
    void                   *data,
    ptrdiff_t               byte_offset,
    blob::runtime_object_t *out_field_info)
{
    uint32_t  field_count         = blob::read_u32(data, byte_offset);
    size_t    names_size          = field_count   * sizeof(uint32_t);
    size_t    offset_size         = field_count   * sizeof(uint32_t);
    ptrdiff_t offset_offset       = byte_offset   + sizeof(uint32_t) + names_size;
    ptrdiff_t names_offset        = byte_offset   + sizeof(uint32_t);
    ptrdiff_t field_offset        = offset_offset + offset_size;
    size_t    object_size         = blob::runtime_object_total_size(data, byte_offset);
    out_field_info->object_size   = object_size;
    out_field_info->field_count   = field_count;
    out_field_info->field_names   = blob::data_at<uint32_t>(data, names_offset);
    out_field_info->field_offsets = blob::data_at<uint32_t>(data, offset_offset);
    out_field_info->field_values  = blob::data_at<uint8_t> (data, field_offset);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::runtime_object_field_at(
    void                         *data,
    ptrdiff_t                     byte_offset,
    blob::runtime_object_field_t *out_field_info)
{
    ptrdiff_t  type_offset      = byte_offset + 0;
    ptrdiff_t  data_offset      = byte_offset + sizeof(int32_t);
    int32_t    field_type       = blob::read_s32(data, type_offset);
    size_t     field_size       = blob::field_data_size(data, byte_offset);
    uint32_t   total_size       = field_size  + sizeof(int32_t);
    out_field_info->total_size  = total_size;
    out_field_info->field_type  = field_type;
    out_field_info->field_size  = field_size;
    out_field_info->field_data  = ((uint8_t*) data) + data_offset;
    return field_type;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void blob::prototype_at(
    void              *data,
    ptrdiff_t          byte_offset,
    blob::prototype_t *out_field_info)
{
    uint32_t  field_count       = blob::read_u32(data, byte_offset);
    size_t    names_size        = field_count * sizeof(uint32_t);
    size_t    types_size        = field_count * sizeof(int32_t);
    ptrdiff_t names_offset      = byte_offset + sizeof(uint32_t);
    ptrdiff_t types_offset      = byte_offset + sizeof(uint32_t)   + names_size;
    out_field_info->proto_size  = sizeof(field_count) + names_size + types_size;
    out_field_info->field_count = field_count;
    out_field_info->field_names = blob::data_at<uint32_t>(data, names_offset);
    out_field_info->field_types = blob::data_at< int32_t>(data, types_offset);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::generic_object_search(
    blob::generic_object_t const *object,
    uint32_t                      field_name,
    blob::generic_object_field_t *out_field_info)
{
    void   *b_ptr = object->field_values; // base pointer value
    size_t  b_ofs = 0;                    // base offset value

    // generic objects require checking each field in the blob.
    // this can be inefficient for complex types, since it requires
    // touching a lot of memory (since fields are variable-length.)
    for (size_t i = 0; i < object->field_count; ++i)
    {
        blob::generic_object_field_t field_info = {0};
        blob::generic_object_field_at(b_ptr, b_ofs, &field_info);
        if (field_info.field_name == field_name)
        {
            if (out_field_info != NULL) *out_field_info = field_info;
            return  field_info.field_type;
        }
        // update our offset to the start of the next field.
        b_ofs += field_info.total_size;
    }
    return blob::FIELD_TYPE_NONE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::runtime_object_search(
    blob::runtime_object_t const *object,
    uint32_t                      field_name,
    blob::runtime_object_field_t *out_field_info)
{
    uint32_t const *name_list   = object->field_names;
    size_t          field_count = object->field_count;
    for (size_t i = 0; i < field_count; ++i)
    {
        if (name_list[i] == field_name)
        {
            blob::runtime_object_field_t field_info = {0};
            blob::runtime_object_field_at(
                object->field_values,
                object->field_offsets[i],
                &field_info);
            if (out_field_info  != NULL) *out_field_info = field_info;
            return  field_info.field_type;
        }
    }
    return blob::FIELD_TYPE_NONE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t blob::prototype_search(
    blob::prototype_t const *prototype,
    uint32_t                 field_name)
{
    uint32_t const *name_list   = prototype->field_names;
    size_t          field_count = prototype->field_count;
    for (size_t i = 0; i < field_count; ++i)
    {
        if (name_list[i] == field_name) return prototype->field_types[i];
    }
    return blob::FIELD_TYPE_NONE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_boolean(
    void      *data,
    ptrdiff_t  byte_offset,
    bool       value)
{
    int32_t type  = blob::FIELD_TYPE_BOOLEAN;
    uint8_t val   = value  ? 1 : 0;
    blob::write_s32(data, byte_offset,  type);
    blob::write_u8 (data, byte_offset + sizeof(type), val);
    return (sizeof (type)  + sizeof(val));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_s8(
    void      *data,
    ptrdiff_t  byte_offset,
    int8_t     value)
{
    int32_t  type = blob::FIELD_TYPE_SINT8;
    blob::write_s32(data, byte_offset,  type);
    blob::write_s8 (data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_u8(
    void      *data,
    ptrdiff_t  byte_offset,
    uint8_t    value)
{
    int32_t  type = blob::FIELD_TYPE_UINT8;
    blob::write_s32(data, byte_offset,  type);
    blob::write_u8 (data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_s16(
    void      *data,
    ptrdiff_t  byte_offset,
    int16_t    value)
{
    int32_t  type = blob::FIELD_TYPE_SINT16;
    blob::write_s32(data, byte_offset,  type);
    blob::write_s16(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_u16(
    void      *data,
    ptrdiff_t  byte_offset,
    uint16_t   value)
{
    int32_t  type = blob::FIELD_TYPE_UINT16;
    blob::write_s32(data, byte_offset,  type);
    blob::write_u16(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_s32(
    void      *data,
    ptrdiff_t  byte_offset,
    int32_t    value)
{
    int32_t  type = blob::FIELD_TYPE_SINT32;
    blob::write_s32(data, byte_offset,  type);
    blob::write_s32(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_u32(
    void      *data,
    ptrdiff_t  byte_offset,
    uint32_t   value)
{
    int32_t  type = blob::FIELD_TYPE_UINT32;
    blob::write_s32(data, byte_offset,  type);
    blob::write_u32(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_s64(
    void      *data,
    ptrdiff_t  byte_offset,
    int64_t    value)
{
    int32_t  type = blob::FIELD_TYPE_SINT64;
    blob::write_s32(data, byte_offset,  type);
    blob::write_s64(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_u64(
    void      *data,
    ptrdiff_t  byte_offset,
    uint64_t   value)
{
    int32_t  type = blob::FIELD_TYPE_UINT64;
    blob::write_s32(data, byte_offset,  type);
    blob::write_u64(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_f32(
    void      *data,
    ptrdiff_t  byte_offset,
    float      value)
{
    int32_t  type = blob::FIELD_TYPE_FLOAT32;
    blob::write_s32(data, byte_offset,  type);
    blob::write_f32(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_f64(
    void      *data,
    ptrdiff_t  byte_offset,
    double     value)
{
    int32_t  type = blob::FIELD_TYPE_FLOAT64;
    blob::write_s32(data, byte_offset,  type);
    blob::write_f64(data, byte_offset + sizeof(type), value);
    return (sizeof (type)  + sizeof(value));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_vec2f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value)
{
    int32_t  type = blob::FIELD_TYPE_VECTOR_2F;
    float   *vec2 = (float*)(((uint8_t*) data) + byte_offset + sizeof(type));
    blob::write_s32(data, byte_offset, type);
    memcpy (vec2, value,  sizeof(float) * 2);
    return (sizeof(type)+(sizeof(float) * 2));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_vec3f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value)
{
    int32_t  type = blob::FIELD_TYPE_VECTOR_3F;
    float   *vec3 = (float*)(((uint8_t*) data) + byte_offset + sizeof(type));
    blob::write_s32(data, byte_offset, type);
    memcpy (vec3, value,  sizeof(float) * 3);
    return (sizeof(type)+(sizeof(float) * 3));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_vec4f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value)
{
    int32_t  type = blob::FIELD_TYPE_VECTOR_4F;
    float   *vec4 = (float*)(((uint8_t*) data) + byte_offset + sizeof(type));
    blob::write_s32(data, byte_offset, type);
    memcpy (vec4, value,  sizeof(float) * 4);
    return (sizeof(type)+(sizeof(float) * 4));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_mat2x2f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value)
{
    int32_t  type = blob::FIELD_TYPE_MATRIX_2X2F;
    float   *mat  = (float*)(((uint8_t*) data) + byte_offset + sizeof(type));
    blob::write_s32(data, byte_offset, type);
    memcpy (mat, value,   sizeof(float) * 4);
    return (sizeof(type)+(sizeof(float) * 4));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_mat3x3f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value)
{
    int32_t  type = blob::FIELD_TYPE_MATRIX_3X3F;
    float   *mat  = (float*)(((uint8_t*) data) + byte_offset + sizeof(type));
    blob::write_s32(data, byte_offset, type);
    memcpy (mat, value,   sizeof(float) * 9);
    return (sizeof(type)+(sizeof(float) * 9));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_mat3x4f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value)
{
    int32_t  type = blob::FIELD_TYPE_MATRIX_3X4F;
    float   *mat  = (float*)(((uint8_t*) data) + byte_offset + sizeof(type));
    blob::write_s32(data, byte_offset, type);
    memcpy (mat, value,   sizeof(float) * 12);
    return (sizeof(type)+(sizeof(float) * 12));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_mat4x4f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value)
{
    int32_t  type = blob::FIELD_TYPE_MATRIX_4X4F;
    float   *mat  = (float*)(((uint8_t*) data) + byte_offset + sizeof(type));
    blob::write_s32(data, byte_offset, type);
    memcpy (mat, value,   sizeof(float) * 16);
    return (sizeof(type)+(sizeof(float) * 16));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_string(
    void       *data,
    ptrdiff_t   byte_offset,
    char const *str)
{
    size_t len= 0;
    // substitute an empty string for NULL.
    if (NULL == str) str = "";
    // determine the length of the supplied string, in bytes.
    while (*str++) ++len;
    // write the value as an array of char.
    return blob::write_field_array(
        data,
        byte_offset,
        blob::FIELD_TYPE_CHAR,
        len + 1, /* include NULL-terminator byte */
        (void*) str);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_array(void *data, ptrdiff_t byte_offset)
{
    int32_t  type = blob::FIELD_TYPE_ARRAY;
    blob::write_s32(data, byte_offset, type);
    return   sizeof(type);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_array(
    void * CMN_RESTRICT data,
    ptrdiff_t           byte_offset,
    int32_t             element_type,
    size_t              element_count,
    void * CMN_RESTRICT array_data)
{
    size_t total_size    = 0;
    size_t element_size  = blob::field_size_for_type(element_type);
    assert(element_size != BLOB_FIELD_SIZE_VARIABLE);
    total_size += blob::write_field_array(data, byte_offset + total_size);
    total_size += blob::write_field_array_info(
        data,
        byte_offset  + total_size,
        element_type,
        element_count);
    total_size += blob::write_field_array_data(
        data,
        byte_offset  + total_size,
        array_data,
        element_size * element_count);
    return total_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_array_info(
    void      *data,
    ptrdiff_t  byte_offset,
    int32_t    element_type,
    size_t     element_count)
{
    uint32_t count = (uint32_t) element_count;
    blob::write_u32(data, byte_offset, count);
    blob::write_s32(data, byte_offset + sizeof(count), element_type);
    return (sizeof (element_type) + sizeof(count));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_array_data(
    void * CMN_RESTRICT data,
    ptrdiff_t           byte_offset,
    void *CMN_RESTRICT array_data,
    size_t              size_in_bytes)
{
    uint8_t *src_ptr = ((uint8_t*) array_data);
    uint8_t *dst_ptr = ((uint8_t*) data)+ byte_offset;
    memcpy(dst_ptr, src_ptr, size_in_bytes);
    return size_in_bytes;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_generic_object(void *data, ptrdiff_t byte_offset)
{
    int32_t type  = blob::FIELD_TYPE_GN_OBJECT;
    blob::write_s32(data, byte_offset, type);
    return  sizeof (type);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_generic_object_info(
    void      *data,
    ptrdiff_t  byte_offset,
    size_t     field_count,
    size_t     field_data_size)
{
    uint32_t count = (uint32_t) field_count;
    uint32_t dsize = (uint32_t) field_data_size;
    blob::write_u32(data, byte_offset, count);
    blob::write_s32(data, byte_offset + sizeof(count), dsize);
    return (sizeof (count) + sizeof(dsize));
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_generic_object_field(
    void * CMN_RESTRICT data,
    ptrdiff_t           byte_offset,
    uint32_t            field_name,
    int32_t             field_type,
    void * CMN_RESTRICT field_data,
    size_t              size_in_bytes)
{
    size_t data_size      = size_in_bytes;
    size_t total_size     = data_size   + (sizeof(uint32_t) * 2) + sizeof(int32_t);
    ptrdiff_t name_offset = byte_offset + 0;
    ptrdiff_t type_offset = byte_offset + sizeof(uint32_t);
    ptrdiff_t size_offset = byte_offset + sizeof(uint32_t) + sizeof(int32_t);
    ptrdiff_t data_offset = size_offset + sizeof(uint32_t);
    uint8_t  *src_ptr     = (uint8_t*)    field_data;
    uint8_t  *dst_ptr     =((uint8_t*)    data)+ data_offset;
    blob::write_u32(data, name_offset,  field_name);
    blob::write_s32(data, type_offset,  field_type);
    blob::write_u32(data, size_offset,  data_size);
    memcpy(dst_ptr, src_ptr, data_size);
    return total_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_runtime_object(
    void     * CMN_RESTRICT data,
    ptrdiff_t               byte_offset,
    size_t                  field_count,
    uint32_t * CMN_RESTRICT field_names,
    uint32_t * CMN_RESTRICT field_offsets,
    void     * CMN_RESTRICT field_values,
    size_t                  field_values_size)
{
    uint32_t count = (uint32_t) field_count;
    int32_t  type  = blob::FIELD_TYPE_RT_OBJECT;
    size_t   size  = 0;

    blob::write_s32(data, byte_offset, type);
    byte_offset += sizeof(type);
    size        += sizeof(type);

    blob::write_u32(data, byte_offset, count);
    byte_offset += sizeof(count);
    size        += sizeof(count);

    uint32_t *fn = (uint32_t*) (((uint8_t*)data) + byte_offset);
    memcpy(fn, field_names,   field_count * sizeof(uint32_t));
    byte_offset += sizeof(uint32_t) * field_count;
    size        += sizeof(uint32_t) * field_count;

    uint32_t *fo = (uint32_t*) (((uint8_t*)data) + byte_offset);
    memcpy(fo, field_offsets, field_count * sizeof(uint32_t));
    byte_offset += sizeof(uint32_t) * field_count;
    size        += sizeof(uint32_t) * field_count;

    uint8_t  *fv = (uint8_t*)  (((uint8_t*)data) + byte_offset);
    memcpy(fv, field_values, field_values_size);
    byte_offset += field_values_size;
    size        += field_values_size;
    return size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t blob::write_field_prototype(
    void     * CMN_RESTRICT data,
    ptrdiff_t               byte_offset,
    size_t                  field_count,
    uint32_t * CMN_RESTRICT field_names,
    int32_t  * CMN_RESTRICT field_types)
{
    uint32_t count = (uint32_t) field_count;
    int32_t  type  = blob::FIELD_TYPE_PROTOTYPE;
    size_t   size  = 0;

    blob::write_s32(data, byte_offset, type);
    byte_offset += sizeof(type);
    size        += sizeof(type);

    blob::write_u32(data, byte_offset, count);
    byte_offset += sizeof(count);
    size        += sizeof(count);

    uint32_t *fn = (uint32_t*) (((uint8_t*)data) + byte_offset);
    memcpy(fn, field_names, field_count * sizeof(uint32_t));
    byte_offset += sizeof(uint32_t) * field_count;
    size        += sizeof(uint32_t) * field_count;

    int32_t  *ft = ( int32_t*) (((uint8_t*)data) + byte_offset);
    memcpy(ft, field_types, field_count * sizeof( int32_t));
    byte_offset += sizeof(int32_t)  * field_count;
    size        += sizeof(int32_t)  * field_count;
    return size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
