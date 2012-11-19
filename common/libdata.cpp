/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements basic data structures and algorithms, including doubly-
/// linked lists for storage, hash-based structures for acceleration, key-value
/// table manipulation, string data storage and array manipulation. No memory
/// allocation is performed by this library, so higher-level types must be
/// constructed on top of these primitives.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <cassert>
#include "libdata.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

void data::string_data_init(
    data::string_data_t *data,
    void                *memory,
    size_t               memory_size,
    size_t               memory_used /* = 0 */)
{
    assert(memory_used <= memory_size);
    if (memory_size > 0)
    {
        // this is here to catch memory allocation failures in debug mode.
        assert(memory  != NULL);
    }
    if (data != NULL)
    {
        char *start        = (char*) memory;
        data->next_block   = start + memory_used;
        data->memory_start = start;
        data->memory_end   = start + memory_size;
        data->memory_size  = memory_size;
        // zero the unused portion of the memory block.
        memset(start + memory_used, 0, memory_size - memory_used);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void data::string_data_reset(data::string_data_t *data)
{
    if (data != NULL)
    {
        // @note: we zero the contents of the memory block
        // because we may still have external structures
        // with pointers into it. this way they all point
        // to zero-length strings.
        data->next_block = data->memory_start;
        memset(data->memory_start, 0, data->memory_size);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* data::string_data_search(data::string_data_t *data, char const *str)
{
    char const *strp    = str ? str : "";     // pointer to non-null str
    char       *cur_ptr = data->memory_start; // start from the beginning
    char       *end_ptr = data->next_block;   // only search used space
    size_t      len_a   = strlen(strp);
    size_t      len_b   = 0;

    while (cur_ptr < end_ptr)
    {
        // get the length of the string we're comparing to.
        // only perform the actual comparison if lengths match.
        len_b      = strlen(cur_ptr);
        if (len_a != len_b || strcmp(cur_ptr, strp))
        {
            // no match; continue. +1 to skip null byte.
            cur_ptr += len_b + 1;
            continue;
        }
        return cur_ptr;
    }
    return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* data::string_data_intern(data::string_data_t *data, char const *str)
{
    char const *strp = str ? str : "";
    size_t      len  = strlen(strp)+1; // + 1 for null byte
    if (data->next_block   + len <= data->memory_end)
    {
        char *block        = data->next_block;
        data->next_block  += len;
        strcpy(block, strp); // @note: also copies null byte
        return block;
    }
    else return NULL;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t data::string_data_bytes_free(data::string_data_t *data)
{
    return (size_t) (data->memory_end - data->next_block);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t data::string_data_bytes_used(data::string_data_t *data)
{
    return (size_t) (data->next_block - data->memory_start);
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
