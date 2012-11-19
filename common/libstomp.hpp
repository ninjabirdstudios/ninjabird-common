/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines an interface to a parser and formatter for STOMP
/// protocol 1.0 and 1.1 frames. The STOMP protocol is typically used to
/// communicate with a message-oriented middleware such as Apache Apollo.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/
#ifndef LIBSTOMP_HPP_INCLUDED
#define LIBSTOMP_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace stomp {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/
struct message_t;
struct write_state_t;

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// Compile-time define the maximum number of headers we support on a single
/// message. The default of 64 should be sufficient for almost all applications
/// but this value can be overridden if necessary.
#ifndef STOMP_MAX_HEADERS
#define STOMP_MAX_HEADERS          64U
#endif /* !defined(STOMP_MAX_HEADERS) */

/// Compile-time define the maximum length of a header field value, including
/// the terminating NULL byte. This specifies the size of the stack-allocated
/// buffer used by the stomp::write_header_format() function.
#ifndef STOMP_MAX_FIELD_LENGTH
#define STOMP_MAX_FIELD_LENGTH     1024U
#endif /* !defined(STOMP_MAX_FIELD_LENGTH) */

/// The set of string identifiers specifying the STOMP protocol frame names.
/// These strings are always specified in UPPERCASE.
extern char const *FRAME_STOMP;           /// STOMP       (v1.1+, CLIENT)
extern char const *FRAME_CONNECT;         /// CONNECT     (v1.0+, CLIENT)
extern char const *FRAME_CONNECTED;       /// CONNECTED   (v1.0+, SERVER)
extern char const *FRAME_SEND;            /// SEND        (v1.0+, CLIENT)
extern char const *FRAME_SUBSCRIBE;       /// SUBSCRIBE   (v1.0+, CLIENT)
extern char const *FRAME_UNSUBSCRIBE;     /// UNSUBSCRIBE (v1.0+, CLIENT)
extern char const *FRAME_ACK;             /// ACK         (v1.0+, CLIENT)
extern char const *FRAME_NACK;            /// NACK        (v1.1+, CLIENT)
extern char const *FRAME_BEGIN;           /// BEGIN       (v1.0+, CLIENT)
extern char const *FRAME_COMMIT;          /// COMMIT      (v1.0+, CLIENT)
extern char const *FRAME_ABORT;           /// ABORT       (v1.0+, CLIENT)
extern char const *FRAME_DISCONNECT;      /// DISCONNECT  (v1.0+, CLIENT)
extern char const *FRAME_MESSAGE;         /// MESSAGE     (v1.0+, SERVER)
extern char const *FRAME_RECEIPT;         /// RECEIPT     (v1.0+, SERVER)
extern char const *FRAME_ERROR;           /// ERROR       (v1.0+, SERVER)

/// The set of string identifiers specifying the standard STOMP protocol
/// header names. These strings are always specified in lowercase.
extern char const *HEADER_ACCEPT_VERSION; /// accept-version (v1.1, REQ)
extern char const *HEADER_HOST;           /// host           (v1.1, REQ)
extern char const *HEADER_LOGIN;          /// login          (v1.1, OPT)
extern char const *HEADER_PASSCODE;       /// passcode       (v1.1, OPT)
extern char const *HEADER_VERSION;        /// version        (v1.1, REQ)
extern char const *HEADER_SESSION;        /// session        (v1.1, OPT)
extern char const *HEADER_SERVER;         /// server         (v1.1, OPT)
extern char const *HEADER_DESTINATION;    /// destination    (v1.0, REQ)
extern char const *HEADER_CONTENT_TYPE;   /// content-type   (v1.0, OPT)
extern char const *HEADER_CONTENT_LENGTH; /// content-length (v1.0, OPT)
extern char const *HEADER_ID;             /// id             (v1.0, REQ)
extern char const *HEADER_ACK;            /// ack            (v1.1, OPT)
extern char const *HEADER_HEARTBEAT;      /// heart-beat     (v1.1, OPT)
extern char const *HEADER_MESSAGE;        /// message        (v1.0, OPT)
extern char const *HEADER_MESSAGE_ID;     /// message-id     (v1.0, REQ)
extern char const *HEADER_SUBSCRIPTION;   /// subscription   (v1.0, REQ)
extern char const *HEADER_TRANSACTION;    /// transaction    (v1.0, REQ)
extern char const *HEADER_RECEIPT;        /// receipt        (v1.0, REQ)
extern char const *HEADER_RECEIPT_ID;     /// receipt-id     (v1.0, REQ)

