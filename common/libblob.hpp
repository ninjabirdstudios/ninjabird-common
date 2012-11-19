/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libblob.hpp
///  Defines the interfaces used to read and write blobs, which are used for
///  storing open-ended data structures, encapsulated as raw blocks of memory.
///  Blobs can be easily passed around at runtime, serialized to and from disk,
///  or sent across a network. They are designed for runtime efficiency, not
///  necessarily space efficiency.
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIBBLOB_HPP_INCLUDED
#define LIBBLOB_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace blob {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// The value returned by the blob::size_for_blob_type() function when a
/// non-fixed-length field type (array, object or prototype) is specified.
#ifndef BLOB_FIELD_SIZE_VARIABLE
#define BLOB_FIELD_SIZE_VARIABLE             0xFFFFFFFFUL
#endif /* !defined(BLOB_FIELD_SIZE_VARIABLE)  */

/// The value returned by the blob::field_index() function when the specified
/// field cannot be found within a blob object definition.
#ifndef BLOB_FIELD_INDEX_INVALID
#define BLOB_FIELD_INDEX_INVALID             0xFFFFFFFFUL
#endif /* !defined(BLOB_FIELD_INDEX_INVALID)  */

/// The field offset value representing an invalid offset. This offset is safe
/// to consider invalid because it specifies the maximum size of a data blob.
#ifndef BLOB_FIELD_OFFSET_INVALID
#define BLOB_FIELD_OFFSET_INVALID            0xFFFFFFFFUL
#endif /* !defined(BLOB_FIELD_OFFSET_INVALID) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef BLOB_ENDIANESS_LSB_FIRST
#define BLOB_ENDIANESS_LSB_FIRST             0
#endif /* !defined(BLOB_ENDIANESS_LSB_FIRST) - little endian */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef BLOB_ENDIANESS_MSB_FIRST
#define BLOB_ENDIANESS_MSB_FIRST             1
#endif /* !defined(BLOB_ENDIANESS_MSB_FIRST) -    big endian */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef BLOB_SYSTEM_ENDIANESS
    #ifdef _MSC_VER
        #if   defined(_M_AMD64) || defined(_M_X64)
            #define BLOB_SYSTEM_ENDIANESS    BLOB_ENDIANESS_LSB_FIRST
        #elif defined(_M_IX86)
            #define BLOB_SYSTEM_ENDIANESS    BLOB_ENDIANESS_LSB_FIRST
        #elif defined(_M_IA64)
            #define BLOB_SYSTEM_ENDIANESS    BLOB_ENDIANESS_LSB_FIRST
        #else
            #define BLOB_SYSTEM_ENDIANESS    BLOB_ENDIANESS_MSB_FIRST
        #endif /* architecture */
    #endif /* defined(_MSC_VER) */
    #ifdef __GNUC__
        #if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            #define BLOB_SYSTEM_ENDIANESS    BLOB_ENDIANESS_LSB_FIRST
        #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            #define BLOB_SYSTEM_ENDIANESS    BLOB_ENDIANESS_MSB_FIRST
        #else
            #error libblob: BLOB_SYSTEM_ENDIANESS must be manually defined.
        #endif /* architecture */
    #endif /* defined(__GNUC__) */
#endif /* !defined(BLOB_SYSTEM_ENDIANESS) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef BLOB_SWAP_2
#define BLOB_SWAP_2(v)                                                        \
    ( ((v >> 8) & 0x00FF) |                                                   \
      ((v << 8) & 0xFF00) )
#endif /* !defined(BLOB_SWAP_2) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef BLOB_SWAP_4
#define BLOB_SWAP_4(v)                                                        \
    ( ((v >> 24) & 0x000000FF) |                                              \
      ((v >>  8) & 0x0000FF00) |                                              \
      ((v <<  8) & 0x00FF0000) |                                              \
      ((v << 24) & 0xFF000000) )
#endif /* !defined(BLOB_SWAP_4) */

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef BLOB_SWAP_8
#define BLOB_SWAP_8(v)                                                        \
    ( (((v) >> 56) & 0x00000000000000FFULL) |                                 \
      (((v) >> 40) & 0x000000000000FF00ULL) |                                 \
      (((v) >> 24) & 0x0000000000FF0000ULL) |                                 \
      (((v) >>  8) & 0x00000000FF000000ULL) |                                 \
      (((v) <<  8) & 0x000000FF00000000ULL) |                                 \
      (((v) << 24) & 0x0000FF0000000000ULL) |                                 \
      (((v) << 40) & 0x00FF000000000000ULL) |                                 \
      (((v) << 56) & 0xFF00000000000000ULL) )
#endif /* !defined(BLOB_SWAP_8) */

/*/////////////////////////////////////////////////////////////////////////80*/

/// Defines the different text encodings that can be detected by inspecting the
/// first four bytes of a text document for a byte order marker (BOM).
enum text_encoding_e
{
    /// Indicates that the blob::determine_text_encoding() function is unsure
    /// of the encoding. This typically indicates that no BOM is present.
    TEXT_ENCODING_UNSURE      = 0,
    /// Indicates that the text is encoded using single-byte ASCII.
    TEXT_ENCODING_ASCII       = 1,
    /// Indicates that the text is encoded using UTF-8.
    TEXT_ENCODING_UTF8        = 2,
    /// Indicates that the text is encoded using big-endian UTF-16.
    TEXT_ENCODING_UTF16_BE    = 3,
    /// Indicates that the text is encoded using little-endian UTF-16.
    TEXT_ENCODING_UTF16_LE    = 4,
    /// Indicates that the text is encoded using big-endian UTF-32.
    TEXT_ENCODING_UTF32_BE    = 5,
    /// Indicates that the text is encoded using little-endian UTF-32.
    TEXT_ENCODING_UTF32_LE    = 6,
    /// This type value is unused and serves only to force a minimum of 32-bits
    /// of storage space for values of this enumeration type.
    TEXT_ENCODING_FORCE_32BIT = CMN_FORCE_32BIT
};

