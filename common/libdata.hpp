/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libdata.hpp
///  Defines basic data structures and algorithms, including doubly-linked
///  lists for storage, hash-based structures for acceleration, key-value
///  table manipulation, string data storage and array manipulation. No memory
///  allocation is performed by this library, so higher-level types must be
///  constructed on top of these primitives.
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIBDATA_HPP_INCLUDED
#define LIBDATA_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"
#include "common_traits.hpp"

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace data {

/*//////////////
//   Macros   //
//////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// A simple utility structure representing a contiguous block of memory used
/// for storing NULL-terminated strings. The pubsub system uses this structure
/// to store the topic subscription strings.
struct string_data_t
{
    char           *next_block;                    /// Start of next string
    char           *memory_start;                  /// Start of memory block
    char           *memory_end;                    /// End of memory block
    size_t          memory_size;                   /// Total size of memory
};

/// A simple structure representing a single node in a hash tree. The data
/// itself is not stored in the node, so each item is relatively small.
template <typename T>
struct hash_node_t
{
    hash_node_t<T> *branch[2]; /// Pointers to tree branches.
    T              *data;      /// Pointer to the data.
    uint32_t        hash;      /// The hash value.

    /// Default constructor. Initializes all members to NULL, and the hash
    /// value is initialized to zero.
    hash_node_t(void)
        :
        data(NULL),
        hash(0)
    {
        branch[0] = NULL;
        branch[1] = NULL;
    }
};

/// A single node in a doubly-linked list. The value is stored at the node.
template <typename T>
struct list_node_t
{
    list_node_t<T> *next;      /// Pointer to the next node, or NULL.
    list_node_t<T> *prev;      /// Pointer to the previous node, or NULL.
    T               value;     /// The value stored at this location.

    /// Default constructor. Initializes the next and previous members to NULL.
    list_node_t(void)
        :
        next(NULL),
        prev(NULL)
    { /* empty */ }
};

/// A helper functor for comparing items for sorting and searching.
struct comparer_t
{
    /// Directly compares two items by value.
    ///
    /// @param a The first item.
    /// @param b The second item.
    /// @return A value less than zero if @a a is 'less than' @a b; a value
    /// greater than zero if @a a is 'greater than' @a b, or zero if the two
    /// keys are 'equal'.
    template <typename T>
    inline int operator()(T const &a, T const &b) const
    {
        return ((a < b) ? -1 : ((a > b) ? +1 : 0));
    }

    /// Compares two items by value, using indices into an array.
    ///
    /// @param items A pointer to the start of the array containing the items.
    /// @param a The zero-based index of the first item in @a items.
    /// @param b The zero-based index of the second item in @a items.
    /// @return A value less than zero if @a a is 'less than' @a b; a value
    /// greater than zero if @a a is 'greater than' @a b, or zero if the two
    /// items are 'equal'.
    template <typename T>
    inline int operator()(T const *items, size_t a, size_t b) const
    {
        T const &ia = items[a];
        T const &ib = items[b];
        return ((ia < ib) ? -1 : ((ia > ib) ? +1 : 0));
    }
};

/// Initializes a block of string storage using application-managed memory.
///
/// @param data The string data instance to initialize.
/// @param memory Pointer to the start of an application-managed memory block
/// used to store data for the interned strings.
/// @param memory_size The size of the supplied memory block, in bytes.
/// @param memory_used The number of bytes in @a memory that are currently
/// being used for string storage. This value defaults to 0. Specify a non-zero
/// value if @a memory has been pre-initialized with some string data.
CMN_PUBLIC void string_data_init(
    data::string_data_t *data,
    void                *memory,
    size_t               memory_size,
    size_t               memory_used = 0);

/// Resets a string data storage block, marking all memory as available and
/// zeroing all available memory.
///
/// @param data The string data block to reset.
CMN_PUBLIC void string_data_reset(data::string_data_t *data);

/// Attempts to locate an interned copy of a string within the string data
/// storage block. This operation has O(N) time complexity, with the search
/// string @a str being compared with all other strings in the storage block.
///
/// @param data The string data block to search.
/// @param str Pointer to a NULL-terminated, ASCII or UTF-8 encoded string
/// specifying the string to search for.
/// @return A pointer to the interned copy of the string, if found; otherwise,
/// the function returns NULL.
CMN_PUBLIC char* string_data_search(
    data::string_data_t *data,
    char const          *str);

/// Copies a NULL-terminated, ASCII or UTF-8 encoded string into a string data
/// storage block. The storage block is not searched for an existing copy of
/// the string, so it is possible to intern multiple copies of a string.
///
/// @param data The string data block.
/// @param str Pointer to a NULL-terminated, ASCII or UTF-8 encoded string
/// specifying the string to internalize.
/// @return A pointer to the interned copy of the string, or NULL if there
/// was not enough space left in @a data to internalize the string @a str.
CMN_PUBLIC char* string_data_intern(
    data::string_data_t *data,
    char const          *str);

/// Computes the number of bytes available for string storage within a string
/// data storage block.
///
/// @param data The string data block.
/// @return The number of bytes available in @a data.
CMN_PUBLIC size_t string_data_bytes_free(data::string_data_t *data);

