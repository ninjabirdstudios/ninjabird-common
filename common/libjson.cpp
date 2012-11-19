/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libjson.cpp
///  Implements the interface for parsing JSON documents into a low-cost, easy
///  to traverse runtime representation. This implementation is based on the
///  MIT-licensed vjson library (http://code.google.com/p/vjson/) with custom
///  extensions to allow more flexibility in the JSON document definition and
///  to provide custom memory management.
///
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <cstdlib>
#include "libjson.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

#ifndef PRIu64
#define PRIu64       "I64u"
#endif /* !defined(PRIu64) */

/*/////////////////////////////////////////////////////////////////////////80*/

#define JSON_ERROR(it, desc, err)                                             \
    if (err != NULL)                                                          \
    {                                                                         \
        err->description = (char*) desc;                                      \
        err->position    = it;                                                \
        err->line        = 1   - escaped_newlines;                            \
        for (char *c = it; c  != text; --c)                                   \
        {                                                                     \
            if (*c == '\n') err->line += 1;                                   \
        }                                                                     \
    }                                                                         \
    if (root != NULL)                                                         \
    {                                                                         \
        json::free(root, allocator);                                          \
    }                                                                         \
    return false

/*/////////////////////////////////////////////////////////////////////////80*/

static inline bool is_digit(char ch)
{
    return (ch >= '0' && ch <= '9');
}

/*/////////////////////////////////////////////////////////////////////////80*/