/// An enumeration defining the types of fields that can be stored in blobs.
/// The blob field type is stored as a 4-byte integer value within the blob.
enum field_type_e
{
    /// Indicates that there an invalid operation has occurred, such as
    /// attempting to read past the end of a blob.
    FIELD_TYPE_NONE           = 0,
    /// Indicates that no value is present for a particular field; that is,
    /// the value has a zero-byte length.
    FIELD_TYPE_NULL           = 1,
    /// Indicates a 1-byte boolean value (0 = false, 1 = true).
    FIELD_TYPE_BOOLEAN        = 2,
    /// Indicates a 1-byte signed integer value that should be interpreted as
    /// a character byte (part of a string.)
    FIELD_TYPE_CHAR           = 3,
    /// Indicates a 1-byte signed integer value.
    FIELD_TYPE_SINT8          = 4,
    /// Indicates a 1-byte unsigned integer value.
    FIELD_TYPE_UINT8          = 5,
    /// Indicates a 2-byte signed integer value.
    FIELD_TYPE_SINT16         = 6,
    /// Indicates a 2-byte unsigned integer value.
    FIELD_TYPE_UINT16         = 7,
    /// Indicates a 4-byte signed integer value.
    FIELD_TYPE_SINT32         = 8,
    /// Indicates a 4-byte unsigned integer value.
    FIELD_TYPE_UINT32         = 9,
    /// Indicates an 8-byte signed integer value.
    FIELD_TYPE_SINT64         = 10,
    /// Indicates an 8-byte unsigned integer value.
    FIELD_TYPE_UINT64         = 11,
    /// Indicates a 4-byte IEEE-754 single-precision floating point value.
    FIELD_TYPE_FLOAT32        = 12,
    /// Indicates an 8-byte IEEE-754 double-precision floating point value.
    FIELD_TYPE_FLOAT64        = 13,
    /// Indicates a 2-component vector value of 4-byte IEEE-754 single-
    /// precision floating point values (8 bytes total size.)
    FIELD_TYPE_VECTOR_2F      = 14,
    /// Indicates a 3-component vector value of 4-byte IEEE-754 single-
    /// precision floating point values (12 bytes total size.)
    FIELD_TYPE_VECTOR_3F      = 15,
    /// Indicates a 4-component vector value of 4-byte IEEE-754 single-
    /// precision floating point values (16 bytes total size.)
    FIELD_TYPE_VECTOR_4F      = 16,
    /// Indicates a 2x2 matrix of 4-byte IEEE-754 single-precision floating
    /// point values (16 bytes total size.)
    FIELD_TYPE_MATRIX_2X2F    = 17,
    /// Indicates a 3x3 matrix of 4-byte IEEE-754 single-precision floating
    /// point values (36 bytes total size.)
    FIELD_TYPE_MATRIX_3X3F    = 18,
    /// Indicates a 3x4 matrix of 4-byte IEEE-754 single-precision floating
    /// point values (48 bytes total size.)
    FIELD_TYPE_MATRIX_3X4F    = 19,
    /// Indicates a 4x4 matrix of 4-byte IEEE-754 single-precision floating
    /// point values (64 bytes total size.)
    FIELD_TYPE_MATRIX_4X4F    = 20,
    /// Indicates an array of values, specified by a 4-byte unsigned integer
    /// count, followed by a 4-byte integer field type, followed by the data.
    FIELD_TYPE_ARRAY          = 21,
    /// Indicates an object prototype, where each field name is mapped to a
    /// corresponding type definition, stored as a 32-bit value (typically the
    /// hash of some string.) The serialized format consists of a 4-byte field
    /// count, followed by an array of field_count 4-byte values specifying
    /// the field names, followed by an array of field_count 4-byte values
    /// specifying the field types.
    FIELD_TYPE_PROTOTYPE      = 22,
    /// Indicates a generic type instance of variable length. The serialized
    /// format consists of a 4-byte field count, followed by a 4-byte field
    /// data size, followed by an array of fields. Each field consists of a
    /// 4-byte unsigned integer name, followed by a 4 byte field type, followed
    /// by a 4-byte data size, followed by variable length data. This form is
    /// not the most efficient, but is easy to construct. For best performance,
    /// these fields should be optimized using blob::optimize().
    FIELD_TYPE_GN_OBJECT      = 23,
    /// Indicates a generic type instance of variable length. The serialized
    /// format consists of a 4-byte field count, followed by a 4-byte field
    /// data size, followed by an array of field_count 4-byte values specifying
    /// the field names, followed by an array of field_count 4-byte values
    /// specifying the field offsets (relative to the start of the value blob),
    /// followed by the value blob, with each value being stored as a 4-byte
    /// field type identifier, followed by the variable-length field data. The
    /// offsets in field_offsets point to the start of the field type ID.
    FIELD_TYPE_RT_OBJECT      = 24,
    /// The maximum valid field type identifier.
    FIELD_TYPE_MAX            = FIELD_TYPE_RT_OBJECT,
    /// This type value is unused and serves only to force a minimum of 32-bits
    /// of storage space for values of this enumeration type.
    FIELD_TYPE_FORCE_32BIT    = CMN_FORCE_32BIT
};

/// Represents a single generic field of any type stored within a blob.
struct field_t
{
    size_t    total_size;    /// The total size of the field, in bytes.
    size_t    field_size;    /// The total size of the field data, in bytes.
    int32_t   field_type;    /// One of blob::field_type_e.
    void     *field_data;    /// A pointer to the raw field data.
};

/// Represents an array field stored within a blob. The array consists of zero
/// or more items, all of the same basic type. Arrays may contain arrays or
/// objects in addition to fixed-length data.
struct array_field_t
{
    size_t    array_size;    /// The total size of the array field, in bytes.
    size_t    item_count;    /// The number of items stored in the array.
    int32_t   item_type;     /// The type of each item in the array.
    void     *item_data;     /// A pointer to the first item in the array.
};

/// Represents an instance of an open-ended type, consisting of zero or more
/// field names and their associated values, optimized for ease of
/// construction.
struct generic_object_t
{
    size_t    object_size;   /// The total size of the object field, in bytes.
    size_t    field_size;    /// The total size of the field data, in bytes.
    size_t    field_count;   /// The number of fields defined on the type.
    void     *field_values;  /// Pointer to the start of field data.
};

/// Represents a single field within an open-ended type optimized for ease of
/// construction.
struct generic_object_field_t
{
    size_t    total_size;    /// The total size of the field, in bytes.
    size_t    field_size;    /// The size of the field_data, in bytes.
    uint32_t  field_name;    /// The integer field identifier (name hash.)
    int32_t   field_type;    /// One of blob::field_type_e.
    void     *field_data;    /// Pointer to the start of the field data.
};

/// Represents an instance of an open-ended type, consisting of zero or more
/// field names and their associated values, optimized for efficient runtime
/// access.
struct runtime_object_t
{
    size_t    object_size;   /// The total size of the object field, in bytes.
    size_t    field_count;   /// The number of fields defined on the type.
    uint32_t *field_names;   /// An array of field_count field identifiers.
    uint32_t *field_offsets; /// An array of byte offsets for field values.
    void     *field_values;  /// Pointer to the start of field data.
};

/// Represents a single field within an open-ended type optimized for runtime
/// efficiency.
struct runtime_object_field_t
{
    size_t    total_size;    /// The total size of the object field, in bytes.
    size_t    field_size;    /// The size of the field data, in bytes.
    int32_t   field_type;    /// One of blob::field_type_e.
    void     *field_data;    /// Pointer to the start of the field data.
};

/// Represents an object prototype instance stored within a blob, consisting
/// of zero or more field names and their associated types.
struct prototype_t
{
    size_t    proto_size;    /// The total size of the field, in bytes.
    size_t    field_count;   /// The number of fields defined on the type.
    uint32_t *field_names;   /// An array of field_count field identifiers.
    int32_t  *field_types;   /// An array of field_count field types.
};

/// Given four bytes possibly representing a Unicode byte-order-marker
/// attempts to determine the text encoding and actual size of the BOM.
///
/// @param BOM The array of four bytes potentially containing a BOM.
/// @param out_size If this value is non-NULL, on return it is updated
/// with the actual size of the BOM, in bytes.
/// @return One of the text_encoding_e constants, specifying the text encoding
/// indicated by the BOM.
CMN_PUBLIC int32_t determine_text_encoding(uint8_t BOM[4], size_t *out_size);

/// Gets the bytes (up to four) representing the Unicode byte-order-marker
/// associated with a specific text encoding.
///
/// @param encoding One of the text encoding constants, specifying the
/// encoding for which the BOM will be returned.
/// @param out_BOM An array of four bytes that will hold the BOM
/// corresponding to the specified text encoding. Between zero and four
/// bytes will be written to this array.
/// @return The number of bytes written to @a out_BOM.
CMN_PUBLIC size_t bom(int32_t encoding, uint8_t out_BOM[4]);