/// Computes the number of bytes used for string storage within a string data
/// storage block.
///
/// @param data The string data block.
/// @return The number of bytes used in @a data.
CMN_PUBLIC size_t string_data_bytes_used(data::string_data_t *data);

/// Performs an O(log N) binary search of an array.
///
/// @param array Pointer to the start of the sorted array to search.
/// @param count The number of items in the search space.
/// @param to_find The item to locate within the array.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(T const& a, T const& b) const
/// @param out_index If non-NULL, on return this value stores the zero-based
/// index of the item within the array. If the item cannot be found, this
/// value is not modified.
/// @return A pointer to the item, if found; returns NULL if the item cannot
/// be found in the array.
template <typename T, typename C>
inline T* binary_search(
    T const *array,
    size_t   count,
    T const &to_find,
    C const &comparer,
    size_t  *out_index)
{
    ptrdiff_t min_idx = 0;
    ptrdiff_t max_idx = (ptrdiff_t) (count - 1);
    while (min_idx   <= max_idx)
    {
        ptrdiff_t cur_idx = (min_idx + max_idx) >> 1; // search range midpoint
        T const  &attrib  = array[cur_idx];

        int result = comparer(to_find, attrib);
        if (result < 0)
        {
            // continue searching the lower half of the range.
            max_idx = cur_idx - 1;
            continue;
        }
        if (result > 0)
        {
            // continue searching the upper half of the range.
            min_idx = cur_idx + 1;
            continue;
        }
        // the search item has been found; return to caller.
        if (out_index != NULL) *out_index = (size_t) cur_idx;
        return (T*) &array[cur_idx];
    }
    return NULL;
}

/// Implements a downward-sift operation, used when building a heap. This
/// function is used by the (array) sort and heapify functions to implement
/// the heapsort algorithm.
///
/// @param array The array being sorted.
/// @param start The starting point of the range to sift down.
/// @param end The ending point of the range to sift down.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(T const& a, T const& b) const
template <typename T, typename C>
inline void sift_down(
    T         *array,
    ptrdiff_t  start,
    ptrdiff_t  end,
    C const   &comparer)
{
    ptrdiff_t root  = start;
    while ((root << 1) <= end)
    {
        ptrdiff_t child = root << 1;
        if ((child < end) && (comparer(array[child], array[child + 1]) < 0))
        {
            child++;
        }
        if (comparer(array[root], array[child]) < 0)
        {
            T tmp        = array[child];
            array[child] = array[root];
            array[root]  = tmp;
            root         = child;
        }
        else return;
    }
}

/// Implements a downward-sift operation, used when building a heap. This
/// function is used by the (key-value) sort and heapify functions to implement
/// the heapsort algorithm.
///
/// @param keys The keys being sorted.
/// @param values The values being sorted.
/// @param start The starting point of the range to sift down.
/// @param end The ending point of the range to sift down.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(K const *k, size_t a, size_t b) const
template <typename K, typename V, typename C>
inline void sift_down(
    K         *keys,
    V         *values,
    ptrdiff_t  start,
    ptrdiff_t  end,
    C const   &comparer)
{
    ptrdiff_t root  = start;
    while ((root << 1) <= end)
    {
        ptrdiff_t child = root << 1;
        if ((child < end) && (comparer(keys[child], keys[child + 1]) < 0))
        {
            child++;
        }
        if (comparer(keys[root], keys[child]) < 0)
        {
            K tmp_k       = keys[child];
            keys[child]   = keys[root];
            keys[root]    = tmp_k;
            V tmp_v       = values[child];
            values[child] = values[root];
            values[root]  = tmp_v;
            root          = child;
        }
        else return;
    }
}

/// Builds a heap. This function is used by the (array) sort function to
/// implement the heapsort algorithm.
///
/// @param array Pointer to the start of the array being sorted.
/// @param count The number of elements in the array to sort.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(T const& a, T const& b) const
template <typename T, typename C>
inline void heapify(
    T       *array,
    size_t   count,
    C const &comparer)
{
    ptrdiff_t start = count >> 1;
    while (start >= 0)
    {
        sift_down(array, start, count - 1, comparer);
        --start;
    }
}

/// Builds a heap using an array of keys to also re-order the array of
/// associated values. This function is used by the key_value_sort function to
/// implement the heapsort algorithm.
///
/// @param keys Pointer to the start of the array of keys being sorted.
/// @param values Pointer to the start of the array of values being sorted.
/// @param count The number of elements in the array to sort.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(K const *k, size_t a, size_t b) const
template <typename K, typename V, typename C>
inline void heapify(
    K       *keys,
    V       *values,
    size_t   count,
    C const &comparer)
{
    ptrdiff_t start = count >> 1;
    while (start >= 0)
    {
        sift_down(keys, values, start, count - 1, comparer);
        --start;
    }
}

