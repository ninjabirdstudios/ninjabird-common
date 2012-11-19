/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libdisk.hpp
///  Defines the runtime interface to the system file I/O library; on Windows
///  CreateFile/ReadFile/WriteFile, etc. are used while on POSIX systems the
///  standard open, read, write, etc. are used. All file I/O is performed in
///  binary mode, and no translation is performed.
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIBDISK_HPP_INCLUDED
#define LIBDISK_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

#if   CMN_IS_APPLE
    #define  _DARWIN_USE_64BIT_INODE
    #include <stdio.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #undef   _DARWIN_USE_64BIT_INODE
#elif CMN_IS_LINUX
    #include <stdio.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
#elif CMN_IS_WINDOWS
    #include <stdio.h>
    #include <windows.h>
    #include <sys/stat.h>
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace disk {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// Defines the flags that can be passed to the disk::open_file() function or
/// its direct I/O variant.
enum file_flags_e
{
    /// Open the file with read access.
    FILE_FLAGS_READ        = (1 << 0),
    /// Open the file with write access.
    FILE_FLAGS_WRITE       = (1 << 1),
    /// Create the file if it does not exist. This attribute implies
    /// write access.
    FILE_FLAGS_CREATE      = (1 << 2),
    /// A value used to force the storage size of the enumeration to 32-bits.
    FILE_FLAGS_FORCE_32BIT = CMN_FORCE_32BIT
};

/// Defines the values that can be passed to the disk::seek_file() function or
/// its direct I/O variant.
enum seek_from_e
{
    /// The offset value is positive and specified relative to the beginning of
    /// the file.
    SEEK_FROM_START        = 0,
    /// The offset value is positive or negative and specified relative to the
    /// current position within the file.
    SEEK_FROM_CURRENT      = 1,
    /// The offset value is negative and specified relative to the end of
    /// the file.
    SEEK_FROM_END          = 2,
    /// A value used to force the storage size of the enumeration to 32-bits.
    SEEK_FROM_FORCE_32BIT  = CMN_FORCE_32BIT
};

/// The disk::file_t type is used for performing standard, buffered file I/O.
/// It relies on the standard C library fopen, fread, fwrite, fseek, etc. set
/// of functions. These functions work well for general-purpose I/O.
/// The disk::direct_t type is used for performing, raw, unbuffered file
/// I/O. Direct file access is primarily used for reading data from disk with
/// minimal buffering. The kernel page cache is disabled.
#if   CMN_IS_APPLE || CMN_IS_LINUX
    typedef FILE*  file_t;
    typedef int    direct_t;
#elif CMN_IS_WINDOWS
    typedef FILE*  file_t;
    typedef HANDLE direct_t;
#endif

/// Queries the filesystem for the current size of a particular file on disk.
///
/// @param path A NULL-terminated string specifying the path of the file to
/// query on disk.
/// @return The number of bytes representing the current size of the specified
/// file. If the file specified by @a path does not exist, zero is returned.
CMN_PUBLIC uint64_t file_size(char const *path);

/// Queries the current size of an open file. The size is determined by seeking
/// to the current end of the file, and then back to the current position.
///
/// @param file The file to query.
/// @return The number of bytes representing the current size of the file.
CMN_PUBLIC uint64_t file_size(disk::file_t file);

/// Queries the current size of an open file. The size is determined by seeking
/// to the current end of the file, and then back to the current position.
///
/// @param file The file to query.
/// @return The number of bytes representing the current size of the file.
CMN_PUBLIC uint64_t file_size(disk::direct_t file);

/// Opens a file, possibly creating it if it does not currently exist.
///
/// @param path A NULL-terminated string specifying the path of the file to
/// open (or create) on disk. If the file exists, and the file is opened for
/// writing, its current contents are lost.
/// @param flags A combination of one or more disk::file_flags_e values.
/// @param out_file On return, this location is updated with the file handle.
/// @return true if the specified file was opened successfully, or false if an
/// error has occurred.
CMN_PUBLIC bool open_file(
    char const   *path,
    int32_t       flags,
    disk::file_t *out_file);

/// Closes an open file.
///
/// @param file The handle of the file to close.
CMN_PUBLIC void close_file(disk::file_t file);

/// Sets the current file pointer position within a file.
///
/// @param file The handle of the file object to modify.
/// @param from One of the disk::seek_from_e values.
/// @param offset The seek offset specifying the number of bytes by which the
/// file pointer will be moved. The sign indicates the direction.
/// @return The new file pointer position, or -1 if an error occurred.
CMN_PUBLIC int64_t seek_file(disk::file_t file, int32_t from, int64_t offset);

/// Queries the current file pointer position.
///
/// @param file The handle of the file object to query.
/// @return The current byte offset within the file, specifying the current
/// file pointer position, or -1 if an error occurred.
CMN_PUBLIC int64_t tell_file(disk::file_t file);

/// Reads bytes from a file into a caller-managed buffer.
///
/// @param file The handle of the file object to read from.
/// @param buffer Pointer to the target buffer.
/// @param offset The byte offset into @a buffer at which to begin writing.
/// @param amount The number of bytes to read from the file into @a buffer,
/// starting at @a offset.
/// @param out_at_end On return, this location is updated with the value of
/// true if the read operation encountered the end of the file.
/// @return The number of bytes read from the file and written to @a buffer.
CMN_PUBLIC size_t read_file(
    disk::file_t  file,
    void         *buffer,
    ptrdiff_t     offset,
    size_t        amount,
    bool         *out_at_end);

/// Writes bytes to a file from a caller-managed buffer.
///
/// @param file The handle of the file object to write to.
/// @param buffer Pointer to the source buffer.
/// @param offset The byte offset into @a buffer at which to begin reading data
/// to be written to the file.
/// @param amount The number of bytes to copy from @a buffer, starting at byte
/// offset @a offset, into the file.
/// @return The number of bytes written to the file.
CMN_PUBLIC size_t write_file(
    disk::file_t  file,
    void const   *buffer,
    ptrdiff_t     offset,
    size_t        amount);

/// Forces the system to commit all buffered write operations for a given file
/// to disk.
///
/// @param file The handle of the file object to flush.
CMN_PUBLIC void flush_file(disk::file_t file);

/// Queries the file system to determine whether the current file pointer for
/// a file is at the end of the file.
///
/// @param file The handle of the file object to query.
/// @return true if the current file pointer for @a file is at the end of file.
CMN_PUBLIC bool end_of_file(disk::file_t file);

/// Opens a file for non-buffered, direct I/O access. Direct I/O files are
/// always opened in read-only mode.
///
/// @param path A NULL-terminated string specifying the path of the file to
/// open (or create) on disk.
/// @param out_file On return, this location is updated with the file handle.
/// @return true if the specified file was opened successfully, or false if an
/// error has occurred.
CMN_PUBLIC bool open_direct(char const *path, disk::direct_t *out_file);

/// Closes a file opened for direct I/O access.
///
/// @param file The handle of the file to close.
CMN_PUBLIC void close_direct(disk::direct_t file);

/// Sets the current file pointer position within a file.
///
/// @param file The handle of the file object to modify.
/// @param from One of the disk::seek_from_e values.
/// @param off The seek offset specifying the number of bytes by which the file
/// pointer will be moved. The sign indicates the direction.
/// @return The new file pointer position.
CMN_PUBLIC int64_t seek_direct(disk::direct_t file, int32_t from, int64_t off);

/// Queries the current file pointer position.
///
/// @param file The handle of the file object to query.
/// @return The current byte offset within the file, specifying the current
/// file pointer position.
CMN_PUBLIC int64_t tell_direct(disk::direct_t file);

/// Reads bytes from a file into a caller-managed buffer. Direct I/O imposes
/// certain restrictions on this operation; namely, the address specified by
/// the combination of @a buffer plus @a offset must be evenly divisible by
/// the system page size. Direct file I/O allows the caller to bypass any
/// kernel or library-level buffering; data is directly copied into the caller
/// buffer using DMA.
///
/// @param file The handle of the file object to read from.
/// @param buffer Pointer to the target buffer.
/// @param offset The byte offset into @a buffer at which to begin writing.
/// @param amount The number of bytes to read from the file into @a buffer,
/// starting at @a offset.
/// @return The number of bytes read from the file and written to @a buffer.
CMN_PUBLIC size_t read_direct(
    disk::direct_t  file,
    void           *buffer,
    ptrdiff_t       offset,
    size_t          amount);

/// Opens a file and reads the complete contents of the file into a caller-
/// managed buffer.
///
/// @param path A NULL-terminated string specifying the path of the file to
/// open and read.
/// @param buffer Pointer to a caller-managed buffer into which the file
/// contents will be read. Data is only written to the buffer if the complete
/// contents of the file can be stored.
/// @param buffer_offset The zero-based byte offset into @a buffer at which to
/// begin writing data.
/// @param buffer_size The maximum number of bytes that can be written to
/// @a buffer (the total allocated size of the buffer.)
/// @param out_file_size Pointer to a memory location that will be updated with
/// the number of bytes written to @a buffer, starting at @a buffer_offset and
/// indicating the size of the file specified by @a path.
/// @return The number of bytes read from the file and written to @a buffer,
/// or zero if the file is too large to fit within the supplied buffer.
CMN_PUBLIC size_t file_contents(
    char const *path,
    void       *buffer,
    ptrdiff_t   buffer_offset,
    size_t      buffer_size,
    size_t     *out_file_size);

/// Opens a file and reads the complete contents of the file into a temporary
/// buffer. The buffer is guaranteed to be NULL-terminated.
///
/// @param path A NULL-terminated string specifying the path of the file to
/// open and read.
/// @param out_file_size Pointer to a memory location that will be updated with
/// the number of bytes written to the returned buffer, not including the
/// trailing NULL-terminator byte.
/// @return A pointer to a buffer, allocated using the standard C library
/// function malloc(), containing the contents of the specified file. The
/// caller should free this buffer using the standard C library function
/// free(). The function returns NULL if an error occurs.
CMN_PUBLIC char* file_contents(char const *path, size_t *out_file_size);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace disk */

#endif /* LIBDISK_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