/// Computes the maximum number of bytes required to base64-encode a binary
/// data block. All data is assumed to be output on one line. One byte is
/// included for a trailing NULL.
///
/// @param binary_size The size of the binary data block, in bytes.
/// @param out_pad_size If non-NULL, on return this value is updated with
/// the number of padding bytes that will be added during encoding.
/// @return The maximum number of bytes required to base64-encode a data
/// block of the specified size.
CMN_PUBLIC size_t base64_size(size_t binary_size, size_t *out_pad_size);

/// Computes the number of raw bytes required to store a block of binary
/// data when converted back from base64. All source data is assumed to
/// be on one line.
///
/// @param base64_size The size of the base64-encoded data.
/// @param pad_size The number of bytes of padding data. If not known,
/// specify a value of zero.
/// @return The number of bytes of binary data generated during decoding.
CMN_PUBLIC size_t binary_size(size_t base64_size, size_t pad_size);

/// Computes the number of raw bytes required to store a block of binary
/// data when converted back from base64. All source data is assumed to
/// be on one line. This version of the function examines the source data
/// directly, and so can provide a precise value.
///
/// @param base64_source Pointer to the start of the base64-encoded data.
/// @param base64_length The number of ASCII characters in the base64 data
/// string. This value can be computed using the standard C library strlen
/// function if the length is not otherwise available.
/// @return The number of bytes of binary data that will be generated during
/// decoding.
CMN_PUBLIC size_t binary_size(
    char const *base64_source,
    size_t      base64_length);

/// Base64-encodes a block of arbitrary data. All data is returned on a
/// single line; no newlines are inserted and no formatting is performed.
/// The output buffer will be NULL-terminated.
///
/// @param input Pointer to the start of the input data.
/// @param input_size The number of bytes of input data to encode.
/// @param output Pointer to the start of the output buffer.
/// @param output_size The maximum number of bytes that can be written to
/// the output buffer. This value must be at least as large as the value
/// returned by the blob::base64_size() function.
/// @return The number of bytes written to the output buffer.
CMN_PUBLIC extern size_t base64_encode(
    void const *input,
    size_t      input_size,
    char       *output,
    size_t      output_size);

/// Decodes a base64-encoded block of text into the corresponding raw
/// binary representation.
///
/// @param input Pointer to the start of the base64-encoded input data.
/// @param input_size The number of bytes of input data to read and decode.
/// @param output Pointer to the start of the output buffer.
/// @param output_size The maximum number of bytes that can be written to
/// the output buffer. This value can be up to two bytes less than the
/// value returned by blob::binary_size() depending on the amount of
/// padding added during encoding.
/// @return The number of bytes written to the output buffer.
CMN_PUBLIC extern size_t base64_decode(
    char const *input,
    size_t      input_size,
    void       *output,
    size_t      output_size);

/// Examines the value stored in an integer to determine the smallest field
/// type that can store the value without loss of information within a blob.
///
/// @param max_value The largest integer value that must be supported.
/// @param support_signed Specify true if the field type must also support
/// signed values.
/// @return A blob::field_type_e corresponding to the smallest type that can
/// store the integer value @a max_value within a blob.
CMN_PUBLIC int32_t field_type_minimum_integer(
    int64_t max_value,
    bool    support_signed);

/// Determines whether a particular blob::field_type_e value can store
/// signed values.
///
/// @param field_type The blob::field_type_e value to check.
/// @return true if @a field_type represents an signed type.
CMN_PUBLIC bool field_type_is_signed(int32_t field_type);

/// Determines whether a particular blob::field_type_e value can store
/// only unsigned values.
///
/// @param field_type The blob::field_type_e value to check.
/// @return true if @a field_type represents an unsigned type.
CMN_PUBLIC bool field_type_is_unsigned(int32_t field_type);

/// Computes the size of a field value of a particular type, for fixed-size
/// types only. Variable-length types (arrays, objects and prototypes) will
/// always return BLOB_FIELD_SIZE_VARIABLE.
///
/// @param field_type One of the blob::field_type_e constants.
/// @return The size of the specified field or BLOB_FIELD_SIZE_VARIABLE if
/// @a field_type indicates an array, object or prototype.
CMN_PUBLIC size_t field_size_for_type(int32_t field_type);

/// Computes the total size of a field value of a particular type, for fixed-
/// size types only. Variable-length types (arrays, objects and prototypes)
/// will always return BLOB_FIELD_SIZE_VARIABLE.
///
/// @param field_type One of the blob::field_type_e constants.
/// @return The total size of the specified field including any field metadata,
/// or BLOB_FIELD_SIZE_VARIABLE if @a field_type indicates an array, prototype
/// or other variable-length type.
CMN_PUBLIC size_t total_size_for_type(int32_t field_type);

/// Copies one blob into another storage location, optimizing any raw object
/// fields for efficient runtime performance. The source and destination
/// pointers must not overlap.
///
/// @param blob_dst A pointer to the destination blob. This memory block should
/// be at least @a blob_size bytes.
/// @param blob_src A pointer to the source blob. This memory block should be
/// at least @a blob_size bytes.
/// @param blob_size The total size of the blob, in bytes.
/// @return The number of bytes consumed from @a blob_src and written to
/// @a blob_dst.
CMN_PUBLIC size_t optimize(
    void * CMN_RESTRICT blob_dst,
    void * CMN_RESTRICT blob_src,
    size_t              blob_size);

/// Performs a copy-and-optimize operation on an unoptimized blob array field.
/// This is primarily an internal function used by blob::optimize().
///
/// @param blob_dst A pointer to the destination blob.
/// @param blob_dst_offset The current byte offset into the destination blob.
/// @param blob_src A pointer to the source blob.
/// @param blob_src_offset The current byte offset into the source blob. This
/// should be the byte offset of a blob::field_type_e constant with the value
/// blob::FIELD_TYPE_ARRAY.
/// @return The number of bytes consumed from @a blob_src and written to
/// @a blob_dst.
CMN_PUBLIC size_t optimize_array_field(
    void * CMN_RESTRICT blob_dst,
    ptrdiff_t           blob_dst_offset,
    void * CMN_RESTRICT blob_src,
    ptrdiff_t           blob_src_offset);

/// Performs a copy-and-optimize operation on an unoptimized blob object field.
/// This is primarily an internal function used by blob::optimize().
///
/// @param blob_dst A pointer to the destination blob.
/// @param blob_dst_offset The current byte offset into the destination blob.
/// @param blob_src A pointer to the source blob.
/// @param blob_src_offset The current byte offset into the source blob. This
/// should be the byte offset of a blob::field_type_e constant with the value
/// blob::FIELD_TYPE_GN_OBJECT.
/// @return The number of bytes consumed from @a blob_src and written to
/// @a blob_dst.
CMN_PUBLIC size_t optimize_object_field(
    void * CMN_RESTRICT blob_dst,
    ptrdiff_t           blob_dst_offset,
    void * CMN_RESTRICT blob_src,
    ptrdiff_t           blob_src_offset);

/// Computes the size of a field value stored within a data blob. This function
/// computes valid values for both fixed and variable-length field types. Only
/// the size of the field data is returned.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the field stored at the specified byte offset.
CMN_PUBLIC size_t field_data_size(void *data, ptrdiff_t byte_offset);

