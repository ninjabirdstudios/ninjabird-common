/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements a default set of hashing and CRC calculation functions
/// with various performance characteristics. All functions operate on raw
/// memory buffers.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include "libhash.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

#ifdef  _MSC_VER
    #include <intrin.h>
    #define ROTL32(_x, _y)    _rotl((_x), (_y))
    #define ROTL64(_x, _y)    _rotl64((_x), (_y))
    #define BIG_CONSTANT(_x) (_x)
#else
    #define ROTL32(_x, _y)    rotl32((_x), (_y))
    #define ROTL64(_x, _y)    rotl64((_x), (_y))
    #define BIG_CONSTANT(_x) (_x##LLU)
#endif

/*/////////////////////////////////////////////////////////////////////////80*/

#define HAS_NULL_BYTE(_x)    (((_x) - 0x01010101) & (~(_x) & 0x80808080))

/*/////////////////////////////////////////////////////////////////////////80*/

static uint32_t const CRC[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/*/////////////////////////////////////////////////////////////////////////80*/

static CMN_FORCE_INLINE uint32_t rotl32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

/*/////////////////////////////////////////////////////////////////////////80*/

static CMN_FORCE_INLINE uint64_t rotl64(uint64_t x, int8_t r)
{
    return (x << r) | (x >> (64 - r));
}

/*/////////////////////////////////////////////////////////////////////////80*/

static CMN_FORCE_INLINE uint32_t get_block(uint32_t const *p, int i)
{
    // @note: if your platform can't handle non-aligned reads, or
    // must perform endianess/byte swapping, do that here.
    return p[i];
}

/*/////////////////////////////////////////////////////////////////////////80*/

static CMN_FORCE_INLINE uint64_t get_block(uint64_t const *p, int i)
{
    // @note: if your platform can't handle non-aligned reads, or
    // must perform endianess/byte swapping, do that here.
    return p[i];
}

/*/////////////////////////////////////////////////////////////////////////80*/

static CMN_FORCE_INLINE uint32_t fmix(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85EBCA6B;
    h ^= h >> 13;
    h *= 0xC2B2AE35;
    h ^= h >> 16;
    return h;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static CMN_FORCE_INLINE uint64_t fmix(uint64_t k)
{
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xFF51AFD7ED558CCD);
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xC4CEB9FE1A85EC53);
    k ^= k >> 33;
    return k;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint32_t hash::hash32_string(char const *str)
{
    uint32_t hash = 0;
    if (str  != NULL)
    {
        // hash the majority of the data in 4-byte chunks.
        while (!HAS_NULL_BYTE(*((uint32_t*)str)))
        {
            hash = ROTL32(hash, 7) + str[0];
            hash = ROTL32(hash, 7) + str[1];
            hash = ROTL32(hash, 7) + str[2];
            hash = ROTL32(hash, 7) + str[3];
            str += 4;
        }
        // hash the remaining 0-3 bytes.
        while (*str) hash = ROTL32(hash, 7) + *str++;
    }
    return hash;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint32_t hash::hash32_string(char const *start, char const *end)
{
    uint32_t hash = 0;
    // @note: implies that @a end is non-NULL.
    if (start != NULL && start < end)
    {
        size_t len  = end - start;
        // hash the majority of the data in 4-byte chunks.
        while (len >= 4)
        {
            hash    = ROTL32(hash, 7) + start[0];
            hash    = ROTL32(hash, 7) + start[1];
            hash    = ROTL32(hash, 7) + start[2];
            hash    = ROTL32(hash, 7) + start[3];
            start  += 4;
            len    -= 4;
        }
        // hash the remaining 0-3 bytes.
        while (start != end) hash = ROTL32(hash, 7) + *start++;
    }
    return hash;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint32_t hash::hash32_string(char const *str, size_t length_bytes)
{
    char const *end  = str + length_bytes;
    uint32_t    hash = 0;
    // @note: implies that @a end is non-NULL.
    if (str != NULL && str < end)
    {
        size_t len  = end - str;
        // hash the majority of the data in 4-byte chunks.
        while (len >= 4)
        {
            hash    = ROTL32(hash, 7) + str[0];
            hash    = ROTL32(hash, 7) + str[1];
            hash    = ROTL32(hash, 7) + str[2];
            hash    = ROTL32(hash, 7) + str[3];
            str    += 4;
            len    -= 4;
        }
        // hash the remaining 0-3 bytes.
        while (str != end) hash = ROTL32(hash, 7) + *str++;
    }
    return hash;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint32_t hash::hash32(
    void const *data,
    size_t      length,
    uint32_t    seed)
{
    uint8_t  const *key     = (uint8_t  const*) data;
    int      const  nblocks = length / 4;
    uint32_t const *blocks  = (uint32_t const*)(key + nblocks * 4);
    uint8_t  const *tail    = (uint8_t  const*)(key + nblocks * 4);
    uint32_t        h1      = seed;
    uint32_t        c1      = 0xCC9E2D51;
    uint32_t        c2      = 0x1B873593;
    uint32_t        k1      = 0;

    for(int i = -nblocks; i; ++i)
    {
        uint32_t k1 = get_block(blocks, i);
        k1 *= c1;
        k1  = ROTL32(k1,15);
        k1 *= c2;
        h1 ^= k1;
        h1  = ROTL32(h1,13);
        h1  = h1 * 5 + 0xE6546B64;
    }
    switch(length & 3)
    {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] <<  8;
        case 1: k1 ^= tail[0];
                k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
    };
    h1  ^= length;
    h1   = fmix(h1);
    return h1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void hash::hash128_32(
    void const *data,
    size_t      length,
    uint32_t    seed,
    void       *out_hash)
{
    uint8_t  const *key     = (uint8_t  const*) data;
    int      const  nblocks = length / 16;
    uint32_t const *blocks  = (uint32_t const*)(key + nblocks * 16);
    uint8_t  const *tail    = (uint8_t  const*)(key + nblocks * 16);
    uint32_t        h1      = seed;
    uint32_t        h2      = seed;
    uint32_t        h3      = seed;
    uint32_t        h4      = seed;
    uint32_t        c1      = 0x239B961B;
    uint32_t        c2      = 0xAB0E9789;
    uint32_t        c3      = 0x38B34AE5;
    uint32_t        c4      = 0xA1E38B93;
    uint32_t        k1      = 0;
    uint32_t        k2      = 0;
    uint32_t        k3      = 0;
    uint32_t        k4      = 0;

    for(int i = -nblocks; i; i++)
    {
        k1  = get_block(blocks, i * 4 + 0);
        k2  = get_block(blocks, i * 4 + 1);
        k3  = get_block(blocks, i * 4 + 2);
        k4  = get_block(blocks, i * 4 + 3);
        k1 *= c1;
        k1  = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        h1  = ROTL32(h1, 19);
        h1 += h2;
        h1  = h1 * 5 + 0x561CCD1B;
        k2 *= c2;
        k2  = ROTL32(k2, 16);
        k2 *= c3;
        h2 ^= k2;
        h2  = ROTL32(h2, 17);
        h2 += h3;
        h2  = h2 * 5 + 0x0BCAA747;
        k3 *= c3;
        k3  = ROTL32(k3, 17);
        k3 *= c4;
        h3 ^= k3;
        h3  = ROTL32(h3, 15);
        h3 += h4;
        h3  = h3 * 5 + 0x96CD1C35;
        k4 *= c4;
        k4  = ROTL32(k4, 18);
        k4 *= c1;
        h4 ^= k4;
        h4  = ROTL32(h4, 13);
        h4 += h1;
        h4  = h4 * 5 + 0x32AC3B17;
    }
    k1 = k2 = k3 = k4 = 0;
    switch(length & 15)
    {
        case 15: k4 ^= tail[14] << 16;
        case 14: k4 ^= tail[13] <<  8;
        case 13: k4 ^= tail[12] <<  0;
                 k4 *= c4; k4 = ROTL32(k4, 18); k4 *= c1; h4 ^= k4;
        case 12: k3 ^= tail[11] << 24;
        case 11: k3 ^= tail[10] << 16;
        case 10: k3 ^= tail[ 9] <<  8;
        case  9: k3 ^= tail[ 8] <<  0;
                 k3 *= c3; k3 = ROTL32(k3, 17); k3 *= c4; h3 ^= k3;
        case  8: k2 ^= tail[ 7] << 24;
        case  7: k2 ^= tail[ 6] << 16;
        case  6: k2 ^= tail[ 5] <<  8;
        case  5: k2 ^= tail[ 4] <<  0;
                 k2 *= c2; k2 = ROTL32(k2, 16); k2 *= c3; h2 ^= k2;
        case  4: k1 ^= tail[ 3] << 24;
        case  3: k1 ^= tail[ 2] << 16;
        case  2: k1 ^= tail[ 1] <<  8;
        case  1: k1 ^= tail[ 0] <<  0;
                 k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
    };
    h1 ^= (int) length;
    h2 ^= (int) length;
    h3 ^= (int) length;
    h4 ^= (int) length;
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    h1  = fmix(h1);
    h2  = fmix(h2);
    h3  = fmix(h3);
    h4  = fmix(h4);
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    ((uint32_t*)out_hash)[0] = h1;
    ((uint32_t*)out_hash)[1] = h2;
    ((uint32_t*)out_hash)[2] = h3;
    ((uint32_t*)out_hash)[3] = h4;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void hash::hash128_64(
    void const *data,
    size_t      length,
    uint32_t    seed,
    void       *out_hash)
{
    uint8_t  const *key     = (uint8_t  const*) data;
    int      const  nblocks = length / 16;
    uint64_t const *blocks  = (uint64_t const*) key;
    uint8_t  const *tail    = (uint8_t  const*)(key + nblocks * 16);
    uint64_t        h1      = seed;
    uint64_t        h2      = seed;
    uint64_t        c1      = BIG_CONSTANT(0x87C37B91114253D5);
    uint64_t        c2      = BIG_CONSTANT(0x4CF5AD432745937F);
    uint64_t        k1      = 0;
    uint64_t        k2      = 0;

    for(int i = 0; i < nblocks; i++)
    {
        k1  = get_block(blocks, i * 2 + 0);
        k2  = get_block(blocks, i * 2 + 1);
        k1 *= c1;
        k1  = ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;
        h1  = ROTL64(h1, 27);
        h1 += h2;
        h1  = h1 * 5 + 0x52DCE729;
        k2 *= c2;
        k2  = ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;
        h2  = ROTL64(h2, 31);
        h2 += h1;
        h2  = h2 * 5 + 0x38495AB5;
    }
    switch(length & 15)
    {
        case 15: k2 ^= uint64_t(tail[14]) << 48;
        case 14: k2 ^= uint64_t(tail[13]) << 40;
        case 13: k2 ^= uint64_t(tail[12]) << 32;
        case 12: k2 ^= uint64_t(tail[11]) << 24;
        case 11: k2 ^= uint64_t(tail[10]) << 16;
        case 10: k2 ^= uint64_t(tail[ 9]) <<  8;
        case  9: k2 ^= uint64_t(tail[ 8]) <<  0;
                 k2 *= c2; k2 = ROTL64(k2, 33); k2 *= c1; h2 ^= k2;
        case  8: k1 ^= uint64_t(tail[ 7]) << 56;
        case  7: k1 ^= uint64_t(tail[ 6]) << 48;
        case  6: k1 ^= uint64_t(tail[ 5]) << 40;
        case  5: k1 ^= uint64_t(tail[ 4]) << 32;
        case  4: k1 ^= uint64_t(tail[ 3]) << 24;
        case  3: k1 ^= uint64_t(tail[ 2]) << 16;
        case  2: k1 ^= uint64_t(tail[ 1]) <<  8;
        case  1: k1 ^= uint64_t(tail[ 0]) <<  0;
                 k1 *= c1; k1 = ROTL64(k1, 31); k1 *= c2; h1 ^= k1;
    };
    h1 ^= (int) length;
    h2 ^= (int) length;
    h1 += h2;
    h2 += h1;
    h1  = fmix(h1);
    h2  = fmix(h2);
    h1 += h2;
    h2 += h1;
    ((uint64_t*)out_hash)[0] = h1;
    ((uint64_t*)out_hash)[1] = h2;
}

/*/////////////////////////////////////////////////////////////////////////80*/

uint32_t hash::crc32(
    void const *data,
    ptrdiff_t   offset,
    size_t      length,
    uint32_t    seed)
{
    uint32_t crc = seed;
    if (data    != NULL)
    {
        uint8_t const *ptr = ((uint8_t const*) (data)) + offset;
        uint8_t const *end = ptr + length;
        while (ptr != end) crc = CRC[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
    }
    return (crc ^ seed);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool hash::generate_name(char const *name_str, uint32_t *out_name_int)
{
    uint32_t name_hash  = hash::hash32_string(name_str);
    if (name_hash != 0 && out_name_int != NULL)
    {
        *out_name_int   = name_hash;
        return true;
    }
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool hash::generate_name(
    char const *name_str_beg,
    char const *name_str_end,
    uint32_t   *out_name_int)
{
    uint32_t name_hash  = hash::hash32_string(name_str_beg, name_str_end);
    if (name_hash != 0 && out_name_int != NULL)
    {
        *out_name_int   = name_hash;
        return true;
    }
    return false;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t hash::generate_names(
    char const **name_strs,
    uint32_t    *name_ints,
    size_t       name_count,
    size_t       name_offset /* = 0 */)
{
    size_t ngen    = 0;
    if (name_strs != NULL && name_ints != NULL)
    {
        for (size_t nameid = 0; nameid < name_count; ++nameid)
        {
            uint32_t name_hash = hash::hash32_string(name_strs[nameid]);
            if (name_hash != 0)
            {
                // valid name string; search for hash collisions.
                for (size_t i = 0; i  < name_offset; ++i)
                {
                    if (name_ints[i] != name_hash)
                    {
                        // no collision so far; continue searching.
                        continue;
                    }
                    else return ngen; // hash collision; fail.
                }
                // no hash collision, so store the integer name.
                name_ints[name_offset++] = name_hash; ++ngen;
            }
            else break; // invalid name string (NULL or empty string.)
        }
    }
    return ngen;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