/// Function signature for the callback invoked when an error is encountered
/// when streaming a STOMP message frame.
///
/// @param writer The writer state which encountered the error.
/// @param frame The STOMP message frame being serialized.
/// @param context Optional, opaque data passed by the application.
typedef void (CMN_CALL_C *writer_error_fn)(
    struct stomp::write_state_t *writer,
    struct stomp::message_t     *frame,
    void                        *context);

/// Function signature for the callback invoked when the internal buffer is
/// full and needs to be flushed while streaming a STOMP message frame.
///
/// @param writer The writer state being flushed.
/// @param buffer The buffer containing the data to write.
/// @param buffer_size The number of bytes of data to copy from @a buffer.
/// @param context Optional opaque data passed by the application.
/// @return true to continue serialization.
typedef bool (CMN_CALL_C *writer_flush_fn)(
    struct stomp::write_state_t *writer,
    void                        *buffer,
    size_t                       buffer_size,
    void                        *context);

/// An enumeration defining the various global STOMP message parser states.
/// These values are used by the application to determine when a complete
/// message is received, an error has occurred, or the parser needs more data.
enum parse_state_e
{
    /// Indicates that the parser has encountered an error, and recovery needs
    /// to be performed using the stomp::parse_state_recover() function.
    PARSE_STATE_ERROR               = 0,
    /// Indicates that the parser is waiting for additional input bytes to be
    /// provided to the stomp::parse_state_update() function.
    PARSE_STATE_NEED_MORE           = 1,
    /// Indicates that the parser has successfully received a complete STOMP
    /// message. Call stomp::get_message() to access the message data, followed
    /// by stomp::parse_state_reset() to reset the parser state. Note that
    /// there may still be unparsed data in the message buffer.
    PARSE_STATE_MESSAGE_COMPLETE    = 2,
    /// An unused value; force the storage size of the enumeration to 32-bits.
    PARSE_STATE_FORCE_32BIT         = CMN_FORCE_32BIT
};

/// An enumeration defining the various STOMP frame parse states. These values
/// are used internally by the library.
enum frame_parse_state_e
{    /// Indicates that the parser is seeking the start of a new STOMP frame.
    FRAME_PARSE_STATE_NEW_FRAME     = 0,
    /// Indicates that the parser is parsing the header data for a STOMP frame.
    FRAME_PARSE_STATE_FRAME_HEAD    = 1,
    /// Indicates that the parser is parsing the body data for a STOMP frame.
    FRAME_PARSE_STATE_FRAME_BODY    = 2,
    /// Indicates that the parser is recovering from an error that occurred
    /// while parsing the header data of a STOMP frame.
    FRAME_PARSE_STATE_SYNC_HEAD     = 3,
    /// Indicates that the parser is recovering from an error that occurred
    /// while parsing the body data of a STOMP frame.
    FRAME_PARSE_STATE_SYNC_BODY     = 4,
    /// An unused value; force the storage size of the enumeration to 32-bits.
    FRAME_PARSE_STATE_FORCE_32BIT   = CMN_FORCE_32BIT
};