/// Computes the total size of a field value stored within a data blob. This
/// function computes valid values for both fixed and variable-length field
/// types. The size of the field metadata is included in the returned size.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The total size of the field. This value can be added to the value
/// of @a byte_offset to skip over the field entirely.
CMN_PUBLIC size_t field_total_size(void *data, ptrdiff_t byte_offset);

/// Computes the size of an array field stored within a data blob. This
/// function only returns the size of the array data, not including any item
/// metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the array stored at the specified byte offset.
CMN_PUBLIC size_t array_data_size(void *data, ptrdiff_t byte_offset);

/// Computes the size if an array field stored within a data blob, including
/// the item metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The total size of the field. This value can be added to the value
/// of @a byte_offset to skip over the field entirely.
CMN_PUBLIC size_t array_total_size(void *data, ptrdiff_t byte_offset);

/// Computes the size of an object instance stored within a data blob, not
/// including the object metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the object instance stored at the specified byte offset.
CMN_PUBLIC size_t generic_object_data_size(void *data, ptrdiff_t byte_offset);

/// Computes the size of an object instance stored within a data blob,
/// including the object metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the object instance stored at the specified byte offset.
CMN_PUBLIC size_t generic_object_total_size(void *data, ptrdiff_t byte_offset);

/// Computes the size of an object instance stored within a data blob, not
/// including the object metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the object instance stored at the specified byte offset.
CMN_PUBLIC size_t runtime_object_data_size(void *data, ptrdiff_t byte_offset);

/// Computes the size of an object instance stored within a data blob,
/// including the object metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the object instance stored at the specified byte offset.
CMN_PUBLIC size_t runtime_object_total_size(void *data, ptrdiff_t byte_offset);

/// Computes the size of an prototype instance stored within a data blob, not
/// including the field metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the prototype instance stored at the specified byte
/// offset.
CMN_PUBLIC size_t prototype_data_size(void *data, ptrdiff_t byte_offset);

/// Computes the size of an prototype instance stored within a data blob,
/// including the field metadata.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return The size of the prototype instance stored at the specified byte
/// offset.
CMN_PUBLIC size_t prototype_total_size(void *data, ptrdiff_t byte_offset);

/// Populates a simple structure representing a single field encoded within a
/// data blob.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @param out_field_info Pointer to a structure that will be populated with
/// information describing the field. The structure can be passed and copied
/// around as needed.
/// @return The blob::field_type_e indicating the type of the returned field.
CMN_PUBLIC int32_t field_at(
    void          *data,
    ptrdiff_t      byte_offset,
    blob::field_t *out_field_info);

/// Populates a simple structure representing an array definition encoded
/// within a data blob.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @param out_field_info Pointer to a structure that will be populated with
/// information describing the field. The structure can be passed and copied
/// around as needed.
CMN_PUBLIC void array_field_at(
    void                *data,
    ptrdiff_t            byte_offset,
    blob::array_field_t *out_field_info);

/// Populates a simple structure representing an object definition encoded
/// within a data blob.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @param out_field_info Pointer to a structure that will be populated with
/// information describing the field. The structure can be passed and copied
/// around as needed.
CMN_PUBLIC void generic_object_at(
    void                   *data,
    ptrdiff_t               byte_offset,
    blob::generic_object_t *out_field_info);

/// Populates a simple structure representing a field definition within a
/// generic object encoded in a data blob.
///
/// @param data A pointer to the data block to read from. This value should be
/// the field_values pointer of the blob::generic_object_t structure.
/// @param byte_offset The byte offset of the data to access within the blob.
/// This value is typically specified relative to the field_values pointer of
/// the blob::generic_object_t structure.
/// @param out_field_info Pointer to a structure that will be populated with
/// information describing the field. The structure can be passed and copied
/// around as needed.
/// @return One of blob::field_info_e indicating the field type.
CMN_PUBLIC int32_t generic_object_field_at(
    void                         *data,
    ptrdiff_t                     byte_offset,
    blob::generic_object_field_t *out_field_info);

/// Populates a simple structure representing an optimized object definition
/// encoded within a data blob.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @param out_field_info Pointer to a structure that will be populated with
/// information describing the field. The structure can be passed and copied
/// around as needed.
CMN_PUBLIC void runtime_object_at(
    void                   *data,
    ptrdiff_t               byte_offset,
    blob::runtime_object_t *out_field_info);

/// Populates a simple structure representing a field definition within a
/// runtime object encoded in a data blob.
///
/// @param data A pointer to the data block to read from. This value should be
/// the field_values pointer of the blob::runtime_object_t structure.
/// @param byte_offset The byte offset of the data to access within the blob.
/// This value is typically specified relative to the field_values pointer of
/// the blob::runtime_object_t structure.
/// @param out_field_info Pointer to a structure that will be populated with
/// information describing the field. The structure can be passed and copied
/// around as needed.
/// @return One of blob::field_info_e indicating the field type.
CMN_PUBLIC int32_t runtime_object_field_at(
    void                         *data,
    ptrdiff_t                     byte_offset,
    blob::runtime_object_field_t *out_field_info);

/// Populates a simple structure representing a prototype instance encoded
/// within a data blob.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @param out_field_info Pointer to a structure that will be populated with
/// information describing the field. The structure can be passed and copied
/// around as needed.
CMN_PUBLIC void prototype_at(
    void              *data,
    ptrdiff_t          byte_offset,
    blob::prototype_t *out_field_info);

/// Searches a generic object instance encoded within a data blob for a named
/// field. This is a generic implementation that does not take advantage of
/// any application-specific data organization.
///
/// @param object The information about the object to search.
/// @param field_name The integer name of the field to search for.
/// @param out_field_info On return, this location is updated with information
/// about the field identified by @a field_name.
/// @return One of blob::field_info_e indicating the field type, or the value
/// blob::FIELD_TYPE_NONE if no field with the specified name was found.
CMN_PUBLIC int32_t generic_object_search(
    blob::generic_object_t const *object,
    uint32_t                      field_name,
    blob::generic_object_field_t *out_field_info);

/// Searches an object instance encoded within a data blob for a named field.
/// This is a generic implementation that does not take advantage of any
/// application-specific data organization. For example, the application might
/// sort the field_names of the object. It could then use a binary search to
/// locate fields instead of this simple linear search.
///
/// @param object The information about the object to search.
/// @param field_name The integer name of the field to search for.
/// @param out_field_info On return, this location is updated with information
/// about the field identified by @a field_name.
/// @return One of blob::field_info_e indicating the field type, or the value
/// blob::FIELD_TYPE_NONE if no field with the specified name was found.
CMN_PUBLIC int32_t runtime_object_search(
    blob::runtime_object_t const *object,
    uint32_t                      field_name,
    blob::runtime_object_field_t *out_field_info);

/// Searches an prototype instance encoded within a data blob for a named
/// field. This is a generic implementation that does not take advantage of any
/// application-specific data organization. For example, the application might
/// sort the field_names of the prototype. It could then use a binary search to
/// locate fields instead of this simple linear search.
///
/// @param prototype The information about the prototype object to search.
/// @param field_name The integer name of the field to search for.
/// @param out_field_info On return, this location is updated with information
/// about the field identified by @a field_name.
/// @return One of blob::field_info_e indicating the field type, or the value
/// blob::FIELD_TYPE_NONE if no field with the specified name was found.
CMN_PUBLIC int32_t prototype_search(
    blob::prototype_t const *prototype,
    uint32_t                 field_name);

