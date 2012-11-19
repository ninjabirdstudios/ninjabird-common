/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements cryptographic functions including secure key exchange
/// based on the curve25519 paper (see http://cr.yp.to/ecdh.html) as well as
/// SHA hashing and AES symmetric encryption. The idea is to use the key
/// exchange to establish a shared secret, and then use symmetric encryption
/// for secure communications. The curve25519 implementation was chosen because
/// it is fast (so you can change secrets frequently) and the implementation is
/// relatively concise (under 1000 lines of C code.) Cryptographically-secure
/// random data is obtained by reading from /dev/random or /dev/urandom on
/// UNIX-like systems, and by using RtlGenRandom on Windows.
/// The curve25519 implementation: https://github.com/agl/curve25519-donna
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include "libcrypto.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

static void fsum(int64_t *output, int64_t const *in)
{
    for (size_t i = 0; i < 10; i += 2)
    {
        output[0+i] = (output[0+i] + in[0+i]);
        output[1+i] = (output[1+i] + in[1+i]);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fdifference(int64_t *output, int64_t const *in)
{
    for (size_t i = 0;  i  < 10; ++i)
    {
        output[i] = (in[i] - output[i]);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fscalar_product(
    int64_t       *output,
    int64_t const *in,
    int64_t const  scalar)
{
    for (size_ti = 0; i  < 10; ++i)
    {
       output[i] = in[i] * scalar;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fproduct(int64_t *output, int64_t const *in2, int64_t const *in)
{
    output[0]  =       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[0]);
    output[1]  =       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[1])  +
                       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[0]);
    output[2]  = 2 *   ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[1])  +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[2])  +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[0]);
    output[3]  =       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[2])  +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[1])  +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[0]);
    output[4]  =       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[2])  +
                 2 *  (((int64_t) ((int32_t) in2[1])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[1])) +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[4])  +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[0]);
    output[5]  =       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[2])  +
                       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[4])  +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[1])  +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[0]);
    output[6]  = 2 *  (((int64_t) ((int32_t) in2[3])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[1])) +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[4])  +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[2])  +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[6])  +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[0]);
    output[7]  =       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[4])  +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[2])  +
                       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[6])  +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[1])  +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[0]);
    output[8]  =       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[4])  +
                 2 *  (((int64_t) ((int32_t) in2[3])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[1])) +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[6])  +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[2])  +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[0]);
    output[9]  =       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[4])  +
                       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[6])  +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[2])  +
                       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[1])  +
                       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[0]);
    output[10] = 2 *  (((int64_t) ((int32_t) in2[5])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[1])) +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[6])  +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[4])  +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[2]);
    output[11] =       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[6])  +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[4])  +
                       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[3])  +
                       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[2]);
    output[12] =       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[6])  +
                 2 *  (((int64_t) ((int32_t) in2[5])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[3])) +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[4]);
    output[13] =       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[6])  +
                       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[5])  +
                       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[4]);
    output[14] = 2 *  (((int64_t) ((int32_t) in2[7])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[5])) +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[6]);
    output[15] =       ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[8])  +
                       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[7])  +
                       ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[6]);
    output[16] =       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[8])  +
                 2 *  (((int64_t) ((int32_t) in2[7])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[7]));
    output[17] =       ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[9])  +
                       ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[8]);
    output[18] = 2 *   ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[9]);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void freduce_degree(int64_t *output)
{
    output[8] += output[18] << 4;
    output[8] += output[18] << 1;
    output[8] += output[18];
    output[7] += output[17] << 4;
    output[7] += output[17] << 1;
    output[7] += output[17];
    output[6] += output[16] << 4;
    output[6] += output[16] << 1;
    output[6] += output[16];
    output[5] += output[15] << 4;
    output[5] += output[15] << 1;
    output[5] += output[15];
    output[4] += output[14] << 4;
    output[4] += output[14] << 1;
    output[4] += output[14];
    output[3] += output[13] << 4;
    output[3] += output[13] << 1;
    output[3] += output[13];
    output[2] += output[12] << 4;
    output[2] += output[12] << 1;
    output[2] += output[12];
    output[1] += output[11] << 4;
    output[1] += output[11] << 1;
    output[1] += output[11];
    output[0] += output[10] << 4;
    output[0] += output[10] << 1;
    output[0] += output[10];
}

/*/////////////////////////////////////////////////////////////////////////80*/