/// Sorts an array. The sort is not stable (but can be made stable with an
/// appropriate comparison function) and runs in O(N log N) time, with a small
/// constant space overhead. To make the sort stable, use a secondary key in
/// the comparison function.
///
/// @param array Pointer to the start of the array to be sorted.
/// @param count The number of items to be sorted.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(T const& a, T const& b) const
template <typename T, typename C>
inline void sort(
    T       *array,
    size_t   count,
    C const &comparer)
{
    heapify(array, count, comparer);
    ptrdiff_t end = count - 1;
    while (end > 0)
    {
        T tmp      = array[0];
        array[0]   = array[end];
        array[end] = tmp;
        sift_down(array, 0, end - 1, comparer);
        --end;
    }
}

/// Sorts arrays of associated keys and values. The sort is not stable
/// (but can be made stable with an appropriate comparison function) and runs
/// in O(N log N) time, with a small constant space overhead. To make the
/// sort stable, use a secondary key in the comparison function.
///
/// @param keys Pointer to the start of the array of keys to be sorted.
/// @param values Pointer to the start of the array of values to be sorted.
/// @param count The number of items to be sorted.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(K const *k, size_t a, size_t b) const
template <typename K, typename V, typename C>
inline void sort(
    K       *keys,
    V       *values,
    size_t   count,
    C const &comparer)
{
    heapify(keys, values, count, comparer);
    ptrdiff_t end = count - 1;
    while (end > 0)
    {
        K tmp_k     = keys[0];
        keys[0]     = keys[end];
        keys[end]   = tmp_k;
        V tmp_v     = values[0];
        values[0]   = values[end];
        values[end] = tmp_v;
        sift_down(keys, values, 0, end - 1, comparer);
        --end;
    }
}

/// Attempts to add an item to an unordered key-value table. This operation has
/// O(N) time complexity because the table is unordered. When the table has
/// been fully populated, use the runtime sort function in combination with an
/// instance of comparer_t to order the arrays - at this point, the much more
/// efficient binary_search function may be used for queries.
///
/// @param keys Pointer to the array of keys.
/// @param values Pointer to the array of values.
/// @param capacity The maximum number of items that can be stored in @a keys
/// and @a values.
/// @param size The current number of items in the hash list.
/// @param key The key associated with @a value.
/// @param value The value associated with @a key.
/// @param out_index On return, stores the zero-based index of the new item
/// (if no existing item has key @a key) or, if a key collision has occurred,
/// stores the zero-based index of the existing item.
/// @return Returns one if the operation was successful. Returns zero if the
/// table is full, or if a key collision has occurred. If a key collision
/// occurs, out_index is set to the zero-based index of the existing item.
template <typename K, typename V>
inline size_t kv_add(
    K       *keys,
    V       *values,
    size_t   capacity,
    size_t   size,
    K const &key,
    V const &value,
    size_t  *out_index)
{
    if (size < capacity)
    {
        // search the existing table for a pair with the same key.
        for (size_t i = 0; i < size; ++i)
        {
            if (keys[i] != key)
            {
                // keep searching.
                continue;
            }
            else
            {
                // key collision.
                if (out_index != NULL) *out_index = i;
                return 0;
            }
        }

        // no key collisions, so add the pair to the end of the table.
        keys[size]     = key;
        values[size]   = value;
        if (out_index != NULL) *out_index = size;
        return 1;
    }
    // the table is at capacity; cannot add the pair.
    return 0;
}

/// Attempts to remove a pair from an unordered key-value table. This operation
/// has O(N) time complexity because the table is unordered. The pair at the
/// end of the table is swapped in to replace the removed pair.
///
/// @param keys Pointer to the start of the contiguous key storage.
/// @param values Pointer to the start of the contiguous value storage.
/// @param size The current number of items in @a keys and @a values.
/// @param key The key associated with the value to be removed.
/// @param out_value On return, a copy of the removed value is stored at the
/// address specified in this parameter. If no value is associated with the
/// specified key, this parameter is not updated.
/// @return Returns one if the specified key was found and the associated pair
/// removed, or zero if the key could not be found.
template <typename K, typename V>
inline size_t kv_remove(
    K       *keys,
    V       *values,
    size_t   size,
    K const &key,
    V       *out_value)
{
    size_t e = size > 0 ? size - 1 : 0;
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        if (keys[i] != key)
        {
            // keep searching.
            continue;
        }
        else
        {
            if (out_value != NULL)
            {
                // found the key; store the value for the caller.
                *out_value = values[i];
            }
            // swap the last entry, last, into the removed entry slot.
            // this keeps us from having to move a ton of data around
            // at the expense of needing to perform a sort operation later.
            if (i != e)
            {
                keys[i]    = keys[e];
                values[i]  = values[e];
            }
            return 1;
        }
    }
    return 0;
}

/// Sorts arrays of associated keys and values. The sort is not stable
/// (but can be made stable with an appropriate comparison function) and runs
/// in O(N log N) time, with a small constant space overhead. To make the
/// sort stable, use a secondary key in the comparison function.
///
/// @param keys Pointer to the start of the array of keys to be sorted.
/// @param values Pointer to the start of the array of values to be sorted.
/// @param count The number of items to be sorted.
/// @param comparer An object implementing an operator with the signature:
/// int operator()(K const *k, size_t a, size_t b) const
template <typename K, typename V, typename C>
inline void kv_order(
    K      *keys,
    V      *values,
    size_t  size,
    C const &comparer)
{
    sort(keys, values, size, comparer);
}