/// Writes a boolean value to a data block. The value is preceeded by the
/// 4-byte field type, and is written as an unsigned 8-bit integer with a value
/// of either 0 (false) or 1 (true).
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_boolean(
    void      *data,
    ptrdiff_t  byte_offset,
    bool       value);

/// Writes a signed, 8-bit integer field value to a data block. The value is
/// preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_s8(
    void      *data,
    ptrdiff_t  byte_offset,
    int8_t     value);

/// Writes an unsigned, 8-bit integer field value to a data block. The value is
/// preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_u8(
    void      *data,
    ptrdiff_t  byte_offset,
    uint8_t    value);

/// Writes a signed, 16-bit integer field value to a data block. The value is
/// preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_s16(
    void      *data,
    ptrdiff_t  byte_offset,
    int16_t    value);

/// Writes an unsigned, 16-bit integer field value to a data block. The value
/// is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_u16(
    void      *data,
    ptrdiff_t  byte_offset,
    uint16_t   value);

/// Writes a signed, 32-bit integer field value to a data block. The value is
/// preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_s32(
    void      *data,
    ptrdiff_t  byte_offset,
    int32_t    value);

/// Writes an unsigned, 32-bit integer field value to a data block. The value
/// is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_u32(
    void      *data,
    ptrdiff_t  byte_offset,
    uint32_t   value);

/// Writes a signed, 64-bit integer field value to a data block. The value is
/// preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_s64(
    void      *data,
    ptrdiff_t  byte_offset,
    int64_t    value);

/// Writes an unsigned, 64-bit integer field value to a data block. The value
/// is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_u64(
    void      *data,
    ptrdiff_t  byte_offset,
    uint64_t   value);

/// Writes a 32-bit single-precision floating point field value to a data
/// block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_f32(
    void      *data,
    ptrdiff_t  byte_offset,
    float      value);

/// Writes a 64-bit double-precision floating point field value to a data
/// block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_f64(
    void      *data,
    ptrdiff_t  byte_offset,
    double     value);

/// Writes a 2-component vector of 32-bit single-precision floating point
/// values to a data block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_vec2f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value);

/// Writes a 3-component vector of 32-bit single-precision floating point
/// values to a data block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_vec3f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value);

/// Writes a 4-component vector of 32-bit single-precision floating point
/// values to a data block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_vec4f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value);

/// Writes a 2x2 (4-element) matrix of 32-bit single-precision floating point
/// values to a data block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_mat2x2f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value);

/// Writes a 3x3 (9-element) matrix of 32-bit single-precision floating point
/// values to a data block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_mat3x3f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value);

/// Writes a 3x4 (12-element) matrix of 32-bit single-precision floating point
/// values to a data block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_mat3x4f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value);

/// Writes a 4x4 (16-element) matrix of 32-bit single-precision floating point
/// values to a data block. The value is preceeded by the 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param value The data to write to the data block.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_mat4x4f(
    void      *data,
    ptrdiff_t  byte_offset,
    float     *value);

/// Writes a string value as an array of UINT8 (raw bytes) including a byte for
/// the NULL-terminator character. This helper method is provided as a
/// convenience. The field type is blob::FIELD_TYPE_ARRAY, with an element
/// type of blob::FIELD_TYPE_CHAR and an element count specifying the
/// total size of the string, in bytes, and including the NULL-terminator,
/// which is stored as a single byte.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param str A pointer to a NULL-terminated UTF-8 (or ASCII) string
/// representing the string value to write.
CMN_PUBLIC size_t write_field_string(
    void       *data,
    ptrdiff_t   byte_offset,
    char const *str);

/// Writes the field identifier indicating an array field. The array element
/// count and type must be specified with blob::write_field_array_info()
/// and the array data using blob::write_field_array_data(). Splitting
/// up array writes in this manner allows the use of nested arrays.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_array(void *data, ptrdiff_t byte_offset);

/// Writes out an array field. This is a helper method that can only be used
/// with simple array types, where @a element_type indicates a fixed-length
/// value. Arrays of variable-length data (arrays, objects or prototypes) are
/// not supported.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param element_type One of blob::field_type_e indicating the type of
/// the elements stored within the array. You must specify a fixed-length type.
/// @param element_count The number of elements in the array.
/// @param array_data A pointer to the tightly-packed array data. The size is
/// computed automatically based on @a element_type and @a element_count.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_array(
    void * CMN_RESTRICT data,
    ptrdiff_t           byte_offset,
    int32_t             element_type,
    size_t              element_count,
    void * CMN_RESTRICT array_data);

/// Writes the header block for an array to a data block. The header block is
/// written separately to support arrays of arrays within data blobs. The data
/// consists of a 4-byte element count, followed by a 4-byte element type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param element_type One of blob::field_type_e indicating the type of
/// the elements stored within the array.
/// @param element_count The number of elements in the array.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_array_info(
    void      *data,
    ptrdiff_t  byte_offset,
    int32_t    element_type,
    size_t     element_count);

/// Writes the raw array data for an array to a data block. The data block is
/// written separately to support arrays of arrays within data blobs. The data
/// is copied directly into the data blob.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param array_data A pointer to the start of the array contents.
/// @param size_in_bytes The number of bytes to copy from @a array_data into
/// @a data at @a byte_offset.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_array_data(
    void * CMN_RESTRICT data,
    ptrdiff_t           byte_offset,
    void * CMN_RESTRICT array_data,
    size_t              size_in_bytes);

/// Writes the field identifier indicating a generic object field. The object
/// field count and total field data size must be specified using the function
/// blob::write_generic_object_info().
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_generic_object(void *data, ptrdiff_t byte_offset);

/// Writes the header block for a generic object to a data block.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param field_count The total number of fields defined on the object.
/// @param field_data_size The total size of all of the field data, including
/// its metadata, in bytes.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_generic_object_info(
    void       *data,
    ptrdiff_t   byte_offset,
    size_t      field_count,
    size_t      field_data_size);

/// Writes a single field within a generic object definition to a data block.
/// The field data is copied directly.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param field_name A unique integer identifier for the field.
/// @param field_type One of blob::field_type_e indicating the field type.
/// @param field_data Pointer to the raw field data to be copied into the blob.
/// @param size_in_bytes The number of bytes to copy from @a field_data.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_generic_object_field(
    void * CMN_RESTRICT data,
    ptrdiff_t           byte_offset,
    uint32_t            field_name,
    int32_t             field_type,
    void * CMN_RESTRICT field_data,
    size_t              size_in_bytes);

/// Writes an object field value to a data block. The value is preceeded by a
/// 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param field_count The number of fields defined on the object, specifying
/// the length of the @a field_names and @a field_offsets arrays.
/// @param field_names An array of integer field identifiers defining the
/// fields of the object.
/// @param field_offsets An array of byte offsets within @a field_values
/// specifying the byte offset of each field value within @a field_values.
/// @param field_values A pointer to the start of the raw field value block.
/// @param field_values_size The number of bytes to copy from @a field_values
/// into the data blob.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_runtime_object(
    void     * CMN_RESTRICT data,
    ptrdiff_t               byte_offset,
    size_t                  field_count,
    uint32_t * CMN_RESTRICT field_names,
    uint32_t * CMN_RESTRICT field_offsets,
    void     * CMN_RESTRICT field_values,
    size_t                  field_values_size);