/// An enumeration defining the various STOMP frame header parse states. These
/// values are used internally by the library.
enum head_parse_state_e
{
    /// Indicates that the parser is in the middle of parsing a command.
    HEAD_PARSE_STATE_COMMAND        = 0,
    /// Indicates that the parser is expecting the start of a header key-value
    /// pair, or a blank line indicating the end of the header data.
    HEAD_PARSE_STATE_KEY_START      = 1,
    /// Indicates that the parser is in the middle of receiving data for the
    /// key portion of a header key-value pair.
    HEAD_PARSE_STATE_KEY_DATA       = 2,
    /// Indicates that the parser is expecting the that of the value portion
    /// of a header key-value pair.
    HEAD_PARSE_STATE_VALUE_START    = 3,
    /// Indicates that the parser is in the middle of receiving data for the
    /// value portion of a header key-value pair.
    HEAD_PARSE_STATE_VALUE_DATA     = 4,
    /// An unused value; force the storage size of the enumeration to 32-bits.
    HEAD_PARSE_STATE_FORCE_32BIT    = CMN_FORCE_32BIT
};

/// An enumeration defining the various STOMP frame body parse states. These
/// values are used internally by the library.
enum body_parse_state_e
{
    /// Indicates that the parser is expecting either the first byte of the
    /// message body, or a frame terminator (null byte.)
    BODY_PARSE_STATE_DATA_START     = 0,
    /// Indicates that the parser is in the middle of receiving data for the
    /// STOMP frame body.
    BODY_PARSE_STATE_DATA           = 1,
    /// An unused value; force the storage size of the enumeration to 32-bits.
    BODY_PARSE_STATE_FORCE_32BIT    = CMN_FORCE_32BIT
};

/// An enumeration defining the various top-level STOMP frame writer states.
enum write_state_e
{
    /// Indicates that the writer is currently in an error state because
    /// invalid data was encountered or write operations were performed out of
    /// sequence (trying to write a header in the middle of the body, etc.)
    /// Call stomp::write_state_reset() to recover from the error and lose the
    /// data currently in the write buffer.
    WRITE_STATE_ERROR              = 0,
    /// Indicates that the write buffer is full and data needs to be flushed.
    /// Call stomp_write_state_flush() to retrieve the current buffer
    /// pointer and size and perform a buffer reset.
    WRITE_STATE_FLUSH              = 1,
    /// Indicates that the writer is waiting for more data before the STOMP
    /// frame can be completed.
    WRITE_STATE_NEED_MORE          = 2,
    /// Indicates that a STOMP frame has been completely specified. Call the
    /// stomp::write_state_flush() function to retrieve the buffered data,
    /// followed by stomp_write_state_reset() to reset the writer state.
    WRITE_STATE_FRAME_COMPLETE     = 3,
    /// An unused value; force the storage size of the enumeration to 32-bits.
    WRITE_STATE_FORCE_32BIT        = CMN_FORCE_32BIT
};

/// An enumeration defining the various states a writer object can be in
/// during the construction of a STOMP frame.
enum frame_write_state_e
{
    /// Indicates that the writer is waiting for a new frame to be opened and
    /// a command string specified.
    FRAME_WRITE_STATE_COMMAND       = 0,
    /// Indicates that the writer is ready to receive header key-value pairs.
    FRAME_WRITE_STATE_HEADERS       = 1,
    /// Indicates that the writer is ready to receive message body data. The
    /// message body may be specified in multiple calls.
    FRAME_WRITE_STATE_BODY          = 2,
    /// Indicates that the writer has closed the current STOMP frame and needs
    /// to be reset using stomp::write_state_reset() before another message can
    /// be generated.
    FRAME_WRITE_STATE_CLOSED        = 3,
    /// An unused value; force the storage size of the enumeration to 32-bits.
    FRAME_WRITE_STATE_FORCE_32BIT   = CMN_FORCE_32BIT
};

/// A structure used to store a process STOMP protocol frame header. For
/// messages being received, the pointers point into the receive buffer; no
/// additional memory is allocated.
struct header_t
{
    char   *command;                          /// Frame command identifier
    char   *content_type;                     /// MIME content type, if any
    char   *content_charset;                  /// Pointer to 'charset=xxxxx'
    size_t  content_length;                   /// Parsed content-length header
    size_t  header_count;                     /// The number of header fields
    char   *header_fields[STOMP_MAX_HEADERS]; /// The header field keys
    char   *header_values[STOMP_MAX_HEADERS]; /// The header field values
};