/// Performs an O(log N) binary search of a key-value table. Keys and
/// values are stored contiguously in separate arrays for optimal cache
/// behavior.
///
/// @param keys Pointer to the start of the contiguous key storage.
/// @param values Pointer to the start of the contiguous value storage.
/// @param count The number of elements in the @a keys and @a values arrays.
/// @param to_find The key to search for.
/// @param comparer An object implementing an operator with the following
/// signature: int operator()(K const& a, K const& b) const
/// @param out_index On return, this value is updated with the zero-based
/// index of the key in the @a keys array and the value in the @a values array.
/// If the specified item could not be found, this value is not updated.
/// @return Returns a pointer to the value associated with the key @a to_find,
/// if found; returns NULL otherwise.
template <typename K, typename V, typename C>
inline V* kv_ordered_search(
    K const *keys,
    V const *values,
    size_t   count,
    K const &to_find,
    C const &comparer,
    size_t  *out_index)
{
    size_t  idx = 0;
    K      *key = binary_search(keys, count, to_find, comparer, &idx);
    V      *val = key != NULL ? (V*) &values[idx] : NULL;
    if (out_index != NULL && key != NULL) *out_index = idx;
    return  val;
}

/// Performs a slow search of a key-value table to locate a specific item.
/// The search executes in O(N) time. The search does not handle collisions.
///
/// @param keys Pointer to the start of the contiguous key storage.
/// @param values Pointer to the start of the contiguous value storage.
/// @param count The number of elements in the @a keys and @a values arrays.
/// @param to_find The key to search for.
/// @param comparer An object implementing an operator with the following
/// signature: int operator()(K const& a, K const& b) const
/// @param out_index On return, this value is set to the zero-based index
/// of the hash list entry representing the requested item. If no entry with
/// the specified hash was found, this value is not modified.
/// @return A pointer to the data associated with the specified hash, if found;
/// NULL if the specified hash could not be found.
template <typename K, typename V, typename C>
inline V* kv_unordered_search(
    K const *keys,
    V const *values,
    size_t   count,
    K const &to_find,
    C const &comparer,
    size_t  *out_index)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (comparer(keys[i], to_find) != 0)
        {
            // keep searching.
            continue;
        }
        else
        {
            // found the item.
            if (out_index != NULL) *out_index = i;
            return (V*) &values[i];
        }
    }
    return NULL;
}

/// Inserts one or more items at some arbitrary position within an array. This
/// may be an expensive operation, requiring that items be copied around. The
/// caller must ensure that the array capacity is sufficient to store the new
/// items.
///
/// @param array_ptr Pointer to the start of the array to be modified.
/// @param array_count The number of items currently in the array.
/// @param items The array of items to be inserted.
/// @param items_count The number of items to insert.
/// @param starting_index The zero-based index at which to insert the
/// first item.
template <typename T>
inline void array_insert(
    T       *array_ptr,
    size_t   array_count,
    T const *items,
    size_t   items_count,
    size_t   starting_index)
{
    if (starting_index == array_count)
    {
        // we can just perform an append.
        traits::copy(items, items_count, &array_ptr[array_count]);
    }
    else
    {
        // we have to shift items down in the buffer.
        T      *move_src = &array_ptr[starting_index];
        T      *move_dst = &array_ptr[starting_index + items_count];
        size_t  move_num =  array_count - starting_index;
        // move existing items down in the buffer:
        traits::move(move_src, move_num, move_dst);
        // copy new items into the hole we just created:
        traits::copy(items, items_count, move_src);
    }
}

/// Removes one or more items at some arbitrary position within the array. This
/// may be an expensive operation, requiring that items be copied around.
///
/// @param array_ptr Pointer to the start of the array to be modified.
/// @param array_count The number of items currently in the array.
/// @param items_count The number of items to be removed.
/// @param starting_index The zero-based index of the first item to remove.
/// @param item_buffer If non-null, the removed items will be copied to this
/// location.
template <typename T>
inline void array_remove(
    T       *array_ptr,
    size_t   array_count,
    size_t   items_count,
    size_t   starting_index,
    T       *item_buffer = NULL)
{
    items_count = items_count > array_count ? array_count : items_count;
    if (item_buffer != NULL  && items_count > 0)
    {
        // copy the items for the caller.
        traits::copy(&array_ptr[starting_index], items_count, item_buffer);
    }
    if (starting_index + items_count < array_count)
    {
        // we are cutting out a segment of the buffer.
        T      *move_src = &array_ptr[starting_index + items_count];
        T      *move_dst = &array_ptr[starting_index];
        size_t  move_num =  array_count - starting_index - items_count;
        // move existing items up in the buffer.
        traits::move(move_src, move_num, move_dst);
    }
}