/// Writes a prototype field value to a data block. The value is preceeded by a
/// 4-byte field type.
///
/// @param data A pointer to the data block to write to.
/// @param byte_offset The byte offset of the field to write.
/// @param field_count The number of fields defined on the type, specifying
/// the length of the @a field_names and @a field_types arrays.
/// @param field_names An array of integer field identifiers defining the
/// fields of the object.
/// @param field_types An array of blob::field_type_e specifying field types.
/// @return The number of bytes written to the data blob.
CMN_PUBLIC size_t write_field_prototype(
    void     * CMN_RESTRICT data,
    ptrdiff_t               byte_offset,
    size_t                  field_count,
    uint32_t * CMN_RESTRICT field_names,
    int32_t  * CMN_RESTRICT field_types);

/// Performs runtime detection of the endianess (byte ordering) of the host
/// system.
///
/// @return One of BLOB_ENDIANESS_MSB_FIRST (indicating a big-endian host) or
/// BLOB_ENDIANESS_LSB_FIRST (indicating a little-endian host).
inline int32_t host_endianess(void)
{
    union endianess_test_u
    {
        char    array[4];
        int32_t chars;
    }        u;
    char     c = 'a';
    u.array[0] = c++;
    u.array[1] = c++;
    u.array[2] = c++;
    u.array[3] = c++;
    return (0x61626364 == u.chars)   ?
            BLOB_ENDIANESS_MSB_FIRST :
            BLOB_ENDIANESS_LSB_FIRST ;
}

/// Performs a byte swap of 16-bit integer value.
///
/// @param value The 16-bit value to swap.
/// @return The swapped value.
inline uint16_t byte_swap_16i(uint16_t value)
{
    return BLOB_SWAP_2(value);
}

/// Performs a byte swap of 32-bit integer value.
///
/// @param value The 32-bit value to swap.
/// @return The swapped value.
inline uint32_t byte_swap_32i(uint32_t value)
{
    return BLOB_SWAP_4(value);
}

/// Performs a byte swap of 64-bit integer value.
///
/// @param value The 64-bit value to swap.
/// @return The swapped value.
inline uint64_t byte_swap_64i(uint64_t value)
{
    return BLOB_SWAP_8(value);
}

/// Reads a 16-bit value from a buffer and performs a byte swap operation
/// on it before returning it as the final destination type (16-bit unsigned.)
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The byte-swapped value.
inline uint16_t read_swap_16i(void *ptr, ptrdiff_t offset)
{
    uint8_t *src = ((uint8_t*)ptr) + offset;
    return BLOB_SWAP_2(*(uint16_t*) src);
}

/// Reads a 32-bit value from a buffer and performs a byte swap operation
/// on it before returning it as the final destination type (32-bit unsigned.)
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The byte-swapped value.
inline uint32_t read_swap_32i(void *ptr, ptrdiff_t offset)
{
    uint8_t *src = ((uint8_t*)ptr) + offset;
    return BLOB_SWAP_4(*(uint32_t*) src);
}

/// Reads a 64-bit value from a buffer and performs a byte swap operation
/// on it before returning it as the final destination type (64-bit unsigned.)
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The byte-swapped value.
inline uint64_t read_swap_64i(void *ptr, ptrdiff_t offset)
{
    uint8_t *src = ((uint8_t*)ptr) + offset;
    return BLOB_SWAP_8(*(uint64_t*) src);
}

/// Reads a 32-bit value from a buffer and performs a byte swap operation
/// on it before returning it as the final destination type (32-bit float.)
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The byte-swapped value.
inline float read_swap_32f(void *ptr, ptrdiff_t offset)
{
    uint8_t  *src = ((uint8_t*)ptr) + offset;
    uint32_t  val = BLOB_SWAP_4(*(uint32_t*) src);
    return *(float*) &val;
}

/// Reads a 64-bit value from a buffer and performs a byte swap operation
/// on it before returning it as the final destination type (64-bit float.)
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The byte-swapped value.
inline double read_swap_64f(void *ptr, ptrdiff_t offset)
{
    uint8_t  *src = ((uint8_t*)ptr) + offset;
    uint64_t  val = BLOB_SWAP_8(*(uint64_t*) src);
    return *(double*) &val;
}

/// Performs a byte swapping operation on a signed 16-bit value and writes it
/// to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_16si(void *ptr, ptrdiff_t offset, int16_t value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint16_t*) dst = BLOB_SWAP_2(*(uint16_t*) &value);
}

/// Performs a byte swapping operation on an unsigned 16-bit value and writes
/// it to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_16ui(void *ptr, ptrdiff_t offset, uint16_t value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint16_t*) dst = BLOB_SWAP_2(value);
}

/// Performs a byte swapping operation on a signed 32-bit value and writes it
/// to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_32si(void *ptr, ptrdiff_t offset, int32_t value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint32_t*) dst = BLOB_SWAP_4(*(uint32_t*) &value);
}

/// Performs a byte swapping operation on an unsigned 32-bit value and writes
/// it to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_32ui(void *ptr, ptrdiff_t offset, uint32_t value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint32_t*) dst = BLOB_SWAP_4(value);
}

/// Performs a byte swapping operation on a signed 64-bit value and writes it
/// to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_64si(void *ptr, ptrdiff_t offset, int64_t value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint64_t*) dst = BLOB_SWAP_8(*(uint64_t*) &value);
}

/// Performs a byte swapping operation on an unsigned 64-bit value and writes
/// it to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_64ui(void *ptr, ptrdiff_t offset, uint64_t value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint64_t*) dst = BLOB_SWAP_8(value);
}

/// Performs a byte swapping operation on a 32-bit floating-point value and
/// writes it to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_32f(void *ptr, ptrdiff_t offset, float value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint32_t*) dst = BLOB_SWAP_4(*(uint32_t*) &value);
}

/// Performs a byte swapping operation on a 32-bit floating-point value and
/// writes it to a memory location.
///
/// @param ptr A pointer to the buffer into which the value will be
/// written.
/// @param offset The offset into @a ptr at which the value will be
/// written.
/// @param value The value to write.
inline void swap_write_64f(void *ptr, ptrdiff_t offset, double value)
{
    uint8_t *dst     = ((uint8_t*)ptr) + offset;
    *(uint64_t*) dst = BLOB_SWAP_8(*(uint64_t*) &value);
}

/// Reads a signed 8-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int8_t   read_s8(void *ptr, ptrdiff_t offset)
{
    return *(int8_t  *)(((uint8_t*)ptr) + offset);
}

/// Reads an unsigned 8-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint8_t  read_u8(void *ptr, ptrdiff_t offset)
{
    return *(uint8_t *)(((uint8_t*)ptr) + offset);
}

/// Reads a signed 16-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int16_t  read_s16(void *ptr, ptrdiff_t offset)
{
    return *(int16_t *)(((uint8_t*)ptr) + offset);
}

/// Reads a signed 16-bit integer value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int16_t  read_s16be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(int16_t *)(((uint8_t*)ptr)  + offset);
    #else
        return  (int16_t  ) read_swap_16i(ptr, offset);
    #endif
}

/// Reads a signed 16-bit integer value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int16_t  read_s16le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(int16_t *)(((uint8_t*)ptr)  + offset);
    #else
        return  (int16_t  ) read_swap_16i(ptr, offset);
    #endif
}