/// A structure that nicely packages a complete message for the application.
/// The head and body members are typically pointers into the structure
/// stomp::parse_state_t, but we report the message to the application in this
/// structure.
struct message_t
{
    stomp::header_t *head;                /// Pointer to the header data
    uint8_t         *body;                /// Pointer to the body data
    size_t           body_size;           /// The size of the body, in bytes
};

/// A structure that completely contains the current STOMP message parser
/// state. Multiple instances of this structure may be used to parse messages
/// concurrently.
struct parse_state_t
{
    int32_t          parse_state_global;  /// One of stomp::parse_state_e
    int32_t          parse_state_frame;   /// One of stomp::frame_parse_state_e
    int32_t          parse_state_header;  /// One of stomp::head_parse_state_e
    int32_t          parse_state_body;    /// One of stomp::body_parse_state_e
    uint8_t         *message_buffer;      /// Message composition buffer
    size_t           message_buffer_size; /// Total size of message_buffer
    size_t           message_size;        /// Size of current message in buffer
    uint8_t         *message_body_head;   /// Start of message body
    uint8_t         *message_body_tail;   /// End of message body
    stomp::header_t  message_header;      /// Header data for current message
    char const      *error_description;   /// A brief error description
};

/// A structure that maintains state for dynamic construction of STOMP messages
/// into an application-managed buffer. Multiple instances of this structure
/// may be used to construct multiple messages concurrently.
struct write_state_t
{
    int32_t          global_state;        /// One of stomp::write_state_e
    int32_t          frame_state;         /// One of stomp::frame_write_state_e
    uint8_t         *message_buffer;      /// Message composition buffer
    size_t           message_buffer_size; /// Total size of message_buffer
    size_t           message_size;        /// Size of current data in buffer
    char const      *error_description;   /// A brief error description
};

/// Initiaizes a STOMP message header structure. All fields are set to NULL/0.
///
/// @param header Pointer to the header structure to initialize.
CMN_PUBLIC void header_init(stomp::header_t *header);

/// Searches for a header with a particular field name.
///
/// @param header_info The header information structure to search.
/// @param header_name The NULL-terminated, UTF-8 encoded header field name to
/// search for.
/// @param out_index On return, if this value is not NULL, the zero-based index
/// of the header key-value pair is stored here.
/// @return true if a header field with the specified name was found;
/// otherwise, false.
CMN_PUBLIC bool find_header(
    stomp::header_t *header_info,
    char const      *header_name,
    size_t          *out_index);

/// Computes the number of bytes required to store an escaped string following
/// the rules for the STOMP protocol. The characters '\n', ':' and '\\' are
/// the recognized escape sequences.
///
/// @param value Pointer to a NULL-terminated ASCII or UTF-8 encoded string.
/// @return The number of bytes required to store the escaped string, not
/// including the null terminator byte.
CMN_PUBLIC size_t escaped_size(char const *value);

/// Writes an escaped string following the rules of the STOMP protocol to a
/// buffer. The characters '\n', ':' and '\\' are the recognized escape
/// sequences.
///
/// @param msg_buffer A pointer to the location at which to begin writing the
/// escaped string.
/// @param str_value A pointer to a NULL-terminated ASCII or UTF-8 encoded
/// string that will be written to the buffer @a msg_buffer.
/// @return The pointer in @a msg_buffer representing the next output byte.
CMN_PUBLIC uint8_t* write_escaped_string(
    uint8_t    *msg_buffer,
    char const *str_value);