/// Searches for the insertion point for a particular hash value in the tree.
///
/// @param root Pointer to the node at which to start the search. This value
/// may be updated if the value is inserted at the root of the tree (because
/// the tree was empty.)
/// @return A pointer to the node at which the specified hash value exists.
template <typename T>
inline data::hash_node_t<T>** hash_placement(
    data::hash_node_t<T> **root,
    uint32_t               hash)
{
    hash_node_t<T> **node  = root;
    uint32_t         shift = 0;
    for (shift = 0; *node != NULL && (*node)->hash != hash; ++shift)
    {
        node = &(*node)->branch[(hash >> shift) & 1];
    }
    return node;
}

/// Attempts to insert an item into the hash tree. The operation fails if a
/// hash collision occurs.
///
/// @param root Pointer to the root node of the tree (or sub-tree.)
/// @param node Pointer to the node used to store the data. This storage is
/// allocated and managed by the application.
/// @param hash The 32-bit unsigned integer hash of the data.
/// @param data Pointer to the data to store at the node. This value need not
/// point to the same data used to generate the hash value.
/// @return Returns one if the operation was successful, or zero if a hash
/// collision occurred.
template <typename T>
inline size_t hash_tree_add(
    data::hash_node_t<T> **root,
    data::hash_node_t<T>  *node,
    uint32_t               hash,
    T                     *data)
{
    hash_node_t<T> **pos = hash_placement(root, hash);
    if (NULL     == *pos)
    {
        // no hash collision; insert item.
        node->branch[0]  = NULL;
        node->branch[1]  = NULL;
        node->data       = data;
        node->hash       = hash;
        *pos             = node;
        return 1;
    }
    else
    {
        // a hash collision has occurred.
        return 0;
    }
}

/// Attempts to remove an item from a hash tree. The operation has O(log N)
/// time complexity.
///
/// @param root The root node of the hash tree (or sub-tree) to modify.
/// @param hash The 32-bit unsigned integer hash of the item to remove from
/// the tree.
/// @return A pointer to the node representing the removed item, or NULL if no
/// item with the specified hash exists in the tree.
template <typename T>
inline data::hash_node_t<T>* hash_tree_remove(
    data::hash_node_t<T> **root,
    uint32_t               hash)
{
    hash_node_t<T> **next = hash_placement(root, hash);
    hash_node_t<T>  *curr = NULL;
    if (next[0] != NULL)
    {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127) /* warning: conditional expression is constant */
#endif /* _MSC_VER */
        // start at the node to be removed (next[0]).
        // choose the left node as next if it exists; otherwise
        // choose the right branch as the next node.
        // if the next node does not exist; we're all done.
        // copy the next node data and hash into current node.
        // move to the next node, and repeat from step 2.
        while (true)
#ifdef _MSC_VER
#pragma warning(pop)          /* warning: conditional expression is constant */
#endif /* _MSC_VER */
        {
            curr = next[0];   // next node to be removed.
            if      (curr->branch[0] != NULL) next = &curr->branch[0]; // L
            else if (curr->branch[1] != NULL) next = &curr->branch[1]; // R
            else
            {
                // no next node; we're all done.
                next[0] = NULL;
                return    curr;
            }
            curr->hash  = next[0]->hash;
            curr->data  = next[0]->data;
        }
    }
    else return NULL;
}

/// Searches a hash tree for a particular item given the hash value associated
/// with the item. The search has O(log N) time complexity.
///
/// @param root Pointer to the root node of the tree (or sub-tree) to search.
/// @param hash The hash value to search for.
/// @return A pointer to the data associated with the specified hash value,
/// or NULL if the hash value was not found.
template <typename T>
inline T* hash_tree_find(data::hash_node_t<T> const *root, uint32_t hash)
{
    hash_node_t<T> const    *node  = root;
    for (uint32_t shift = 0; node != NULL; ++shift)
    {
        if (node->hash != hash)
        {
            // choose the next branch based on hash bit.
            node = node->branch[(hash >> shift) & 1];
        }
        else
        {
            // we've found the requested node; return.
            return (T*) node->data;
        }
    }
    return NULL;
}

/// Copies the contents of a hash tree into an unordered pair of hash and value
/// arrays suitable for serialization or better runtime cache behavior. This
/// internal function is called recursively and is not meant to be used
/// directly by applications.
///
/// @param root The starting node of the hash tree (or sub-tree).
/// @param hashes The array of hash values to write to.
/// @param values The array of associated data values to write to.
/// @param offset The zero-based offset into the array at which to write the
/// key-value pairs.
template <typename T>
inline void hash_tree_to_kvt_rec(
    data::hash_node_t<T> const *root,
    uint32_t                   *hashes,
    T                          *values,
    size_t                     &offset,
    size_t                      capacity)
{
    if (root != NULL)
    {
        // store this node in the array.
        hashes[offset] =  root->hash;
        values[offset] =*(root->data);
        ++offset;
        // recurse down the left branch.
        hash_tree_to_kvt_(root->branch[0], hashes, values, offset, capacity);
        // recurse down the right branch.
        hash_tree_to_kvt_(root->branch[1], hashes, values, offset, capacity);
    }
}