/// Reads an unsigned 16-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint16_t read_u16(void *ptr, ptrdiff_t offset)
{
    return *(uint16_t*)(((uint8_t*)ptr) + offset);
}

/// Reads an unsigned 16-bit integer value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint16_t read_u16be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(uint16_t*)(((uint8_t*)ptr)  + offset);
    #else
        return  (uint16_t ) read_swap_16i(ptr, offset);
    #endif
}

/// Reads an unsigned 16-bit integer value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint16_t read_u16le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(uint16_t*)(((uint8_t*)ptr)  + offset);
    #else
        return  (uint16_t ) read_swap_16i(ptr, offset);
    #endif
}

/// Reads a signed 32-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int32_t  read_s32(void *ptr, ptrdiff_t offset)
{
    return *(int32_t *)(((uint8_t*)ptr) + offset);
}

/// Reads a signed 32-bit integer value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int32_t  read_s32be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(int32_t *)(((uint8_t*)ptr)  + offset);
    #else
        return  (int32_t  ) read_swap_32i(ptr, offset);
    #endif
}

/// Reads a signed 32-bit integer value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int32_t  read_s32le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(int32_t *)(((uint8_t*)ptr)  + offset);
    #else
        return  (int32_t  ) read_swap_32i(ptr, offset);
    #endif
}

/// Reads an unsigned 32-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint32_t read_u32(void *ptr, ptrdiff_t offset)
{
    return *(uint32_t*)(((uint8_t*)ptr) + offset);
}

/// Reads an unsigned 32-bit integer value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint32_t read_u32be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(uint32_t*)(((uint8_t*)ptr)  + offset);
    #else
        return  (uint32_t ) read_swap_32i(ptr, offset);
    #endif
}

/// Reads an unsigned 32-bit integer value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint32_t read_u32le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(uint32_t*)(((uint8_t*)ptr)  + offset);
    #else
        return  (uint32_t ) read_swap_32i(ptr, offset);
    #endif
}

/// Reads a signed 64-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int64_t  read_s64(void *ptr, ptrdiff_t offset)
{
    return *(int64_t *)(((uint8_t*)ptr) + offset);
}

/// Reads a signed 64-bit integer value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int64_t  read_s64be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(int64_t *)(((uint8_t*)ptr)  + offset);
    #else
        return  (int64_t  ) read_swap_64i(ptr, offset);
    #endif
}

/// Reads a signed 64-bit integer value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline int64_t  read_s64le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(int64_t *)(((uint8_t*)ptr)  + offset);
    #else
        return  (int64_t  ) read_swap_64i(ptr, offset);
    #endif
}

/// Reads an unsigned 64-bit integer value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint64_t read_u64(void *ptr, ptrdiff_t offset)
{
    return *(uint64_t*)(((uint8_t*)ptr) + offset);
}

/// Reads an unsigned 64-bit integer value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint64_t read_u64be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(uint64_t*)(((uint8_t*)ptr)  + offset);
    #else
        return  (uint64_t ) read_swap_64i(ptr, offset);
    #endif
}

/// Reads an unsigned 64-bit integer value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline uint64_t read_u64le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(uint64_t*)(((uint8_t*)ptr)  + offset);
    #else
        return  (uint64_t ) read_swap_64i(ptr, offset);
    #endif
}

/// Reads a 32-bit floating-point value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline float    read_f32(void *ptr, ptrdiff_t offset)
{
    return *(float   *)(((uint8_t*)ptr) + offset);
}

/// Reads a 32-bit floating-point value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline float    read_f32be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(float   *)(((uint8_t*)ptr)  + offset);
    #else
        return  (float    ) read_swap_32f(ptr, offset);
    #endif
}

/// Reads a 32-bit floating-point value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline float    read_f32le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(float   *)(((uint8_t*)ptr)  + offset);
    #else
        return  (float    ) read_swap_32f(ptr, offset);
    #endif
}

/// Reads a 64-bit floating-point value from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline double   read_f64(void *ptr, ptrdiff_t offset)
{
    return *(double  *)(((uint8_t*)ptr) + offset);
}

/// Reads a 64-bit floating-point value, stored using big-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline double   read_f64be(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        return *(double  *)(((uint8_t*)ptr)  + offset);
    #else
        return  (double   ) read_swap_64f(ptr, offset);
    #endif
}

/// Reads a 64-bit floating-point value, stored using little-endian byte
/// ordering, from a memory location.
///
/// @param ptr A pointer to the buffer from which the value will be read.
/// @param offset The offset into the buffer at which to read the value.
/// @return The value.
inline double   read_f64le(void *ptr, ptrdiff_t offset)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        return *(double  *)(((uint8_t*)ptr)  + offset);
    #else
        return  (double   ) read_swap_64f(ptr, offset);
    #endif
}

/// Reads a specific number of bytes from a memory location, copying them into
/// a buffer. The input and output buffers must not overlap.
///
/// @param ptr A pointer to the buffer from which bytes will be read.
/// @param offset The offset into the source buffer at which to start reading.
/// @param out A pointer to the buffer to which bytes will be written.
/// @param num The number of bytes to read from @a ptr and write to @a out.
/// @return The number of bytes read from @a ptr and written to @a out.
inline size_t  read_bytes(
    void *CMN_RESTRICT ptr,
    ptrdiff_t           offset,
    void * CMN_RESTRICT out,
    size_t              num)
{
    uint8_t *src  = ((uint8_t*) ptr) + offset;
    uint8_t *dst  = ((uint8_t*) out);
    uint8_t *end  = ((uint8_t*) out) + num;
    while   (dst != end) *dst++ = *src++;
    return   num;
}

/// Writes a signed 8-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s8(void *ptr, ptrdiff_t offset, int8_t value)
{
    *(int8_t  *)(((uint8_t*)ptr) + offset) = value;
    return sizeof (int8_t);
}

/// Writes an unsigned 8-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u8(void *ptr, ptrdiff_t offset, uint8_t value)
{
    *(uint8_t *)(((uint8_t*)ptr) + offset) = value;
    return sizeof (uint8_t);
}

/// Writes a signed 16-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s16(void *ptr, ptrdiff_t offset, int16_t value)
{
    *(int16_t *)(((uint8_t*)ptr) + offset) = value;
    return sizeof (int16_t);
}

/// Writes a signed 16-bit integer value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s16be(void *ptr, ptrdiff_t offset, int16_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(int16_t *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_16si(ptr, offset, value);
    #endif
    return sizeof(int16_t);
}

/// Writes a signed 16-bit integer value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s16le(void *ptr, ptrdiff_t offset, int16_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(int16_t *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_16si(ptr, offset, value);
    #endif
    return sizeof(int16_t);
}

/// Writes an unsigned 16-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u16(void *ptr, ptrdiff_t offset, uint16_t value)
{
    *(uint16_t*)(((uint8_t*)ptr) + offset) = value;
    return sizeof (uint16_t);
}

/// Writes an unsigned 16-bit integer value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u16be(void *ptr, ptrdiff_t offset, uint16_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(uint16_t*)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_16ui(ptr, offset, value);
    #endif
    return sizeof(uint16_t);
}

/// Writes an unsigned 16-bit integer value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u16le(void *ptr, ptrdiff_t offset, uint16_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(uint16_t*)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_16ui(ptr, offset, value);
    #endif
    return sizeof(uint16_t);
}