static inline int64_t div_by_2_26(int64_t const v)
{
    const uint32_t highword   = ((uint64_t) v)        >> 32;
    const int32_t  sign       = ((int32_t)  highword) >> 31;
    const int32_t  roundoff   = ((uint32_t) sign)     >>  6;
    return    (v + roundoff) >> 26;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static inline int64_t div_by_2_25(int64_t const v)
{
    const uint32_t highword   = ((uint64_t) v)        >> 32;
    const int32_t  sign       = ((int32_t)  highword) >> 31;
    const int32_t  roundoff   = ((uint32_t) sign)     >>  7;
    return    (v + roundoff) >> 25;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static inline int32_t div_int32_t_by_2_25(int32_t const v)
{
    const int32_t roundoff   = ((uint32_t)(v >> 31)) >> 7;
    return   (v + roundoff) >> 25;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void freduce_coefficients(int64_t *output)
{
    output[10] = 0;
    for (size_t i = 0; i < 10; i += 2)
    {
        int64_t over64   = div_by_2_26(output[i]);
        output[i]       -= over64 << 26;
        output[i+1]     += over64;
        over64           = div_by_2_25(output[i+1]);
        output[i+1]     -= over64 << 25;
        output[i+2]     += over64;
    }
    output[0] += output[10] << 4;
    output[0] += output[10] << 1;
    output[0] += output[10];
    output[10] = 0;
    {
        int64_t over64   = div_by_2_26(output[0]);
        output[0]       -= over64 << 26;
        output[1]       += over64;
    }
    {
        int32_t over32   = div_int32_t_by_2_25(output[1]);
        output[1]       -= over32 << 25;
        output[2]       += over32;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fmul(
    int64_t       *output,
    int64_t const *in,
    int64_t const *in2)
{
    int64_t  t[19];
    fproduct(t, in, in2);
    freduce_degree(t);
    freduce_coefficients(t);
    memcpy(output, t, sizeof(int64_t) * 10);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fsquare_inner(int64_t *output, int64_t const *in)
{
    output[0]  =      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[0]);
    output[1]  = 2 *  ((int64_t) ((int32_t) in[0])) * ((int32_t) in[1]);
    output[2]  = 2 * (((int64_t) ((int32_t) in[1])) * ((int32_t) in[1]) +
                      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[2]));
    output[3]  = 2 * (((int64_t) ((int32_t) in[1])) * ((int32_t) in[2]) +
                      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[3]));
    output[4]  =      ((int64_t) ((int32_t) in[2])) * ((int32_t) in[2]) +
                 4 *  ((int64_t) ((int32_t) in[1])) * ((int32_t) in[3]) +
                 2 *  ((int64_t) ((int32_t) in[0])) * ((int32_t) in[4]);
    output[5]  = 2 * (((int64_t) ((int32_t) in[2])) * ((int32_t) in[3]) +
                      ((int64_t) ((int32_t) in[1])) * ((int32_t) in[4]) +
                      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[5]));
    output[6]  = 2 * (((int64_t) ((int32_t) in[3])) * ((int32_t) in[3]) +
                      ((int64_t) ((int32_t) in[2])) * ((int32_t) in[4]) +
                      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[6]) +
                 2 *  ((int64_t) ((int32_t) in[1])) * ((int32_t) in[5]));
    output[7]  = 2 * (((int64_t) ((int32_t) in[3])) * ((int32_t) in[4]) +
                      ((int64_t) ((int32_t) in[2])) * ((int32_t) in[5]) +
                      ((int64_t) ((int32_t) in[1])) * ((int32_t) in[6]) +
                      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[7]));
    output[8]  =      ((int64_t) ((int32_t) in[4])) * ((int32_t) in[4]) +
                 2 * (((int64_t) ((int32_t) in[2])) * ((int32_t) in[6]) +
                      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[8]) +
                 2 * (((int64_t) ((int32_t) in[1])) * ((int32_t) in[7]) +
                      ((int64_t) ((int32_t) in[3])) * ((int32_t) in[5])));
    output[9]  = 2 * (((int64_t) ((int32_t) in[4])) * ((int32_t) in[5]) +
                      ((int64_t) ((int32_t) in[3])) * ((int32_t) in[6]) +
                      ((int64_t) ((int32_t) in[2])) * ((int32_t) in[7]) +
                      ((int64_t) ((int32_t) in[1])) * ((int32_t) in[8]) +
                      ((int64_t) ((int32_t) in[0])) * ((int32_t) in[9]));
    output[10] = 2 * (((int64_t) ((int32_t) in[5])) * ((int32_t) in[5]) +
                      ((int64_t) ((int32_t) in[4])) * ((int32_t) in[6]) +
                      ((int64_t) ((int32_t) in[2])) * ((int32_t) in[8]) +
                 2 * (((int64_t) ((int32_t) in[3])) * ((int32_t) in[7]) +
                      ((int64_t) ((int32_t) in[1])) * ((int32_t) in[9])));
    output[11] = 2 * (((int64_t) ((int32_t) in[5])) * ((int32_t) in[6]) +
                      ((int64_t) ((int32_t) in[4])) * ((int32_t) in[7]) +
                      ((int64_t) ((int32_t) in[3])) * ((int32_t) in[8]) +
                      ((int64_t) ((int32_t) in[2])) * ((int32_t) in[9]));
    output[12] =      ((int64_t) ((int32_t) in[6])) * ((int32_t) in[6]) +
                 2 * (((int64_t) ((int32_t) in[4])) * ((int32_t) in[8]) +
                 2 * (((int64_t) ((int32_t) in[5])) * ((int32_t) in[7]) +
                      ((int64_t) ((int32_t) in[3])) * ((int32_t) in[9])));
    output[13] = 2 * (((int64_t) ((int32_t) in[6])) * ((int32_t) in[7]) +
                      ((int64_t) ((int32_t) in[5])) * ((int32_t) in[8]) +
                      ((int64_t) ((int32_t) in[4])) * ((int32_t) in[9]));
    output[14] = 2 * (((int64_t) ((int32_t) in[7])) * ((int32_t) in[7]) +
                      ((int64_t) ((int32_t) in[6])) * ((int32_t) in[8]) +
                 2 *  ((int64_t) ((int32_t) in[5])) * ((int32_t) in[9]));
    output[15] = 2 * (((int64_t) ((int32_t) in[7])) * ((int32_t) in[8]) +
                      ((int64_t) ((int32_t) in[6])) * ((int32_t) in[9]));
    output[16] =      ((int64_t) ((int32_t) in[8])) * ((int32_t) in[8]) +
                 4 *  ((int64_t) ((int32_t) in[7])) * ((int32_t) in[9]);
    output[17] = 2 *  ((int64_t) ((int32_t) in[8])) * ((int32_t) in[9]);
    output[18] = 2 *  ((int64_t) ((int32_t) in[9])) * ((int32_t) in[9]);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fsquare(int64_t *output, int64_t const *in)
{
    int64_t t[19];
    fsquare_inner (t, in);
    freduce_degree(t);
    freduce_coefficients(t);
    memcpy(output, t, sizeof(int64_t) * 10);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fexpand(int64_t *output, uint8_t const *input)
{
#define F(n,start,shift,mask) \
    output[n] = ((((int64_t) input[start + 0]) |       \
                  ((int64_t) input[start + 1]) << 8 |  \
                  ((int64_t) input[start + 2]) << 16 | \
                  ((int64_t) input[start + 3]) << 24) >> shift) & mask;
    F(0,  0, 0, 0x3ffffff);
    F(1,  3, 2, 0x1ffffff);
    F(2,  6, 3, 0x3ffffff);
    F(3,  9, 5, 0x1ffffff);
    F(4, 12, 6, 0x3ffffff);
    F(5, 16, 0, 0x1ffffff);
    F(6, 19, 1, 0x3ffffff);
    F(7, 22, 3, 0x1ffffff);
    F(8, 25, 4, 0x3ffffff);
    F(9, 28, 6, 0x1ffffff);
#undef F
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fcontract(uint8_t *output, int64_t *input)
{
    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 9; ++i)
        {
            if ((i & 1) == 1)
            {
                const int32_t mask  =    (int32_t)   (input[i]) >> 31;
                const int32_t carry = -(((int32_t)   (input[i]) & mask) >> 25);
                input[i]   = (int32_t)(input[i])   + (carry << 25);
                input[i+1] = (int32_t)(input[i+1]) -  carry;
            }
            else
            {
                const int32_t mask  =    (int32_t)   (input[i]) >> 31;
                const int32_t carry = -(((int32_t)   (input[i]) & mask) >> 26);
                input[i]   = (int32_t)(input[i])   + (carry << 26);
                input[i+1] = (int32_t)(input[i+1]) -  carry;
            }
        }
        const int32_t mask  =    (int32_t)(input[9]) >> 31;
        const int32_t carry = -(((int32_t)(input[9]) & mask) >> 25);
        input[9] = (int32_t)(input[9]) +  (carry << 25);
        input[0] = (int32_t)(input[0]) -  (carry  * 19);
    }

    const int32_t mask  =    (int32_t) (input[0]) >> 31;
    const int32_t carry = -(((int32_t) (input[0]) & mask) >> 26);
    input[0]   = (int32_t)(input[0]) + (carry << 26);
    input[1]   = (int32_t)(input[1]) -  carry;
    input[1] <<= 2;
    input[2] <<= 3;
    input[3] <<= 5;
    input[4] <<= 6;
    input[6] <<= 1;
    input[7] <<= 3;
    input[8] <<= 4;
    input[9] <<= 6;
#define F(i, s) \
    output[s+0] |=  input[i] & 0xff;        \
    output[s+1]  = (input[i] >>  8) & 0xff; \
    output[s+2]  = (input[i] >> 16) & 0xff; \
    output[s+3]  = (input[i] >> 24) & 0xff;
    output[0]    = 0;
    output[16]   = 0;
    F(0,  0);
    F(1,  3);
    F(2,  6);
    F(3,  9);
    F(4, 12);
    F(5, 16);
    F(6, 19);
    F(7, 22);
    F(8, 25);
    F(9, 28);
#undef F
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fmonty(
    int64_t       *x2,
    int64_t       *z2,
    int64_t       *x3,
    int64_t       *z3,
    int64_t       *x,
    int64_t       *z,
    int64_t       *xprime,
    int64_t       *zprime,
    int64_t const *qmqp)
{
    int64_t origx[10];
    int64_t origxprime[10];
    int64_t zzz[19];
    int64_t xx[19];
    int64_t zz[19];
    int64_t xxprime[19];
    int64_t zzprime[19];
    int64_t zzzprime[19];
    int64_t xxxprime[19];

    memcpy(origx, x, sizeof(int64_t) * 10);
    fsum(x, z);
    fdifference(z, origx);
    memcpy(origxprime, xprime, sizeof(int64_t) * 10);
    fsum(xprime, zprime);
    fdifference(zprime, origxprime);
    fproduct(xxprime, xprime, z);
    fproduct(zzprime, x, zprime);
    freduce_degree(xxprime);
    freduce_coefficients(xxprime);
    freduce_degree(zzprime);
    freduce_coefficients(zzprime);
    memcpy(origxprime, xxprime, sizeof(int64_t) * 10);
    fsum(xxprime, zzprime);
    fdifference(zzprime, origxprime);
    fsquare(xxxprime, xxprime);
    fsquare(zzzprime, zzprime);
    fproduct(zzprime, zzzprime, qmqp);
    freduce_degree(zzprime);
    freduce_coefficients(zzprime);
    memcpy(x3, xxxprime, sizeof(int64_t) * 10);
    memcpy(z3, zzprime,  sizeof(int64_t) * 10);
    fsquare(xx, x);
    fsquare(zz, z);
    fproduct(x2, xx, zz);
    freduce_degree(x2);
    freduce_coefficients(x2);
    fdifference(zz, xx);
    memset(zzz + 10, 0, sizeof(int64_t) * 9);
    fscalar_product(zzz, zz, 121665);
    freduce_coefficients(zzz);
    fsum(zzz, xx);
    fproduct(z2, zz, zzz);
    freduce_degree(z2);
    freduce_coefficients(z2);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void swap_conditional(int64_t a[19], int64_t b[19], int64_t iswap)
{
    const int32_t swap = -iswap;
    for (size_t i = 0; i < 10; ++i)
    {
        const  int32_t x = swap & (((int32_t) a[i]) ^ ((int32_t) b[i]));
        a[i] = ((int32_t) a[i]) ^ x;
        b[i] = ((int32_t) b[i]) ^ x;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void cmult(
    int64_t       *resultx,
    int64_t       *resultz,
    uint8_t const *n,
    int64_t const *q)
{
    int64_t  a[19]  = {0};
    int64_t  b[19]  = {1};
    int64_t  c[19]  = {1};
    int64_t  d[19]  = {0};
    int64_t *nqpqx  =  a;
    int64_t *nqpqz  =  b;
    int64_t *nqx    =  c;
    int64_t *nqz    =  d;
    int64_t *t;
    int64_t  e[19]  = {0};
    int64_t  f[19]  = {1};
    int64_t  g[19]  = {0};
    int64_t  h[19]  = {1};
    int64_t *nqpqx2 =  e;
    int64_t *nqpqz2 =  f;
    int64_t *nqx2   =  g;
    int64_t *nqz2   =  h;

    memcpy(nqpqx, q, sizeof(int64_t) * 10);
    for (size_t i = 0; i < 32; ++i)
    {
        uint8_t  byte = n[31 - i];
        for (size_t j = 0; j < 8; ++j)
        {
            const int64_t bit   =  byte >> 7;
            swap_conditional(nqx,  nqpqx,  bit);
            swap_conditional(nqz,  nqpqz,  bit);
            fmonty    (nqx2, nqz2, nqpqx2, nqpqz2, nqx, nqz, nqpqx, nqpqz, q);
            swap_conditional(nqx2, nqpqx2, bit);
            swap_conditional(nqz2, nqpqz2, bit);
            t      = nqx;
            nqx    = nqx2;
            nqx2   = t;
            t      = nqz;
            nqz    = nqz2;
            nqz2   = t;
            t      = nqpqx;
            nqpqx  = nqpqx2;
            nqpqx2 = t;
            t      = nqpqz;
            nqpqz  = nqpqz2;
            nqpqz2 = t;
            byte <<= 1;
        }
    }
    memcpy(resultx, nqx, sizeof(int64_t) * 10);
    memcpy(resultz, nqz, sizeof(int64_t) * 10);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void crecip(int64_t *out, int64_t const *z)
{
    int64_t z2[10];
    int64_t z9[10];
    int64_t z11[10];
    int64_t z2_5_0[10];
    int64_t z2_10_0[10];
    int64_t z2_20_0[10];
    int64_t z2_50_0[10];
    int64_t z2_100_0[10];
    int64_t t0[10];
    int64_t t1[10];

    fsquare(z2, z);
    fsquare(t1, z2);
    fsquare(t0, t1);
    fmul(z9, t0, z);
    fmul(z11, z9, z2);
    fsquare(t0, z11);
    fmul(z2_5_0, t0, z9);

    fsquare(t0, z2_5_0);
    fsquare(t1, t0);
    fsquare(t0, t1);
    fsquare(t1, t0);
    fsquare(t0, t1);
    fmul(z2_10_0, t0, z2_5_0);

    fsquare(t0, z2_10_0);
    fsquare(t1, t0);
    for (size_t i = 2; i < 10; i += 2)
    {
        fsquare(t0, t1);
        fsquare(t1, t0);
    }
    fmul(z2_20_0, t1, z2_10_0);

    fsquare(t0, z2_20_0);
    fsquare(t1, t0);
    for (size_t i = 2; i < 20; i += 2)
    {
        fsquare(t0, t1);
        fsquare(t1, t0);
    }
    fmul(t0, t1, z2_20_0);

    fsquare(t1, t0);
    fsquare(t0, t1);
    for (size_t i = 2; i < 10; i += 2)
    {
        fsquare(t1, t0);
        fsquare(t0, t1);
    }
    fmul(z2_50_0, t0, z2_10_0);

    fsquare(t0, z2_50_0);
    fsquare(t1, t0);
    for (size_t i = 2; i < 50; i += 2)
    {
        fsquare(t0, t1);
        fsquare(t1, t0);
    }
    fmul(z2_100_0, t1, z2_50_0);

    fsquare(t1, z2_100_0);
    fsquare(t0, t1);
    for (size_t i = 2; i < 100; i += 2)
    {
        fsquare(t1, t0);
        fsquare(t0, t1);
    }
    fmul(t1, t0, z2_100_0);

    fsquare(t0, t1);
    fsquare(t1, t0);
    for (size_t i = 2; i < 50; i += 2)
    {
        fsquare(t0, t1);
        fsquare(t1, t0);
    }
    fmul(t0, t1, z2_50_0);

    fsquare(t1, t0);
    fsquare(t0, t1);
    fsquare(t1, t0);
    fsquare(t0, t1);
    fsquare(t1, t0);
    fmul(out, t1, z11);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void crypto::secret_key(uint8_t *out_key)
{
    // @todo: read 32 bytes from the CSPRNG into out_key.
    out_key[0]  &= 248;
    out_key[31] &= 127;
    out_key[31] |=  64;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void crypto::public_key(uint8_t const *secret, uint8_t *out_public)
{
    const uint8_t basepoint[32] = {9};
    crypto::curve25519(out_public, secret, basepoint);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void crypto::shared_secret(
    uint8_t const *our_secret,
    uint8_t const *her_public,
    uint8_t       *out_shared)
{
    crypto::curve25519(out_shared, our_secret, her_public);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void crypto::curve25519(
    uint8_t       *out_buf,
    uint8_t const *secret,
    uint8_t const *basepoint)
{
    uint8_t e [32];
    int64_t bp[10];
    int64_t x [10];
    int64_t z [11];
    int64_t zmone[10];

    for (size_t i = 0; i < 32; ++i)
    {
        e[i] = secret[i];
    }
    e[0]  &= 248;
    e[31] &= 127;
    e[31] |=  64;

    fexpand(bp, basepoint);
    cmult(x, z, e, bp);
    crecip(zmone, z);
    fmul(z, x, zmone);
    freduce_coefficients(z);
    fcontract(out_buf, z);
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