/// Copies the contents of a hash tree into an ordered list of keys and values
/// suitable for serialization or better runtime cache behavior. This operation
/// has O(N log N) time complexity.
///
/// @param root The starting node of the hash tree (or sub-tree).
/// @param hashes The array of hash values to write to.
/// @param values The array of associated data values to write to.
/// @param offset The zero-based offset into the array at which to write the
/// key-value pairs.
template <typename T>
inline void hash_tree_to_kvt(
    data::hash_node_t<T> const *root,
    uint32_t                   *hashes,
    T                          *values,
    size_t                      offset,
    size_t                      capacity)
{
    size_t ofs = offset;
    hash_tree_to_kvt_rec(root, hashes, values, ofs, capacity);
    sort(hashes, values, capacity, data::comparer_t());
}

/// Initializes a list node structure to the default initial state. This
/// function does not attempt to allocate or free any memory.
///
/// @param node The node to initialize.
template <typename T>
inline void initialize_list_node(data::list_node_t<T> *node)
{
    node->next = NULL;
    node->prev = NULL;
}

/// Pushes a single item onto the front of the list.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param node The node structure that will store the new item.
/// @param value The value to store at the node.
/// @return A value of 1 if the operation was successful, or 0 otherwise.
template <typename T>
inline size_t list_push(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    data::list_node_t<T>  *node,
    T const               &value)
{
    list_node_t<T> *headp = *head;

    // store the value at the new node.
    node->value = value;

    // insert the node at the front of the list.
    if (headp  != NULL)
    {
        // inserting into a non-empty list.
        node->next  = headp;
        node->prev  = NULL;
        headp->prev = node;
        *head       = node;
    }
    else
    {
        // inserting into an empty list.
        node->next  = NULL;
        node->prev  = NULL;
        *head       = node;
        *tail       = node;
    }
    return 1;
}

/// Pushes a single item onto the back of the list.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param node The node structure that will store the new item.
/// @param value The value to store in the list.
/// @return A value of 1 if the operation was successful, or 0 otherwise.
template <typename T>
inline size_t list_append(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    data::list_node_t<T>  *node,
    T const               &value)
{
    list_node_t<T> *tailp = *tail;

    // store the value into the new node.
    node->value = value;

    // insert the node at the end of the list.
    if (tailp  != NULL)
    {
        // inserting into a non-empty list.
        node->next  = NULL;
        node->prev  = tailp;
        tailp->next = node;
        *tail       = node;
    }
    else
    {
        // inserting into a empty list.
        node->next  = NULL;
        node->prev  = NULL;
        *head       = node;
        *tail       = node;
    }
    return 1;
}

/// Inserts a single item immediately after a particular node in the list.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param node The node that will store the new item.
/// @param pos Pointer to the node after which the new node will be inserted.
/// @param value The value to store in the list.
/// @return A value of 1 if the operation was successful, or 0 otherwise.
template <typename T>
inline size_t list_insert_after(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    data::list_node_t<T>  *node,
    data::list_node_t<T>  *pos,
    const T               &value)
{
    list_node_t<T> *headp =*head;
    list_node_t<T> *tailp =*tail;

    if (NULL == pos)
    {
        // insert the node at the back of the list.
        pos   = tailp;
    }

    // store the value into the new node.
    node->value = value;

    // insert the new node into the list.
    if (headp != NULL)
    {
        if (pos != tailp)
        {
            // inserting after a non-tail node.
            node->next      = pos->next;
            node->prev      = pos;
            pos->next->prev = node;
            pos->next       = node;
        }
        else
        {
            // inserting after the end of the list.
            node->next      = NULL;
            node->prev      = tailp;
            tailp->next     = node;
            *tail           = node;
        }
    }
    else
    {
        // inserting into a empty list.
        node->next = NULL;
        node->prev = NULL;
        *head      = node;
        *tail      = node;
    }
    return 1;
}

/// Inserts a single item immediately before a particular node in the list.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param node The node that will store the new item.
/// @param pos Pointer to the node before which the new node will be inserted.
/// @param value The value to store in the list.
/// @return A value of 1 if the operation was successful, or 0 otherwise.
template <typename T>
inline size_t list_insert_before(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    data::list_node_t<T>  *node,
    data::list_node_t<T>  *pos,
    T const               &value)
{
    list_node_t<T> *headp = *head;

    if (NULL == pos)
    {
        // insert the node at the front of the list.
        pos   = headp;
    }

    // store the value into the new node.
    node->value = value;

    // insert the new node into the list.
    if (headp != NULL)
    {
        if (pos != headp)
        {
            // inserting before a non-head node.
            node->next       = pos;
            node->prev       = pos->prev;
            pos->prev->next  = node;
            pos->prev        = node;
        }
        else
        {
            // inserting before the head of the list.
            node->next       = headp;
            node->prev       = NULL;
            headp->prev      = node;
            *head            = node;
        }
    }
    else
    {
        // inserting into a empty list.
        node->next = NULL;
        node->prev = NULL;
        *head      = node;
        *tail      = node;
    }
    return 1;
}