/// Writes a signed 32-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s32(void *ptr, ptrdiff_t offset, int32_t value)
{
    *(int32_t *)(((uint8_t*)ptr) + offset) = value;
    return sizeof (int32_t);
}

/// Writes a signed 32-bit integer value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s32be(void *ptr, ptrdiff_t offset, int32_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(int32_t *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_32si(ptr, offset, value);
    #endif
    return sizeof(int32_t);
}

/// Writes a signed 32-bit integer value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s32le(void *ptr, ptrdiff_t offset, int32_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(int32_t *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_32si(ptr, offset, value);
    #endif
    return sizeof(int32_t);
}

/// Writes an unsigned 32-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u32(void *ptr, ptrdiff_t offset, uint32_t value)
{
    *(uint32_t*)(((uint8_t*)ptr) + offset) = value;
    return sizeof (uint32_t);
}

/// Writes an unsigned 32-bit integer value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u32be(void *ptr, ptrdiff_t offset, uint32_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(uint32_t*)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_32ui(ptr, offset, value);
    #endif
    return sizeof(uint32_t);
}

/// Writes an unsigned 32-bit integer value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u32le(void *ptr, ptrdiff_t offset, uint32_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(uint32_t*)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_32ui(ptr, offset, value);
    #endif
    return sizeof(uint32_t);
}

/// Writes a signed 64-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s64(void *ptr, ptrdiff_t offset, int64_t value)
{
    *(int64_t *)(((uint8_t*)ptr) + offset) = value;
    return sizeof (int64_t);
}

/// Writes a signed 64-bit integer value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s64be(void *ptr, ptrdiff_t offset, int64_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(int64_t *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_64si(ptr, offset, value);
    #endif
    return sizeof(int64_t);
}

/// Writes a signed 64-bit integer value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_s64le(void *ptr, ptrdiff_t offset, int64_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(int64_t *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_64si(ptr, offset, value);
    #endif
    return sizeof(int64_t);
}

/// Writes an unsigned 64-bit integer value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u64(void *ptr, ptrdiff_t offset, uint64_t value)
{
    *(uint64_t*)(((uint8_t*)ptr) + offset) = value;
    return sizeof (uint64_t);
}

/// Writes an unsigned 64-bit integer value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u64be(void *ptr, ptrdiff_t offset, uint64_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(uint64_t*)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_64ui(ptr, offset, value);
    #endif
    return sizeof(uint64_t);
}

/// Writes an unsigned 64-bit integer value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_u64le(void *ptr, ptrdiff_t offset, uint64_t value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(uint64_t*)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_64ui(ptr, offset, value);
    #endif
    return sizeof(uint64_t);
}

/// Writes a 32-bit floating-point value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_f32(void *ptr, ptrdiff_t offset, float value)
{
    *(float   *)(((uint8_t*)ptr) + offset) = value;
    return sizeof (float);
}

/// Writes a 32-bit floating-point value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_f32be(void *ptr, ptrdiff_t offset, float value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(float   *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_32f(ptr, offset, value);
    #endif
    return sizeof(float);
}

/// Writes a 32-bit floating-point value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_f32le(void *ptr, ptrdiff_t offset, float value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(float   *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_32f(ptr, offset, value);
    #endif
    return sizeof(float);
}

/// Writes a 64-bit floating-point value to a memory location.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_f64(void *ptr, ptrdiff_t offset, double value)
{
    *(double  *)(((uint8_t*)ptr) + offset) = value;
    return sizeof (double);
}

/// Writes a 64-bit floating-point value to a memory location using
/// big-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_f64be(void *ptr, ptrdiff_t offset, double value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_MSB_FIRST
        *(double  *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_64f(ptr, offset, value);
    #endif
    return sizeof(double);
}

/// Writes a 64-bit floating-point value to a memory location using
/// little-endian byte ordering.
///
/// @param ptr A pointer to the buffer to which the value will be written.
/// @param offset The offset into the buffer at which to store the value.
/// @param value The value to store in the buffer.
/// @return The number of bytes written.
inline size_t write_f64le(void *ptr, ptrdiff_t offset, double value)
{
    #if BLOB_SYSTEM_ENDIANESS == BLOB_ENDIANESS_LSB_FIRST
        *(double  *)(((uint8_t*)ptr) + offset) = value;
    #else
        swap_write_64f(ptr, offset, value);
    #endif
    return sizeof(double);
}

/// Copies bytes from a buffer to a memory location. The input and output
/// buffers must not overlap.
///
/// @param ptr A pointer to the buffer to which bytes will be written.
/// @param offset The offset into the buffer at which to begin writing.
/// @param inp A pointer to the buffer from which bytes will be read.
/// @param num The number of bytes to copy from @a inp to @a ptr.
/// @return The number of bytes read from @a inp and written to @a ptr.
inline size_t write_bytes(
    void * CMN_RESTRICT ptr,
    ptrdiff_t          offset,
    void * CMN_RESTRICT inp,
    size_t              num)
{
    uint8_t *src  = ((uint8_t*) inp);
    uint8_t *dst  = ((uint8_t*) ptr) + offset;
    uint8_t *end  = ((uint8_t*) ptr) + offset + num;
    while   (dst != end) *dst++ = *src++;
    return   num;
}

/// Retrieves a pointer to the data stored at a particular byte offset within
/// a data block, casting the returned value to a desired type.
///
/// @param data A pointer to the data block.
/// @param byte_offset The byte offset of the data to access within the blob.
/// @return A pointer to the specified data within the blob.
template <typename T>
inline T* data_at(void *data, ptrdiff_t byte_offset)
{
    return (T*) (((uint8_t*) data) + byte_offset);
}

/// Reads the field type identifier from a data block. The field type
/// identifier immediately preceeds the field data.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the field to access within the blob.
/// @return One of the field_type_e constants representing the field type.
inline int32_t field_type_at(void *data, ptrdiff_t byte_offset)
{
    return *(int32_t*)(((uint8_t*) data) + byte_offset);
}

/// Computes the byte offset of the actual field data stored at a particular
/// byte offset within a data block.
///
/// @param data A pointer to the data block to read from.
/// @param byte_offset The byte offset of the field to access within the blob.
/// @return A pointer to the actual data associated with the field, not
/// including the leading field type identifier.
template <typename T>
inline T* field_data_at(void *data, ptrdiff_t byte_offset)
{
    return (T*) (((uint8_t*) data) + byte_offset + sizeof(int32_t));
}

/// Performs a basic test to determine whether the offset of a field within a
/// data blob is valid.
///
/// @param byte_offset The byte offset to check.
/// @return true if the specified offset is not invalid.
inline bool field_offset_valid(ptrdiff_t byte_offset)
{
    size_t  offset = (size_t) byte_offset;
    return (offset < BLOB_FIELD_OFFSET_INVALID);
}

/// Performs a basic test to determine whether the offset of a field within a
/// data blob is valid.
///
/// @param byte_offset The byte offset to check.
/// @param max_size The size of the data blob, in bytes.
/// @return true if the specified offset is not invalid.
inline bool field_offset_valid(ptrdiff_t byte_offset, size_t max_size)
{
    size_t  offset = (size_t) byte_offset;
    return (offset < max_size && offset < BLOB_FIELD_OFFSET_INVALID);
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace blob */

#endif /* LIBBLOB_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
