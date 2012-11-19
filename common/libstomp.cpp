/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements an interface to a parser and formatter for STOMP
/// protocol 1.0 and 1.1 frames. The STOMP protocol is typically used to
/// communicate with a message-oriented middleware such as Apache Apollo.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "libstomp.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

char const *STOMP_ERROR_STR_NONE         = "No error";
char const *STOMP_ERROR_STR_UNKNOWN      = "Unknown error";
char const *STOMP_ERROR_STR_BADSTATE     = "Invalid state";
char const *STOMP_ERROR_STR_BADBYTE      = "Unexpected byte in stream";
char const *STOMP_ERROR_STR_BADCMDBYTE   = "Unexpected byte in command";
char const *STOMP_ERROR_STR_BADHDRBYTE   = "Unexpected byte in header";
char const *STOMP_ERROR_STR_BADESCAPE    = "Unrecognized escape sequence";
char const *STOMP_ERROR_STR_NOCOMMAND    = "No command string specified";
char const *STOMP_ERROR_STR_BUFFERSPACE  = "Not enough space in buffer";
char const *STOMP_ERROR_STR_NOHDRFIELD   = "No header field name specified";

/*/////////////////////////////////////////////////////////////////////////80*/

char const *stomp::FRAME_STOMP           = "STOMP";
char const *stomp::FRAME_CONNECT         = "CONNECT";
char const *stomp::FRAME_CONNECTED       = "CONNECTED";
char const *stomp::FRAME_SEND            = "SEND";
char const *stomp::FRAME_SUBSCRIBE       = "SUBSCRIBE";
char const *stomp::FRAME_UNSUBSCRIBE     = "UNSUBSCRIBE";
char const *stomp::FRAME_ACK             = "ACK";
char const *stomp::FRAME_NACK            = "NACK";
char const *stomp::FRAME_BEGIN           = "BEGIN";
char const *stomp::FRAME_COMMIT          = "COMMIT";
char const *stomp::FRAME_ABORT           = "ABORT";
char const *stomp::FRAME_DISCONNECT      = "DISCONNECT";
char const *stomp::FRAME_MESSAGE         = "MESSAGE";
char const *stomp::FRAME_RECEIPT         = "RECEIPT";
char const *stomp::FRAME_ERROR           = "ERROR";

/*/////////////////////////////////////////////////////////////////////////80*/

char const *stomp::HEADER_ACCEPT_VERSION = "accept-version";
char const *stomp::HEADER_HOST           = "host";
char const *stomp::HEADER_LOGIN          = "login";
char const *stomp::HEADER_PASSCODE       = "passcode";
char const *stomp::HEADER_VERSION        = "version";
char const *stomp::HEADER_SESSION        = "session";
char const *stomp::HEADER_SERVER         = "server";
char const *stomp::HEADER_DESTINATION    = "destination";
char const *stomp::HEADER_CONTENT_TYPE   = "content-type";
char const *stomp::HEADER_CONTENT_LENGTH = "content-length";
char const *stomp::HEADER_ID             = "id";
char const *stomp::HEADER_ACK            = "ack";
char const *stomp::HEADER_HEARTBEAT      = "heart-beat";
char const *stomp::HEADER_MESSAGE        = "message";
char const *stomp::HEADER_MESSAGE_ID     = "message-id";
char const *stomp::HEADER_SUBSCRIPTION   = "subscription";
char const *stomp::HEADER_TRANSACTION    = "transaction";
char const *stomp::HEADER_RECEIPT        = "receipt";
char const *stomp::HEADER_RECEIPT_ID     = "receipt-id";

/*/////////////////////////////////////////////////////////////////////////80*/