/// Computes the total serialized size of a STOMP message given an unescaped
/// header and a body size. The computed size includes any newlines and nulls.
///
/// @param header A pointer to a populated stomp_header_t structure that
/// defines the STOMP command and unescaped header key-value pairs. This header
/// might be modified to fill in the content-type and content-length header
/// if the command type supports having a message body and @a body_size is
/// greater than zero.
/// @param body_size The total size of the message body, in bytes, not
/// including the trailing null byte used to terminate the frame.
/// @return The total size of the serialized message, in bytes, including all
/// data added due to escape sequences, newlines, and nulls.
CMN_PUBLIC size_t wire_size(
    stomp::header_t const *header,
    size_t                 body_size);

/// Serializes a STOMP protocol message to an application-managed buffer in
/// preparation for transmission. This method can be used to write a STOMP
/// message that is prepared entirely by the application.
///
/// @param header A pointer to the populated header structure that defines the
/// STOMP command and unescaped header key-value pairs.
/// @param body_data A pointer to the opaque body data blob. This value may be
/// NULL, in which case @a body_size must also be 0.
/// @param body_size The size of the buffer pointed to by @a body_data, in
/// bytes.
/// @param buffer The buffer to which serialized data will be written. This
/// value cannot be NULL.
/// @param buffer_size The maximum number of bytes which can be written to
/// @a buffer. This value cannot be zero.
/// @param buffer_offset The byte offset into @a buffer at which to begin
/// writing serialized message data.
/// @param out_wire_size An optional value that, on return, is updated with the
/// number of bytes written to @a buffer.
/// @return true if serialization is successful; false otherwise.
CMN_PUBLIC bool serialize(
    stomp::header_t const *header,
    void const            *body_data,
    size_t                 body_size,
    void                  *buffer,
    size_t                 buffer_size,
    size_t                 buffer_offset,
    size_t                *out_wire_size);

/// Attempts to convert a string representing a floating-point number to its
/// corresponding binary equivalent. Signed numbers and exponent notation
/// are supported.
///
/// @param string_value A pointer to the string value to parse.
/// @param out_number A pointer to a signed double-precision floating point
/// value that will be updated with the parsed value.
/// @return true if a number value could be parsed from @a string_value.
CMN_PUBLIC bool atof(char const *string_value, double *out_number);

/// Attempts to convert a decimal string value into its corresponding integer
/// equivalent. Signed values are supported.
///
/// @param string_value A pointer to the string value to parse.
/// @param out_signed A pointer to a signed integer value that will be updated
/// with the parsed value.
/// @return true if an integer value could be parsed from @a string_value.
CMN_PUBLIC bool atois(char const *string_value, int64_t *out_signed);

/// Attempts to convert a decimal string value into its corresponding integer
/// equivalent. Only unsigned values are supported.
///
/// @param string_value A pointer to the string value to parse.
/// @param out_unsigned A pointer to an unsigned integer value that will be
/// updated with the parsed value.
/// @return true if an integer value could be parsed from @a string_value.
CMN_PUBLIC bool atoiu(char const *string_value, uint64_t *out_unsigned);

/// Attempts to convert a hexadecimal string value into its corresponding
/// unsigned integer equivalent.
///
/// @param string_value A pointer to the string value to parse, not including
/// any leading '0x' or '0X'.
/// @param out_unsigned A pointer to an unsigned integer value that will be
/// updated with the parsed value.
/// @return true if an integer value could be parsed from @a string_value.
CMN_PUBLIC bool atoix(char const *string_value, uint64_t *out_unsigned);

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
/// characters that woud be written to @a output_buffer, not including the
/// trailing NULL, assuming that @a output_buffer is large enough to hold all
/// output without truncation.
/// @return true if no truncation occurred, or false if the output was
/// truncated.
CMN_PUBLIC bool format(
    char const *format_string,
    char       *output_buffer,
    size_t      max_length,
    size_t     *out_length, ...);

