/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the runtime interface to the system sockets library; on
/// Windows WinSock2 is used while on OS X and Linux the standard BSD-style
/// socket APIs are used.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <string.h>
#include "libnetwork.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

// because having this statement everywhere is ugly.
#define SOCKET_SET_ERROR_RESULT(error_code) \
    if (out_error) *out_error = error_code

/*/////////////////////////////////////////////////////////////////////////80*/

/// Define the timeout used (in microseconds) to wait for a socket to become
/// available if a read or write operation would block because buffers are
/// full (WSAENOBUFS/ENOBUFS/WSAEWOULDBLOCK/EAGAIN). If this time interval
/// elapses, the socket is shutdown. There are 1000000 microseconds per second;
/// the default value is five seconds.
#ifndef SOCKETS_WAIT_TIMEOUT_USEC
    #define SOCKETS_WAIT_TIMEOUT_USEC 5000000U
#endif /* !defined(SOCKETS_WAIT_TIMEOUT_USEC) */

/*/////////////////////////////////////////////////////////////////////////80*/

/// Define the maximum number of times the network::read() or network::write()
/// functions will retry receiving or sending of data after recovering from
/// a full buffer condition. If this many retries occur, without successfully
/// completing the operation, the socket is disconnected.
#ifndef SOCKETS_MAX_RETRIES
    #define SOCKETS_MAX_RETRIES       5U
#endif /* !defined(SOCKETS_MAX_RETRIES) */

/*/////////////////////////////////////////////////////////////////////////80*/

static bool wait_for_read(
    network::socket_t const &sockfd,
    uint64_t                 timeout_usec)
{
    const  uint64_t  usec_per_sec = 1000000;
    struct timeval   timeout      = {0};
    fd_set           fd_read;
    fd_set           fd_error;

    // build the timeout structure.
    timeout.tv_sec  = (long) (timeout_usec / usec_per_sec);
    timeout.tv_usec = (long) (timeout_usec % usec_per_sec);

    // build the file descriptor list we're polling.
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_error);
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4127)
#endif /* _MSC_VER */
    FD_SET(sockfd, &fd_read);
    FD_SET(sockfd, &fd_error);
#ifdef _MSC_VER
#pragma warning (pop)
#endif /* _MSC_VER */

    // poll the client socket using select().
    int res  = select(FD_SETSIZE, &fd_read, NULL, &fd_error, &timeout);
    if (res == 1)
    {
        // the socket is now ready; we didn't time out.
        // reads to the socket should not block.
        return true;
    }
    if (res == 0)
    {
        // the wait timed out.
        return false;
    }
    // else, an error occurred during the wait.
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool wait_for_write(
    network::socket_t const &sockfd,
    uint64_t                 timeout_usec)
{
    const  uint64_t  usec_per_sec = 1000000;
    struct timeval   timeout      = {0};
    fd_set           fd_write;
    fd_set           fd_error;

    // build the timeout structure.
    timeout.tv_sec  = (long) (timeout_usec / usec_per_sec);
    timeout.tv_usec = (long) (timeout_usec % usec_per_sec);

    // build the file descriptor list we're polling.
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_error);
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4127)
#endif /* _MSC_VER */
    FD_SET(sockfd, &fd_write);
    FD_SET(sockfd, &fd_error);
