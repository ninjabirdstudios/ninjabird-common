/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libdisk.cpp
///  Implements the runtime interface to the system file I/O library; on
///  Windows CreateFile/ReadFile/WriteFile, etc. are used while on POSIX
///  systems the standard open, read, write, etc. are used. All file I/O is
///  performed in binary mode, and no translation is performed.
///
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <assert.h>
#include "libdisk.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

#ifdef _MSC_VER
    #define STAT64_STRUCT struct __stat64
    #define STAT64_FUNC   _stat64
    #define FTELLO_FUNC   _ftelli64
    #define FSEEKO_FUNC   _fseeki64
#else
    #define STAT64_STRUCT struct stat
    #define STAT64_FUNC   stat
    #define FTELLO_FUNC   ftello
    #define FSEEKO_FUNC   fseeko
#endif /* defined(_MSC_VER) */

/*/////////////////////////////////////////////////////////////////////////80*/

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

/*/////////////////////////////////////////////////////////////////////////80*/

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

/*/////////////////////////////////////////////////////////////////////////80*/

uint64_t disk::file_size(char const *path)
{
    STAT64_STRUCT file_info = {0};
    if (!STAT64_FUNC(path, &file_info))
    {
        // retrieved file information successfully.
        return file_info.st_size;
    }
    return 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint64_t disk::file_size(disk::file_t file)
{
    int64_t curr  = FTELLO_FUNC(file);
    FSEEKO_FUNC(file, 0,    SEEK_END);
    int64_t endp  = FTELLO_FUNC(file);
    FSEEKO_FUNC(file, curr, SEEK_SET);
    return endp;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint64_t disk::file_size(disk::direct_t file)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    off_t curr  = lseek(file, 0, SEEK_CUR);
    off_t endp  = lseek(file, 0, SEEK_END);
    lseek(file, curr, SEEK_SET);
    return endp;
#elif CMN_IS_WINDOWS
    LARGE_INTEGER size = {0};
    GetFileSizeEx(file, &size);
    return (uint64_t) size.QuadPart;
#else
    #error No implementation of disk::file_size(direct_t) for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool disk::open_file(char const *path, int32_t flags, disk::file_t *out_file)
{
    disk::file_t   fp = NULL;
    char const    *om = NULL;
    if ((flags & disk::FILE_FLAGS_CREATE) != 0) flags |= disk::FILE_FLAGS_WRITE;
    if ((flags & disk::FILE_FLAGS_READ)   != 0) om = "r+b";
    if ((flags & disk::FILE_FLAGS_WRITE)  != 0) om = "w+b";
    fp = fopen(path, om);
    if (fp != NULL)
    {
        if (out_file != NULL) *out_file = fp;
        return true;
    }
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void disk::close_file(disk::file_t file)
{
    if (file != NULL) fclose(file);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int64_t disk::seek_file(disk::file_t file, int32_t from, int64_t offset)
{
    int whence = SEEK_SET;
    switch(from)
    {
        case disk::SEEK_FROM_START:   whence = SEEK_SET; break;
        case disk::SEEK_FROM_CURRENT: whence = SEEK_CUR; break;
        case disk::SEEK_FROM_END:     whence = SEEK_END; break;
        default:                      return 0;
    }
    FSEEKO_FUNC(file, offset, whence);
    return FTELLO_FUNC(file);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int64_t disk::tell_file(disk::file_t file)
{
    return FTELLO_FUNC(file);
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t disk::read_file(
    disk::file_t  file,
    void         *buffer,
    ptrdiff_t     offset,
    size_t        amount,
    bool         *out_at_end)
{
    uint8_t *out_ptr = ((uint8_t*) buffer) + offset;
    size_t   count   =  fread(out_ptr, 1, amount, file);
    if (count < amount && out_at_end) *out_at_end = feof(file) ? true : false;
    else if (out_at_end) *out_at_end  = false;
    return count;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t disk::write_file(
    disk::file_t  file,
    void const   *buffer,
    ptrdiff_t     offset,
    size_t        amount)
{
    uint8_t *in_ptr = ((uint8_t*) buffer) + offset;
    return fwrite(in_ptr, 1, amount, file);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void disk::flush_file(disk::file_t file)
{
    fflush(file);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool disk::end_of_file(disk::file_t file)
{
    return feof(file) ? true : false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool disk::open_direct(char const *path, disk::direct_t *out_file)
{
#if   CMN_IS_APPLE
    // @note: need to read in multiples of the page size.
    // @note: need to read into a page-aligned address.
    disk::direct_t file = open(path, O_RDONLY);
    if (file >= 0)
    {
        fcntl(file,  F_RDAHEAD, 0); // disable disk read-ahead
        fcntl(file,  F_NOCACHE, 1); // turn off kernel page cache
        if (out_file != NULL) *out_file = file;
        return true;
    }
    return false;
#elif CMN_IS_LINUX
    // @note: need to read in multiples of the page size.
    // @note: need to read into a page-aligned address.
    disk::direct_t file = open(path, O_RDONLY | O_DIRECT);
    if (file >= 0)
    {
        if (out_file != NULL) *out_file = file;
        return true;
    }
    return false;
#elif CMN_IS_WINDOWS
    disk::direct_t file = CreateFileA(
        path, 
        GENERIC_READ, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        FILE_FLAG_NO_BUFFERING, 
        NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        if (out_file != NULL) *out_file = file;
        return true;
    }
    return false;
#else
    #error No implementation of disk::open_direct() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void disk::close_direct(disk::direct_t file)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    if (file != -1) close(file);
#elif CMN_IS_WINDOWS
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
#else
    #error No implementation of disk::close_direct() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

int64_t disk::seek_direct(
    disk::direct_t file,
    int32_t        from,
    int64_t        offset)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    int whence = SEEK_SET;
    switch (from)
    {
        case disk::SEEK_FROM_START:   whence = SEEK_SET; break;
        case disk::SEEK_FROM_CURRENT: whence = SEEK_CUR; break;
        case disk::SEEK_FROM_END:     whence = SEEK_END; break;
        default:                      return 0;
    }
    // @note: can move only to physical sector-aligned positions.
    return lseek(file, (off_t) offset, whence);
#elif CMN_IS_WINDOWS
    DWORD whence = FILE_BEGIN;
    switch (from)
    {
        case disk::SEEK_FROM_START:   whence = FILE_BEGIN;   break;
        case disk::SEEK_FROM_CURRENT: whence = FILE_CURRENT; break;
        case disk::SEEK_FROM_END:     whence = FILE_END;     break;
        default:                      return 0;
    }
    // @note: can move only to physical sector-aligned positions.
    LARGE_INTEGER dist = {0};
    LARGE_INTEGER pos  = {0};
    dist.QuadPart      = offset;
    SetFilePointerEx(file, dist, &pos, whence);
    return pos.QuadPart;
#else
    #error No implementation of disk::seek_direct() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

int64_t disk::tell_direct(disk::direct_t file)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    return lseek(file, 0, SEEK_CUR);
#elif CMN_IS_WINDOWS
    LARGE_INTEGER dist = {0};
    LARGE_INTEGER pos  = {0};
    SetFilePointerEx(file, dist, &pos, FILE_CURRENT);
    return pos.QuadPart;
#else
    #error No implementation of disk::tell_direct() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t disk::read_direct(
    disk::direct_t  file,
    void           *buffer,
    ptrdiff_t       offset,
    size_t          amount)
{
#if   CMN_IS_APPLE || CMN_IS_LINUX
    static size_t page_size =  (size_t)   getpagesize();
    uint8_t      *out_ptr   = ((uint8_t*) buffer) + offset;
    // the amount being read must be a multiple of the page size.
    assert(aligned_to(amount,  page_size));
    // the target address must be aligned to the system page size.
    assert(aligned_to(out_ptr, page_size));
    // perform the read. this should DMA directly into the buffer.
    ssize_t count  = read(file, out_ptr, amount);
    return (count >= 0) ? (size_t) count : 0;
#elif CMN_IS_WINDOWS
    uint8_t      *out_ptr   = ((uint8_t*) buffer) + offset;
    // the amount being read must be a multiple of the page size.
    assert(aligned_to(amount,  4096U));
    // the target address must be aligned to the system page size.
    assert(aligned_to(out_ptr, 4096U));
    // perform the read. this should DMA directly into the buffer.
    DWORD count    = 0;
    ReadFile(file, out_ptr, amount, &count, NULL);
    return (count >= 0) ? (size_t) count : 0;
#else
    #error No implementation of disk::read_direct() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t disk::file_contents(
    char const *path,
    void       *buffer,
    ptrdiff_t   buffer_offset,
    size_t      buffer_size,
    size_t     *out_file_size)
{
    FILE *file = fopen(path, "r+b");
    if (NULL  == file)
    {
        // the file can't be found, or there's a permissions issue.
        if (out_file_size) *out_file_size = 0;
        return 0;
    }

    // figure out the total size of the input file, in bytes.
    int64_t beg =  FTELLO_FUNC(file);
    FSEEKO_FUNC(file, 0,   SEEK_END);
    int64_t end =  FTELLO_FUNC(file);
    FSEEKO_FUNC(file, 0,   SEEK_SET);
    size_t sz =  (size_t) (end - beg);

    // determine whether our buffer is large enough to hold the file data.
    uint8_t *read_ptr = ((uint8_t*) buffer) + buffer_offset;
    uint8_t *end_ptr  = ((uint8_t*) buffer) + buffer_size;
    if (read_ptr+sz   > end_ptr)
    {
        // the buffer isn't large enough to hold the complete file.
        if (out_file_size) *out_file_size = sz;
        fclose(file);
        return 0;
    }

    // read the entire file contents into the buffer and close the file.
    size_t nr = fread(read_ptr, 1, sz, file);
    fclose(file);
    if (out_file_size != NULL) *out_file_size = sz;
    return nr;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char* disk::file_contents(char const *path, size_t *out_file_size)
{
    FILE *file = fopen(path, "r+b");
    if (NULL  == file)
    {
        if (out_file_size) *out_file_size = 0;
        return NULL;
    }

    // figure out the total size of the input file, in bytes.
    int64_t beg =  FTELLO_FUNC(file);
    FSEEKO_FUNC(file, 0,   SEEK_END);
    int64_t end =  FTELLO_FUNC(file);
    FSEEKO_FUNC(file, 0,   SEEK_SET);
    size_t sz =  (size_t) (end - beg);

    // allocate a temporary buffer to hold the file contents.
    // @note: add 1 to the size for a terminating null byte.
    char  *fd =  (char*) malloc(sz + 1);
    if (NULL ==  fd)
    {
        fclose(file);
        if (out_file_size) *out_file_size = sz;
        return NULL;
    }

    // read the entire file contents into the buffer.
    // terminate the buffer with a NULL byte.
    size_t nr = fread(fd, 1, sz, file);
    fd[nr]    = 0;

    // we're done, close the file and return success.
    // use free_buffer to free the returned buffer.
    fclose(file);
    if (out_file_size) *out_file_size = nr;
    return fd;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