/// A convenience function to construct a combined content-type and charset
/// header field value into a user-supplied buffer.
///
/// @param mime_type Pointer to a null-terminated string specifying the MIME
/// content type, such as 'text/plain'. This value cannot be NULL.
/// @param charset Pointer to a null-terminated string specifying the character
/// encoding, such as 'utf-8'. This value is optional and may be NULL.
/// @param value_buffer Pointer to the start of a user-managed buffer that will
/// be populated with the composed header field value.
/// @param buffer_size The maximum number of bytes that can be written to
/// @a value_buffer (including a byte for the null terminator.)
/// @param out_value_size On return, this location is updated with the number
/// of bytes written to @a value_buffer, including the null terminator.
/// @return true if the content type value was written successfully, or
/// false if there was not enough space in @a value_buffer.
CMN_PUBLIC bool build_content_type(
    char const *mime_type,
    char const *charset,
    char       *value_buffer,
    size_t      buffer_size,
    size_t     *out_value_size);

/// A convenience function used to construct a content-length header field
/// value into a user-supplied buffer.
///
/// @param body_size The number of bytes in the message body, not including the
/// frame terminator.
///  @param value_buffer Pointer to the start of a user-managed buffer that will
/// be populated with the composed header field value.
/// @param buffer_size The maximum number of bytes that can be written to
/// @a value_buffer (including a byte for the null terminator.)
/// @param out_value_size On return, this location is updated with the number
/// of bytes written to @a value_buffer, including the null terminator.
/// @return true if the content length value was written successfully, or
/// false if there was not enough space in @a value_buffer.
CMN_PUBLIC bool build_content_length(
    size_t  body_size,
    char   *value_buffer,
    size_t  buffer_size,
    size_t *out_value_size);

/// Initializes a new STOMP message parser instance.
///
/// @param state A pointer to the message parser state object to initialize.
/// @param buffer A pointer to the application-managed message composition
/// buffer. The parser consumes bytes and writes them into this buffer. This
/// buffer should be large enough to hold the largest message the application
/// expects to receive.
/// @param buffer_size The size of @a buffer, in bytes, indicating the maximum
/// amount of data that can be written to @a buffer for a single message.
CMN_PUBLIC void parse_state_init(
    stomp::parse_state_t *state,
    void                 *buffer,
    size_t                buffer_size);

/// Resets the state of a STOMP message parser after receiving a complete
/// message. If the current state is an error state, this function redirects to
/// stomp::parse_state_recover().
///
/// @param state The parser state to reset.
/// @return One of the stomp::parse_state_e values indicating the current
/// global parser state.
CMN_PUBLIC int32_t parse_state_reset(stomp::parse_state_t *state);

/// Updates the state of a STOMP message parser after an error has occurred so
/// that the parser can re-sync with the message stream.
///
/// @param state The parser state to recover.
/// @return One of the stomp::parse_state_e values indicating the current
/// global parser state.
CMN_PUBLIC int32_t parse_state_recover(stomp::parse_state_t *state);

/// Checks the state of a STOMP message parser to determine whether it is
/// currently in an error state.
///
/// @param state The parser state to inspect.
/// @return true if the parser is currently in the error state.
CMN_PUBLIC bool parse_state_error(stomp::parse_state_t *state);

/// Checks the state of a STOMP message parser to ensure that it is valid, that
/// is, that it has a valid message buffer set.
///
/// @param state The parser state to inspect.
/// @return true if the current parser state is valid.
CMN_PUBLIC bool parse_state_valid(stomp::parse_state_t *state);

/// Checkes the state of a STOMP message parser to determine whether it is
/// waiting for more input.
///
/// @param state The parser state to inspect.
/// @return true if the current parser state is ready to receive more input.
CMN_PUBLIC bool parse_state_ready(stomp::parse_state_t *state);