static char* str_to_int(char *first, char *last, int64_t *out)
{
    int64_t  sign   = 1;
    int64_t  result = 0;

    if (first != last)
    {
        if ('-' == *first)
        {
            sign = -1;
            ++first;
        }
        else if ('+' == *first)
        {
            sign = +1;
            ++first;
        }
    }
    for (; first != last && is_digit(*first); ++first)
    {
        result = 10 * result + (*first - '0');
    }
    *out = result * sign;
    return first;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static char* str_to_hex(char *first, char *last, uint32_t *out)
{
    uint32_t result = 0;
    for (; first != last; ++first)
    {
        unsigned int digit;
        if (is_digit(*first))
        {
            digit = *first - '0';
        }
        else if (*first >= 'a' && *first <= 'f')
        {
            digit = *first - 'a' + 10;
        }
        else if (*first >= 'A' && *first <= 'F')
        {
            digit = *first - 'A' + 10;
        }
        else break;
        result = 16 * result + digit;
    }
    *out = result;
    return first;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static char* str_to_num(char *first, char *last, double *out)
{
    double sign     = 1.0;
    double result   = 0.0;
    bool   exp_neg  = false;
    int    exponent = 0;

    if (first != last)
    {
        if ('-' == *first)
        {
            sign = -1.0;
            ++first;
        }
        else if ('+' == *first)
        {
            sign = +1.0;
            ++first;
        }
    }
    for (; first != last && is_digit(*first); ++first)
    {
        result = 10 * result + (*first - '0');
    }
    if (first != last && '.' == *first)
    {
        double inv_base = 0.1;
        ++first;
        for (; first != last && is_digit(*first); ++first)
        {
            result   += (*first - '0') * inv_base;
            inv_base *= 0.1;
        }
    }
    result *= sign;
    if (first != last && ('e' == *first || 'E' == *first))
    {
        ++first;
        if ('-' == *first)
        {
            exp_neg = true;
            ++first;
        }
        else if ('+' == *first)
        {
            exp_neg = false;
            ++first;
        }
        for (; first != last && is_digit(*first); ++first)
        {
            exponent = 10 * exponent + (*first - '0');
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
    *out = result;
    return first;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static json::item_t* CMN_CALL_C libc_alloc(size_t size_in_bytes, void *context)
{
    CMN_UNUSED(context);
    return (json::item_t*) ::malloc(size_in_bytes);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void CMN_CALL_C libc_free(
    json::item_t *item,
    size_t        size_in_bytes,
    void         *context)
{
    CMN_UNUSED(context);
    CMN_UNUSED(size_in_bytes);
    if (item != NULL) ::free(item);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void CMN_CALL_C null_free(
    json::item_t *item,
    size_t        size_in_bytes,
    void         *context)
{
    CMN_UNUSED(item);
    CMN_UNUSED(size_in_bytes);
    CMN_UNUSED(context);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fixup_allocator(json::alloc_t *allocator)
{
    if (NULL == allocator->alloc && NULL == allocator->free)
    {
        allocator->alloc = libc_alloc;
        allocator->free  = libc_free;
    }
    if (NULL != allocator->alloc && NULL == allocator->free)
    {
        // free is a no-op, but we don't want to have to constantly NULL-check.
        allocator->free  = null_free;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static json::item_t* json_alloc(json::alloc_t *allocator)
{
    size_t        size = sizeof(json::item_t);
    json::item_t *item = allocator->alloc(size, allocator->context);
    item->parent       = NULL;
    item->next_sibling = NULL;
    item->first_child  = NULL;
    item->last_child   = NULL;
    item->key          = NULL;
    item->value_type   = json::TYPE_UNKNOWN;
    item->value.string = NULL;
    return item;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void json_free(json::item_t *item, json::alloc_t *allocator)
{
    if (item != NULL)
    {
        allocator->free(item, sizeof(json::item_t), allocator->context);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void json_indent(FILE *fp, size_t tab_count)
{
    for (size_t i = 0; i < tab_count; ++i)
    {
        fprintf(fp, "  ");
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void json_print(FILE *fp, json::item_t *node, size_t indent_level)
{
    if (NULL == node) return;
    json_indent(fp, indent_level);
    if (node->key != NULL) fprintf(fp, "\"%s\" : ", node->key);
    switch (node->value_type)
    {
        case json::TYPE_OBJECT:
            {
                fprintf(fp, "\n");
                json_indent(fp, indent_level);
                fprintf(fp, "{\n");
                json::item_t *item_iter = node->first_child;
                while (item_iter != NULL)
                {
                    json_print(fp, item_iter, indent_level + 1);
                    if (item_iter->next_sibling != NULL) fprintf(fp, ",\n");
                    else                                 fprintf(fp, " \n");
                    item_iter    = item_iter->next_sibling;
                }
                json_indent(fp, indent_level);
                fprintf(fp, "}");
            }
            break;

        case json::TYPE_ARRAY:
            {
                fprintf(fp, "\n");
                json_indent(fp, indent_level);
                fprintf(fp, "[\n");
                json::item_t *item_iter = node->first_child;
                while (item_iter != NULL)
                {
                    json_print(fp, item_iter, indent_level + 1);
                    if (item_iter->next_sibling != NULL) fprintf(fp, ",\n");
                    else                                 fprintf(fp, " \n");
                    item_iter    = item_iter->next_sibling;
                }
                json_indent(fp, indent_level);
                fprintf(fp, "]\n");
            }
            break;

        case json::TYPE_STRING:
            fprintf(fp, "\"%s\"", node->value.string);
            break;

        case json::TYPE_INTEGER:
            fprintf(fp, "%"PRIu64, node->value.integer);
            break;

        case json::TYPE_NUMBER:
            fprintf(fp, "%f", node->value.number);
            break;

        case json::TYPE_BOOLEAN:
            fprintf(fp, node->value.boolean ? "true" : "false");
            break;

        case json::TYPE_NULL:
            fprintf(fp, "null");
            break;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void json::allocator_init(
    json::alloc_t  *alloc,
    json::alloc_fn  alloc_func,
    json::free_fn   free_func, /* = NULL */
    void           *context    /* = NULL */)
{
    if (alloc != NULL)
    {
        alloc->alloc   = alloc_func;
        alloc->free    = free_func;
        alloc->context = context;
        fixup_allocator(alloc);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void json::append(json::item_t *lhs, json::item_t *rhs)
{
    rhs->parent = lhs;
    if (lhs->last_child)
    {
        lhs->last_child->next_sibling = rhs;
        lhs->last_child  = rhs;
    }
    else
    {
        lhs->last_child  = rhs;
        lhs->first_child = rhs;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool json::parse(
    char           *text,
    json::alloc_t  *allocator,
    json::item_t  **out_root,
    json::error_t  *out_error)
{
    json::alloc_t  libc = {libc_alloc, libc_free};
    json::item_t  *root = NULL;
    json::item_t  *top  = NULL;
    char          *name = NULL;
    char          *it   = text;
    size_t         escaped_newlines  = 0;

    if (out_root != NULL) *out_root  = NULL;
    if (NULL == allocator) allocator = &libc;
    fixup_allocator(allocator);

    while (*it)
    {
        switch (*it)
        {
            case '{':
            case '[':
                {
                    json::item_t *object = json_alloc(allocator);
                    object->key          = name;
                    object->value_type   = ('{' == *it) ? json::TYPE_OBJECT : json::TYPE_ARRAY;
                    name                 = NULL;
                    ++it;
                    if (top)
                    {
                        // the new object is a child of the parent object, top.
                        json::append(top, object);
                    }
                    else if (!root)
                    {
                        // this is the start of the root object.
                        root = object;
                    }
                    else
                    {
                        json_free(object, allocator);
                        JSON_ERROR(it, "Multiple root objects", out_error);
                        // returns false.
                    }
                    top = object;
                }
                break;

            case '}':
            case ']':
                {
                    int32_t type = (('}' == *it) ? json::TYPE_OBJECT : json::TYPE_ARRAY);
                    if (!top || top->value_type != type)
                    {
                        JSON_ERROR(it, "Closing brace mismatch", out_error);
                        // returns false.
                    }
                    ++it;
                    top = top->parent;
                }
                break;

            case ':':
            case '=':
                {
                    if (!top || top->value_type != json::TYPE_OBJECT)
                    {
                        JSON_ERROR(it, "Unexpected character \':\' or \'=\'", out_error);
                        // returns false.
                    }
                    ++it;
                }
                break;

            case ',':
                {
                    if (!top)
                    {
                        JSON_ERROR(it, "Unexpected character \',\'", out_error);
                        // returns false.
                    }
                    ++it;
                }
                break;

            case '"':
            case '\'':
                {
                    if (!top)
                    {
                        JSON_ERROR(it, "Unexpected quote character", out_error);
                        // returns false.
                    }
                    ++it;

                    char   *first = it;
                    char   *last  = it;
                    while (*it)
                    {
                        if ((unsigned char)*it < '\x20')
                        {
                            JSON_ERROR(it, "Unexpected control character", out_error);
                            // returns false.
                        }
                        else if ('\\' == *it)
                        {
                            switch (it[1])
                            {
                                case '"':  *last = '"';  break;
                                case '\'': *last = '\''; break;
                                case '\\': *last = '\\'; break;
                                case '/':  *last = '/';  break;
                                case 'b':  *last = '\b'; break;
                                case 'f':  *last = '\f'; break;
                                case 'r':  *last = '\r'; break;
                                case 't':  *last = '\t'; break;
                                case 'n':
                                    {
                                        *last = '\n';
                                        ++escaped_newlines;
                                    }
                                    break;
                                case 'u':
                                    {
                                        uint32_t cp;
                                        if (str_to_hex(it + 2, it + 6, &cp) != it + 6)
                                        {
                                            JSON_ERROR(it, "Invalid Unicode codepoint", out_error);
                                            // returns false.
                                        }
                                        if (cp < 0x7F)
                                        {
                                            *last = (char) cp;
                                        }
                                        else if (cp <= 0x7FF)
                                        {
                                            *last++ = (char)(0xC0 | (cp >> 6));
                                            *last   = (char)(0x80 & (cp &  0x3F));
                                        }
                                        else if (cp < 0xFFFF)
                                        {
                                            *last++ = (char)(0xE0 | (cp >> 12));
                                            *last++ = (char)(0x80 |((cp >>  6) & 0x3F));
                                            *last   = (char)(0x80 | (cp & 0x3F));
                                        }
                                        it += 4;
                                    }
                                    break;
                                default:
                                    {
                                        JSON_ERROR(it, "Unrecognized escape sequence", out_error);
                                        // returns false.
                                    }
                            } // end switch (it[1])
                            ++last;
                            it += 2; // skip the escape sequence.
                        } // end else if ('\\' == *it)
                        else if (first[-1] == *it) // match quote type
                        {
                            // end of the string.
                            *last = 0; // NULL-terminator.
                            ++it;
                            break;
                        }
                        else
                        {
                            // regular character in string.
                            *last++ = *it++;
                        }
                    } // end while (*it)

                    if (!name && top->value_type == json::TYPE_OBJECT)
                    {
                        // this is a key name in the object.
                        name = first;
                    }
                    else
                    {
                        // this is a new string value.
                        json::item_t *value = json_alloc(allocator);
                        value->key          = name;
                        value->value_type   = json::TYPE_STRING;
                        value->value.string = first;
                        name                = NULL;
                        json::append(top, value);
                    }
                }
                break;

            case 'n':
            case 'N':
            case 't':
            case 'T':
            case 'f':
            case 'F':
                {
                    if (!top)
                    {
                        JSON_ERROR(it, "Unexpected character", out_error);
                        // returns false.
                    }

                    json::item_t *value  = json_alloc(allocator);
                    if ((it[0] == 'n' || it[0] == 'N') &&
                        (it[1] == 'u' || it[1] == 'U') &&
                        (it[2] == 'l' || it[2] == 'L') &&
                        (it[3] == 'l' || it[3] == 'L'))
                    {
                        value->key           = name;
                        value->value_type    = json::TYPE_NULL;
                        value->value.string  = NULL;
                        name                 = NULL;
                        it                  += 4;
                        json::append(top, value);
                        break;
                    }
                    if ((it[0] == 't' || it[0] == 'T') &&
                        (it[1] == 'r' || it[1] == 'R') &&
                        (it[2] == 'u' || it[2] == 'U') &&
                        (it[3] == 'e' || it[3] == 'E'))
                    {
                        value->key           = name;
                        value->value_type    = json::TYPE_BOOLEAN;
                        value->value.boolean = true;
                        name                 = NULL;
                        it                  += 4;
                        json::append(top, value);
                        break;
                    }
                    if ((it[0] == 'f' || it[0] == 'F') &&
                        (it[1] == 'a' || it[1] == 'A') &&
                        (it[2] == 'l' || it[2] == 'L') &&
                        (it[3] == 's' || it[3] == 'S') &&
                        (it[4] == 'e' || it[4] == 'E'))
                    {
                        value->key           = name;
                        value->value_type    = json::TYPE_BOOLEAN;
                        value->value.boolean = false;
                        name                 = NULL;
                        it                  += 5;
                        json::append(top, value);
                        break;
                    }
                    json_free (value, allocator);
                    JSON_ERROR(it, "Unknown identifier", out_error);
                    // returns false.
                }
                break;

            case '-':
            case '+':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                {
                    if (!top)
                    {
                        JSON_ERROR(it, "Unexpected character", out_error);
                        // returns false.
                    }

                    char         *first = it;
                    json::item_t *value = json_alloc(allocator);
                    value->key          = name;
                    value->value_type   = json::TYPE_INTEGER;
                    name                = NULL;

                    // find the end of the number and determine whether it's
                    // a floating-point value instead of an integer.
                    while (*it != '\x20' &&
                           *it != '\x9'  &&
                           *it != '\xD'  &&
                           *it != '\xA'  &&
                           *it != ','    &&
                           *it != ']'    &&
                           *it != '}')
                    {
                        if ('.' == *it || 'e' == *it || 'E' == *it)
                        {
                            value->value_type = json::TYPE_NUMBER;
                        }
                        ++it;
                    }
                    if (json::TYPE_INTEGER  == value->value_type     &&
                        str_to_int(first, it, &value->value.integer) != it)
                    {
                        json_free (value, allocator);
                        JSON_ERROR(first, "Bad integer value", out_error);
                        // returns false.
                    }
                    if (json::TYPE_NUMBER   == value->value_type    &&
                        str_to_num(first, it, &value->value.number) != it)
                    {
                        json_free (value, allocator);
                        JSON_ERROR(first, "Bad number value", out_error);
                        // returns false.
                    }
                    json::append(top, value);
                }
                break;

            default:
                {
                    JSON_ERROR(it, "Unexpected character", out_error);
                    // returns false.
                }
        } // end switch (*it)

        // skip whitespace
        while ('\x20' == *it || '\x9' == *it || '\xD' == *it || '\xA' == *it)
        {
            ++it;
        }
    } // end while (*it)

    if (top)
    {
        JSON_ERROR(it, "Not all objects or arrays were closed", out_error);
        // returns false.
    }

    if (out_root != NULL) *out_root = root;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void json::write(FILE *fp, json::item_t *root)
{
    if (NULL == fp)   return;
    if (NULL != root) json_print(fp, root, 0);
    fprintf(fp, "\n");
}

/*/////////////////////////////////////////////////////////////////////////80*/

void json::free(json::item_t *node, json::alloc_t *allocator)
{
    if (NULL == node) return;
    // recurse across the list:
    json::free(node->next_sibling, allocator);
    // recurse down the tree:
    json::free(node->last_child,   allocator);
    // delete this node.
    json_free (node, allocator);
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