#ifdef _MSC_VER
#pragma warning (pop)
#endif /* _MSC_VER */

    // poll the client socket using select().
    int res  = select(FD_SETSIZE, NULL, &fd_write, &fd_error, &timeout);
    if (res == 1)
    {
        // the socket is now ready; we didn't time out.
        // writes to the socket should not block.
        return true;
    }
    if (res == 0)
    {
        // the wait timed out.
        return false;
    }
    // else, an error occurred during the wait.
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool network::startup(void)
{
#if CMN_IS_WINDOWS
    WORD    version = MAKEWORD(2, 2);
    WSAData wsaData = {0};
    if (WSAStartup(version, &wsaData) != 0)
    {
        // the winsock library could not be initialized.
        return false;
    }
#endif
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void network::cleanup(void)
{
#if CMN_IS_WINDOWS
    WSACleanup();
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool network::socket_error(int return_value)
{
#if CMN_IS_WINDOWS
    return (return_value == SOCKET_ERROR);
#else
    return (return_value  < 0);
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool network::socket_valid(network::socket_t const &sockfd)
{
    // @note: INVALID_SOCKET_ID = INVALID_SOCKET on Windows or -1 on *nix.
    return (sockfd != INVALID_SOCKET_ID);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void network::close(network::socket_t const &sockfd)
{
#if CMN_IS_WINDOWS
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool network::listen(
    char const        *service_or_port,
    size_t             backlog,
    bool               local_only,
    network::socket_t *out_server_sockfd,
    int               *out_error /* = NULL */)
{
    struct addrinfo    hints = {0};
    struct addrinfo   *info  = NULL;
    struct addrinfo   *iter  = NULL;
    network::socket_t  sock  = INVALID_SOCKET_ID;
    int                yes   =  1;
    int                res   =  0;

    SOCKET_SET_ERROR_RESULT(0);

    if (NULL == service_or_port || NULL == out_server_sockfd)
    {
        // invalid parameter. fail immediately.
        if (out_server_sockfd != NULL) *out_server_sockfd = INVALID_SOCKET_ID;
        return false;
    }

    // ask the system to fill out the addrinfo structure we'll use to set up
    // the socket. this method works with both IPv4 and IPv6.
    hints.ai_family   = AF_UNSPEC;   // AF_INET, AF_INET6 or AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags    = local_only ? 0 : AI_PASSIVE;
    res = getaddrinfo(NULL, service_or_port, &hints, &info);
    if (res != 0)
    {
        // getaddrinfo failed. inspect res to see what the problem was.
        *out_server_sockfd = INVALID_SOCKET_ID;
        SOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // iterate through the returned interface information and attempt to bind
    // a socket to the interface. stop as soon as we succeed.
    for (iter = info; iter != NULL; iter = iter->ai_next)
    {
        sock  = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
        if (INVALID_SOCKET_ID == sock) continue;
        // prevent an 'address already in use' error message which could
        // occur if a socket connected to this port previously hasn't finished
        // being shut down by the operating system yet. we don't want to
        // wait for the operating system timeout to occur.
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes));
        // bind to the specified address/port.
        res = bind(sock, iter->ai_addr, iter->ai_addrlen);
        if (network::socket_error(res))
        {
            network::close(sock);
            continue;
        }
        // if we've gone this far, we're done.
        break;
    }

    // were we able to bind to the interface?
    if (NULL == iter)
    {
        freeaddrinfo(info);
        *out_server_sockfd = INVALID_SOCKET_ID;
        SOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // we no longer need the addrinfo list, so free it.
    freeaddrinfo(info); info = NULL;

    // start the socket in listen mode.
    res = ::listen(sock, (int) backlog);
    if (network::socket_error(res))
    {
        network::close(sock);
        *out_server_sockfd = INVALID_SOCKET_ID;
        SOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // we've completed socket setup successfully.
    *out_server_sockfd = sock;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool network::accept(
    network::socket_t const &server_sockfd,
    bool                     non_blocking,
    network::socket_t       *out_client_sockfd,
    struct addrinfo         *out_client_info,
    size_t                  *out_client_info_size,
    int                     *out_error /*  = NULL */)
{
    socklen_t         client_size = sizeof(sockaddr_storage);
    sockaddr_storage  client_addr = {0};
    network::socket_t sock        = INVALID_SOCKET_ID;

    SOCKET_SET_ERROR_RESULT(0);

    if (INVALID_SOCKET_ID == server_sockfd || NULL  == out_client_sockfd)
    {
        // invalid parameter. fail immediately.
        if (out_client_sockfd    != NULL) *out_client_sockfd    = INVALID_SOCKET_ID;
        if (out_client_info_size != NULL) *out_client_info_size = 0;
        return false;
    }

    // accept() will block until a connection is ready or an error occurs.
    sock = ::accept(server_sockfd, (struct sockaddr*)&client_addr, &client_size);
    if (INVALID_SOCKET_ID == sock)
    {
        *out_client_sockfd = INVALID_SOCKET_ID;
        if (out_client_info_size != NULL) *out_client_info_size = 0;
        return false;
    }

#if CMN_IS_WINDOWS
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        u_long nbio_mode = 1;
        ioctlsocket(sock, FIONBIO, &nbio_mode);
    }
#else
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        fcntl(sock, F_SETFL, O_NONBLOCK);
    }
#endif

    // we are done; store information for the caller.
    *out_client_sockfd   = sock;
    if (out_client_info != NULL)
    {
        // copy client data for the caller.
        memcpy(out_client_info, &client_addr, client_size);
    }
    if (out_client_info_size != NULL) *out_client_info_size = client_size;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool network::connect(
    char const        *host_or_address,
    char const        *service_or_port,
    bool               non_blocking,
    network::socket_t *out_remote_sockfd,
    int               *out_error /* = NULL */)
{
    struct addrinfo    hints = {0};
    struct addrinfo   *info  = NULL;
    struct addrinfo   *iter  = NULL;
    network::socket_t  sock  = INVALID_SOCKET_ID;
    int                res   =  0;

    SOCKET_SET_ERROR_RESULT(0);

    if (NULL == host_or_address ||
        NULL == service_or_port ||
        NULL == out_remote_sockfd)
    {
        // invalid parameter. fail immediately.
        if (out_remote_sockfd != NULL) *out_remote_sockfd = INVALID_SOCKET_ID;
        return false;
    }

    // ask the system to fill out the addrinfo structure we'll use to set up
    // the socket. this method works with both IPv4 and IPv6.
    hints.ai_family   = AF_UNSPEC;   // v4 (AF_INET), v6 (AF_INET6), AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // TCP
    res = getaddrinfo(host_or_address, service_or_port, &hints, &info);
    if (res != 0)
    {
        // getaddrinfo failed. inspect res to see what the problem was.
        *out_remote_sockfd = INVALID_SOCKET_ID;
        SOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // iterate through the returned interfce information and attempt to
    // connect. stop as soon as we succeed.
    for (iter = info; iter != NULL; iter = iter->ai_next)
    {
        sock  = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
        if (INVALID_SOCKET_ID == sock) continue;
        res   = ::connect(sock, iter->ai_addr, iter->ai_addrlen);
        if (network::socket_error(res))
        {
            network::close(sock);
            continue;
        }
        // we've successfully established a connection.
        break;
    }

    // were we able to connect to the server?
    if (NULL == iter)
    {
        freeaddrinfo(info);
        *out_remote_sockfd = INVALID_SOCKET_ID;
        SOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // we no longer need the addrinfo list, so free it.
    freeaddrinfo(info); info = NULL;

#if CMN_IS_WINDOWS
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        u_long nbio_mode = 1;
        ioctlsocket(sock, FIONBIO, &nbio_mode);
    }
#else
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        fcntl(sock, F_SETFL, O_NONBLOCK);
    }
#endif

    // we've completed socket connection successfully.
    *out_remote_sockfd = sock;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t network::read(
    network::socket_t const &sockfd,
    void                    *buffer,
    size_t                   buffer_size,
    size_t                   buffer_offset,
    bool                    *out_disconnected,
    int                     *out_error /* = NULL */)
{
    char *buf = (char*) buffer      + buffer_offset;
    int   nba = (int)  (buffer_size - buffer_offset);
    int   res = -1;

    SOCKET_SET_ERROR_RESULT(0);

    if (NULL == buffer             ||
        0    == buffer_size        ||
        NULL == out_disconnected   ||
        buffer_offset >= buffer_size)
    {
        // invalid parameter. return immediately.
        if (out_disconnected != NULL) *out_disconnected = false;
        return 0;
    }

    // ideally this loop only executes once, but if the OS buffers fill up
    // we may need to re-try the receive operation multiple times.
    size_t retry_count = 0;
    bool   try_recv    = true;
    while (try_recv)
    {
        if (retry_count >= SOCKETS_MAX_RETRIES)
        {
            // this socket has exceeded its retry count. disconnect it.
            network::shutdown(sockfd, NULL, NULL);
            *out_disconnected = true;
            return 0;
        }

        // assume the receive operation will succeed:
        try_recv = false;

        // read as much data as possible from the socket:
        res = recv(sockfd, buf, nba, 0);
        if  (!network::socket_error(res))
        {
            if (res != 0)
            {
                *out_disconnected = false;
                return (size_t) res;
            }
            else
            {
                // the socket was shutdown gracefully.
                network::shutdown(sockfd, NULL, NULL);
                *out_disconnected = true;
                return 0;
            }
        }

#if IGT_IS_WINDOWS
        // an error occurred. try to determine whether we are still connected.
        int     err = WSAGetLastError();
        switch (err)
        {
            case WSAEWOULDBLOCK:
                {
                    // no data is available to read on the socket.
                    // this is not actually an error.
                    *out_disconnected = false;
                }
                return 0;

            case WSANOTINITIALISED:
            case WSAENETDOWN:
            case WSAENOTCONN:
            case WSAENETRESET:
            case WSAENOTSOCK:
            case WSAESHUTDOWN:
            case WSAECONNABORTED:
            case WSAETIMEDOUT:
            case WSAECONNRESET:
                {
                    // we were either never connected, or the connection has
                    // somehow been terminated in a non-graceful way.
                    network::shutdown(sockfd, NULL, NULL);
                    SOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return 0;

            default:
                {
                    // an error occurred; try again.
                    try_recv = true;
                    retry_count++;
                }
                break;
        }
#else
        // an error occurred. try to determine whether we are still connected.
        int     err = errno;
        switch (err)
        {
            case EWOULDBLOCK:
                {
                    // no data is available to read on the socket.
                    // this is not actually an error.
                    *out_disconnected = false;
                }
                return 0;

            case ENOBUFS:
                {
                    // insufficient resources available in the system to
                    // perform the operation. we'll wait (in select()) for
                    // a bit to see if the socket becomes readable again;
                    // otherwise, the socket is shutdown.
                    if (wait_for_read(sockfd, SOCKETS_WAIT_TIMEOUT_USEC))
                    {
                        // the socket is available again. retry.
                        try_recv = true;
                        retry_count++;
                        continue; // back to the loop start
                    }
                    else
                    {
                        // the wait timed out. disconnect the socket.
                        network::shutdown(sockfd, NULL, NULL);
                        SOCKET_SET_ERROR_RESULT(err);
                        *out_disconnected = true;
                    }
                }
                return 0;

            case EBADF:
            case ECONNRESET:
            case ENOTCONN:
            case ENOTSOCK:
            case ETIMEDOUT:
                {
                    // we were either never connected, or the connection has
                    // somehow been terminated in a non-graceful way.
                    network::shutdown(sockfd, NULL, NULL);
                    SOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return 0;

            default:
                {
                    // an error occurred; try again.
                    try_recv = true;
                    retry_count++;
                }
                break;
        }
#endif
    }
    // no data was received.
    return 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t network::write(
    network::socket_t const &sockfd,
    void const              *buffer,
    size_t                   buffer_size,
    size_t                   buffer_offset,
    size_t                   amount_to_send,
    bool                    *out_disconnected,
    int                     *out_error /* = NULL */)
{
    char const *buf = (char*) buffer + buffer_offset;

    SOCKET_SET_ERROR_RESULT(0);

    if (NULL == buffer               ||
        0    == buffer_size          ||
        NULL == out_disconnected     ||
        buffer_offset  > buffer_size ||
        amount_to_send > buffer_size - buffer_offset)
    {
        // no data or invalid parameter. return immediately.
        if (out_disconnected != NULL) *out_disconnected = false;
        return 0;
    }

    // by default, we have not disconnected. if we detect a disconnection
    // while sending, we will set this value to true.
    *out_disconnected    = false;

    // enter a loop to ensure that all of the data gets sent.
    // we may have to issue multiple send calls to the socket.
    size_t retry_count   = 0;
    size_t bytes_sent    = 0;
    size_t bytes_total   = amount_to_send;
    while (bytes_sent    < bytes_total)
    {
        if (retry_count >= SOCKETS_MAX_RETRIES)
        {
            // this socket has exceeded its retry count. disconnect it.
            network::shutdown(sockfd, NULL, NULL);
            *out_disconnected = true;
            return 0;
        }

        // attempt to send as much data as we can write to the socket.
        int n_to_send   = (int) bytes_total - bytes_sent;
        int n_sent      = send(sockfd, buf, n_to_send, 0);
        if (!network::socket_error(n_sent) && n_sent > 0)
        {
            // @note: do NOT reset retry_count to zero in this case.
            // under Windows at least, the socket will become available
            // again, but immediately fail. awesomeness.
            bytes_sent  += n_sent;
            buf         += n_sent;
            continue;
        }

#if IGT_IS_WINDOWS
        // an error occurred. determine whether we are still connected.
        int     err = WSAGetLastError();
        switch (err)
        {
            case WSANOTINITIALISED:
            case WSAENETDOWN:
            case WSAENETRESET:
            case WSAENOTCONN:
            case WSAENOTSOCK:
            case WSAESHUTDOWN:
            case WSAEHOSTUNREACH:
            case WSAEINVAL:
            case WSAECONNABORTED:
            case WSAECONNRESET:
            case WSAETIMEDOUT:
                {
                    // something has gone wrong with the connection.
                    network::shutdown(sockfd, NULL, NULL);
                    SOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case WSAEACCES:
                {
                    // attempted to send to a broadcast address without
                    // setting the appropriate socket options.
                    network::shutdown(sockfd, NULL, NULL);
                    SOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case WSAENOBUFS:     // ENOBUFS
            case WSAEWOULDBLOCK: // EAGAIN
                {
                    // the resource is temporarily unavailable, probably
                    // because the socket is being flooded with data. we
                    // wait until the socket becomes available again, or
                    // our timeout interval elapses. if the socket becomes
                    // available, we continue sending; if the select times
                    // out, we forcibly disconnect the client.
                    if (wait_for_write(sockfd, SOCKETS_WAIT_TIMEOUT_USEC))
                    {
                        // the socket is available again. retry.
                        retry_count++;
                        break;
                    }
                    else
                    {
                        // the wait timed out. disconnect the socket.
                        network::shutdown(sockfd, NULL, NULL);
                        SOCKET_SET_ERROR_RESULT(err);
                        *out_disconnected = true;
                    }
                }
                return bytes_sent;

            default:
                {
                    // unknown error; try again.
                    SOCKET_SET_ERROR_RESULT(err);
                    retry_count++;
                }
                break;
        }
#else
        // an error occurred. determine whether we are still connected.
        int     err = errno;
        switch (err)
        {
            case EBADF:
            case ECONNRESET:
            case ENOTCONN:
            case ENOTSOCK:
            case ETIMEDOUT:
            case EHOSTUNREACH:
            case ENETDOWN:
            case ENETUNREACH:
            case EPIPE:
                {
                    // something has gone wrong with the connection.
                    network::shutdown(sockfd, NULL, NULL);
                    SOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case EACCES:
                {
                    // attempted to send to a broadcast address without
                    // setting the appropriate socket options.
                    network::shutdown(sockfd, NULL, NULL);
                    SOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case ENOBUFS:
            case EAGAIN:
                {
                    // the resource is temporarily unavailable, probably
                    // because the socket is being flooded with data. we
                    // wait until the socket becomes available again, or
                    // our timeout interval elapses. if the socket becomes
                    // available, we continue sending; if the select times
                    // out, we forcibly disconnect the client.
                    if (wait_for_write(sockfd, SOCKETS_WAIT_TIMEOUT_USEC))
                    {
                        // the socket is available again. retry.
                        retry_count++;
                        break;
                    }
                    else
                    {
                        // the wait timed out. disconnect the socket.
                        network::shutdown(sockfd, NULL, NULL);
                        SOCKET_SET_ERROR_RESULT(err);
                        *out_disconnected = true;
                    }
                }
                return bytes_sent;

            default:
                {
                    // unknown error; try again.
                    SOCKET_SET_ERROR_RESULT(err);
                    retry_count++;
                }
                break;
        }
#endif
    }
    return bytes_sent;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void network::shutdown(
    network::socket_t const  &sockfd,
    network::socket_flush_fn  rxdata_callback,
    void                     *rxdata_context)
{
    size_t const SDBUF_SIZE         = 4096;
    uint8_t      buffer[SDBUF_SIZE] = {0};
    size_t       rx_size            =  0;
    bool         disconn            = false;
#if CMN_IS_WINDOWS
    int          res                = SOCKET_ERROR;
#else
    int          res                = -1;
#endif

    if (INVALID_SOCKET_ID == sockfd)
    {
        // invalid socket. complete immediately.
        return;
    }

    // shutdown the socket in the send direction. we are saying we won't
    // be sending any more data from this side of the connection.
#if CMN_IS_WINDOWS
    res = ::shutdown(sockfd, SD_SEND);
#else
    res = ::shutdown(sockfd, SHUT_WR);
#endif
    if (network::socket_error(res))
    {
        // just close the socket immediately.
        network::close(sockfd);
        return;
    }

    // continue to receive data until the other end disconnects.
    while (rxdata_callback != NULL && !disconn)
    {
        rx_size = network::read(sockfd, buffer, SDBUF_SIZE, 0, &disconn);
        if (rx_size > 0 && rxdata_callback != NULL)
        {
            // let the caller process the data.
            rxdata_callback(buffer, rx_size, rxdata_context);
        }
    }

    // finally, close the socket down completely.
    network::close(sockfd);
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