/// Updates a STOMP message parser with one or more units of input data. This
/// is the primary parser entry point from the application perspective.
///
/// @param state The parser state to update.
/// @param rx_buffer A pointer to the application-managed buffer containing the
/// input data.
/// @param rx_buffer_count The number of bytes of valid data in @a rx_buffer.
/// @param rx_buffer_offset The offset into @a rx_buffer at which to begin
/// reading message data.
/// @param amount_consumed On return, this location is updated with the number
/// of bytes consumed in @a rx_buffer, starting at @a rx_buffer_offset. This
/// value may be less than @rx_buffer_count, in which case either an error
/// occurred, or a complete message was parsed.
/// @return One of the stomp::parse_state_e values indicating the current
/// global parser state. If this value is stomp::PARSE_STATE_ERROR, an error
/// occurred. If this value is stomp::PARSE_STATE_MESSAGE_COMPLETE, a complete
/// message is available in the message_buffer of @a state. In either case,
/// call the stomp::parse_state_reset() function to reset the parser state for
/// the next message. If this value is stomp::PARSE_STATE_NEED_MORE, the caller
/// must provide more input, and an incomplete message is in the buffer.
CMN_PUBLIC int32_t parse_state_update(
    stomp::parse_state_t *state,
    void const           *rx_buffer,
    size_t                rx_buffer_count,
    size_t                rx_buffer_offset,
    size_t               *amount_consumed);

/// A convenience function to populate a stomp::message_t based on the current
/// parser state when stomp::parse_state_update() returns the indicator value
/// stomp::PARSE_STATE_MESSAGE_COMPLETE.
///
/// @param state The parser state to query.
/// @param out_message A pointer to the stomp message structure to populate.
/// The structure will point into data in @a state. Do not call any functions
/// that update the parser state while this data is needed.
/// @return true if a complete message was available and returned in the
/// @a out_message structure, or false if no complete message is available.
CMN_PUBLIC bool get_message(
    stomp::parse_state_t *state,
    stomp::message_t     *out_message);

/// Initializes a new STOMP message writer instance.
///
/// @param state A pointer to the message writer state object to initialize.
/// @param buffer A pointer to the application-managed message composition
/// buffer. The writer appends message data into this buffer. This buffer
/// should be large enough to hold the largest message the application
/// expects to transmit.
/// @param buffer_size The size of @a buffer, in bytes, indicating the maximum
/// amount of data that can be written to @a buffer for a single message.
CMN_PUBLIC void write_state_init(
    stomp::write_state_t *state,
    void                 *buffer,
    size_t                buffer_size);

/// Checks the state of a STOMP message writer to determine whether it is
/// currently in an error state.
///
/// @param state The writer state to inspect.
/// @return true if the writer is currently in the error state.
CMN_PUBLIC bool write_state_error(stomp::write_state_t *state);

/// Checks the state of a STOMP message writer to ensure that it is valid, that
/// is, that it has a valid message buffer set.
///
/// @param state The writer state to inspect.
/// @return true if the current writer state is valid.
CMN_PUBLIC bool write_state_valid(stomp::write_state_t *state);

/// Resets the state of a STOMP message writer after generating a message.
///
/// @param state The writer state to reset.
/// @return One of stomp::write_state_e indicating the current writer state.
CMN_PUBLIC int32_t write_state_reset(stomp::write_state_t *state);

/// Flushes the write buffer without resetting the frame state. The global
/// writer state is reset to stomp::WRITE_STATE_NEED_MORE. Copy the data out of
/// the buffer before calling stomp::write_state_reset().
///
/// @param state The writer state to flush.
/// @param out_buffer On return, this value is updated to point to the start of
/// the writer's internal message buffer.
/// @param out_buffer_size On return, this value is updated with the number of
/// bytes in the writer's internal message buffer.
/// @return One of the stomp::write_state_e values indicating the current
/// writer state. Typically this will be stomp::WRITE_STATE_NEED_MORE.
CMN_PUBLIC int32_t write_state_flush(
    stomp::write_state_t  *state,
    void                 **out_buffer,
    size_t                *out_buffer_size);

/// Begins a new STOMP frame within a message buffer.
///
/// @param state The message writer state used to store the composed message.
/// @param command A NULL-terminated, UTF-8 encoded string specifying the
/// command identifier (typically one of the stomp::FRAME_* values).
/// @return The current writer state value, one of stomp::write_state_e.
CMN_PUBLIC int32_t write_begin_frame(
    stomp::write_state_t *state,
    char const           *command);