/// Pops a single node from the front of the list.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param out_front On return, this value points to the node that was
/// removed. If the list was empty, this value is not modified. The value
/// stored at the node is unchanged, and may be accessed by the application.
/// @return Non-zero if the list was not empty; zero if the list was empty.
template <typename T>
inline size_t list_pop_front(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    data::list_node_t<T> **out_front)
{
    list_node_t<T> *node   = NULL;
    list_node_t<T> *headp  =*head;
    size_t          result = 0;

    if (headp != NULL)
    {
        // the list is non-empty; pop a node.
        result = 1;
        node   = headp;
        if (node->next != NULL)
        {
            // the list has more than one element.
            node->next->prev = NULL;
        }
        *head     = node->next;
        if (NULL == headp)
        {
            // the list is now empty; NULL tail ref.
            *tail = NULL;
        }
        if (out_front != NULL)
        {
            // store the node for the caller.
            *out_front = node;
        }
    }
    else
    {
        // the list is empty.
        result = 0;
    }
    return result;
}

/// Pops a single node from the back of the list.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param out_back On return, this value points to the node that was
/// removed. If the list was empty, this value is not modified. The value
/// stored at the node is unchanged, and may be accessed by the application.
/// @return Non-zero if the list was not empty; zero if the list was empty.
template <typename T>
inline size_t list_pop_back(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    data::list_node_t<T> **out_back)
{
    list_node_t<T> *node   = NULL;
    list_node_t<T> *headp  =*head;
    list_node_t<T> *tailp  =*tail;
    size_t          result = 0;

    if (headp  != NULL)
    {
        // the list is non-empty; pop a node.
        result  = 1;
        node    = tailp;
        if (node->prev != NULL)
        {
            // the list has more than one element.
            node->prev->next = NULL;
        }
        *tail     = node->prev;
        if (NULL == tailp)
        {
            // the list is now empty; NULL head ref.
            *head = NULL;
        }
        if (out_back != NULL)
        {
            // store the node for the caller.
            *out_back = node;
        }
    }
    else
    {
        // the list is empty.
        result = 0;
    }
    return result;
}

/// Removes a single item from the list.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param pos Pointer to the node to remove.
/// @return Non-zero if the list was not empty; zero if the list was empty.
template <typename T>
inline size_t list_remove(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    data::list_node_t<T>  *pos)
{
    list_node_t<T> *node   = pos;
    list_node_t<T> *headp  =*head;
    list_node_t<T> *tailp  =*tail;
    size_t          result = 0;

    if (headp != NULL && headp->next != NULL)
    {
        // the list contains more than one item.
        // the list will not be empty post-removal.
        if (pos != headp && pos != tailp)
        {
            // removing a node in the list interior.
            node->prev->next = pos->next;
            node->next->prev = pos->prev;
            result           = 1;
        }
        else if (pos == headp)
        {
            // removing the list head node.
            node->next->prev = NULL;
            *head            = node->next;
            result           = 1;
        }
        else if (pos == tailp)
        {
            // removing the list tail node.
            node->prev->next = NULL;
            *tail            = node->prev;
            result           = 1;
        }
    }
    else if (headp != NULL && NULL == headp->next)
    {
        // the list will be empty after removal.
        *head  = NULL;
        *tail  = NULL;
        result = 1;
    }
    else
    {
        // the list is empty.
        result = 0;
    }
    return result;
}

/// Performs a linear search of a list for a specific item. The search runs
/// in O(N) [linear] time and performs N comparisons.
///
/// @param start The node representing the starting point of the search.
/// @param value The value to search for.
/// @return A pointer to the first list node found with the specified value.
template <typename T>
inline data::list_node_t<T>* list_find_first(
    data::list_node_t<T>   *start,
    T const                &value)
{
    list_node_t<T> *iter = start;
    while (iter != NULL)
    {
        if (iter->value != value)
        {
            // not the node we're looking for.
            iter = iter->next;
        }
        else
        {
            // found the node we're looking for.
            return iter;
        }
    }
    return NULL;
}

/// Performs a linear search of a list for a specific item. The search runs
/// in O(N) [linear] time and performs N comparisons. The list is searched
/// in reverse order.
///
/// @param start The node representing the starting point of the search. The
/// search is performed backwards from this node.
/// @param value The value to search for.
/// @return A pointer to the first list node found with the specified value.
template <typename T>
inline data::list_node_t<T>* list_find_last(
    data::list_node_t<T>   *start,
    T const                &value)
{
    list_node_t<T> *iter = start;
    while (iter != NULL)
    {
        if (iter->value != value)
        {
            // not the node we're looking for.
            iter = iter->prev;
        }
        else
        {
            // found the node we're looking for.
            return iter;
        }
    }
    return NULL;
}