static bool is_digit(uint8_t curr_byte)
{
    return (curr_byte >= '0' && curr_byte <= '9');
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool is_upper(uint8_t curr_byte)
{
    return (curr_byte >= 'A' && curr_byte <= 'Z') ? 1 : 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool is_whitespace(uint8_t curr_byte)
{
    return ((curr_byte == '\n') || (curr_byte == ' ')  ||
            (curr_byte == '\r') || (curr_byte == '\t') ||
            (curr_byte == '\f') || (curr_byte == '\v'));
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool has_body(char const *command)
{
    // only SEND, MESSAGE and ERROR can have content.
    if (!strcmp(command, stomp::FRAME_SEND))    return 1;
    if (!strcmp(command, stomp::FRAME_ERROR))   return 1;
    if (!strcmp(command, stomp::FRAME_MESSAGE)) return 1;
    return 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static size_t header_pair_size(stomp::header_t const *h, size_t index)
{
    char const *key      = h->header_fields[index];
    char const *val      = h->header_values[index];
    size_t      key_size = stomp::escaped_size(key);
    size_t      val_size = stomp::escaped_size(val);
    // @note: two bytes added for ':' and '\n'.
    return key_size + val_size + 2;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool parse_common_headers(stomp::header_t *h)
{
    bool     has_type   = false;
    bool     has_length = false;
    size_t   type_idx   = 0;
    size_t   length_idx = 0;
    uint64_t length_val = 0;

    has_type   = stomp::find_header(h, stomp::HEADER_CONTENT_TYPE,   &type_idx);
    has_length = stomp::find_header(h, stomp::HEADER_CONTENT_LENGTH, &length_idx);
    if (has_type)
    {
        char *ct_iter   = h->header_values[type_idx];
        h->content_type = ct_iter; // MIME content type
        while (*ct_iter)
        {
            if (';' == *ct_iter)
            {
                // they've also specified a charset.
                *ct_iter++         = '\0';
                h->content_charset = ct_iter;
                break;
            }
            ++ct_iter;
        }
    }
    if (has_length)
    {
        if (stomp::atoiu(h->header_values[length_idx], &length_val))
        {
            has_length        = 1;
            h->content_length = (size_t) length_val;
        }
        else has_length = 0;
    }
    return has_length;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t stomp_parse_error(
    stomp::parse_state_t *state,
    char const           *error)
{
    state->parse_state_global = stomp::PARSE_STATE_ERROR;
    state->error_description  =(error!=NULL) ? error : STOMP_ERROR_STR_UNKNOWN;
    return stomp::PARSE_STATE_ERROR;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t stomp_write_error(
    stomp::write_state_t *state,
    char const           *error)
{
    state->global_state      = stomp::WRITE_STATE_ERROR;
    state->error_description =(error != NULL) ? error : STOMP_ERROR_STR_UNKNOWN;
    return stomp::WRITE_STATE_ERROR;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t parse_state_update_unit_new_frame(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    // we're looking for the start of a new frame.
    // each STOMP frame begins with command, which
    // consists of only uppercase characters 'A'-'Z'.
    CMN_UNUSED(end);
    if (is_upper(*iter))
    {
        // we've found the start of a command.
        // consume and write it to the output.
        state->message_header.command = (char*) msg_buffer;
        state->parse_state_frame      = stomp::FRAME_PARSE_STATE_FRAME_HEAD;
        state->parse_state_header     = stomp::HEAD_PARSE_STATE_COMMAND;
        *msg_buffer++                 = *iter;
        *amount_output                = 1;
        *amount_consumed              = 1;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else if (is_whitespace(*iter))
    {
        // a blank line appears before the command.
        // this is a common occurrence with telnet.
        *amount_output   = 0;
        *amount_consumed = 1;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else
    {
        // unexpected byte in input. eat the byte.
        *amount_output   = 0;
        *amount_consumed = 1;
        return stomp_parse_error(state, STOMP_ERROR_STR_BADBYTE);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t head_parse_state_command(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    // we're adding bytes to the command.
    // commands are all uppercase characters 'A'-'Z'.
    // commands are terminated with a newline.
    CMN_UNUSED(end);
    if (is_upper(*iter))
    {
        // this byte is a part of the command.
        *msg_buffer++    = *iter;
        *amount_output   = 1;
        *amount_consumed = 1;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else if (*iter == '\n')
    {
        // this is the end of the command.
        *msg_buffer      = '\0';
        *amount_output   = 1;
        *amount_consumed = 1;
        state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_START;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    // this byte is unexpected. error.
    *amount_output   = 0;
    *amount_consumed = 0;
    return stomp_parse_error(state, STOMP_ERROR_STR_BADCMDBYTE);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t head_parse_state_key_start(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    CMN_UNUSED(end);
    if (*iter == '\n')
    {
        // this is the end of the header block, a blank line.
        // don't output anything to the message buffer.
        *amount_output   = 0;
        *amount_consumed = 1;
        state->parse_state_frame = stomp::FRAME_PARSE_STATE_FRAME_BODY;
        state->parse_state_body  = stomp::BODY_PARSE_STATE_DATA_START;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else
    {
        // this is the first byte of the key.
        size_t i         = state->message_header.header_count;
        state->message_header.header_fields[i] = (char*) msg_buffer;
        *msg_buffer      = *iter;
        *amount_output   = 1;
        *amount_consumed = 1;
        state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_DATA;
        return stomp::PARSE_STATE_NEED_MORE;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t head_parse_state_key_data(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    CMN_UNUSED(end);
    if (*iter == ':')
    {
        // the separator between key and value.
        *msg_buffer      = '\0';
        *amount_output   = 1;
        *amount_consumed = 1;
        state->parse_state_header = stomp::HEAD_PARSE_STATE_VALUE_START;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else if (*iter == '\\')
    {
        // an escape sequence. translate it.
        if (*(iter + 1) == 'n')
        {
            *msg_buffer      = '\n';
            *amount_output   = 1;
            *amount_consumed = 2;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        if (*(iter + 1) == 'c')
        {
            *msg_buffer      = ':';
            *amount_output   = 1;
            *amount_consumed = 2;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        if (*(iter + 1) == '\\')
        {
            *msg_buffer      = '\\';
            *amount_output   = 1;
            *amount_consumed = 2;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        // invalid escape sequence. error.
        *amount_output   = 0;
        *amount_consumed = 0;
        return stomp_parse_error(state, STOMP_ERROR_STR_BADESCAPE);
    }
    else
    {
        // this is a normal byte in the key.
        *msg_buffer      = *iter;
        *amount_output   = 1;
        *amount_consumed = 1;
        return stomp::PARSE_STATE_NEED_MORE;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t head_parse_state_value_start(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    CMN_UNUSED(end);
    if (*iter == '\\')
    {
        // an escape sequence. translate it.
        if (*(iter + 1) == 'n')
        {
            size_t i         = state->message_header.header_count;
            state->message_header.header_values[i] = (char*) msg_buffer;
            *msg_buffer      = '\n';
            *amount_output   = 1;
            *amount_consumed = 2;
            state->parse_state_header = stomp::HEAD_PARSE_STATE_VALUE_DATA;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        if (*(iter + 1) == 'c')
        {
            size_t i         = state->message_header.header_count;
            state->message_header.header_values[i] = (char*) msg_buffer;
            *msg_buffer      = ':';
            *amount_output   = 1;
            *amount_consumed = 2;
            state->parse_state_header = stomp::HEAD_PARSE_STATE_VALUE_DATA;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        if (*(iter + 1) == '\\')
        {
            size_t i         = state->message_header.header_count;
            state->message_header.header_values[i] = (char*) msg_buffer;
            *msg_buffer      = '\\';
            *amount_output   = 1;
            *amount_consumed = 2;
            state->parse_state_header = stomp::HEAD_PARSE_STATE_VALUE_DATA;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        // invalid escape sequence. error.
        *amount_output   = 0;
        *amount_consumed = 0;
        return stomp_parse_error(state, STOMP_ERROR_STR_BADESCAPE);
    }
    else if (*iter == '\n')
    {
        // no value was specified, so output an empty value
        // and we will now be expecting another key, or terminator.
        size_t i         = state->message_header.header_count++;
        state->message_header.header_values[i] = (char*) msg_buffer;
        *msg_buffer      = '\0';
        *amount_output   = 1;
        *amount_consumed = 1;
        state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_START;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else
    {
        // this is the first byte of the value.
        size_t i         = state->message_header.header_count;
        state->message_header.header_values[i] = (char*) msg_buffer;
        *msg_buffer      = *iter;
        *amount_output   = 1;
        *amount_consumed = 1;
        state->parse_state_header = stomp::HEAD_PARSE_STATE_VALUE_DATA;
        return stomp::PARSE_STATE_NEED_MORE;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t head_parse_state_value_data(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    CMN_UNUSED(end);
    if (*iter == '\\')
    {
        // an escape sequence. translate it.
        if (*(iter + 1) == 'n')
        {
            *msg_buffer      = '\n';
            *amount_output   = 1;
            *amount_consumed = 2;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        if (*(iter + 1) == 'c')
        {
            *msg_buffer      = ':';
            *amount_output   = 1;
            *amount_consumed = 2;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        if (*(iter + 1) == '\\')
        {
            *msg_buffer      = '\\';
            *amount_output   = 1;
            *amount_consumed = 2;
            return stomp::PARSE_STATE_NEED_MORE;
        }
        // invalid escape sequence. error.
        *amount_output   = 0;
        *amount_consumed = 0;
        return stomp_parse_error(state, STOMP_ERROR_STR_BADESCAPE);
    }
    else if (*iter == '\n')
    {
        // this is the end of the current key-value pair.
        // we now expect another key-value pair, or a terminator.
        state->message_header.header_count++;
        *msg_buffer          = '\0';
        *amount_output       = 1;
        *amount_consumed     = 1;
        state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_START;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else
    {
        // this is a single byte of the value.
        *msg_buffer      = *iter;
        *amount_output   = 1;
        *amount_consumed = 1;
        return stomp::PARSE_STATE_NEED_MORE;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t parse_state_update_unit_frame_head(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    // we're in the middle of parsing the header data.
    // we could be still reading the command, or
    // we could be reading a header, or
    // we could be looking for the end-of-header marker.
    switch (state->parse_state_header)
    {
        case stomp::HEAD_PARSE_STATE_COMMAND:
            return head_parse_state_command(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::HEAD_PARSE_STATE_KEY_START:
            return head_parse_state_key_start(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::HEAD_PARSE_STATE_KEY_DATA:
            return head_parse_state_key_data(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::HEAD_PARSE_STATE_VALUE_START:
            return head_parse_state_value_start(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::HEAD_PARSE_STATE_VALUE_DATA:
            return head_parse_state_value_data(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        default:
            {
                // we should never get here. if we do,
                // there's something seriously wrong.
                *amount_output   = 0;
                *amount_consumed = 0;
            }
            return stomp_parse_error(state, STOMP_ERROR_STR_BADSTATE);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t body_parse_state_data_start(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    CMN_UNUSED(end);
    if (has_body(state->message_header.command))
    {
        // this message type could have some content.
        // parse the content-type and content-length
        // headers, if they are present, for data.
        parse_common_headers(&state->message_header);
        if (*iter == '\0'  &&  0 == state->message_header.content_length)
        {
            // this completes the message.
            state->message_body_head = msg_buffer;
            state->message_body_tail = msg_buffer;
            *msg_buffer      = '\0';
            *amount_output   = 1;
            *amount_consumed = 1;
            return stomp::PARSE_STATE_MESSAGE_COMPLETE;
        }
        else
        {
            // output a single byte of body content.
            // there should be additional bytes of data.
            state->message_body_head = msg_buffer;
            state->message_body_tail = msg_buffer + 1;
            *msg_buffer      = *iter;
            *amount_output   = 1;
            *amount_consumed = 1;
            state->parse_state_body = stomp::BODY_PARSE_STATE_DATA;
            return stomp::PARSE_STATE_NEED_MORE;
        }
    }
    else
    {
        // this message type cannot have any content.
        // we are expecting iter to point to a null.
        if (*iter == '\0')
        {
            // this completes the message.
            state->message_body_head = msg_buffer;
            state->message_body_tail = msg_buffer;
            *msg_buffer      = '\0';
            *amount_output   = 1;
            *amount_consumed = 1;
            return stomp::PARSE_STATE_MESSAGE_COMPLETE;
        }
        // this is an unexpected byte not valid in this state.
        *amount_output   = 0;
        *amount_consumed = 0;
        return stomp_parse_error(state, STOMP_ERROR_STR_BADBYTE);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t body_parse_state_data_variable(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    CMN_UNUSED(end);
    // we're adding another byte to the message body.
    state->message_body_tail++;
    *msg_buffer      = *iter;
    *amount_output   = 1;
    *amount_consumed = 1;
    if (*iter == '\0')
    {
        // this completes the message.
        // compute the content length.
        // content length does not include the null byte.
        size_t tail = (size_t)(state->message_body_tail - 1);
        size_t head = (size_t)(state->message_body_head);
        state->message_body_tail--; // don't include null.
        state->message_header.content_length = tail - head;
        return stomp::PARSE_STATE_MESSAGE_COMPLETE;
    }
    else return stomp::PARSE_STATE_NEED_MORE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t body_parse_state_data_fixed(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    size_t tail          = (size_t) state->message_body_tail;
    size_t head          = (size_t) state->message_body_head;
    size_t expect_length = state->message_header.content_length;
    size_t actual_length = tail - head;

    CMN_UNUSED(end);

    if (actual_length   != expect_length)
    {
        // we still have data to read.
        state->message_body_tail++;
        *msg_buffer      = *iter;
        *amount_output   = 1;
        *amount_consumed = 1;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else
    {
        // we're expecting a null byte to terminate the frame.
        if (*iter == '\0')
        {
            *msg_buffer      = *iter;
            *amount_output   = 1;
            *amount_consumed = 1;
            return stomp::PARSE_STATE_MESSAGE_COMPLETE;
        }
        // this is an unexpected byte in the body.
        *amount_output   = 0;
        *amount_consumed = 0;
        return stomp_parse_error(state, STOMP_ERROR_STR_BADBYTE);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t parse_state_update_unit_frame_body(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    // we're about to parse the message body.
    // per the spec, only SEND, MESSAGE and ERROR frames
    // may have body data. everything else should just have a NULL byte.
    switch (state->parse_state_body)
    {
        case stomp::BODY_PARSE_STATE_DATA_START:
            return body_parse_state_data_start(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::BODY_PARSE_STATE_DATA:
            {
                if (state->message_header.content_length > 0)
                {
                    // this is a fixed-size message body.
                    return body_parse_state_data_fixed(
                        state,
                        iter, end,
                        msg_buffer,
                        amount_output,
                        amount_consumed);
                }
            }
            // this is a variable-length message body.
            return body_parse_state_data_variable(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        default:
            {
                // we should never get here. if we do,
                // there's something seriously wrong.
                *amount_output   = 0;
                *amount_consumed = 0;
            }
            return stomp_parse_error(state, STOMP_ERROR_STR_BADSTATE);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t parse_state_sync_head(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    // we were in the middle of parsing a header and encountered some error.
    // we're looking for either a blank line, or a null byte. if we encounter
    // a blank line, we've hit the end of the header block, and we go to the
    // sync_body state; if we hit a null then we've found the end of a body.
    // we hijack the parse_state_header to track our precise state.
    CMN_UNUSED(end);
    CMN_UNUSED(msg_buffer);
    if (*iter == '\0')
    {
        // we hit the end of a frame, so reset.
        state->parse_state_global = stomp::PARSE_STATE_NEED_MORE;
        state->parse_state_frame  = stomp::FRAME_PARSE_STATE_NEW_FRAME;
        state->parse_state_header = stomp::HEAD_PARSE_STATE_COMMAND;
        state->parse_state_body   = stomp::BODY_PARSE_STATE_DATA_START;
        return stomp::PARSE_STATE_NEED_MORE;
    }
    // we're still in the middle of the frame.
    // keep trying to parse it.
    // no data gets output to the message buffer.
    switch (state->parse_state_header)
    {
        case stomp::HEAD_PARSE_STATE_COMMAND:
            {
                // we're adding bytes to the command.
                // commands are all uppercase characters 'A'-'Z'.
                // commands are terminated with a newline.
                if (*iter == '\n')
                {
                    // this is the end of the command.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_START;
                }
                else
                {
                    // some byte that's part of the command.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                }
            }
            return stomp::PARSE_STATE_NEED_MORE;

        case stomp::HEAD_PARSE_STATE_KEY_START:
            {
                if (*iter == '\n')
                {
                    // this is the end of the header block, a blank line.
                    // don't output anything to the message buffer.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    state->parse_state_frame = stomp::FRAME_PARSE_STATE_SYNC_BODY;
                    state->parse_state_body  = stomp::BODY_PARSE_STATE_DATA_START;
                }
                else
                {
                    // this is the first byte of the key.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_DATA;
                }
            }
            return stomp::PARSE_STATE_NEED_MORE;

        case stomp::HEAD_PARSE_STATE_KEY_DATA:
            {
                if (*iter == ':')
                {
                    // the separator between key and value.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    state->parse_state_header = stomp::HEAD_PARSE_STATE_VALUE_START;
                }
                else
                {
                    // this is some byte that's part of the key.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                }
            }
            return stomp::PARSE_STATE_NEED_MORE;

        case stomp::HEAD_PARSE_STATE_VALUE_START:
            {
                if (*iter == '\n')
                {
                    // no value was specified. we will now be
                    // expecting another key, or terminator.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_START;
                }
                else
                {
                    // some byte that's part of the value.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                }
            }
            return stomp::PARSE_STATE_NEED_MORE;

        case stomp::HEAD_PARSE_STATE_VALUE_DATA:
            {
                if (*iter == '\n')
                {
                    // hit the end of the value. expect
                    // another key, or a terminator.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    state->parse_state_header = stomp::HEAD_PARSE_STATE_KEY_START;
                }
                else
                {
                    // some byte that's part of the value.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                }
            }
            return stomp::PARSE_STATE_NEED_MORE;

        default:
            {
                // some invalid state. should never get here.
                *amount_output   = 0;
                *amount_consumed = 1;
            }
            return stomp::PARSE_STATE_NEED_MORE;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t parse_state_sync_body(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    // we either come here after syncing from the header state,
    // or we didn't find a null byte where we expected it at
    // the end of the frame.
    CMN_UNUSED(end);
    CMN_UNUSED(msg_buffer);
    switch (state->parse_state_body)
    {
        case stomp::BODY_PARSE_STATE_DATA_START:
            {
                // we came here from the header state. we should have
                // headers, so try parsing them to get a content length.
                if (has_body(state->message_header.command))
                {
                    parse_common_headers(&state->message_header);
                }
                if (*iter == '\0' && 0 == state->message_header.content_length)
                {
                    // we hit the end of the frame, so we're good.
                    stomp::parse_state_reset(state);
                    *amount_output   = 0;
                    *amount_consumed = 1;
                }
                else
                {
                    // still looking for a null byte, or we
                    // expect more data in the message body.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    if (state->message_header.content_length > 0)
                    {
                        // decrement the expected number of bytes remaining.
                        state->message_header.content_length--;
                    }
                    state->parse_state_body = stomp::BODY_PARSE_STATE_DATA;
                }
            }
            return stomp::PARSE_STATE_NEED_MORE;

        case stomp::BODY_PARSE_STATE_DATA:
            {
                // we died in the middle of the body.
                if (*iter == '\0' && 0 == state->message_header.content_length)
                {
                    // we hit the end of the frame, so we're good.
                    stomp::parse_state_reset(state);
                    *amount_output   = 0;
                    *amount_consumed = 1;
                }
                else
                {
                    // still looking for a null byte, or we
                    // expect more data in the message body.
                    *amount_output   = 0;
                    *amount_consumed = 1;
                    if (state->message_header.content_length > 0)
                    {
                        // decrement the expected number of bytes remaining.
                        state->message_header.content_length--;
                    }
                }
            }
            return stomp::PARSE_STATE_NEED_MORE;

        default:
            {
                // invalid state; just eat bytes.
                if (*iter == '\0')
                {
                    // we hit the end of the frame.
                    stomp::parse_state_reset(state);
                }
                *amount_output   = 0;
                *amount_consumed = 1;
            }
            return stomp::PARSE_STATE_NEED_MORE;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int32_t parse_state_update_unit(
    stomp::parse_state_t *state,
    uint8_t const        *iter,
    uint8_t const        *end,
    uint8_t              *msg_buffer,
    size_t               *amount_output,
    size_t               *amount_consumed)
{
    switch (state->parse_state_frame)
    {
        case stomp::FRAME_PARSE_STATE_NEW_FRAME:
            return parse_state_update_unit_new_frame(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::FRAME_PARSE_STATE_FRAME_HEAD:
            return parse_state_update_unit_frame_head(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::FRAME_PARSE_STATE_FRAME_BODY:
            return parse_state_update_unit_frame_body(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::FRAME_PARSE_STATE_SYNC_HEAD:
            return parse_state_sync_head(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        case stomp::FRAME_PARSE_STATE_SYNC_BODY:
            return parse_state_sync_body(
                state,
                iter, end,
                msg_buffer,
                amount_output,
                amount_consumed);

        default:
            {
                // we should never get here. if we do,
                // there's something seriously wrong.
                *amount_output   = 0;
                *amount_consumed = 0;
            }
            return stomp_parse_error(state, STOMP_ERROR_STR_BADSTATE);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void CMN_CALL_C default_stream_error_func(
    stomp::write_state_t *writer,
    stomp::message_t     *frame,
    void                 *context)
{
    CMN_UNUSED(writer);
    CMN_UNUSED(frame);
    CMN_UNUSED(context);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool CMN_CALL_C default_stream_flush_func(
    stomp::write_state_t *writer,
    void                 *buffer,
    size_t                buffer_size,
    void                 *context)
{
    CMN_UNUSED(writer);
    CMN_UNUSED(buffer);
    CMN_UNUSED(buffer_size);
    CMN_UNUSED(context);
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool stream_command(
    stomp::write_state_t   *writer,
    stomp::message_t       *frame,
    stomp::writer_flush_fn  flush_func,
    stomp::writer_error_fn  error_func,
    void                   *context)
{
    stomp::header_t *header = frame->head;
    bool             result = true;

    switch (stomp::write_begin_frame(writer, header->command))
    {
        case stomp::WRITE_STATE_ERROR:
            {
                error_func(writer, frame, context);
                stomp::write_state_reset(writer);
                result = false;
            }
            break;

        case stomp::WRITE_STATE_FLUSH:
            {
                void  *out_buf  = NULL;
                size_t out_size = 0;
                stomp::write_state_flush(writer, &out_buf, &out_size);
                result = flush_func(writer, out_buf, out_size, context);
                // retry writing the command. stomp::write_begin_frame()
                // doesn't write anything unless the entire write fits.
                stomp::write_begin_frame(writer, header->command);
            }
            break;

        default: break;
    }
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool stream_headers(
    stomp::write_state_t   *writer,
    stomp::message_t       *frame,
    stomp::writer_flush_fn  flush_func,
    stomp::writer_error_fn  error_func,
    void                   *context)
{
    stomp::header_t *header = frame->head;
    char           **hk     = frame->head->header_fields;
    char           **hv     = frame->head->header_values;
    size_t           index  = 0;
    bool             result = true;

    // serialize the header fields:
    while (index < header->header_count)
    {
        switch (stomp::write_header_direct(writer, hk[index], hv[index]))
        {
            case stomp::WRITE_STATE_ERROR:
                {
                    // in case of error, we write some null bytes.
                    // we want to make sure the frame is terminated.
                    uint8_t nullnull[2] = {0, 0};
                    error_func(writer, frame, context);
                    flush_func(writer, nullnull, 2, context);
                    stomp::write_state_reset(writer);
                    result = false;
                }
                return result;

            case stomp::WRITE_STATE_FLUSH:
                {
                    void  *out_buf  = NULL;
                    size_t out_size = 0;
                    stomp::write_state_flush(writer, &out_buf, &out_size);
                    result = flush_func(writer, out_buf, out_size, context);
                    if (!result) return result;
                }
                // don't increment index; retry this header.
                // stomp::write_header_direct() doesn't write anything
                // unless the entire write will fit in the buffer.
                continue;

            default: break;
        }
        ++index;
    }

    // terminate the header block:
    switch (stomp::write_header_end(writer))
    {
        case stomp::WRITE_STATE_ERROR:
            {
                uint8_t nullnull[2] = {0, 0};
                error_func(writer, frame, context);
                flush_func(writer, nullnull, 2, context);
                stomp::write_state_reset(writer);
                result = false;
            }
            return result;

        case stomp::WRITE_STATE_FLUSH:
            {
                // flush the existing data:
                void  *out_buf  = NULL;
                size_t out_size = 0;
                stomp::write_state_flush(writer, &out_buf, &out_size);
                result = flush_func(writer, out_buf, out_size, context);
                if (!result) return result;
                // retry writing the null byte. stomp::write_header_end()
                // doesn't write anything unless the entire write fits.
                stomp::write_header_end(writer);
            }
            break;

        default: break;
    }
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static bool stream_body(
    stomp::write_state_t   *writer,
    stomp::message_t       *frame,
    stomp::writer_flush_fn  flush_func,
    stomp::writer_error_fn  error_func,
    void                   *context)
{
    uint8_t *body_data = frame->body;
    size_t   body_size = frame->body_size;
    size_t   body_ofs  = 0;         // offset from body_data
    size_t   rem       = body_size; // number of bytes remaining
    bool     res       = false;

    // write the main body data:
    while (body_ofs < body_size)
    {
        size_t  nwr = 0; // number of bytes written
        switch (stomp::write_body_data(writer, body_data, rem, body_ofs, &nwr))
        {
            case stomp::WRITE_STATE_ERROR:
                {
                    // in case of error, we write some null bytes.
                    // we want to make sure the frame is terminated.
                    uint8_t nullbyte[1] = {0};
                    error_func(writer, frame, context);
                    flush_func(writer, nullbyte, 1, context);
                    stomp::write_state_reset(writer);
                    res = false;
                }
                return res;

            case stomp::WRITE_STATE_FLUSH:
                {
                    void  *out_buf  = NULL;
                    size_t out_size = 0;
                    stomp::write_state_flush(writer, &out_buf, &out_size);
                    res = flush_func(writer, out_buf, out_size, context);
                    if (!res) return res;
                }
                break;

            default: break;
        }
        body_ofs += nwr;
        rem      -= nwr;
    }

    // end the frame:
    switch (stomp::close_frame(writer))
    {
        case stomp::WRITE_STATE_ERROR:
            {
                // in case of error, we write some null bytes.
                // we want to make sure the frame is terminated.
                uint8_t nullbyte[1] = {0};
                error_func(writer, frame, context);
                flush_func(writer, nullbyte, 1, context);
                stomp::write_state_reset(writer);
                res = false;
            }
            return res;

        case stomp::WRITE_STATE_FLUSH:
            {
                void  *out_buf  = NULL;
                size_t out_size = 0;
                stomp::write_state_flush(writer, &out_buf, &out_size);
                res = flush_func(writer, out_buf, out_size, context);
                if (!res) return res;
                stomp::close_frame(writer);
                res = flush_func(writer, out_buf, out_size, context);
            }
            break;

        default:
            {
                void  *out_buf  = NULL;
                size_t out_size = 0;
                stomp::write_state_flush(writer, &out_buf, &out_size);
                res = flush_func(writer, out_buf, out_size, context);
            }
            break;
    }
    return res;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void stomp::header_init(stomp::header_t *header)
{
    size_t i                = 0;
    header->command         = NULL;
    header->content_type    = NULL;
    header->content_charset = NULL;
    header->content_length  = 0;
    header->header_count    = 0;
    for (i = 0; i < STOMP_MAX_HEADERS; ++i)
    {
        header->header_fields[i] = NULL;
        header->header_values[i] = NULL;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::find_header(
    stomp::header_t *header_info,
    char const      *header_name,
    size_t          *out_index)
{
    size_t i = 0;
    for   (i = 0; i < header_info->header_count; ++i)
    {
        if (!strcmp(header_info->header_fields[i], header_name))
        {
            if (out_index != NULL) *out_index = i;
            return true;
        }
    }
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t stomp::escaped_size(char const *value)
{
    size_t      size = 0;
    char const *iter = value;
    while (iter && *iter)
    {
        if (*iter == '\n') ++size;
        if (*iter == ':')  ++size;
        if (*iter == '\\') ++size;
        ++size;
        ++iter;
    }
    return size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint8_t* stomp::write_escaped_string(
    uint8_t    *msg_buffer,
    char const *str_value)
{
    uint8_t const *iter = (uint8_t const*) str_value;
    while (iter && *iter)
    {
        if (*iter == '\n')
        {
            *msg_buffer++ = '\\';
            *msg_buffer++ = 'n';
        }
        else if (*iter == ':')
        {
            *msg_buffer++ = '\\';
            *msg_buffer++ = 'c';
        }
        else if (*iter == '\\')
        {
            *msg_buffer++ = '\\';
            *msg_buffer++ = '\\';
        }
        else
        {
            *msg_buffer++ = *iter;
        }
        ++iter;
    }
    return msg_buffer;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t stomp::wire_size(
    stomp::header_t const *header,
    size_t                 body_size)
{
    size_t i            = 0;
    size_t wire_size    = 0;
    size_t headers_size = 0;
    size_t command_size = strlen(header->command)+1; // +1 \n

    // sum the size of each header.
    for (i = 0; i < header->header_count; ++i)
    {
        headers_size   += header_pair_size(header, i);
    }
    // add the frame header sizes.
    wire_size += command_size;
    wire_size += headers_size;
    wire_size += 1; /* '\n' */
    // add the body data size.
    wire_size +=body_size;
    // add one byte for the frame terminator.
    wire_size += 1;
    return wire_size;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::serialize(
    stomp::header_t const *header,
    void const            *body_data,
    size_t                 body_size,
    void                  *buffer,
    size_t                 buffer_size,
    size_t                 buffer_offset,
    size_t                *out_wire_size)
{
    size_t         i          = 0;
    size_t         wire_size  = stomp::wire_size(header, body_size);
    uint8_t       *msg_buffer = ((uint8_t*) buffer) + buffer_offset;
    uint8_t const *content    =  (uint8_t*) body_data;
    uint8_t const *iter       = NULL;

    if (out_wire_size != NULL)
    {
        // store the wire size for the caller.
        *out_wire_size = wire_size;
    }
    if (buffer_size    < wire_size)
    {
        // the buffer is not large enough. fail immediately.
        return false;
    }

    // serialize the command.
    iter = (uint8_t const*) header->command;
    while  (*iter)  *msg_buffer++ = *iter++;
    *msg_buffer++   = '\n';

    // serialize the headers. headers are escaped.
    for (i = 0; i < header->header_count; ++i)
    {
        char const *key = (char const*) header->header_fields[i];
        char const *val = (char const*) header->header_values[i];
         msg_buffer     = stomp::write_escaped_string(msg_buffer, key);
        *msg_buffer++   = ':';
         msg_buffer     = stomp::write_escaped_string(msg_buffer, val);
        *msg_buffer++   = '\n';
    }

    // a single blank line is the delimiter between header and body.
    *msg_buffer++ = '\n';

    // write the body, if any.
    for (i = 0; i < body_size; ++i) *msg_buffer++ = *content++;

    // write a single null byte to terminate the frame.
    *msg_buffer++ = '\0';
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::atof(char const *string_value, double *out_number)
{
    char const *iter     = string_value;
    ptrdiff_t   length   = 1;
    double      sign     = 1.0;
    double      result   = 0.0;
    char        first    = 0;
    int         exp_neg  = 0;
    int         exponent = 0;

    if (NULL == string_value) return false;

    // determine the sign.
    first = *iter;
    if (first == '-')
    {
        sign   = -1.0;
        length = 2;
        iter++;
    }
    if (first == '+')
    {
        sign   = +1.0;
        length = 2;
        iter++;
    }
    // determine the value (integer part.)
    for (; *iter && is_digit(*iter); ++iter)
    {
        result = 10 * result + (*iter - '0');
    }
    // determine the value (fractional part.)
    if (iter != string_value && '.' == *iter)
    {
        double inv_base = 0.1; ++iter;
        for (; *iter && is_digit(*iter); ++iter)
        {
            result   += (*iter - '0') * inv_base;
            inv_base *= 0.1;
        }
    }
    // apply the sign.
    result *= sign;
    // handle the exponent, if any.
    if (iter != string_value && ('e' == *iter || 'E' == *iter))
    {
        ++iter;
        if ('-' == *iter)
        {
            exp_neg = 1;
            iter++;
        }
        else if ('+' == *iter)
        {
            exp_neg = 0;
            iter++;
        }
        for (; *iter && is_digit(*iter); ++iter)
        {
            exponent = 10 * exponent + (*iter - '0');
        }
    }
    if (exponent != 0)
    {
        double power_of_ten = 10;
        for (; exponent > 1; exponent--)
        {
            power_of_ten *= 10;
        }
        if (exp_neg) result /= power_of_ten;
        else         result *= power_of_ten;
    }
    *out_number = result;
    return (iter - string_value) >= length ? true : false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::atois(char const *string_value, int64_t *out_signed)
{
    char const *iter   = string_value;
    char        first  = 0;
    signed      sign   = 1;
    int64_t     length = 1;
    int64_t     result = 0;

    if (NULL  == string_value) return false;

    // determine the sign.
    first      = *iter;
    if (first == '-')
    {
        sign   =-1;
        length = 2;
        iter++;
    }
    if (first == '+')
    {
        sign   =+1;
        length = 2;
        iter++;
    }
    // determine the value.
    for (; *iter && is_digit(*iter); ++iter)
    {
        result = 10 * result + (*iter - '0');
    }
    // store the signed value.
    *out_signed = result * sign;
    return (iter - string_value) >= length ? true : false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::atoiu(char const *string_value, uint64_t *out_unsigned)
{
    char const *iter   = string_value;
    uint64_t    result = 0;

    if (NULL  == string_value) return false;

    // determine the value.
    for (; *iter && is_digit(*iter); ++iter)
    {
        result = 10 * result + (*iter - '0');
    }
    // store the unsigned value.
    *out_unsigned = result;
    return (iter - string_value) >= 1 ? true : false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::atoix(char const *string_value, uint64_t *out_unsigned)
{
    char const *iter   = string_value;
    uint64_t    result = 0;

    if (NULL  == string_value) return false;

    // determine the value.
    for (; *iter; ++iter)
    {
        unsigned int digit = 0;
        if (is_digit(*iter))
        {
            digit = *iter  - '0';
        }
        else if (*iter >= 'a' && *iter <= 'f')
        {
            digit = *iter  - 'a' + 10;
        }
        else if (*iter >= 'A' && *iter <= 'F')
        {
            digit = *iter  - 'A' + 10;
        }
        else break;
        result = 16 * result + digit;
    }
    // store the unsigned value.
    *out_unsigned = result;
    return (iter - string_value) >= 1 ? true : false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::format(
    char const *format_string,
    char       *output_buffer,
    size_t      max_length,
    size_t     *out_length, ...)
{
    int      length;
    va_list  varargs;
    va_start(varargs, out_length);
#ifdef _MSC_VER
    // on Windows, vsnprintf will return -1 if truncation would occur, so
    // we need to use the _vscprintf function first to figure out how many
    // characters would be generated (not including the trailing NULL.)
    // vsnprintf_s will always NULL-terminate the output buffer.
    length = _vscprintf(format_string, varargs);
    vsnprintf_s(output_buffer, max_length, _TRUNCATE, format_string, varargs);
#else
    // on OSX and *nix systems, vsnprintf will return the number of characters
    // that would have been written to the output buffer, even if fewer
    // characters were written because the buffer is not long enough. the
    // vsnprintf function will always NULL-terminate the output buffer.
    length = vsnprintf(output_buffer, max_length, format_string, varargs);
#endif
    if (out_length != NULL)
    {
        // store the output length for the caller.
        *out_length = (size_t) length;
    }
    va_end(varargs);
    return ((size_t) (length + 1) <= max_length) ? true : false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::build_content_type(
    char const *mime_type,
    char const *charset,
    char       *value_buffer,
    size_t      buffer_size,
    size_t     *out_value_size)
{
    assert(mime_type    != NULL);
    assert(value_buffer != NULL);
    assert(buffer_size   > 0);
    if (NULL == charset)
    {
        // mime-type only.
        size_t total_len = 0; // not including null
        if (stomp::format("%s", value_buffer, buffer_size, &total_len, mime_type))
        {
            // everything fit in the output buffer.
            if (out_value_size != NULL) *out_value_size = total_len+1;
            return true;
        }
        else
        {
            // not enough space in the output buffer.
            if (out_value_size != NULL) *out_value_size = buffer_size;
            return false;
        }
    }
    else
    {
        // mime-type + charset.
        size_t      total_len = 0; // not including null
        char const *format    = "%s;charset=%s";
        if (stomp::format(format, value_buffer, buffer_size, &total_len, mime_type, charset))
        {
            // everything fit in the output buffer.
            if (out_value_size != NULL) *out_value_size = total_len+1;
            return true;
        }
        else
        {
            // not enough space in the output buffer.
            if (out_value_size != NULL) *out_value_size = buffer_size;
            return false;
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::build_content_length(
    size_t  body_size,
    char   *value_buffer,
    size_t  buffer_size,
    size_t *out_value_size)
{
    uint64_t content_size  = (uint64_t) body_size;
    size_t   total_len     = 0;
    assert(value_buffer   != NULL);
    assert(buffer_size     > 0);
    if (stomp::format("%"PRIu64, value_buffer, buffer_size, &total_len, content_size))
    {
        // everything fit in the output buffer.
        if (out_value_size != NULL) *out_value_size = total_len+1;
        return true;
    }
    else
    {
        // not enough space in the output buffer.
        if (out_value_size != NULL) *out_value_size = buffer_size;
        return false;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void stomp::parse_state_init(
    stomp::parse_state_t *state,
    void                 *buffer,
    size_t                buffer_size)
{
    assert(state    != NULL);
    assert(buffer   != NULL);
    assert(buffer_size  > 0);
    state->parse_state_global  = stomp::PARSE_STATE_NEED_MORE;
    state->parse_state_frame   = stomp::FRAME_PARSE_STATE_NEW_FRAME;
    state->parse_state_header  = stomp::HEAD_PARSE_STATE_COMMAND;
    state->parse_state_body    = stomp::BODY_PARSE_STATE_DATA_START;
    state->message_buffer      = (uint8_t*) buffer;
    state->message_buffer_size = buffer_size;
    state->message_size        = 0;
    state->message_body_head   = NULL;
    state->message_body_tail   = NULL;
    state->error_description   = STOMP_ERROR_STR_NONE;
    stomp::header_init(&state->message_header);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::parse_state_reset(stomp::parse_state_t *state)
{
    if (state->parse_state_global != stomp::PARSE_STATE_ERROR)
    {
        state->parse_state_global  = stomp::PARSE_STATE_NEED_MORE;
        state->parse_state_frame   = stomp::FRAME_PARSE_STATE_NEW_FRAME;
        state->parse_state_header  = stomp::HEAD_PARSE_STATE_COMMAND;
        state->parse_state_body    = stomp::BODY_PARSE_STATE_DATA_START;
        state->message_size        = 0;
        state->message_body_head   = NULL;
        state->message_body_tail   = NULL;
        state->error_description   = STOMP_ERROR_STR_NONE;
        stomp::header_init(&state->message_header);
        return stomp::PARSE_STATE_NEED_MORE;
    }
    else return stomp::parse_state_recover(state);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::parse_state_recover(stomp::parse_state_t *state)
{
    if (state->parse_state_global == stomp::PARSE_STATE_ERROR)
    {
        // clear out any existing message buffer contents.
        state->message_size        = 0;
        state->message_body_head   = NULL;
        state->message_body_tail   = NULL;
        state->error_description   = STOMP_ERROR_STR_NONE;

        // recover based on our state at the time of the error.
        switch (state->parse_state_frame)
        {
            case stomp::FRAME_PARSE_STATE_NEW_FRAME:
                {
                    // error encountered while looking for a command start.
                    // just keep on looking as if there was no error.
                    state->parse_state_global = stomp::PARSE_STATE_NEED_MORE;
                    state->parse_state_header = stomp::HEAD_PARSE_STATE_COMMAND;
                    state->parse_state_body   = stomp::BODY_PARSE_STATE_DATA_START;
                }
                return stomp::PARSE_STATE_NEED_MORE;

            case stomp::FRAME_PARSE_STATE_FRAME_HEAD:
                {
                    // error encountered while parsing the frame header data.
                    // re-synchronize with the stream from within the header.
                    state->parse_state_global = stomp::PARSE_STATE_NEED_MORE;
                    state->parse_state_frame  = stomp::FRAME_PARSE_STATE_SYNC_HEAD;
                }
                return stomp::PARSE_STATE_NEED_MORE;

            case stomp::FRAME_PARSE_STATE_FRAME_BODY:
                {
                    // error encountered while parsing the frame body data.
                    // re-synchronize with the stream from within the body.
                    state->parse_state_global = stomp::PARSE_STATE_NEED_MORE;
                    state->parse_state_frame  = stomp::FRAME_PARSE_STATE_SYNC_BODY;
                }
                return stomp::PARSE_STATE_NEED_MORE;

            case stomp::FRAME_PARSE_STATE_SYNC_HEAD:
            case stomp::FRAME_PARSE_STATE_SYNC_BODY:
                {
                    // we're already re-synchronizing with the stream.
                    state->parse_state_global = stomp::PARSE_STATE_NEED_MORE;
                }
                return stomp::PARSE_STATE_NEED_MORE;

            default:
                {
                    // invalid frame parse state. just reset.
                    state->parse_state_global = stomp::PARSE_STATE_NEED_MORE;
                    state->parse_state_frame  = stomp::FRAME_PARSE_STATE_NEW_FRAME;
                    state->parse_state_header = stomp::HEAD_PARSE_STATE_COMMAND;
                    state->parse_state_body   = stomp::BODY_PARSE_STATE_DATA_START;
                    state->message_size       = 0;
                    state->message_body_head  = NULL;
                    state->message_body_tail  = NULL;
                    state->error_description  = NULL;
                    stomp::header_init(&state->message_header);
                }
                return stomp::PARSE_STATE_NEED_MORE;
        }
    }
    else return stomp::parse_state_reset(state);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::parse_state_error(stomp::parse_state_t *s)
{
    return (stomp::PARSE_STATE_ERROR == s->parse_state_global);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::parse_state_valid(stomp::parse_state_t *s)
{
    return (s->message_buffer != NULL && s->message_buffer_size > 0);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::parse_state_ready(stomp::parse_state_t *s)
{
    return (stomp::PARSE_STATE_NEED_MORE == s->parse_state_global);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::parse_state_update(
    stomp::parse_state_t *state,
    void const           *rx_buffer,
    size_t                rx_buffer_count,
    size_t                rx_buffer_offset,
    size_t               *amount_consumed)
{
    uint8_t const *end = ((uint8_t const*) rx_buffer) + rx_buffer_count;
    uint8_t const *it  = ((uint8_t const*) rx_buffer) + rx_buffer_offset;
    size_t         num = 0;

    if (stomp::parse_state_valid(state) == 0)
    {
        // call stomp_parse_state_init().
        return stomp_parse_error(state, STOMP_ERROR_STR_BADSTATE);
    }
    if (stomp::parse_state_error(state) != 0)
    {
        // call stomp_parse_state_reset() or stomp_parse_state_recover().
        return stomp::PARSE_STATE_ERROR;
    }
    if (stomp::parse_state_ready(state) == 0)
    {
        // call stomp_parse_state_reset().
        return state->parse_state_global;
    }

    while (it != end)
    {
        // consume input one unit at a time.
        size_t   num_consumed = 0;
        size_t   num_produced = 0;
        uint8_t *msg_buffer   =&state->message_buffer[state->message_size];
        int32_t  new_state    = parse_state_update_unit(
            state,
            it, end,
            msg_buffer,
            &num_produced,
            &num_consumed);
        // update our top-level parser state.
        state->parse_state_global = new_state;
        state->message_size      += num_produced;
        num += num_consumed;
        it  += num_consumed;
        // are we at a stop state?
        if (new_state != stomp::PARSE_STATE_NEED_MORE) break;
    }

    if (amount_consumed != NULL) *amount_consumed = num;
    return state->parse_state_global;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::get_message(
    stomp::parse_state_t *state,
    stomp::message_t     *out_message)
{
    if (stomp::PARSE_STATE_MESSAGE_COMPLETE == state->parse_state_global)
    {
        size_t       tofs      = (size_t) state->message_body_tail;
        size_t       hofs      = (size_t) state->message_body_head;
        out_message->head      =&state->message_header;
        out_message->body      = state->message_body_head;
        out_message->body_size = tofs - hofs;
        return true;
    }
    else
    {
        out_message->head      = NULL;
        out_message->body      = NULL;
        out_message->body_size = 0;
        return false;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void stomp::write_state_init(
    stomp::write_state_t *state,
    void                 *buffer,
    size_t                buffer_size)
{
    assert(state    != NULL);
    assert(buffer   != NULL);
    assert(buffer_size  > 0);
    state->global_state        = stomp::WRITE_STATE_NEED_MORE;
    state->frame_state         = stomp::FRAME_WRITE_STATE_COMMAND;
    state->message_buffer      = (uint8_t*) buffer;
    state->message_buffer_size = buffer_size;
    state->message_size        = 0;
    state->error_description   = STOMP_ERROR_STR_NONE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::write_state_error(stomp::write_state_t *s)
{
    return (stomp::WRITE_STATE_ERROR == s->global_state);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::write_state_valid(stomp::write_state_t *s)
{
    return (s->message_buffer != NULL && s->message_buffer_size > 0);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::write_state_reset(stomp::write_state_t *state)
{
    state->global_state      = stomp::WRITE_STATE_NEED_MORE;
    state->frame_state       = stomp::FRAME_WRITE_STATE_COMMAND;
    state->message_size      = 0;
    state->error_description = STOMP_ERROR_STR_NONE;
    return stomp::WRITE_STATE_NEED_MORE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::write_state_flush(
    stomp::write_state_t  *state,
    void                 **out_buffer,
    size_t                *out_buffer_size)
{
    if (state->global_state != stomp::WRITE_STATE_ERROR)
    {
        if (out_buffer      != NULL) *out_buffer      = state->message_buffer;
        if (out_buffer_size != NULL) *out_buffer_size = state->message_size;
        state->message_size  = 0;
        state->global_state  = stomp::WRITE_STATE_NEED_MORE;
        // @note: don't update the frame state.
        return stomp::WRITE_STATE_NEED_MORE;
    }
    return stomp::WRITE_STATE_ERROR;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::write_begin_frame(
    stomp::write_state_t *state,
    char const           *command)
{
    size_t         length     = 0;
    uint8_t const *iter       = (uint8_t const*) command;
    uint8_t       *msg_buffer = &state->message_buffer[state->message_size];

    if (stomp::WRITE_STATE_NEED_MORE     != state->global_state &&
        stomp::FRAME_WRITE_STATE_COMMAND != state->frame_state)
    {
        // invalid state.
        return stomp_write_error(state, STOMP_ERROR_STR_BADSTATE);
    }
    if (command != NULL)
    {
        // compute the length of the command; +1 for trailing newline.
        length = strlen(command) + 1;
    }
    if (length < 2)
    {
        // an invalid command was specified.
        return stomp_write_error(state, STOMP_ERROR_STR_NOCOMMAND);
    }
    if (state->message_size + length >= state->message_buffer_size)
    {
        // not enough space in the output buffer.
        state->global_state = stomp::WRITE_STATE_FLUSH;
        return stomp::WRITE_STATE_FLUSH;
    }
    // copy the command string; terminate with a newline.
    while (*iter) *msg_buffer++ = *iter++;
    *msg_buffer  = '\n';
    state->message_size += length;
    state->global_state  = stomp::WRITE_STATE_NEED_MORE;
    state->frame_state   = stomp::FRAME_WRITE_STATE_HEADERS;
    return stomp::WRITE_STATE_NEED_MORE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::write_header_direct(
    stomp::write_state_t *state,
    char const           *header_field,
    char const           *header_value)
{
    size_t   length_key   = stomp::escaped_size(header_field);
    size_t   length_val   = stomp::escaped_size(header_value);
    size_t   length_total = length_key + length_val + 2;
    uint8_t *msg_buffer   = &state->message_buffer[state->message_size];

    if (stomp::WRITE_STATE_NEED_MORE     != state->global_state ||
        stomp::FRAME_WRITE_STATE_HEADERS != state->frame_state)
    {
        // invalid state.
        return stomp_write_error(state, STOMP_ERROR_STR_BADSTATE);
    }
    if (0 == length_key)
    {
        // the header key must be specified.
        return stomp_write_error(state, STOMP_ERROR_STR_NOHDRFIELD);
    }
    if (state->message_size + length_total >= state->message_buffer_size)
    {
        // not enough space in the output buffer.
        state->global_state = stomp::WRITE_STATE_FLUSH;
        return stomp::WRITE_STATE_FLUSH;
    }
    // copy the header field name; terminate with a colon.
     msg_buffer   = stomp::write_escaped_string(msg_buffer, header_field);
    *msg_buffer++ = ':';
    // copy the header value data; terminate with a newline.
     msg_buffer   = stomp::write_escaped_string(msg_buffer, header_value);
    *msg_buffer++ = '\n';
    state->message_size += length_total;
    state->global_state  = stomp::WRITE_STATE_NEED_MORE;
    state->frame_state   = stomp::FRAME_WRITE_STATE_HEADERS;
    return stomp::FRAME_WRITE_STATE_HEADERS;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::write_header_format(
    stomp::write_state_t *state,
    char const           *header_field,
    char const           *value_format, ...)
{
    size_t   length_max = STOMP_MAX_FIELD_LENGTH;
    size_t   length_val = 0;
    char     fmt_buffer[STOMP_MAX_FIELD_LENGTH];
    va_list  varargs;

    // format the header value as a convenience for the caller.
    va_start(varargs, value_format);
#ifdef _MSC_VER
    // on Windows, vsnprintf will return -1 if truncation would occur, so
    // we need to use the _vscprintf function first to figure out how many
    // characters would be generated (not including the trailing NULL.)
    // vsnprintf_s will always NULL-terminate the output buffer.
    length_val = _vscprintf(value_format, varargs);
    vsnprintf_s(fmt_buffer, length_max, _TRUNCATE, value_format, varargs);
#else
    // on OSX and *nix systems, vsnprintf will return the number of characters
    // that would have been written to the output buffer, even if fewer
    // characters were written because the buffer is not long enough. the
    // vsnprintf function will always NULL-terminate the output buffer.
    length_val = vsnprintf(fmt_buffer, length_max, value_format, varargs);
#endif
    va_end(varargs);

    // let stomp::write_header_direct() do the work of copying to output.
    return stomp::write_header_direct(state, header_field, fmt_buffer);
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::write_header_end(stomp::write_state_t *state)
{
    uint8_t *msg_buffer =&state->message_buffer[state->message_size];

    if (stomp::WRITE_STATE_NEED_MORE     != state->global_state ||
        stomp::FRAME_WRITE_STATE_HEADERS != state->frame_state)
    {
        // invalid state.
        return stomp_write_error(state, STOMP_ERROR_STR_BADSTATE);
    }
    if (state->message_size + 1 >= state->message_buffer_size)
    {
        // not enough space in the output buffer.
        state->global_state = stomp::WRITE_STATE_FLUSH;
        return stomp::WRITE_STATE_FLUSH;
    }
    // write a single blank line.
    *msg_buffer++        = '\n';
    state->message_size +=  1;
    state->global_state  = stomp::WRITE_STATE_NEED_MORE;
    state->frame_state   = stomp::FRAME_WRITE_STATE_BODY;
    return stomp::FRAME_WRITE_STATE_BODY;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::write_body_data(
    stomp::write_state_t *state,
    void const           *body_data,
    size_t                body_size,
    size_t                body_offset,
    size_t               *out_written)
{
    uint8_t const *src = ((uint8_t*) body_data) + body_offset;
    uint8_t       *dst = &state->message_buffer[state->message_size];
    size_t         amt = body_size;

    if (stomp::WRITE_STATE_NEED_MORE  != state->global_state ||
        stomp::FRAME_WRITE_STATE_BODY != state->frame_state)
    {
        // invalid state.
        return stomp_write_error(state, STOMP_ERROR_STR_BADSTATE);
    }
    if (state->message_size + body_size >= state->message_buffer_size)
    {
        // not enough space in the output buffer.
        // copy as much data as we can, and flush.
        amt  = state->message_buffer_size - state->message_size;
        memcpy(dst, src, amt);
        if (out_written)     *out_written  = amt;
        state->message_size += amt;
        state->global_state  = stomp::WRITE_STATE_FLUSH;
        return stomp::WRITE_STATE_FLUSH;
    }
    else
    {
        // copy the data directly to the buffer.
        memcpy(dst, src, amt);
        if (out_written)     *out_written = amt;
        state->message_size += amt;
        state->global_state  = stomp::WRITE_STATE_NEED_MORE;
        state->frame_state   = stomp::FRAME_WRITE_STATE_BODY;
        return stomp::FRAME_WRITE_STATE_BODY;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t stomp::close_frame(stomp::write_state_t *state)
{
    uint8_t *msg_buffer = &state->message_buffer[state->message_size];

    if (stomp::WRITE_STATE_NEED_MORE  != state->global_state ||
        stomp::FRAME_WRITE_STATE_BODY != state->frame_state)
    {
        // invalid state.
        return stomp_write_error(state, STOMP_ERROR_STR_BADSTATE);
    }
    if (state->message_size + 1 >= state->message_buffer_size)
    {
        // not enough space in the output buffer.
        state->global_state = stomp::WRITE_STATE_FLUSH;
        return stomp::WRITE_STATE_FLUSH;
    }
    // write a single null byte.
    *msg_buffer++        = '\0';
    state->message_size +=  1;
    state->global_state  = stomp::WRITE_STATE_FRAME_COMPLETE;
    state->frame_state   = stomp::FRAME_WRITE_STATE_CLOSED;
    return stomp::WRITE_STATE_FRAME_COMPLETE;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool stomp::stream_frame(
    stomp::message_t       *frame,
    stomp::writer_flush_fn  flush_fn,
    stomp::writer_error_fn  error_fn,
    void                   *ctx)
{
    uint8_t                 buffer[4096];
    stomp::write_state_t    writer;

    // set no-op callbacks for any left unspecified.
    if (NULL == flush_fn)   flush_fn  = default_stream_flush_func;
    if (NULL == error_fn)   error_fn  = default_stream_error_func;

    stomp::write_state_init(&writer, buffer, 4096);
    if (!stream_command(&writer, frame, flush_fn, error_fn, ctx)) return false;
    if (!stream_headers(&writer, frame, flush_fn, error_fn, ctx)) return false;
    if (!stream_body   (&writer, frame, flush_fn, error_fn, ctx)) return false;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
