/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libmath.cpp
///  Implements a set of mathematics functions for performing comparisons
///  between floating point numbers, interpolating scalar quantities,
///  performing 2/3/4 component vector operations, and working with quaternions
///  and 4x4 matrices. The emphasis is on correctness and usability, not
///  performance.
///
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <math.h>
#include <float.h>
#include <assert.h>
#include "libmath.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

static const uint32_t F32_MaskSgn  = 0x80000000; /* mask sign bit      */
static const uint32_t F32_MaskExp  = 0x7F800000; /* mask exponent bits */
static const uint32_t F32_MaskMnt  = 0x007FFFFF; /* mask mantissa bits */
static const uint32_t F32_PZero    = 0x00000000;
static const uint32_t F32_NZero    = 0x80000000;
static const uint32_t F32_PInf     = 0x7F800000;
static const uint32_t F32_NInf     = 0xFF800000;
static const uint32_t F32_QNaN     = 0x7FC00000;
static const uint32_t F32_SNaN     = 0x7F800001;

/*/////////////////////////////////////////////////////////////////////////80*/

float math::min2(float a, float b)
{
    return (a < b ? a : b);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::max2(float a, float b)
{
    return (a < b ? b : a);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::min3(float a, float b, float c)
{
    return (a < b ? (a < c ? a : c) : (b < c ? b : c));
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::max3(float a, float b, float c)
{
    return (a > b ? (a > c ? a : c) : (b > c ? b : c));
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::eq(float a, float b)
{
    return (fabsf(a-b) <= (FLT_EPSILON * math::max2(fabsf(a), fabsf(b))));
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::eq_abs(float a, float b, float tol)
{
    return (fabsf(a-b) <= tol);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::eq_rel(float a, float b, float tol)
{
    return (fabsf(a-b) <= (tol * math::max2(fabsf(a), fabsf(b))));
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::eq_com(float a, float b, float t_a, float t_r)
{
    return (fabsf(a-b) <= math::max2(t_a, t_r*math::max2(fabsf(a), fabsf(b))));
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::is_nan(float a)
{
    const uint32_t   mask   = 0xFFC00000;  // sign + exponent
    const uint32_t   snan   = 0x7FC00000;  // all exponent + top-most mantissa
    uint32_t         value  = *(uint32_t*) &a;
    return ((value & mask) == snan);
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::is_inf(float a)
{
    const uint32_t   mask    = 0x7FFFFFFF;  // all exponent + all mantissa
    const uint32_t   inf     = 0x7F800000;  // all exponent; no mantissa
    uint32_t         value   = *(uint32_t*) &a;
    return ((value & mask)  == inf);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::rcp(float a)
{
    return 1.0f / a;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::rad(float degrees)
{
    // degrees * (PI/180)
    return (degrees * 0.017453292519943295769236907684886f);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::deg(float radians)
{
    // radians * (180/PI)
    return (radians * 57.29577951308232087679815481410500f);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::linear(float a, float b, float t)
{
    return (a + ((b - a) * t));
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::bezier(
    float a,
    float b,
    float in_t,
    float out_t,
    float t)
{
    float d  =  b - a;
    float a2 = (d * 3.0f) - (in_t + (out_t * 2.0f));
    float a3 =   out_t    +  in_t - (d  * 2.0f);
    return a + ((out_t    +  (a2  + (a3 * t)) * t) * t);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::hermite(
    float a,
    float b,
    float in_t,
    float out_t,
    float t)
{
    float t2 =  t  * t;
    float t3 =  t2 * t;
    return ((+2.0f * t3   - 3.0f * t2  + 1.0f) * a     +
            (-2.0f * t3   + 3.0f * t2)         * b     +
            (  t3  - 2.0f *   t2 +  t)         * out_t +
            (  t3  -  t2)                      * in_t);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_xy(float *dst_xy, float x, float y)
{
    dst_xy[0] = x;
    dst_xy[1] = y;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_vec2(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy)
{
    dst_xy[0] = src_xy[0];
    dst_xy[1] = src_xy[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_vec3(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xyz)
{
    dst_xy[0] = src_xyz[0];
    dst_xy[1] = src_xyz[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_vec4(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xyzw)
{
    dst_xy[0] = src_xyzw[0];
    dst_xy[1] = src_xyzw[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_xyz(float *dst_xyz, float x, float y, float z)
{
    dst_xyz[0] = x;
    dst_xyz[1] = y;
    dst_xyz[2] = z;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_vec2(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xy,
    float                      z)
{
    dst_xyz[0] = src_xy[0];
    dst_xyz[1] = src_xy[1];
    dst_xyz[2] = z;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_vec3(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz)
{
    dst_xyz[0] = src_xyz[0];
    dst_xyz[1] = src_xyz[1];
    dst_xyz[2] = src_xyz[2];
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_vec4(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyzw)
{
    dst_xyz[0] = src_xyzw[0];
    dst_xyz[1] = src_xyzw[1];
    dst_xyz[2] = src_xyzw[2];
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set(float *dst_xyzw, float x, float y, float z, float w)
{
    dst_xyzw[0] = x;
    dst_xyzw[1] = y;
    dst_xyzw[2] = z;
    dst_xyzw[3] = w;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_vec2(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xy,
    float         z,
    float         w)
{
    dst_xyzw[0] = src_xy[0];
    dst_xyzw[1] = src_xy[1];
    dst_xyzw[2] = z;
    dst_xyzw[3] = w;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_vec3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyz,
    float                      w)
{
    dst_xyzw[0] = src_xyz[0];
    dst_xyzw[1] = src_xyz[1];
    dst_xyzw[2] = src_xyz[2];
    dst_xyzw[3] = w;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_vec4(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw)
{
    dst_xyzw[0] = src_xyzw[0];
    dst_xyzw[1] = src_xyzw[1];
    dst_xyzw[2] = src_xyzw[2];
    dst_xyzw[3] = src_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_nan(float *dst_xy)
{
    float qnan = *(float*) &F32_QNaN;
    dst_xy[0]  = qnan;
    dst_xy[1]  = qnan;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_nan(float *dst_xyz)
{
    float qnan = *(float*) &F32_QNaN;
    dst_xyz[0] = qnan;
    dst_xyz[1] = qnan;
    dst_xyz[2] = qnan;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_nan(float *dst_xyzw)
{
    float qnan  = *(float*) &F32_QNaN;
    dst_xyzw[0] = qnan;
    dst_xyzw[1] = qnan;
    dst_xyzw[2] = qnan;
    dst_xyzw[3] = qnan;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_one(float *dst_xy)
{
    dst_xy[0] = 1.0f;
    dst_xy[1] = 1.0f;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_one(float *dst_xyz)
{
    dst_xyz[0] = 1.0f;
    dst_xyz[1] = 1.0f;
    dst_xyz[2] = 1.0f;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_one(float *dst_xyzw)
{
    dst_xyzw[0] = 1.0f;
    dst_xyzw[1] = 1.0f;
    dst_xyzw[2] = 1.0f;
    dst_xyzw[3] = 1.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_zero(float *dst_xy)
{
    dst_xy[0] = 0.0f;
    dst_xy[1] = 0.0f;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_zero(float *dst_xyz)
{
    dst_xyz[0] = 0.0f;
    dst_xyz[1] = 0.0f;
    dst_xyz[2] = 0.0f;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_zero(float *dst_xyzw)
{
    dst_xyzw[0] = 0.0f;
    dst_xyzw[1] = 0.0f;
    dst_xyzw[2] = 0.0f;
    dst_xyzw[3] = 0.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_ninf(float *dst_xy)
{
    float ninf = *(float*) &F32_NInf;
    dst_xy[0]  = ninf;
    dst_xy[1]  = ninf;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_ninf(float *dst_xyz)
{
    float ninf = *(float*) &F32_NInf;
    dst_xyz[0] = ninf;
    dst_xyz[1] = ninf;
    dst_xyz[2] = ninf;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_ninf(float *dst_xyzw)
{
    float ninf  = *(float*) &F32_NInf;
    dst_xyzw[0] = ninf;
    dst_xyzw[1] = ninf;
    dst_xyzw[2] = ninf;
    dst_xyzw[3] = ninf;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_pinf(float *dst_xy)
{
    float pinf = *(float*) &F32_PInf;
    dst_xy[0]  = pinf;
    dst_xy[1]  = pinf;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_pinf(float *dst_xyz)
{
    float pinf = *(float*) &F32_PInf;
    dst_xyz[0] = pinf;
    dst_xyz[1] = pinf;
    dst_xyz[2] = pinf;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_pinf(float *dst_xyzw)
{
    float pinf  = *(float*) &F32_PInf;
    dst_xyzw[0] = pinf;
    dst_xyzw[1] = pinf;
    dst_xyzw[2] = pinf;
    dst_xyzw[3] = pinf;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_unit_x(float *dst_xy)
{
    dst_xy[0] = 1.0f;
    dst_xy[1] = 0.0f;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_unit_x(float *dst_xyz)
{
    dst_xyz[0] = 1.0f;
    dst_xyz[1] = 0.0f;
    dst_xyz[2] = 0.0f;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_unit_x(float *dst_xyzw)
{
    dst_xyzw[0] = 1.0f;
    dst_xyzw[1] = 0.0f;
    dst_xyzw[2] = 0.0f;
    dst_xyzw[3] = 0.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_set_unit_y(float *dst_xy)
{
    dst_xy[0] = 0.0f;
    dst_xy[1] = 1.0f;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_unit_y(float *dst_xyz)
{
    dst_xyz[0] = 0.0f;
    dst_xyz[1] = 1.0f;
    dst_xyz[2] = 0.0f;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_unit_y(float *dst_xyzw)
{
    dst_xyzw[0] = 0.0f;
    dst_xyzw[1] = 1.0f;
    dst_xyzw[2] = 0.0f;
    dst_xyzw[3] = 0.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_set_unit_z(float *dst_xyz)
{
    dst_xyz[0] = 0.0f;
    dst_xyz[1] = 0.0f;
    dst_xyz[2] = 1.0f;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_unit_z(float *dst_xyzw)
{
    dst_xyzw[0] = 0.0f;
    dst_xyzw[1] = 0.0f;
    dst_xyzw[2] = 1.0f;
    dst_xyzw[3] = 0.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_set_unit_w(float *dst_xyzw)
{
    dst_xyzw[0] = 0.0f;
    dst_xyzw[1] = 0.0f;
    dst_xyzw[2] = 0.0f;
    dst_xyzw[3] = 1.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::vec2_eq(
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy)
{
    if (!math::eq(a_xy[0], b_xy[0])) return false;
    if (!math::eq(a_xy[1], b_xy[1])) return false;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::vec3_eq(
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz)
{
    if (!math::eq(a_xyz[0], b_xyz[0])) return false;
    if (!math::eq(a_xyz[1], b_xyz[1])) return false;
    if (!math::eq(a_xyz[2], b_xyz[2])) return false;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool math::vec4_eq(
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    if (!math::eq(a_xyzw[0], b_xyzw[0])) return false;
    if (!math::eq(a_xyzw[1], b_xyzw[1])) return false;
    if (!math::eq(a_xyzw[2], b_xyzw[2])) return false;
    if (!math::eq(a_xyzw[3], b_xyzw[3])) return false;
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_add(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy)
{
    dst_xy[0] = a_xy[0] + b_xy[0];
    dst_xy[1] = a_xy[1] + b_xy[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_add(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz)
{
    dst_xyz[0] = a_xyz[0] + b_xyz[0];
    dst_xyz[1] = a_xyz[1] + b_xyz[1];
    dst_xyz[2] = a_xyz[2] + b_xyz[2];
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_add(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    dst_xyzw[0] = a_xyzw[0] + b_xyzw[0];
    dst_xyzw[1] = a_xyzw[1] + b_xyzw[1];
    dst_xyzw[2] = a_xyzw[2] + b_xyzw[2];
    dst_xyzw[3] = a_xyzw[3] + b_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_sub(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy)
{
    dst_xy[0] = a_xy[0] - b_xy[0];
    dst_xy[1] = a_xy[1] - b_xy[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_sub(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz)
{
    dst_xyz[0] = a_xyz[0] - b_xyz[0];
    dst_xyz[1] = a_xyz[1] - b_xyz[1];
    dst_xyz[2] = a_xyz[2] - b_xyz[2];
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_sub(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    dst_xyzw[0] = a_xyzw[0] - b_xyzw[0];
    dst_xyzw[1] = a_xyzw[1] - b_xyzw[1];
    dst_xyzw[2] = a_xyzw[2] - b_xyzw[2];
    dst_xyzw[3] = a_xyzw[3] - b_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_mul(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy)
{
    dst_xy[0] = a_xy[0] * b_xy[0];
    dst_xy[1] = a_xy[1] * b_xy[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_mul(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz)
{
    dst_xyz[0] = a_xyz[0] * b_xyz[0];
    dst_xyz[1] = a_xyz[1] * b_xyz[1];
    dst_xyz[2] = a_xyz[2] * b_xyz[2];
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_mul(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    dst_xyzw[0] = a_xyzw[0] * b_xyzw[0];
    dst_xyzw[1] = a_xyzw[1] * b_xyzw[1];
    dst_xyzw[2] = a_xyzw[2] * b_xyzw[2];
    dst_xyzw[3] = a_xyzw[3] * b_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_div(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy)
{
    dst_xy[0] = a_xy[0] / b_xy[0];
    dst_xy[1] = a_xy[1] / b_xy[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_div(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz)
{
    dst_xyz[0] = a_xyz[0] / b_xyz[0];
    dst_xyz[1] = a_xyz[1] / b_xyz[1];
    dst_xyz[2] = a_xyz[2] / b_xyz[2];
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_div(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    dst_xyzw[0] = a_xyzw[0] / b_xyzw[0];
    dst_xyzw[1] = a_xyzw[1] / b_xyzw[1];
    dst_xyzw[2] = a_xyzw[2] / b_xyzw[2];
    dst_xyzw[3] = a_xyzw[3] / b_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_scl(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float                      b)
{
    dst_xy[0] = a_xy[0] * b;
    dst_xy[1] = a_xy[1] * b;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_scl(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float                      b)
{
    dst_xyz[0] = a_xyz[0] * b;
    dst_xyz[1] = a_xyz[1] * b;
    dst_xyz[2] = a_xyz[2] * b;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_scl(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float                      b)
{
    dst_xyzw[0] = a_xyzw[0] * b;
    dst_xyzw[1] = a_xyzw[1] * b;
    dst_xyzw[2] = a_xyzw[2] * b;
    dst_xyzw[3] = a_xyzw[3] * b;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_scl3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float                      b)
{
    dst_xyzw[0] = a_xyzw[0] * b;
    dst_xyzw[1] = a_xyzw[1] * b;
    dst_xyzw[2] = a_xyzw[2] * b;
    dst_xyzw[3] = a_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_neg(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy)
{
    dst_xy[0] = -src_xy[0];
    dst_xy[1] = -src_xy[1];
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_neg(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz)
{
    dst_xyz[0]  = -src_xyz[0];
    dst_xyz[1]  = -src_xyz[1];
    dst_xyz[2]  = -src_xyz[2];
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_neg(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw)
{
    dst_xyzw[0] = -src_xyzw[0];
    dst_xyzw[1] = -src_xyzw[1];
    dst_xyzw[2] = -src_xyzw[2];
    dst_xyzw[3] = -src_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_neg3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw)
{
    dst_xyzw[0] = -src_xyzw[0];
    dst_xyzw[1] = -src_xyzw[1];
    dst_xyzw[2] = -src_xyzw[2];
    dst_xyzw[3] =  src_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec2_dot(
    float                      &dst,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy)
{
    dst = (a_xy[0] * b_xy[0] + a_xy[1] * b_xy[1]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec3_dot(
    float                      &dst,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz)
{
    dst = (a_xyz[0] * b_xyz[0] + a_xyz[1] * b_xyz[1] + a_xyz[2] * b_xyz[2]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec4_dot(
    float                      &dst,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    dst =
        (a_xyzw[0] * b_xyzw[0] +
         a_xyzw[1] * b_xyzw[1] +
         a_xyzw[2] * b_xyzw[2] +
         a_xyzw[3] * b_xyzw[3]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec4_dot3(
    float                      &dst,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    dst =
        (a_xyzw[0] * b_xyzw[0] +
         a_xyzw[1] * b_xyzw[1] +
         a_xyzw[2] * b_xyzw[2]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec2_len(float &dst, float const *a_xy)
{
    dst  = sqrtf(a_xy[0] * a_xy[0] + a_xy[1] * a_xy[1]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec3_len(float &dst, float const *a_xyz)
{
    dst = sqrtf(
        a_xyz[0] * a_xyz[0] +
        a_xyz[1] * a_xyz[1] +
        a_xyz[2] * a_xyz[2]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec4_len(float &dst, float const *a_xyzw)
{
    dst = sqrtf(
        a_xyzw[0] * a_xyzw[0] +
        a_xyzw[1] * a_xyzw[1] +
        a_xyzw[2] * a_xyzw[2] +
        a_xyzw[3] * a_xyzw[3]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec4_len3(float &dst, float const *a_xyzw)
{
    dst = sqrtf(
        a_xyzw[0] * a_xyzw[0] +
        a_xyzw[1] * a_xyzw[1] +
        a_xyzw[2] * a_xyzw[2]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec2_len_sq(float &dst, float const *a_xy)
{
    dst = (a_xy[0] * a_xy[0] + a_xy[1] * a_xy[1]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec3_len_sq(float &dst, float const *a_xyz)
{
    dst = (a_xyz[0] * a_xyz[0] + a_xyz[1] * a_xyz[1] + a_xyz[2] * a_xyz[2]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec4_len_sq(float &dst, float const *a_xyzw)
{
    dst =
        (a_xyzw[0] * a_xyzw[0] +
         a_xyzw[1] * a_xyzw[1] +
         a_xyzw[2] * a_xyzw[2] +
         a_xyzw[3] * a_xyzw[3]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::vec4_len3_sq(float &dst, float const *a_xyzw)
{
    dst =
        (a_xyzw[0] * a_xyzw[0] +
         a_xyzw[1] * a_xyzw[1] +
         a_xyzw[2] * a_xyzw[2]);
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_nrm(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy)
{
    float len;
    if (!math::equal(math::vec2_len(len, src_xy), 0))
    {
        float rcp = 1.0f / len;
        dst_xy[0]  = src_xy[0] * rcp;
        dst_xy[1]  = src_xy[1] * rcp;
        return dst_xy;
    }
    else return math::vec2_set_pinf(dst_xy);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_nrm(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz)
{
    float len;
    if (!math::equal(math::vec3_len(len, src_xyz), 0))
    {
        float rcp = 1.0f / len;
        dst_xyz[0]  = src_xyz[0] * rcp;
        dst_xyz[1]  = src_xyz[1] * rcp;
        dst_xyz[2]  = src_xyz[2] * rcp;
        return dst_xyz;
    }
    else return math::vec3_set_pinf(dst_xyz);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_nrm(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw)
{
    float len;
    if (!math::equal(math::vec4_len(len, src_xyzw), 0))
    {
        float rcp = 1.0f / len;
        dst_xyzw[0]  = src_xyzw[0] * rcp;
        dst_xyzw[1]  = src_xyzw[1] * rcp;
        dst_xyzw[2]  = src_xyzw[2] * rcp;
        dst_xyzw[3]  = src_xyzw[3] * rcp;
        return dst_xyzw;
    }
    else return math::vec4_set_pinf(dst_xyzw);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_nrm3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw)
{
    float len;
    if (!math::equal(math::vec4_len3(len, src_xyzw), 0))
    {
        float rcp = 1.0f / len;
        dst_xyzw[0]  = src_xyzw[0] * rcp;
        dst_xyzw[1]  = src_xyzw[1] * rcp;
        dst_xyzw[2]  = src_xyzw[2] * rcp;
        dst_xyzw[3]  = src_xyzw[3];
        return dst_xyzw;
    }
    else
    {
        math::vec3_set_pinf(dst_xyzw);
        dst_xyzw[3]  = src_xyzw[3];
        return dst_xyzw;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_perp(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy)
{
    float x  = src_xy[0];
    float y  = src_xy[1];
    dst_xy[0] = -y;
    dst_xy[1] =  x;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_cross(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz)
{
    float ax   = a_xyz[0], ay = a_xyz[1], az = a_xyz[2];
    float bx   = b_xyz[0], by = b_xyz[1], bz = b_xyz[2];
    dst_xyz[0] = ay * bz - az * by;
    dst_xyz[1] = az * bx - ax * bz;
    dst_xyz[2] = ax * by - ay * bx;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_cross(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw)
{
    float ax    = a_xyzw[0], ay = a_xyzw[1], az = a_xyzw[2];
    float bx    = b_xyzw[0], by = b_xyzw[1], bz = b_xyzw[2];
    dst_xyzw[0] = ay * bz  - az * by;
    dst_xyzw[1] = az * bx  - ax * bz;
    dst_xyzw[2] = ax * by  - ay * bx;
    dst_xyzw[3] = 0.0f; // cross product always results in a vector
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_swizzle(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy,
    size_t                     x,
    size_t                     y)
{
    float a   = src_xy[x];
    float b   = src_xy[y];
    dst_xy[0] = a;
    dst_xy[1] = b;
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_swizzle(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz,
    size_t                     x,
    size_t                     y,
    size_t                     z)
{
    float a    = src_xyz[x];
    float b    = src_xyz[y];
    float c    = src_xyz[z];
    dst_xyz[0] = a;
    dst_xyz[1] = b;
    dst_xyz[2] = c;
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_swizzle(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw,
    size_t                     x,
    size_t                     y,
    size_t                     z,
    size_t                     w)
{
    float a     = src_xyzw[x];
    float b     = src_xyzw[y];
    float c     = src_xyzw[z];
    float d     = src_xyzw[w];
    dst_xyzw[0] = a;
    dst_xyzw[1] = b;
    dst_xyzw[2] = c;
    dst_xyzw[3] = d;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_linear(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy,
    float                      t)
{
    dst_xy[0] = math::linear(a_xy[0], b_xy[0], t);
    dst_xy[1] = math::linear(a_xy[1], b_xy[1], t);
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_linear(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz,
    float                      t)
{
    dst_xyz[0] = math::linear(a_xyz[0], b_xyz[0], t);
    dst_xyz[1] = math::linear(a_xyz[1], b_xyz[1], t);
    dst_xyz[2] = math::linear(a_xyz[2], b_xyz[2], t);
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_linear(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float                      t)
{
    dst_xyzw[0] = math::linear(a_xyzw[0], b_xyzw[0], t);
    dst_xyzw[1] = math::linear(a_xyzw[1], b_xyzw[1], t);
    dst_xyzw[2] = math::linear(a_xyzw[2], b_xyzw[2], t);
    dst_xyzw[3] = math::linear(a_xyzw[3], b_xyzw[3], t);
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_linear3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float                      t)
{
    dst_xyzw[0] = math::linear(a_xyzw[0], b_xyzw[0], t);
    dst_xyzw[1] = math::linear(a_xyzw[1], b_xyzw[1], t);
    dst_xyzw[2] = math::linear(a_xyzw[2], b_xyzw[2], t);
    dst_xyzw[3] = a_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_bezier(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy,
    float const * CMN_RESTRICT itan_xy,
    float const * CMN_RESTRICT otan_xy,
    float                      t)
{
    dst_xy[0] = math::bezier(a_xy[0], b_xy[0], itan_xy[0], otan_xy[0], t);
    dst_xy[1] = math::bezier(a_xy[1], b_xy[1], itan_xy[1], otan_xy[1], t);
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_bezier(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t)
{
    dst_xyz[0] = math::bezier(a_xyz[0], b_xyz[0], itan_xyz[0], otan_xyz[0], t);
    dst_xyz[1] = math::bezier(a_xyz[1], b_xyz[1], itan_xyz[1], otan_xyz[1], t);
    dst_xyz[2] = math::bezier(a_xyz[2], b_xyz[2], itan_xyz[2], otan_xyz[2], t);
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_bezier(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyzw,
    float const * CMN_RESTRICT otan_xyzw,
    float                      t)
{
    dst_xyzw[0] = math::bezier(a_xyzw[0], b_xyzw[0], itan_xyzw[0], otan_xyzw[0], t);
    dst_xyzw[1] = math::bezier(a_xyzw[1], b_xyzw[1], itan_xyzw[1], otan_xyzw[1], t);
    dst_xyzw[2] = math::bezier(a_xyzw[2], b_xyzw[2], itan_xyzw[2], otan_xyzw[2], t);
    dst_xyzw[3] = math::bezier(a_xyzw[3], b_xyzw[3], itan_xyzw[3], otan_xyzw[3], t);
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_bezier3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t)
{
    dst_xyzw[0] = math::bezier(a_xyzw[0], b_xyzw[0], itan_xyz[0], otan_xyz[0], t);
    dst_xyzw[1] = math::bezier(a_xyzw[1], b_xyzw[1], itan_xyz[1], otan_xyz[1], t);
    dst_xyzw[2] = math::bezier(a_xyzw[2], b_xyzw[2], itan_xyz[2], otan_xyz[2], t);
    dst_xyzw[3] = a_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec2_hermite(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy,
    float const * CMN_RESTRICT itan_xy,
    float const * CMN_RESTRICT otan_xy,
    float                      t)
{
    dst_xy[0] = math::hermite(a_xy[0], b_xy[0], itan_xy[0], otan_xy[0], t);
    dst_xy[1] = math::hermite(a_xy[1], b_xy[1], itan_xy[1], otan_xy[1], t);
    return dst_xy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec3_hermite(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t)
{
    dst_xyz[0] = math::hermite(a_xyz[0], b_xyz[0], itan_xyz[0], otan_xyz[0], t);
    dst_xyz[1] = math::hermite(a_xyz[1], b_xyz[1], itan_xyz[1], otan_xyz[1], t);
    dst_xyz[2] = math::hermite(a_xyz[2], b_xyz[2], itan_xyz[2], otan_xyz[2], t);
    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_hermite(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyzw,
    float const * CMN_RESTRICT otan_xyzw,
    float                      t)
{
    dst_xyzw[0] = math::hermite(a_xyzw[0], b_xyzw[0], itan_xyzw[0], otan_xyzw[0], t);
    dst_xyzw[1] = math::hermite(a_xyzw[1], b_xyzw[1], itan_xyzw[1], otan_xyzw[1], t);
    dst_xyzw[2] = math::hermite(a_xyzw[2], b_xyzw[2], itan_xyzw[2], otan_xyzw[2], t);
    dst_xyzw[3] = math::hermite(a_xyzw[3], b_xyzw[3], itan_xyzw[3], otan_xyzw[3], t);
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::vec4_hermite3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t)
{
    dst_xyzw[0] = math::hermite(a_xyzw[0], b_xyzw[0], itan_xyz[0], otan_xyz[0], t);
    dst_xyzw[1] = math::hermite(a_xyzw[1], b_xyzw[1], itan_xyz[1], otan_xyz[1], t);
    dst_xyzw[2] = math::hermite(a_xyzw[2], b_xyzw[2], itan_xyz[2], otan_xyz[2], t);
    dst_xyzw[3] = a_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set(float *dst_xyzw, float x, float y, float z, float w)
{
    dst_xyzw[0] = x;
    dst_xyzw[1] = y;
    dst_xyzw[2] = z;
    dst_xyzw[3] = w;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set_quat(float *dst_xyzw, float const *src_xyzw)
{
    dst_xyzw[0] = src_xyzw[0];
    dst_xyzw[1] = src_xyzw[1];
    dst_xyzw[2] = src_xyzw[2];
    dst_xyzw[3] = src_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set_nan(float *dst_xyzw)
{
    float qnan = *(float*) &F32_QNaN;
    dst_xyzw[0] = qnan;
    dst_xyzw[1] = qnan;
    dst_xyzw[2] = qnan;
    dst_xyzw[3] = qnan;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set_one(float *dst_xyzw)
{
    dst_xyzw[0] = 1.0f;
    dst_xyzw[1] = 1.0f;
    dst_xyzw[2] = 1.0f;
    dst_xyzw[3] = 1.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set_zero(float *dst_xyzw)
{
    dst_xyzw[0] = 0.0f;
    dst_xyzw[1] = 0.0f;
    dst_xyzw[2] = 0.0f;
    dst_xyzw[3] = 0.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set_negative_infinity(float *dst_xyzw)
{
    float ninf = *(float*) &F32_NInf;
    dst_xyzw[0] = ninf;
    dst_xyzw[1] = ninf;
    dst_xyzw[2] = ninf;
    dst_xyzw[3] = ninf;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set_positive_infinity(float *dst_xyzw)
{
    float pinf = *(float*) &F32_PInf;
    dst_xyzw[0] = pinf;
    dst_xyzw[1] = pinf;
    dst_xyzw[2] = pinf;
    dst_xyzw[3] = pinf;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_set_identity(float *dst_xyzw)
{
    dst_xyzw[0] = 0.0f;
    dst_xyzw[1] = 0.0f;
    dst_xyzw[2] = 0.0f;
    dst_xyzw[3] = 1.0f;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t math::quat_equal(float const *a_xyzw, float const *b_xyzw)
{
    if (!math::equal(a_xyzw[0], b_xyzw[0]))
        return 0;
    if (!math::equal(a_xyzw[1], b_xyzw[1]))
        return 0;
    if (!math::equal(a_xyzw[2], b_xyzw[2]))
        return 0;
    if (!math::equal(a_xyzw[3], b_xyzw[3]))
        return 0;

    return 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_add(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw)
{
    dst_xyzw[0] = a_xyzw[0] + b_xyzw[0];
    dst_xyzw[1] = a_xyzw[1] + b_xyzw[1];
    dst_xyzw[2] = a_xyzw[2] + b_xyzw[2];
    dst_xyzw[3] = a_xyzw[3] + b_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_sub(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw)
{
    dst_xyzw[0] = a_xyzw[0] - b_xyzw[0];
    dst_xyzw[1] = a_xyzw[1] - b_xyzw[1];
    dst_xyzw[2] = a_xyzw[2] - b_xyzw[2];
    dst_xyzw[3] = a_xyzw[3] - b_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_mul(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw)
{
    float ax = a_xyzw[0], ay = a_xyzw[1], az = a_xyzw[2], aw = a_xyzw[3];
    float bx = b_xyzw[0], by = b_xyzw[1], bz = b_xyzw[2], bw = b_xyzw[3];
    dst_xyzw[0] = ((aw * bx) + (ax * bw) + (ay * bz) - (az * by));
    dst_xyzw[1] = ((aw * by) - (ax * bz) + (ay * bw) + (az * bx));
    dst_xyzw[2] = ((aw * bz) + (ax * by) - (ay * bx) + (az * bw));
    dst_xyzw[3] = ((aw * bw) - (ax * bx) - (ay * by) - (az * bz));
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_scale(float *dst_xyzw, float const *a_xyzw, float b)
{
    dst_xyzw[0] = a_xyzw[0] * b;
    dst_xyzw[1] = a_xyzw[1] * b;
    dst_xyzw[2] = a_xyzw[2] * b;
    dst_xyzw[3] = a_xyzw[3] * b;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_scale3(float *dst_xyzw, float const *a_xyzw, float b)
{
    dst_xyzw[0] = a_xyzw[0] * b;
    dst_xyzw[1] = a_xyzw[1] * b;
    dst_xyzw[2] = a_xyzw[2] * b;
    dst_xyzw[3] = a_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_negate(float *dst_xyzw, float const *src_xyzw)
{
    dst_xyzw[0] = -src_xyzw[0];
    dst_xyzw[1] = -src_xyzw[1];
    dst_xyzw[2] = -src_xyzw[2];
    dst_xyzw[3] = -src_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_negate3(float *dst_xyzw, float const *src_xyzw)
{
    dst_xyzw[0] = -src_xyzw[0];
    dst_xyzw[1] = -src_xyzw[1];
    dst_xyzw[2] = -src_xyzw[2];
    dst_xyzw[3] =  src_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_conjugate(float *dst_xyzw, float const *src_xyzw)
{
    dst_xyzw[0] = -src_xyzw[0];
    dst_xyzw[1] = -src_xyzw[1];
    dst_xyzw[2] = -src_xyzw[2];
    dst_xyzw[3] =  src_xyzw[3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::quat_dot(float const *a_xyzw, float const *b_xyzw)
{
    // equivalent to selection(a * conjugate(b))
    return
        (a_xyzw[0] * b_xyzw[0] +
         a_xyzw[1] * b_xyzw[1] +
         a_xyzw[2] * b_xyzw[2] +
         a_xyzw[3] * b_xyzw[3]);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::quat_norm(float const *src_xyzw)
{
    return
        (src_xyzw[0] * src_xyzw[0] +
         src_xyzw[1] * src_xyzw[1] +
         src_xyzw[2] * src_xyzw[2] +
         src_xyzw[3] * src_xyzw[3]);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::quat_length(float const *src_xyzw)
{
    return sqrtf(
        src_xyzw[0] * src_xyzw[0] +
        src_xyzw[1] * src_xyzw[1] +
        src_xyzw[2] * src_xyzw[2] +
        src_xyzw[3] * src_xyzw[3]);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::quat_length_sq(float const *src_xyzw)
{
    return
        (src_xyzw[0] * src_xyzw[0] +
         src_xyzw[1] * src_xyzw[1] +
         src_xyzw[2] * src_xyzw[2] +
         src_xyzw[3] * src_xyzw[3]);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float math::quat_selection(float const *src_xyzw)
{
    return src_xyzw[3];
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_inverse(float *dst_xyzw, float const *src_xyzw)
{
    float norm = math::quat_norm(src_xyzw);
    if (!math::equal(norm, 0))
    {
        float rcp  = 1.0f / norm;
        dst_xyzw[0] = -src_xyzw[0] * rcp;
        dst_xyzw[1] = -src_xyzw[1] * rcp;
        dst_xyzw[2] = -src_xyzw[2] * rcp;
        dst_xyzw[3] =  src_xyzw[3] * rcp;
        return dst_xyzw;
    }
    else
    {
        math::quat_set_positive_infinity(dst_xyzw);
        return dst_xyzw;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_normalize(float *dst_xyzw, float const *src_xyzw)
{
    float length = math::quat_length(src_xyzw);
    if (!math::equal(length, 0))
    {
        float rcp  = 1.0f / length;
        dst_xyzw[0] = src_xyzw[0] * rcp;
        dst_xyzw[1] = src_xyzw[1] * rcp;
        dst_xyzw[2] = src_xyzw[2] * rcp;
        dst_xyzw[3] = src_xyzw[3] * rcp;
        return dst_xyzw;
    }
    else
    {
        math::quat_set_positive_infinity(dst_xyzw);
        return dst_xyzw;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_exp(float *dst_xyzw, float const *src_xyzw)
{
    float ax = src_xyzw[0], ay = src_xyzw[1], az = src_xyzw[2];
    float t  = sqrtf(ax * ax + ay * ay + az * az);
    float st = sinf(t);
    float w  = cosf(t);

    if (math::equal(st, 0))
    {
        dst_xyzw[0] = ax;
        dst_xyzw[1] = ay;
        dst_xyzw[2] = az;
        dst_xyzw[3] = w;
        return dst_xyzw;
    }
    else
    {
        float c    = st / t;
        dst_xyzw[0] = ax * c;
        dst_xyzw[1] = ay * c;
        dst_xyzw[2] = az * c;
        dst_xyzw[3] = w;
        return dst_xyzw;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_log(float *dst_xyzw, float const *src_xyzw)
{
    float ax = src_xyzw[0], ay = src_xyzw[1], az = src_xyzw[2], aw = src_xyzw[3];

    dst_xyzw[0] = ax;
    dst_xyzw[1] = ay;
    dst_xyzw[2] = az;
    dst_xyzw[3] = 0;

    if (fabsf(aw) < 1.0f)
    {
        float t   = acosf(aw);
        float st  = sinf(t);
        if (!math::equal(st, 0))
        {
            float c    = t  / st;
            dst_xyzw[0] = ax * c;
            dst_xyzw[1] = ay * c;
            dst_xyzw[2] = az * c;
        }
    }
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_closest(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw)
{
    if (math::quat_dot(a_xyzw, b_xyzw) < 0.0f)
    {
        return math::quat_negate(dst_xyzw, a_xyzw);
    }
    else
    {
        dst_xyzw[0] = a_xyzw[0];
        dst_xyzw[1] = a_xyzw[1];
        dst_xyzw[2] = a_xyzw[2];
        dst_xyzw[3] = a_xyzw[3];
        return dst_xyzw;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_vec3(
    float       *dst_xyzw,
    float const *from_xyz,
    float const *to_xyz)
{
    float unitFrom[3];
    float unitTo[3];
    float xFromTo[3];
    float dpFromTo;
    float rcpScale;

    math::vec3_normalize(unitFrom, from_xyz);
    math::vec3_normalize(unitTo,   to_xyz);
    math::vec3_cross(xFromTo, unitFrom, unitTo);
    dpFromTo   = math::vec3_dot(unitFrom, unitTo);
    rcpScale   = 1.0f / (sqrtf((1.0f + dpFromTo) * 2.0f));
    dst_xyzw[0] = xFromTo[0] * rcpScale;
    dst_xyzw[1] = xFromTo[1] * rcpScale;
    dst_xyzw[2] = xFromTo[2] * rcpScale;
    dst_xyzw[3] = 0.5f       * rcpScale;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_vec4(
    float       *dst_xyzw,
    float const *from_xyzw,
    float const *to_xyzw)
{
    float unitFrom[3];
    float unitTo[3];
    float xFromTo[3];
    float dpFromTo;
    float rcpScale;

    math::vec3_normalize(unitFrom, from_xyzw);
    math::vec3_normalize(unitTo,   to_xyzw);
    math::vec3_cross(xFromTo, unitFrom, unitTo);
    dpFromTo   = math::vec3_dot(unitFrom, unitTo);
    rcpScale   = 1.0f / sqrtf((1.0f + dpFromTo) * 2.0f);
    dst_xyzw[0] = xFromTo[0] * rcpScale;
    dst_xyzw[1] = xFromTo[1] * rcpScale;
    dst_xyzw[2] = xFromTo[2] * rcpScale;
    dst_xyzw[3] = 0.5f       * rcpScale;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_mat4x4(float *dst_xyzw, float const *m16)
{
    float tracePlusOne = 1.0f + m16[0] + m16[5] + m16[10];
    if   (tracePlusOne > 0.0f)
    {
        float s    = 2.0f  * sqrtf(tracePlusOne);
        float rcpS = 1.0f  / s;
        dst_xyzw[0] = rcpS  * (m16[6] - m16[9]);
        dst_xyzw[1] = rcpS  * (m16[8] - m16[2]);
        dst_xyzw[2] = rcpS  * (m16[1] - m16[4]);
        dst_xyzw[3] = 0.25f * s;
        return dst_xyzw;
    }
    else
    {
        if (m16[0] > m16[5] && m16[0] > m16[10])
        {
            float s    = 2.0f  * sqrtf(1.0f + m16[0] - m16[5] - m16[10]);
            float rcpS = 1.0f  / s;
            dst_xyzw[0] = 0.25f * s;
            dst_xyzw[1] = rcpS  * (m16[1] + m16[4]);
            dst_xyzw[2] = rcpS  * (m16[8] + m16[2]);
            dst_xyzw[3] = rcpS  * (m16[6] - m16[9]);
            return dst_xyzw;
        }
        else if (m16[5] > m16[10])
        {
            float s    = 2.0f  * sqrtf(1.0f + m16[5] - m16[0] - m16[10]);
            float rcpS = 1.0f  / s;
            dst_xyzw[0] = rcpS  * (m16[1] + m16[4]);
            dst_xyzw[1] = 0.25f * s;
            dst_xyzw[2] = rcpS  * (m16[6] + m16[9]);
            dst_xyzw[3] = rcpS  * (m16[8] - m16[2]);
            return dst_xyzw;
        }
        else
        {
            float s    = 2.0f  * sqrtf(1.0f + m16[10] - m16[0] - m16[5]);
            float rcpS = 1.0f  / s;
            dst_xyzw[0] = rcpS  * (m16[8] + m16[2]);
            dst_xyzw[1] = rcpS  * (m16[6] + m16[9]);
            dst_xyzw[2] = 0.25f * s;
            dst_xyzw[3] = rcpS  * (m16[1] - m16[4]);
            return dst_xyzw;
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_euler_deg(
    float *dst_xyzw,
    float  degX,
    float  degY,
    float  degZ)
{
    return math::quat_orientation_euler_rad(
        dst_xyzw,
        math::to_radians(degX),
        math::to_radians(degY),
        math::to_radians(degZ));
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_euler_rad(
    float *dst_xyzw,
    float  radX,
    float  radY,
    float  radZ)
{
    float qx[4], qy[4], qz[4], qxy[4];
    float halfAngleX    = 0.5f * radX;
    float halfAngleY    = 0.5f * radY;
    float halfAngleZ    = 0.5f * radZ;
    float sinHalfAngleX = sinf(halfAngleX);
    float sinHalfAngleY = sinf(halfAngleY);
    float sinHalfAngleZ = sinf(halfAngleZ);
    float cosHalfAngleX = cosf(halfAngleX);
    float cosHalfAngleY = cosf(halfAngleY);
    float cosHalfAngleZ = cosf(halfAngleZ);

    // angle-axis to quat: x-axis
    qx[0] = sinHalfAngleX;
    qx[1] = 0.0f;
    qx[2] = 0.0f;
    qx[3] = cosHalfAngleX;

    // angle-axis to quat: y-axis
    qy[0] = 0.0f;
    qy[1] = sinHalfAngleY;
    qy[2] = 0.0f;
    qy[3] = cosHalfAngleY;

    // angle-axis to quat: z-axis
    qz[0] = 0.0f;
    qz[1] = 0.0f;
    qz[2] = sinHalfAngleZ;
    qz[3] = cosHalfAngleZ;

    math::quat_mul(qxy, qx, qy);
    return math::quat_mul(dst_xyzw, qxy, qz);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_angle_axis_normalized_deg(
    float       *dst_xyzw,
    float        angleDeg,
    float const *axis_xyz)
{
    return math::quat_orientation_angle_axis_normalized_rad(
        dst_xyzw,
        math::to_radians(angleDeg),
        axis_xyz);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_angle_axis_normalized_rad(
    float       *dst_xyzw,
    float        angleRad,
    float const *axis_xyz)
{
    float halfAngle    = 0.5f * angleRad;
    float sinHalfAngle = sinf(halfAngle);
    float cosHalfAngle = cosf(halfAngle);
    dst_xyzw[0] = axis_xyz[0] * sinHalfAngle;
    dst_xyzw[1] = axis_xyz[1] * sinHalfAngle;
    dst_xyzw[2] = axis_xyz[2] * sinHalfAngle;
    dst_xyzw[3] = cosHalfAngle;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_angle_axis_unnormalized_deg(
    float       *dst_xyzw,
    float        angleDeg,
    float const *axis_xyz)
{
    return math::quat_orientation_angle_axis_unnormalized_rad(
        dst_xyzw,
        math::to_radians(angleDeg),
        axis_xyz);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_orientation_angle_axis_unnormalized_rad(
    float       *dst_xyzw,
    float        angleRad,
    float const *axis_xyz)
{
    float halfAngle    = 0.5f * angleRad;
    float sinHalfAngle = sinf(halfAngle);
    float cosHalfAngle = cosf(halfAngle);
    float unitAxis[3];
    math::vec3_normalize(unitAxis, axis_xyz);
    dst_xyzw[0] = unitAxis[0] * sinHalfAngle;
    dst_xyzw[1] = unitAxis[1] * sinHalfAngle;
    dst_xyzw[2] = unitAxis[2] * sinHalfAngle;
    dst_xyzw[3] = cosHalfAngle;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_interpolate_linear(
    float       *dst_xyzw,
    float const *a_xyzw,
    float const *b_xyzw,
    float        t)
{
    dst_xyzw[0] = math::interpolate_linear(a_xyzw[0], b_xyzw[0], t);
    dst_xyzw[1] = math::interpolate_linear(a_xyzw[1], b_xyzw[1], t);
    dst_xyzw[2] = math::interpolate_linear(a_xyzw[2], b_xyzw[2], t);
    dst_xyzw[3] = math::interpolate_linear(a_xyzw[3], b_xyzw[3], t);
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_interpolate_spherical(
    float       *dst_xyzw,
    float const *a_xyzw,
    float const *b_xyzw,
    float        t)
{
    float ax  = a_xyzw[0], ay = a_xyzw[1], az = a_xyzw[2], aw = a_xyzw[3];
    float bx  = b_xyzw[0], by = b_xyzw[1], bz = b_xyzw[2], bw = b_xyzw[3];
    float omt = 1.0f - t;
    float co  = cosf(ax * bx + ay * by + az * bz + aw * bw);
    float s1  = 0.0f;
    float s2  = 0.0f;
    float q[4];

    math::quat_set_quat(q, b_xyzw);

    if (co < 0.0f)
    {
        co   = -co;
        q[0] = -bx;
        q[1] = -by;
        q[2] = -bz;
        q[3] = -bw;
    }

    if (!math::equal(1.0f - co, 0))
    {
        float om = acosf(co);
        float so = sinf(om);
        s1 = sinf(omt * om) / so;
        s2 = sinf(t   * om) / so;
    }
    else
    {
        // a and b are close; perform linear interpolation.
        s1 = omt;
        s2 = t;
    }

    dst_xyzw[0] = ax * s1 + q[0] * s2;
    dst_xyzw[1] = ay * s1 + q[1] * s2;
    dst_xyzw[2] = az * s1 + q[2] * s2;
    dst_xyzw[3] = aw * s1 + q[3] * s2;
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_interpolate_quadratic(
    float       *dst_xyzw,
    float const *p_xyzw,
    float const *a_xyzw,
    float const *b_xyzw,
    float const *q_xyzw,
    float        t)
{
    float at[4];
    float bt[4];
    math::quat_interpolate_spherical(at,      p_xyzw,  q_xyzw, t);
    math::quat_interpolate_spherical(bt,      a_xyzw,  b_xyzw, t);
    math::quat_interpolate_spherical(dst_xyzw, at,     bt,    2.0f * t * (1.0f * t));
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::quat_interpolate_spline(
    float       *dst_xyzw,
    float const *a_xyzw,
    float const *b_xyzw,
    float const *c_xyzw)
{
    float ab[4];
    float ac[4];
    float invA[4];
    float invAB[4];
    float invAC[4];
    float logIAB[4];
    float logIAC[4];
    float sumLABAC[4];
    float sclLABAC[4];
    float expLABAC[4];
    math::quat_inverse(invA, a_xyzw);
    math::quat_mul(ab, invA, b_xyzw);
    math::quat_mul(ac, invA, c_xyzw);
    math::quat_normalize(invAB, ab);
    math::quat_normalize(invAC, ac);
    math::quat_log(logIAB, invAB);
    math::quat_log(logIAC, invAC);
    math::quat_add(sumLABAC, logIAC, logIAB);
    math::quat_scale(sclLABAC, sumLABAC, -0.25f);
    math::quat_exp(expLABAC, sclLABAC);
    math::quat_mul(dst_xyzw, a_xyzw, expLABAC);
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set(
    float *dst16,
    float  r0c0, float r0c1, float r0c2, float r0c3,
    float  r1c0, float r1c1, float r1c2, float r1c3,
    float  r2c0, float r2c1, float r2c2, float r2c3,
    float  r3c0, float r3c1, float r3c2, float r3c3)
{
    dst16[0]  = r0c0; dst16[1]  = r1c0; dst16[2]  = r2c0; dst16[3]  = r3c0;
    dst16[4]  = r0c1; dst16[5]  = r1c1; dst16[6]  = r2c1; dst16[7]  = r3c1;
    dst16[8]  = r0c2; dst16[9]  = r1c2; dst16[10] = r2c2; dst16[11] = r3c2;
    dst16[12] = r0c3; dst16[13] = r1c3; dst16[14] = r2c3; dst16[15] = r3c3;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_mat4x4(float *dst16, float const *src16)
{
    int  i = 0;
    for (i = 0; i < 16; ++i)
        dst16[i] = src16[i];
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_nan(float *dst16)
{
    float qnan = *(float*) &F32_QNaN;
    int   i    = 0;
    for (i = 0; i < 16; ++i)
        dst16[i] = qnan;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_one(float *dst16)
{
    int  i = 0;
    for (i = 0; i < 16; ++i)
        dst16[i] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_zero(float *dst16)
{
    int  i = 0;
    for (i = 0; i < 16; ++i)
        dst16[i] = 0.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_negative_infinity(float *dst16)
{
    float ninf = *(float*) &F32_NInf;
    int   i    = 0;
    for (i = 0; i < 16; ++i)
        dst16[i] = ninf;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_positive_infinity(float *dst16)
{
    float pinf = *(float*) &F32_PInf;
    int   i    = 0;
    for (i = 0; i < 16; ++i)
        dst16[i] = pinf;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_identity(float *dst16)
{
    dst16[0]  = 1.0f; dst16[1]  = 0.0f; dst16[2]  = 0.0f; dst16[3]  = 0.0f;
    dst16[4]  = 0.0f; dst16[5]  = 1.0f; dst16[6]  = 0.0f; dst16[7]  = 0.0f;
    dst16[8]  = 0.0f; dst16[9]  = 0.0f; dst16[10] = 1.0f; dst16[11] = 0.0f;
    dst16[12] = 0.0f; dst16[13] = 0.0f; dst16[14] = 0.0f; dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t math::mat4x4_equal(float const *a16, float const *b16)
{
    int  i = 0;
    for (i = 0; i < 16; ++i)
    {
        if (!math::equal(a16[i], b16[i]))
            return 0;
    }
    return 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t math::mat4x4_is_identity(float const *src16)
{
    if (!math::equal(src16[0],  1.0f))
        return 0;
    if (!math::equal(src16[5],  1.0f))
        return 0;
    if (!math::equal(src16[10], 1.0f))
        return 0;
    if (!math::equal(src16[15], 1.0f))
        return 0;
    if (!math::equal(src16[12], 0.0f))
        return 0;
    if (!math::equal(src16[13], 0.0f))
        return 0;
    if (!math::equal(src16[14], 0.0f))
        return 0;
    if (!math::equal(src16[1],  0.0f))
        return 0;
    if (!math::equal(src16[2],  0.0f))
        return 0;
    if (!math::equal(src16[3],  0.0f))
        return 0;
    if (!math::equal(src16[4],  0.0f))
        return 0;
    if (!math::equal(src16[6],  0.0f))
        return 0;
    if (!math::equal(src16[7],  0.0f))
        return 0;
    if (!math::equal(src16[8],  0.0f))
        return 0;
    if (!math::equal(src16[9],  0.0f))
        return 0;
    if (!math::equal(src16[11], 0.0f))
        return 0;

    return 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_rows(
    float       *dst16,
    float const *r0_xyzw,
    float const *r1_xyzw,
    float const *r2_xyzw,
    float const *r3_xyzw)
{
    dst16[0]  = r0_xyzw[0]; dst16[1] = r1_xyzw[0]; dst16[2] = r2_xyzw[0]; dst16[3] = r3_xyzw[0];
    dst16[4]  = r0_xyzw[1]; dst16[5] = r1_xyzw[1]; dst16[6] = r2_xyzw[1]; dst16[7] = r3_xyzw[1];
    dst16[8]  = r0_xyzw[2]; dst16[9] = r1_xyzw[2]; dst16[10]= r2_xyzw[2]; dst16[11]= r3_xyzw[2];
    dst16[12] = r0_xyzw[3]; dst16[13]= r1_xyzw[3]; dst16[14]= r2_xyzw[3]; dst16[15]= r3_xyzw[3];
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_columns(
    float       *dst16,
    float const *c0_xyzw,
    float const *c1_xyzw,
    float const *c2_xyzw,
    float const *c3_xyzw)
{
    dst16[0]  = c0_xyzw[0]; dst16[1] = c0_xyzw[1]; dst16[2] = c0_xyzw[2]; dst16[3] = c0_xyzw[3];
    dst16[4]  = c1_xyzw[0]; dst16[5] = c1_xyzw[1]; dst16[6] = c1_xyzw[2]; dst16[7] = c1_xyzw[3];
    dst16[8]  = c2_xyzw[0]; dst16[9] = c2_xyzw[1]; dst16[10]= c2_xyzw[2]; dst16[11]= c2_xyzw[3];
    dst16[12] = c3_xyzw[0]; dst16[13]= c3_xyzw[1]; dst16[14]= c3_xyzw[2]; dst16[15]= c3_xyzw[3];
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_get_row(float *dst_xyzw, size_t row, float const *src16)
{
    dst_xyzw[0] = src16[row + 0];
    dst_xyzw[1] = src16[row + 4];
    dst_xyzw[2] = src16[row + 8];
    dst_xyzw[3] = src16[row + 12];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_row(float *dst16,   size_t row, float const *src_xyzw)
{
    dst16[row + 0]  = src_xyzw[0];
    dst16[row + 4]  = src_xyzw[1];
    dst16[row + 8]  = src_xyzw[2];
    dst16[row + 12] = src_xyzw[3];
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_get_column(float *dst_xyzw, size_t col, float const *src16)
{
    dst_xyzw[0] = src16[col * 4 + 0];
    dst_xyzw[1] = src16[col * 4 + 1];
    dst_xyzw[2] = src16[col * 4 + 2];
    dst_xyzw[3] = src16[col * 4 + 3];
    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_set_column(float *dst16,   size_t col, float const *src_xyzw)
{
    dst16[col * 4 + 0] = src_xyzw[0];
    dst16[col * 4 + 1] = src_xyzw[1];
    dst16[col * 4 + 2] = src_xyzw[2];
    dst16[col * 4 + 3] = src_xyzw[3];
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float  math::mat4x4_trace(float const *src16)
{
    return (src16[0] + src16[5] + src16[10] + src16[15]);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float  math::mat4x4_determinant(float const *src16)
{
    float c0  = src16[5] * src16[10] - src16[6] * src16[9];
    float c4  = src16[2] * src16[9]  - src16[1] * src16[10];
    float c8  = src16[1] * src16[6]  - src16[2] * src16[5];
    return src16[0] * c0 + src16[4]  * c4 + src16[8] * c8;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_transpose(float *dst16, float const *src16)
{
    float src0   = src16[0];
    float src1   = src16[1];
    float src2   = src16[2];
    float src3   = src16[3];
    float src4   = src16[4];
    float src5   = src16[5];
    float src6   = src16[6];
    float src7   = src16[7];
    float src8   = src16[8];
    float src9   = src16[9];
    float src10  = src16[10];
    float src11  = src16[11];
    float src12  = src16[12];
    float src13  = src16[13];
    float src14  = src16[14];
    float src15  = src16[15];

    dst16[0]  = src0;
    dst16[1]  = src4;
    dst16[2]  = src8;
    dst16[3]  = src12;

    dst16[4]  = src1;
    dst16[5]  = src5;
    dst16[6]  = src9;
    dst16[7]  = src13;

    dst16[8]  = src2;
    dst16[9]  = src6;
    dst16[10] = src10;
    dst16[11] = src14;

    dst16[12] = src3;
    dst16[13] = src7;
    dst16[14] = src11;
    dst16[15] = src15;

    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_concatenate(float *dst16, float const *a16, float const *b16)
{
    /* transformation 'a' is applied first, followed by transformation 'b' */
    /* result is the dot product of the columns of 'a' and the rows of 'b' */
    /* read a16 and b16 into temporary variables so the compiler doesn't   */
    /* compiler doesn't issue reads to memory due to aliasing              */
    float a0   = a16[0];
    float a1   = a16[1];
    float a2   = a16[2];
    float a3   = a16[3];
    float a4   = a16[4];
    float a5   = a16[5];
    float a6   = a16[6];
    float a7   = a16[7];
    float a8   = a16[8];
    float a9   = a16[9];
    float a10  = a16[10];
    float a11  = a16[11];
    float a12  = a16[12];
    float a13  = a16[13];
    float a14  = a16[14];
    float a15  = a16[15];
    float b0   = b16[0];
    float b1   = b16[1];
    float b2   = b16[2];
    float b3   = b16[3];
    float b4   = b16[4];
    float b5   = b16[5];
    float b6   = b16[6];
    float b7   = b16[7];
    float b8   = b16[8];
    float b9   = b16[9];
    float b10  = b16[10];
    float b11  = b16[11];
    float b12  = b16[12];
    float b13  = b16[13];
    float b14  = b16[14];
    float b15  = b16[15];

    dst16[0]  = b0 * a0  + b4 * a1  + b8  * a2  + b12 * a3;
    dst16[1]  = b1 * a0  + b5 * a1  + b9  * a2  + b13 * a3;
    dst16[2]  = b2 * a0  + b6 * a1  + b10 * a2  + b14 * a3;
    dst16[3]  = b3 * a0  + b7 * a1  + b11 * a2  + b15 * a3;

    dst16[4]  = b0 * a4  + b4 * a5  + b8  * a6  + b12 * a7;
    dst16[5]  = b1 * a4  + b5 * a5  + b9  * a6  + b13 * a7;
    dst16[6]  = b2 * a4  + b6 * a5  + b10 * a6  + b14 * a7;
    dst16[7]  = b3 * a4  + b7 * a5  + b11 * a6  + b15 * a7;

    dst16[8]  = b0 * a8  + b4 * a9  + b8  * a10 + b12 * a11;
    dst16[9]  = b1 * a8  + b5 * a9  + b9  * a10 + b13 * a11;
    dst16[10] = b2 * a8  + b6 * a9  + b10 * a10 + b14 * a11;
    dst16[11] = b3 * a8  + b7 * a9  + b11 * a10 + b15 * a11;

    dst16[12] = b0 * a12 + b4 * a13 + b8  * a14 + b12 * a15;
    dst16[13] = b1 * a12 + b5 * a13 + b9  * a14 + b13 * a15;
    dst16[14] = b2 * a12 + b6 * a13 + b10 * a14 + b14 * a15;
    dst16[15] = b3 * a12 + b7 * a13 + b11 * a14 + b15 * a15;

    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_inverse_affine(float *dst16, float const *src16)
{
    float c0  = src16[5] * src16[10] - src16[6] * src16[9];
    float c4  = src16[2] * src16[9]  - src16[1] * src16[10];
    float c8  = src16[1] * src16[6]  - src16[2] * src16[5];
    float det = src16[0] * c0 + src16[4] * c4 + src16[8] * c8;

    /* singluar matrix? */
    if (math::equal(det, 0))
    {
        return math::mat4x4_set_zero(dst16);
    }
    else
    {
        float rcp  = 1.0f / det;
        float r0c0 = rcp  * c0;
        float r1c0 = rcp  * c4;
        float r2c0 = rcp  * c8;
        float r0c1 = rcp  * (src16[6] * src16[8] - src16[4]   * src16[10]);
        float r1c1 = rcp  * (src16[0] * src16[10]- src16[2]   * src16[8]);
        float r2c1 = rcp  * (src16[2] * src16[4] - src16[0]   * src16[6]);
        float r0c2 = rcp  * (src16[4] * src16[9] - src16[5]   * src16[8]);
        float r1c2 = rcp  * (src16[1] * src16[8] - src16[0]   * src16[9]);
        float r2c2 = rcp  * (src16[0] * src16[5] - src16[1]   * src16[4]);
        float r0c3 =-r0c0 * src16[12] - r0c1 * src16[13] - r0c2 * src16[14];
        float r1c3 =-r1c0 * src16[12] - r1c1 * src16[13] - r1c2 * src16[14];
        float r2c3 =-r2c0 * src16[12] - r2c1 * src16[13] - r2c2 * src16[14];

        dst16[0]  = r0c0;  dst16[1]  = r1c0;  dst16[2]  = r2c0;  dst16[3]  = 0.0f;
        dst16[4]  = r0c1;  dst16[5]  = r1c1;  dst16[6]  = r2c1;  dst16[7]  = 0.0f;
        dst16[8]  = r0c2;  dst16[9]  = r1c2;  dst16[10] = r2c2;  dst16[11] = 0.0f;
        dst16[12] = r0c3;  dst16[13] = r1c3;  dst16[14] = r2c3;  dst16[15] = 1.0f;
        return dst16;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_quat(float *dst16, float const *src_xyzw)
{
    float xx = src_xyzw[0] * src_xyzw[0];
    float xy = src_xyzw[0] * src_xyzw[1];
    float xz = src_xyzw[0] * src_xyzw[2];
    float xw = src_xyzw[0] * src_xyzw[3];
    float yy = src_xyzw[1] * src_xyzw[1];
    float yz = src_xyzw[1] * src_xyzw[2];
    float yw = src_xyzw[1] * src_xyzw[3];
    float zz = src_xyzw[2] * src_xyzw[2];
    float zw = src_xyzw[2] * src_xyzw[3];
    dst16[0]  = 1.0f - 2.0f * (yy + zz);  dst16[1]  = 2.0f * (xy + zw);        dst16[2]  = 2.0f * (xz - yw);        dst16[3]  = 0.0f;
    dst16[4]  = 2.0f * (xy - zw);         dst16[5]  = 1.0f - 2.0f * (xx + zz); dst16[6]  = 2.0f * (yz + xw);        dst16[7]  = 0.0f;
    dst16[8]  = 2.0f * (xz + yw);         dst16[9]  = 2.0f * (yz - xw);        dst16[10] = 1.0f - 2.0f * (xx + yy); dst16[11] = 0.0f;
    dst16[12] = 0.0f;                     dst16[13] = 0.0f;                    dst16[14] = 0.0f;                    dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_x_deg(float *dst16, float degX)
{
    return math::mat4x4_orientation_x_rad(dst16, math::to_radians(degX));
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_x_rad(float *dst16, float radX)
{
    float sa  = sinf(radX);
    float ca  = cosf(radX);
    dst16[0]  = 1.0f;  dst16[1]  = 0.0f;  dst16[2]  = 0.0f;  dst16[3]  = 0.0f;
    dst16[4]  = 0.0f;  dst16[5]  = ca;    dst16[6]  = sa;    dst16[7]  = 0.0f;
    dst16[8]  = 0.0f;  dst16[9]  =-sa;    dst16[10] = ca;    dst16[11] = 0.0f;
    dst16[12] = 0.0f;  dst16[13] = 0.0f;  dst16[14] = 0.0f;  dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_y_deg(float *dst16, float degY)
{
    return math::mat4x4_orientation_y_rad(dst16, math::to_radians(degY));
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_y_rad(float *dst16, float radY)
{
    float sa  = sinf(radY);
    float ca  = cosf(radY);
    dst16[0]  = ca;    dst16[1]  = 0.0f;  dst16[2]  =-sa;    dst16[3]  = 0.0f;
    dst16[4]  = 0.0f;  dst16[5]  = 1.0f;  dst16[6]  = 0.0f;  dst16[7]  = 0.0f;
    dst16[8]  = sa;    dst16[9]  = 0.0f;  dst16[10] = ca;    dst16[11] = 0.0f;
    dst16[12] = 0.0f;  dst16[13] = 0.0f;  dst16[14] = 0.0f;  dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_z_deg(float *dst16, float degZ)
{
    return math::mat4x4_orientation_z_rad(dst16, math::to_radians(degZ));
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_z_rad(float *dst16, float radZ)
{
    float sa  = sinf(radZ);
    float ca  = cosf(radZ);
    dst16[0]  = ca;    dst16[1]  = sa;    dst16[2]  = 0.0f;  dst16[3]  = 0.0f;
    dst16[4]  =-sa;    dst16[5]  = ca;    dst16[6]  = 0.0f;  dst16[7]  = 0.0f;
    dst16[8]  = 0.0f;  dst16[9]  = 0.0f;  dst16[10] = 1.0f;  dst16[11] = 0.0f;
    dst16[12] = 0.0f;  dst16[13] = 0.0f;  dst16[14] = 0.0f;  dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_euler_deg(
    float *dst16,
    float  degX,
    float  degY,
    float  degZ)
{
    return math::mat4x4_orientation_euler_rad(
        dst16,
        math::to_radians(degX),
        math::to_radians(degY),
        math::to_radians(degZ));
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_euler_rad(
    float *dst16,
    float  radX,
    float  radY,
    float  radZ)
{
    float sx  = sinf(radX);
    float cx  = cosf(radX);
    float sy  = sinf(radY);
    float cy  = cosf(radY);
    float sz  = sinf(radZ);
    float cz  = cosf(radZ);
    dst16[0]  = (cy * cz);  dst16[1]  = (sx * sy * cz) + (cx * sz);  dst16[2]  = -(cx * sy * cz) + (sx * sz);  dst16[3]  = 0.0f;
    dst16[4]  =-(cy * sz);  dst16[5]  =-(sx * sy * sz) + (cx * cz);  dst16[6]  =  (cx * sy * sz) + (sx * cz);  dst16[7]  = 0.0f;
    dst16[8]  = (sy);       dst16[9]  =-(sx * cy);                   dst16[10] =  (cx * cy);                   dst16[11] = 0.0f;
    dst16[12] = 0.0f;       dst16[13] = 0.0f;                        dst16[14] =  0.0f;                        dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_look_at(
    float       *dst16,
    float const *pos_xyz,
    float const *target_xyz,
    float const *up_xyz)
{
    float z[3];
    float x[3];
    float y[3];
    float zn[3];
    float xn[3];
    float tx, ty, tz;
    math::vec3_sub(z, pos_xyz, target_xyz); /* left-handed would be sub(target, pos) */
    math::vec3_normalize(zn, z);
    math::vec3_cross(x, up_xyz, zn);
    math::vec3_normalize(xn, x);
    math::vec3_cross(y, zn, xn);
    tx = -math::vec3_dot(xn, pos_xyz);
    ty = -math::vec3_dot(y,  pos_xyz);
    tz = -math::vec3_dot(zn, pos_xyz);
    dst16[0]  = x[0];  dst16[1]  = y[0];  dst16[2]  = z[0];  dst16[3]  = 0.0f;
    dst16[4]  = x[1];  dst16[5]  = y[1];  dst16[6]  = z[1];  dst16[7]  = 0.0f;
    dst16[8]  = x[2];  dst16[9]  = y[2];  dst16[10] = z[2];  dst16[11] = 0.0f;
    dst16[12] = tx;    dst16[13] = ty;    dst16[14] = tz;    dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_angle_axis_normalized_deg(
    float       *dst16,
    float        angleDeg,
    float const *axis_xyz)
{
    return math::mat4x4_orientation_angle_axis_normalized_rad(
        dst16,
        math::to_radians(angleDeg),
        axis_xyz);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_angle_axis_normalized_rad(
    float       *dst16,
    float        angleRad,
    float const *axis_xyz)
{
    float sa  = sinf(angleRad);
    float ca  = cosf(angleRad);
    float t   = 1.0f - ca;
    float tx  = t   * axis_xyz[0];
    float ty  = t   * axis_xyz[1];
    float tz  = t   * axis_xyz[2];
    float sx  = sa  * axis_xyz[0];
    float sy  = sa  * axis_xyz[1];
    float sz  = sa  * axis_xyz[2];
    float txy = tx  * axis_xyz[1];
    float tyz = ty  * axis_xyz[2];
    float txz = tx  * axis_xyz[2];
    dst16[0]  = tx  * axis_xyz[0] + ca;  dst16[1]  = txy + sz;               dst16[2]  = txz - sy;               dst16[3]  = 0.0f;
    dst16[4]  = txy - sz;               dst16[5]  = ty  * axis_xyz[1] + ca;  dst16[6]  = tyz + sx;               dst16[7]  = 0.0f;
    dst16[8]  = txz + sy;               dst16[9]  = tyz - sx;               dst16[10] = tz  * axis_xyz[2] + ca;  dst16[11] = 0.0f;
    dst16[12] = 0.0f;                   dst16[13] = 0.0f;                   dst16[14] = 0.0f;                   dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_angle_axis_unnormalized_deg(
    float       *dst16,
    float        angleDeg,
    float const *axis_xyz)
{
    float n[3];
    math::vec3_normalize(n, axis_xyz);
    return math::mat4x4_orientation_angle_axis_normalized_rad(
        dst16,
        math::to_radians(angleDeg),
        n);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orientation_angle_axis_unnormalized_rad(
    float       *dst16,
    float        angleRad,
    float const *axis_xyz)
{
    float n[3];
    math::vec3_normalize(n, axis_xyz);
    return math::mat4x4_orientation_angle_axis_normalized_rad(dst16, angleRad, n);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_scale(float *dst16, float sX, float sY, float sZ)
{
    dst16[0]  = sX;    dst16[1]  = 0.0f;  dst16[2]  = 0.0f;  dst16[3]  = 0.0f;
    dst16[4]  = 0.0f;  dst16[5]  = sY;    dst16[6]  = 0.0f;  dst16[7]  = 0.0f;
    dst16[8]  = 0.0f;  dst16[9]  = 0.0f;  dst16[10] = sZ;    dst16[11] = 0.0f;
    dst16[12] = 0.0f;  dst16[13] = 0.0f;  dst16[14] = 0.0f;  dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_translation(float *dst16, float tX, float tY, float tZ)
{
    dst16[0]  = 1.0f;  dst16[1]  = 0.0f;  dst16[2]  = 0.0f;  dst16[3]  = 0.0f;
    dst16[4]  = 0.0f;  dst16[5]  = 1.0f;  dst16[6]  = 0.0f;  dst16[7]  = 0.0f;
    dst16[8]  = 0.0f;  dst16[9]  = 0.0f;  dst16[10] = 1.0f;  dst16[11] = 0.0f;
    dst16[12] = tX;    dst16[13] = tY;    dst16[14] = tZ;    dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_orthographic(
    float *dst16,
    float  left,
    float  right,
    float  bottom,
    float  top,
    float  nearClipDepth,
    float  farClipDepth)
{
    float rcpx = 1.0f / (right         - left);
    float rcpy = 1.0f / (top           - bottom);
    float rcpz = 1.0f / (nearClipDepth - farClipDepth);
    float rpl  = right         + left;
    float tpb  = top           + bottom;
    float npf  = nearClipDepth + farClipDepth;
    dst16[0]   = 2.0f * rcpx;  dst16[1]  = 0.0f;         dst16[2]  = 0.0f;        dst16[3]  = 0.0f;
    dst16[4]   = 0.0f;         dst16[5]  = 2.0f * rcpy;  dst16[6]  = 0.0f;        dst16[7]  = 0.0f;
    dst16[8]   = 0.0f;         dst16[9]  = 0.0f;         dst16[10] = 2.0f * rcpz; dst16[11] = 0.0f;
    dst16[12]  =-rpl  * rcpx;  dst16[13] =-tpb  * rcpy;  dst16[14] = npf  * rcpz; dst16[15] = 1.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_perspective_deg(
    float *dst16,
    float  fovDeg,
    float  aspectRatio,
    float  nearClipDepth,
    float  farClipDepth)
{
    return math::mat4x4_perspective_rad(
        dst16,
        math::to_radians(fovDeg),
        aspectRatio,
        nearClipDepth,
        farClipDepth);
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_perspective_rad(
    float *dst16,
    float  fovRad,
    float  aspectRatio,
    float  nearClipDepth,
    float  farClipDepth)
{
    float a   = aspectRatio;
    float d   = 1.0f / tanf(fovRad * 0.5f);
    float r   = 1.0f / (nearClipDepth - farClipDepth);
    float nf  = nearClipDepth * farClipDepth;
    float npf = nearClipDepth + farClipDepth;
    dst16[0]  = d / a;  dst16[1]  = 0.0f;  dst16[2]  = 0.0f;          dst16[3]  = 0.0f;
    dst16[4]  = 0.0f;   dst16[5]  = d;     dst16[6]  = 0.0f;          dst16[7]  = 0.0f;
    dst16[8]  = 0.0f;   dst16[9]  = 0.0f;  dst16[10] = npf * r;       dst16[11] =-1.0f;
    dst16[12] = 0.0f;   dst16[13] = 0.0f;  dst16[14] = 2.0f * nf * r; dst16[15] = 0.0f;
    return dst16;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_perspective_2d(float *dst16, float width, float height)
{
    float sX = 1.0f / (width  * 0.5f);
    float sY = 1.0f / (height * 0.5f);
    float s[16];
    float t[16];
    math::mat4x4_scale(s, sX, -sY, 1.0f);
    math::mat4x4_translation(t, -1.0f, 1.0f, 0.0f);
    return math::mat4x4_concatenate(dst16, s, t);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void math::mat4x4_extract_frustum_normalized(
    float       *left_xyzD,
    float       *right_xyzD,
    float       *top_xyzD,
    float       *bottom_xyzD,
    float       *near_xyzD,
    float       *far_xyzD,
    float const *src16)
{
    float l[4];
    float r[4];
    float t[4];
    float b[4];
    float n[4];
    float f[4];
    math::mat4x4_extract_frustum_unnormalized(l, r, t, b, n, f, src16);
    math::vec3_normalize(left_xyzD,   l);
    math::vec3_normalize(right_xyzD,  r);
    math::vec3_normalize(top_xyzD,    t);
    math::vec3_normalize(bottom_xyzD, b);
    math::vec3_normalize(near_xyzD,   n);
    math::vec3_normalize(far_xyzD,    f);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void math::mat4x4_extract_frustum_unnormalized(
    float       *left_xyzD,
    float       *right_xyzD,
    float       *top_xyzD,
    float       *bottom_xyzD,
    float       *near_xyzD,
    float       *far_xyzD,
    float const *src16)
{
    left_xyzD[0]   = src16[3]  + src16[0];
    left_xyzD[1]   = src16[7]  + src16[4];
    left_xyzD[2]   = src16[11] + src16[8];
    left_xyzD[3]   = src16[15] + src16[12];

    right_xyzD[0]  = src16[3]  - src16[0];
    right_xyzD[1]  = src16[7]  - src16[4];
    right_xyzD[2]  = src16[11] - src16[8];
    right_xyzD[3]  = src16[15] - src16[12];

    top_xyzD[0]    = src16[3]  - src16[1];
    top_xyzD[1]    = src16[7]  - src16[5];
    top_xyzD[2]    = src16[11] - src16[9];
    top_xyzD[3]    = src16[15] - src16[13];

    bottom_xyzD[0] = src16[3]  + src16[1];
    bottom_xyzD[1] = src16[7]  + src16[5];
    bottom_xyzD[2] = src16[11] + src16[9];
    bottom_xyzD[3] = src16[15] + src16[13];

    near_xyzD[0]   = src16[3]  + src16[2];
    near_xyzD[1]   = src16[7]  + src16[6];
    near_xyzD[2]   = src16[11] + src16[10];
    near_xyzD[3]   = src16[15] + src16[14];

    far_xyzD[0]    = src16[3]  - src16[2];
    far_xyzD[1]    = src16[7]  - src16[6];
    far_xyzD[2]    = src16[11] - src16[10];
    far_xyzD[3]    = src16[15] - src16[14];
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_transform_vec4(
    float       *dst_xyzw,
    float const *src_xyzw,
    float const *t16)
{
    float vx = src_xyzw[0];
    float vy = src_xyzw[1];
    float vz = src_xyzw[2];
    float vw = src_xyzw[3];

    dst_xyzw[0] = t16[0] * vx + t16[4] * vy + t16[8]  * vz + t16[12] * vw;
    dst_xyzw[1] = t16[1] * vx + t16[5] * vy + t16[9]  * vz + t16[13] * vw;
    dst_xyzw[2] = t16[2] * vx + t16[6] * vy + t16[10] * vz + t16[14] * vw;
    dst_xyzw[3] = t16[3] * vx + t16[7] * vy + t16[11] * vz + t16[15] * vw;

    return dst_xyzw;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_transform_point(
    float       *dst_xyz,
    float const *src_xyz,
    float const *t16)
{
    float vx = src_xyz[0];
    float vy = src_xyz[1];
    float vz = src_xyz[2];

    /* for points, w = 1.0 */
    dst_xyz[0] = t16[0] * vx + t16[4] * vy + t16[8]  * vz + t16[12];
    dst_xyz[1] = t16[1] * vx + t16[5] * vy + t16[9]  * vz + t16[13];
    dst_xyz[2] = t16[2] * vx + t16[6] * vy + t16[10] * vz + t16[14];

    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_transform_vector(
    float       *dst_xyz,
    float const *src_xyz,
    float const *t16)
{
    float vx = src_xyz[0];
    float vy = src_xyz[1];
    float vz = src_xyz[2];

    /* for vectors, w = 0.0 */
    dst_xyz[0] = t16[0] * vx + t16[4] * vy + t16[8]  * vz;
    dst_xyz[1] = t16[1] * vx + t16[5] * vy + t16[9]  * vz;
    dst_xyz[2] = t16[2] * vx + t16[6] * vy + t16[10] * vz;

    return dst_xyz;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_transform_array_vec4(
    float       *dstArr_xyzw,
    float const *srcArr_xyzw,
    float const *t16,
    size_t       countVec4)
{
    float  *dst = dstArr_xyzw;
    size_t  i   = 0;

    for (i = 0; i < countVec4; ++i)
    {
        float x = srcArr_xyzw[0];
        float y = srcArr_xyzw[1];
        float z = srcArr_xyzw[2];
        float w = srcArr_xyzw[3];

        dstArr_xyzw[0] = t16[0] * x + t16[4] * y + t16[8]  * z + t16[12] * w;
        dstArr_xyzw[1] = t16[1] * x + t16[5] * y + t16[9]  * z + t16[13] * w;
        dstArr_xyzw[2] = t16[2] * x + t16[6] * y + t16[10] * z + t16[14] * w;
        dstArr_xyzw[3] = t16[3] * x + t16[7] * y + t16[11] * z + t16[15] * w;

        srcArr_xyzw += 4;
        dstArr_xyzw += 4;
    }
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_transform_array_point(
    float       *dstArr_xyz,
    float const *srcArr_xyz,
    float const *t16,
    size_t       countVec3)
{
    float  *dst = dstArr_xyz;
    size_t  i   = 0;

    for (i = 0; i < countVec3; ++i)
    {
        float x = srcArr_xyz[0];
        float y = srcArr_xyz[1];
        float z = srcArr_xyz[2];

        dstArr_xyz[0] = t16[0] * x + t16[4] * y + t16[8]  * z + t16[12];
        dstArr_xyz[1] = t16[1] * x + t16[5] * y + t16[9]  * z + t16[13];
        dstArr_xyz[2] = t16[2] * x + t16[6] * y + t16[10] * z + t16[14];

        srcArr_xyz += 3;
        dstArr_xyz += 3;
    }
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float* math::mat4x4_transform_array_vector(
    float       *dstArr_xyz,
    float const *srcArr_xyz,
    float const *t16,
    size_t       countVec3)
{
    float  *dst = dstArr_xyz;
    size_t  i   = 0;

    for (i = 0; i < countVec3; ++i)
    {
        float x = srcArr_xyz[0];
        float y = srcArr_xyz[1];
        float z = srcArr_xyz[2];

        dstArr_xyz[0] = t16[0] * x + t16[4] * y + t16[8]  * z;
        dstArr_xyz[1] = t16[1] * x + t16[5] * y + t16[9]  * z;
        dstArr_xyz[2] = t16[2] * x + t16[6] * y + t16[10] * z;

        srcArr_xyz += 3;
        dstArr_xyz += 3;
    }
    return dst;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t math::interval_intersect(
    float *wSE,
    float  u0,
    float  u1,
    float  v0,
    float  v1)
{
    /* interval [u0, u1]; u0 < u1                       */
    /* interval [v0, v1]; v0 < v1                       */
    /* returns 0 if the intervals do not intersect      */
    /* returns 1 if the intervals intersect in a point  */
    /* and sets the first output value to that point    */
    /* returns 2 if the intervals overlap and sets both */
    /* output values to the interval of overlap         */
    if (u1 < v0 || u0 > v1)
    {
        wSE[0] = 0.0f;
        wSE[1] = 0.0f;
        return 0;
    }
    if (u1 > v0)
    {
        if (u0 < v1)
        {
            if (u0 < v0) wSE[0] = v0;
            else         wSE[0] = u0;
            if (u1 > v1) wSE[1] = v1;
            else         wSE[1] = u1;
            return 2;
        }
        else
        {
            //u0 = v0:
            wSE[0] = u0;
            wSE[1] = 0.0f;
            return 1;
        }
    }
    else
    {
        //u1 = v0:
        wSE[0] = u1;
        wSE[1] = 0.0f;
        return 1;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t math::line2_intersect(
    float       *i0_xy,
    float const *p0_xy,
    float const *d0_xy,
    float const *p1_xy,
    float const *d1_xy)
{
    /* lines are defined in parametric form X(t) = P - tD  */
    /* where P is a point on the line and D is a direction */
    /* vector of the line. Given two points on the line,   */
    /* P_0 and P_1, D is computed as D = P_1 - P_0.        */
    /* returns 0 if the lines are parallel, but different  */
    /* returns 1 if the lines intersect in a single point  */
    /* returns 2 if the lines are the same line            */
    float Ex       = p1_xy[0] - p0_xy[0];
    float Ey       = p1_xy[1] - p0_xy[1];
    float kross    = d0_xy[0] * d1_xy[1] - d0_xy[1] * d1_xy[0];
    float sqrKross = kross   * kross;
    float sqrLen0  = d0_xy[0] * d0_xy[0] + d0_xy[1] * d0_xy[1];
    float sqrLen1  = d1_xy[0] * d1_xy[0] + d1_xy[1] * d1_xy[1];
    float sqrLenE  = 0.0f;

    if (sqrKross > FLT_EPSILON * sqrLen0 * sqrLen1)
    {
        /* lines are not parallel */
        float s = (Ex * d1_xy[1] - Ey * d1_xy[0]) / kross;

        i0_xy[0] = p0_xy[0] + s * d0_xy[0];
        i0_xy[1] = p0_xy[1] + s * d0_xy[1];
        return 1;
    }

    /* the lines are parallel */
    sqrLenE  = Ex    * Ex      + Ey * Ey;
    kross    = Ex    * d0_xy[1] - Ey * d0_xy[0];
    sqrKross = kross * kross;
    if (sqrKross > FLT_EPSILON * sqrLen0 * sqrLenE)
    {
        /* the lines are parallel (but different) */
        i0_xy[0] = 0.0f;
        i0_xy[1] = 0.0f;
        return 0;
    }

    /* the lines are parallel (they are the same line) */
    i0_xy[0] = 0.0f;
    i0_xy[1] = 0.0f;
    return 2;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t math::segment2_intersect(
    float       *i0_xy,
    float       *i1_xy,
    float const *p0_xy,
    float const *d0_xy,
    float const *p1_xy,
    float const *d1_xy)
{
    /* lines are defined in parametric form X(t) = P - tD      */
    /* where P is a point on the line and D is a direction     */
    /* vector of the line. Given two points on the line,       */
    /* P_0 and P_1, D is computed as D = P_1 - P_0.            */
    /* returns 0 if the segments do not intersect              */
    /* returns 1 if the segments intersect in a single point   */
    /* and sets out_I0_xy to the point of intersection          */
    /* returns 2 if the segments are the same line and overlap */
    /* and sets out_I0_xy and out_I1_xy to the start and end     */
    /* points of line segment intersection                     */
    float Ex       = p1_xy[0] - p0_xy[0];
    float Ey       = p1_xy[1] - p0_xy[1];
    float kross    = d0_xy[0] * d1_xy[1] - d0_xy[1] * d1_xy[0];
    float sqrKross = kross   * kross;
    float sqrLen0  = d0_xy[0] * d0_xy[0] + d0_xy[1] * d0_xy[1];
    float sqrLen1  = d1_xy[0] * d1_xy[0] + d1_xy[1] * d1_xy[1];

    /* segments are defined as:              */
    /* S0 = p0 + s * d0 for s in [0, 1] and  */
    /* S1 = p1 + t * d1 for t in [0, 1].     */

    if (sqrKross > FLT_EPSILON * sqrLen0 * sqrLen1)
    {
        /* the lines of the segments are not parallel,   */
        /* and intersect (but maybe not on the segments) */
        float s = (Ex * d1_xy[1] - Ey * d1_xy[0]) / kross;

        if (s < 0.0f || s > 1.0f)
        {
            /* the point of intersection does not lie on the segment */
            i0_xy[0] = 0.0f;
            i0_xy[1] = 0.0f;
            i1_xy[0] = 0.0f;
            i1_xy[1] = 0.0f;
            return 0;
        }
        else
        {
            float t = (Ex * d0_xy[1] - Ey * d0_xy[0]) / kross;

            if (t < 0.0f || t > 1.0f)
            {
                /* the point of intersection does not lie on the segment */
                i0_xy[0] = 0.0f;
                i0_xy[1] = 0.0f;
                i1_xy[0] = 0.0f;
                i1_xy[1] = 0.0f;
                return 0;
            }

            /* intersection of the lines is a point on each segment */
            i0_xy[0] = p0_xy[0] + s * d0_xy[0];
            i0_xy[1] = p0_xy[1] + s * d0_xy[1];
            i1_xy[0] = 0.0f;
            i1_xy[1] = 0.0f;
            return 1;
        }
    }
    else
    {
        /* the lines of the segments are parallel */
        float sqrLenE = Ex    * Ex      + Ey * Ey;
        kross         = Ex    * d0_xy[1] - Ey * d0_xy[0];
        sqrKross      = kross * kross;

        if (sqrKross > FLT_EPSILON * sqrLen0 * sqrLenE)
        {
            /* the lines of the segments are parallel (but different) */
            i0_xy[0] = 0.0f;
            i0_xy[1] = 0.0f;
            i1_xy[0] = 0.0f;
            i1_xy[1] = 0.0f;
            return 0;
        }
        else
        {
            /* the lines of the segments are parallel (but the same line) */
            /* test for overlap of the line segments, and if an overlap   */
            /* exists, return the line segment representing the overlap   */
            float  s0   =      (d0_xy[0] * Ex      + d0_xy[1] * Ey)      / sqrLen0;
            float  s1   = s0 + (d0_xy[0] * d1_xy[0] + d0_xy[1] * d1_xy[1]) / sqrLen0;
            float  smin = (s0 < s1) ? s0 : s1; /* min(s0, s1) */
            float  smax = (s0 > s1) ? s0 : s1; /* max(s0, s1) */
            float  w[2] = {0.0f, 0.0f};
            size_t n    = math::interval_intersect(w, 0.0f, 1.0f, smin, smax);

            if (0 == n)
            {
                // the intervals do not overlap.
                i0_xy[0] = 0.0f;
                i0_xy[1] = 0.0f;
                i1_xy[0] = 0.0f;
                i1_xy[1] = 0.0f;
            }
            else if (1 == n)
            {
                // the intervals overlap in a single point.
                i0_xy[0] = p0_xy[0] + w[0] * d0_xy[0];
                i0_xy[1] = p0_xy[1] + w[0] * d0_xy[1];
                i1_xy[0] = 0.0f;
                i1_xy[1] = 0.0f;
            }
            else
            {
                i0_xy[0] = p0_xy[0] + w[0] * d0_xy[0];
                i0_xy[1] = p0_xy[1] + w[0] * d0_xy[1];
                i1_xy[0] = p0_xy[0] + w[1] * d0_xy[0];
                i1_xy[1] = p0_xy[1] + w[1] * d0_xy[1];
            }

            return n;
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void math::segment2_parallel_offset(
    float       *a0_xy,
    float       *a1_xy,
    float       *b0_xy,
    float       *b1_xy,
    float const *p0_xy,
    float const *p1_xy,
    float        offset)
{
    float dx     =  p1_xy[0] - p0_xy[0];
    float dy     =  p1_xy[1] - p0_xy[1];
    float perpX  = -dy;
    float perpY  =  dx;
    float length =  sqrtf(perpX * perpX + perpY * perpY);
    float rcp    =  1.0f  / length;
    float npx    =  perpX * rcp;
    float npy    =  perpY * rcp;
    a0_xy[0] = p0_xy[0] + offset * npx;
    a0_xy[1] = p0_xy[1] + offset * npy;
    a1_xy[0] = p1_xy[0] + offset * npx;
    a1_xy[1] = p1_xy[1] + offset * npy;
    b0_xy[0] = p0_xy[0] - offset * npx;
    b0_xy[1] = p0_xy[1] - offset * npy;
    b1_xy[0] = p1_xy[0] - offset * npx;
    b1_xy[1] = p1_xy[1] - offset * npy;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