/// Copies the contents of a list (or sublist) into an array.
///
/// @param head Pointer to the start of the list (or sublist) to copy to
/// the array.
/// @param array Pointer to the array to write to.
/// @param array_offset The offset into @a array at which to begin writing
/// values copied from the list.
/// @param list_offset The zero-based offset into the list of the node to
/// start copying from.
/// @param count The number of values to copy into the array, or -1 to copy
/// all items from the starting node to the end of the list.
/// @return The number of values copied into the array.
template <typename T>
inline size_t list_to_array(
    data::list_node_t<T> const *head,
    T                          *array,
    size_t                      array_offset,
    size_t                      list_offset,
    ptrdiff_t                   count)
{
    list_node_t<T> const *iter  = head;
    size_t                index = 0;
    size_t                end   = 0;
    size_t                n     = 0;

    // find the starting node.
    while (index < list_offset && iter != NULL)
    {
        iter = iter->next;
        ++index;
    }

    if (count < 0)
    {
        // copy the entire remaining list.
        index = array_offset;
        while (iter != NULL)
        {
            array[index++] = iter->value;
            iter           = iter->next;
            ++n;
        }
    }
    else
    {
        // copy a specific number of elements.
        end   = array_offset + count;
        index = array_offset;
        while (index < end && iter != NULL)
        {
            array[index++] = iter->value;
            iter           = iter->next;
            ++n;
        }
    }
    return n;
}

/// Sorts a linked list. The sort is stable and runs in O(N log N) time, with
/// a small constant space overhead.
///
/// @param head A pointer to the location used to store the reference to the
/// list head node. On return, this value may be updated with a new reference.
/// @param tail A pointer to the location used to store the reference to the
/// list tail node. On return, this value may be updated with a new reference.
/// @param comparer An object implementing an operator with the following
/// signature: int operator()(T const &a, T const &b) const
template <typename T, typename C>
inline void sort_list(
    data::list_node_t<T> **head,
    data::list_node_t<T> **tail,
    C const                   &comparer)
{
    list_node_t<T> *p       = NULL;
    list_node_t<T> *q       = NULL;
    list_node_t<T> *e       = NULL;
    list_node_t<T> *t       = NULL;
    list_node_t<T> *l       =*head;
    int32_t         in_size = 1;
    int32_t         merges  = 0;
    int32_t         p_size  = 0;
    int32_t         q_size  = 0;
    int32_t         i       = 0;

    if (*head != NULL)
    {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127) /* warning: conditional expression is constant */
#endif /* _MSC_VER */
        while (true)
#ifdef _MSC_VER
#pragma warning(pop)          /* warning: conditional expression is constant */
#endif /* _MSC_VER */
        {
            p      = l;
            l      = NULL;
            t      = NULL;
            merges = 0; // number of merges this pass.

            while (p != NULL)
            {
                // we must perform a merge on this pass.
                // step 'in_size' places forward from 'p'.
                ++merges;
                q       = p;
                p_size  = 0;
                for (i  = 0; i < in_size; ++i)
                {
                    ++p_size;
                    q = q->next;
                    if (NULL == q)
                    {
                        break;
                    }
                }

                // if 'q' hasn't fallen off the end, we
                // have two lists to be merged together.
                q_size = in_size;
                while (p_size > 0 || (q_size > 0 && q != NULL))
                {
                    // decide whether next element 'e' to be
                    // merged comes from list 'p' or list 'q'.
                    if (0 == p_size)
                    {
                        // 'p' is empty; 'e' must come from 'q'.
                        e = q;
                        q = q->next;
                        --q_size;
                    }
                    else if (0 == q_size || NULL == q)
                    {
                        // 'q' is empty; 'e' must come from 'p'.
                        e = p;
                        p = p->next;
                        --p_size;
                    }
                    else if (comparer(p->value, q->value) <= 0)
                    {
                        // head of 'p' <= head of 'q'; must come from 'p'.
                        e = p;
                        p = p->next;
                        --p_size;
                    }
                    else
                    {
                        // head of 'q' < head of 'p'; must come from 'q'.
                        e = q;
                        q = q->next;
                        --q_size;
                    }

                    // add element 'e' to the back of the merged list.
                    if (t != NULL)
                    {
                        // appending into a non-empty list.
                        t->next = e;
                    }
                    else
                    {
                        // appending into an empty list.
                        l = e;
                    }
                    e->prev = t;
                    t       = e;
                }
                // both 'p' and 'q' have stepped 'in_size' places.
                p = q;
            }
            t->next = NULL;
            if (merges <= 1)
            {
                // merge count <= 1; we are done sorting.
                *head = l;
                *tail = t;
                break;
            }

            // not done; repeat merging lists twice the size.
            in_size *= 2;
        }
    }
}

/// Counts the number of nodes in a list or sub-list. If the list is empty,
/// the return value is zero, but if the list has at least one node, the
/// return value is at least 1, even if @a start and @a end point to the
/// same node.
///
/// @param start The starting node.
/// @param end The ending node.
/// @return The number of nodes in [start, end].
template <typename T>
inline size_t sublist_size(
    data::list_node_t<T> const *start,
    data::list_node_t<T> const *end)
{
    if (start != end)
    {
        size_t                count = 0;
        list_node_t<T> const *iter  = start;
        while (iter != end && iter != NULL)
        {
            ++count;
            iter = iter->next;
        }
        return count + 1;
    }
    return (start != NULL) ? 1 : 0;
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace data */

#endif /* LIBDATA_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