/// Adds a pre-formatted header field to a STOMP frame. The header field name
/// and header value will be escaped during serialization.
///
/// @param state The message writer state used to store the composed message.
/// @param header_field A NULL-terminated, UTF-8 encoded string specifying the
/// header field name. This value should not be escaped; escape sequences will
/// be generated during serialization.
/// @param header_value A NULL-terminated, UTF-8 encoded string specifying the
/// formatted header field value. This value should not be escaped; escape
/// sequences will be generated during serialization.
/// @return The current writer state value, one of stomp::write_state_e.
CMN_PUBLIC int32_t write_header_direct(
    stomp::write_state_t *state,
    char const           *header_field,
    char const           *header_value);

/// Formats and adds a header field to a STOMP frame. The header field name and
/// header value will be escaped during serialization.
///
/// @param state The message writer state used to store the composed message.
/// @param header_field A NULL-terminated, UTF-8 encoded string specifying the
/// @param value_format Pointer to a NULL-terminated string following the
/// format specification for the standard C library printf function.
/// @param varargs Substitution arguments for @a value_format.
/// @return The current writer state value, one of stomp::write_state_e.
CMN_PUBLIC int32_t write_header_format(
    stomp::write_state_t *state,
    char const           *header_field,
    char const           *value_format, ...);

/// Writes the end-of-headers (a blank line) marker to a STOMP frame,
/// indicating that no more header fields will be written.
///
/// @param state The message writer state used to store the composed message.
/// @return The current writer state value, one of stomp::write_state_e.
CMN_PUBLIC int32_t write_header_end(stomp::write_state_t *state);

/// Writes some data to the content portion of a STOMP frame. This function may
/// be called repeatedly to write data in multiple chunks.
///
/// @param state The message writer state used to store the composed message.
/// @param body_data Pointer to a buffer containing the body data to write.
/// This data may be either text or binary data. It is written to the message
/// without any interpretation or translation.
/// @param body_size The number of bytes to copy from @a body_data, starting at
/// @a body_offset.
/// @param body_offset The byte offset into @a body_data at which to begin
/// reading data to be copied to the message.
/// @param out_written On return, this value is updated with the number of
/// bytes copied to the message.
/// @return The current writer state value, one of stomp::write_state_e.
CMN_PUBLIC int32_t write_body_data(
    stomp::write_state_t *state,
    void const           *body_data,
    size_t                body_size,
    size_t                body_offset,
    size_t               *out_written);

/// Closes a STOMP frame within a message buffer by writing the terminating
/// null-byte.
///
/// @param state The message writer state used to store the composed message.
/// @return The current writer state value, one of stomp_write_state_e.
/// Typically this value is stomp::WRITE_STATE_FRAME_COMPLETE.
CMN_PUBLIC int32_t close_frame(stomp::write_state_t *state);

/// Implements the logic and buffer management for streaming a STOMP message
/// frame so that the application need only supply a function to write the
/// serialized data to its endpoint.
///
/// @param stomp_frame Data describing the STOMP message frame to be written.
/// @param flush_func A callback function to which serialized data will be
/// send to be flushed to an application-defined endpoint. If this value is
/// NULL, the operation becomes a no-op.
/// @param error_func A callback function to invoke when an error is
/// encountered during serialization. If this value is NULL, the operation
/// becomes a no-op.
/// @param context Optional opaque, application-defined data to be passed back
/// to the @a flush_func and @a error_func callbacks.
/// @return true if the frame was serialized completely without any errors.
CMN_PUBLIC bool stream_frame(
    stomp::message_t       *stomp_frame,
    stomp::writer_flush_fn  flush_func,
    stomp::writer_error_fn  error_func,
    void                   *context);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace stomp */

#endif /* LIBSTOMP_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
