/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libjson.hpp
///  Defines the interface for parsing JSON documents into a low-cost, easy to
///  traverse runtime representation. This implementation is based on the
///  MIT-licensed vjson library (http://code.google.com/p/vjson/) with custom
///  extensions to allow more flexibility in the JSON document definition and
///  to provide custom memory management.
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIBJSON_HPP_INCLUDED
#define LIBJSON_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <stdio.h>
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace json {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/
struct item_t;

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// Function signature for a user-defined function that allocates a new JSON
/// item node. Nodes are always fixed-size.
///
/// @param size_in_bytes The size of a single JSON item node, in bytes.
/// @param context Opaque data associated with the allocator. This value is
/// optional and may be NULL.
/// @return The newly allocated node, or NULL if memory allocation failed.
typedef json::item_t* (CMN_CALL_C *alloc_fn)(
    size_t  size_in_bytes,
    void   *context);

/// Function signature for a user-defined function that releases a JSON item
/// node. The function can release the memory, return the node to a free pool,
/// etc.
///
/// @param item The item being released.
/// @param size_in_bytes The size of a single JSON item node, in bytes.
/// @param context Opaque data associated with the allocator. This value is
/// optional and may be NULL.
typedef void          (CMN_CALL_C *free_fn)(
    json::item_t *item,
    size_t        size_in_bytes,
    void         *context);

/// An enumeration defining the types of JSON nodes that can be stored within
/// a JSON document. The type is stored as a 4-byte field.
enum type_e
{
    /// The node contains a value of unknown type. This typically indicates
    /// an error, as all nodes should have associated types.
    TYPE_UNKNOWN     = 0,
    /// The node contains an object. Objects typically have children that
    /// define their fields as key-value pairs.
    TYPE_OBJECT      = 1,
    /// The node represents an array. The array items are stored in the linked
    /// list of siblings of the first child node.
    TYPE_ARRAY       = 2,
    /// The node represents a string field. The string is a pointer into the
    /// JSON document buffer, and is guaranteed to be NULL-terminated.
    TYPE_STRING      = 3,
    /// The node represents an integer field. Integer fields are stored as
    /// 64-bit signed values, and must be specified using either decimal base.
    TYPE_INTEGER     = 4,
    /// The node represents a floating-point field. Number fields are stored
    /// as a 64-bit IEEE double-precision value.
    TYPE_NUMBER      = 5,
    /// The node represents a boolean field, with a value of either true
    /// or false.
    TYPE_BOOLEAN     = 6,
    /// The node represents a NULL value, as indicated by the constant value
    /// of 'null' or 'NULL'.
    TYPE_NULL        = 7,
    /// This value is unused and serves to force the enumeration to be stored
    /// in (at minimum) a 32-bit value.
    TYPE_FORCE_32BIT = CMN_FORCE_32BIT
};

/// A structure describing a single item or key-value pair within a JSON
/// document. Specific values for strings, integers, numbers and booleans
/// can be read using their own parsing functions.
struct item_t
{
    item_t      *parent;        /// A pointer to the parent node.
    item_t      *next_sibling;  /// A pointer to the next sibling node.
    item_t      *first_child;   /// A pointer to the first child node.
    item_t      *last_child;    /// A pointer to the last child node.
    char        *key;           /// A NULL-terminated string field name.
    int32_t      value_type;    /// One of json::type_e.
    union        value_union    /// A union used to store the field value.
    {
        char    *string;        /// NULL-terminated string value.
        int64_t  integer;       /// Signed integer value.
        double   number;        /// Floating-point value.
        bool     boolean;       /// Boolean true/false value.
    } value;
};

/// A structure used to report errors that occur while parsing a JSON document.
struct error_t
{
    char const  *description;   /// A brief error description (string literal.)
    char const  *position;      /// The error position within the document.
    size_t       line;          /// The line number within the document.
};

/// A structure used to specify callbacks to allocate and release JSON nodes.
/// An instance of this structure should be passed to the JSON document
/// parsing function.
struct alloc_t
{
    alloc_fn     alloc;         /// Callback to allocate a single item node.
    free_fn      free;          /// Callback to release a single item node.
    void        *context;       /// Optional, opaque application data.
};

/// Initializes a node allocator instance.
///
/// @param alloc The allocator structure to initialize.
/// @param alloc_func The allocator function. If this value is NULL, @a alloc
/// will be initialized to use standard C library malloc and free functions.
/// @param free_func The memory free function. This value is optional.
/// @param context Pointer to optional opaque application-specified data to
/// be passed back to the allocation and free functions.
CMN_PUBLIC void allocator_init(
    json::alloc_t  *alloc,
    json::alloc_fn  alloc_func,
    json::free_fn   free_func = NULL,
    void           *context   = NULL);

/// Inserts a single item into a JSON document tree. This is a utility function
/// that can be used when manually constructing a JSON document.
///
/// @param parent The parent item.
/// @param new_node The new child node.
CMN_PUBLIC void append(json::item_t *parent, json::item_t *new_node);

/// Attempts to parse and validate a JSON document. Note that string keys and
/// values are pointers to addresses within the source block, @a document, so
/// the original JSON document must remain in memory while fields are being
/// read. This function is destructive; it modifies the buffer pointed to by
/// @a document to insert NULL bytes.
///
/// @param document The NULL-terminated buffer containing the JSON document.
/// @param allocator The memory allocator implementation used to allocate
/// memory for the json::item_t instances. If this value is NULL, the default
/// implementation is used that relies on the standard C library malloc and
/// free functions.
/// @param out_root On return, this location is updated to point to the root
/// node of the document. If an error occurs, this location is set to NULL.
/// @param out_error The structure that will be populated with error details
/// if an error occurs during parsing.
/// @return true if the JSON document was parsed successfully, or false if an
/// error occurred during parsing, in which case @a out_error is populated.
CMN_PUBLIC bool parse(
    char           *document,
    json::alloc_t  *allocator,
    json::item_t  **out_root,
    json::error_t  *out_error);

/// Writes a JSON document to a stream. The output is formatted for readability
/// and includes whitespace for formatting purposes.
///
/// @param fp The stream to write to. Specify stdout or stderr to write to one
/// of the standard streams.
/// @param root The root node of the JSON document. This should be an object or
/// an array if a valid JSON document is to be generated.
CMN_PUBLIC void write(FILE *fp, json::item_t *root);

/// Releases the memory associated with each node in a JSON document tree. This
/// function is called recursively for each node in the document tree.
///
/// @param node The node to delete. Pass the root node to delete the entire
/// document tree.
/// @param allocator The memory allocator implementation used to allocate
/// memory for the json::item_t instances. If this value is NULL, the default
/// implementation is used that relies on the standard C library malloc and
/// free functions.
CMN_PUBLIC void free(json::item_t *node, json::alloc_t *allocator);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace json */

#endif /* LIBJSON_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
