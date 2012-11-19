/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libmath.hpp
///  Defines a set of mathematics functions for performing comparisons between
///  floating point numbers, interpolating scalar quantities, performing 2/3/4
///  component vector operations, and working with quaternions and 4x4
///  matrices. The emphasis is on correctness and usability, not performance.
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIBMATH_HPP_INCLUDED
#define LIBMATH_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace math {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// Determines the smaller of two floating point values.
///
/// @param a The first value.
/// @param b The second value.
/// @return The smaller of @a a or @a b.
CMN_PUBLIC float min2(float a, float b);

/// Determines the larger of two floating point values.
///
/// @param a The first value.
/// @param b The second value.
/// @return The larger of @a a or @a b.
CMN_PUBLIC float max2(float a, float b);

/// Determines the smallest of three floating point values.
///
/// @param a The first value.
/// @param b The second value.
/// @param c The third value.
/// @return The smaller of @a a, @a b and @a c.
CMN_PUBLIC float min3(float a, float b, float c);

/// Determines the largest of three floating point values.
///
/// @param a The first value.
/// @param b The second value.
/// @param c The third value.
/// @return The largest of @a a, @a b and @a c.
CMN_PUBLIC float max3(float a, float b, float c);

/// Determines whether two floating point values are close enough to be
/// considered equal, using the same value for absolute and relative
/// tolerance (FLT_EPSILON).
///
/// @param a The first value.
/// @param b The second value.
/// @return true if @a a and @a b can be considered equal.
CMN_PUBLIC bool eq(float a, float b);                /* abs & rel tolerance equal   */

/// Determines whether two floating point values are close enough to be
/// considered equal, using the specified absolute tolerance. This test
/// fails when both @a a and @a b are very large.
///
/// @param a The first value.
/// @param b The second value.
/// @param tol The absolute tolerance value.
/// @return true if @a a and @a b can be considered equal.
CMN_PUBLIC bool eq_abs(float a, float b, float tol);

/// Determines whether two floating point values are close enough to be
/// considered equal, using the specified relative tolerance. This test
/// fails when both @a a and @a b are very small.
///
/// @param a The first value.
/// @param b The second value.
/// @param tol The relative tolerance value.
/// @return true if @a a and @a b can be considered equal.
CMN_PUBLIC bool eq_rel(float a, float b, float tol);

/// Determines whether two floating point values are close enough to be
/// considered equal, using the specified absolute and relative tolerance
/// values.
///
/// @param a The first value.
/// @param b The second value.
/// @param tol_a The absolute tolerance value.
/// @param tol_r The relative tolerance value.
/// @return true if @a a and @a b can be considered equal.
CMN_PUBLIC bool eq_com(float a, float b, float tol_a, float tol_r);

/// Determines whether a floating point value has the special Not A Number
/// value.
///
/// @param a The value to check.
/// @return true if @a a is NaN.
CMN_PUBLIC bool is_nan(float a);

/// Determines whether a floating point value is either positive or negative
/// infinity.
///
/// @param a The value to check.
/// @return true if @a a is either the positive or negative infinity value.
CMN_PUBLIC bool is_inf(float a);

/// Computes the reciporical value 1/a for a given value.
///
/// @param a The input value.
/// @return The value 1/a. The function does not check for divide-by-zero.
CMN_PUBLIC float rcp(float a);

/// Converts a value specified in degrees to radians.
///
/// @param degrees The angle measure specified in degrees.
/// @return The angle measure specified in radians.
CMN_PUBLIC float rad(float degrees);

/// Converts a value specified in radians to degrees.
///
/// @param radians The angle measure specified in radians.
/// @return The angle measure specified in degrees.
CMN_PUBLIC float deg(float radians);

/// Performs linear interpolation between two scalar values.
///
/// @param a The value at @a t = 0.
/// @param b The value at @a t = 1.
/// @param t A normalized interpolation parameter.
/// @return The interpolated value.
CMN_PUBLIC float linear(float  a, float b, float t);

/// Performs Bezier interpolation between two scalar values.
///
/// @param a The value at @a t = 0.
/// @param b The value at @a t = 1.
/// @param in_t The tangent value (slope) coming into @a b.
/// @param out_t The tangent value (slope) coming out of @a b.
/// @param t A normalized interpolation parameter.
/// @return The interpolated value.
CMN_PUBLIC float bezier(float  a, float b, float in_t, float out_t, float t);

/// Performs Hermite interpolation between two scalar values.
///
/// @param a The value at @a t = 0.
/// @param b The value at @a t = 1.
/// @param in_t The tangent value (slope) coming into @a b.
/// @param out_t The tangent value (slope) coming out of @a b.
/// @param t A normalized interpolation parameter.
/// @return The interpolated value.
CMN_PUBLIC float hermite(float a, float b, float in_t, float out_t, float t);

/// Sets a 2-component vector or point value.
///
/// @param dst_xy Pointer to the destination storage.
/// @param x The x-component of the vector or point.
/// @param y The y-component of the vector or point.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_xy(float *dst_xy, float x, float y);

/// Copies a 2-component vector or point value. The source and destination
/// values must not overlap.
///
/// @param dst_xy Pointer to the destination storage.
/// @param src_xy Pointer to the source value.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_vec2(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy);

/// Extracts the x- and y-components of a vector or point value into a
/// destination value. The source and destination values must not overlap.
///
/// @param dst_xy Pointer to the destination storage.
/// @param src_xyz Pointer to the source value.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_vec3(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xyz);

/// Extracts the x- and y-components of a vector or point value into a
/// destination value. The source and destination values must not overlap.
///
/// @param dst_xy Pointer to the destination storage.
/// @param src_xyzw Pointer to the source value.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_vec4(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xyzw);

/// Sets a 3-component vector or point value.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param x The x-component of the vector or point.
/// @param y The y-component of the vector or point.
/// @param z The z-component of the vector or point.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_xyz(float *dst_xyz, float x, float y, float z);

/// Extracts the x- and y-components of a vector or point value into a
/// destination value with explicitly specified z-component. The source and
/// destination values must not overlap.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xy Pointer to the source storage from which the x- and y-
/// components will be read.
/// @param z The z-component value.
/// @return The pointer to @a dst_xyz.
CMN_PUBLIC float* vec3_set_vec2(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xy,
    float                      z);

/// Copies a 3-component vector or point value. The source and destination
/// values must not overlap.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xyz Pointer to the source value.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_vec3(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz);

/// Extracts the x- and y- and z-components of a vector or point value into a
/// destination value. The source and destination values must not overlap.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xyzw Pointer to the source storage from which the x- y- and z-
/// components will be read.
/// @return The pointer to @a dst_xyz.
CMN_PUBLIC float* vec3_set_vec4(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyzw);

/// Sets a 4-component vector or point value.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param x The x-component of the vector or point.
/// @param y The y-component of the vector or point.
/// @param z The z-component of the vector or point.
/// @param w The w-component of the vector or point. Vectors typically have a
/// w-component of zero; points typically have a w-component of one.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_xyzw(
    float *dst_xyzw,
    float  x,
    float  y,
    float  z,
    float  w);

/// Extracts the x- and y-components of a vector or point value into a
/// destination value with explicitly specified z- and w-component. The source
/// and destination values must not overlap.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xy Pointer to the source storage from which the x- and y-
/// components will be read.
/// @param z The z-component value.
/// @param w The w-component of the vector or point. Vectors typically have a
/// w-component of zero; points typically have a w-component of one.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_vec2(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xy,
    float                      z,
    float                      w);

/// Extracts the x- y- and z-components of a vector or point value into a
/// destination value with explicitly specified w-component. The source and
/// destination values must not overlap.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xyz Pointer to the source storage from which the x- y- and z-
/// components will be read.
/// @param w The w-component of the vector or point. Vectors typically have a
/// w-component of zero; points typically have a w-component of one.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_vec3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyz,
    float                      w);

/// Copies a 4-component vector or point value. The source and destination
/// values must not overlap.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param src_xyzw Pointer to the source value.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_vec4(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw);

/// Sets all elements of a vector to the IEEE-754 floating point Not-A-Number
/// (NaN) value.
///
/// @param dst_xy Pointer to the destination storage.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_nan(float *dst_xy);

/// Sets all elements of a vector to the IEEE-754 floating point Not-A-Number
/// (NaN) value.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_nan(float *dst_xyz);

/// Sets all elements of a vector to the IEEE-754 floating point Not-A-Number
/// (NaN) value.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_nan(float *dst_xyzw);

/// Sets all elements of a vector to 1.0.
///
/// @param dst_xy Pointer to the destination storage.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_one(float *dst_xy);

/// Sets all elements of a vector to 1.0.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_one(float *dst_xyz);

/// Sets all elements of a vector to 1.0.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_one(float *dst_xyzw);

/// Sets all elements of a vector to 0.0.
///
/// @param dst_xy Pointer to the destination storage.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_zero(float *dst_xy);

/// Sets all elements of a vector to 0.0.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_zero(float *dst_xyz);

/// Sets all elements of a vector to 0.0.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_zero(float *dst_xyzw);

/// Sets all elements of a vector to the IEEE-754 floating point negative
/// infinity value.
///
/// @param dst_xy Pointer to the destination storage.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_ninf(float *dst_xy);

/// Sets all elements of a vector to the IEEE-754 floating point negative
/// infinity value.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_ninf(float *dst_xyz);

/// Sets all elements of a vector to the IEEE-754 floating point negative
/// infinity value.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_ninf(float *dst_xyzw);

/// Sets all elements of a vector to the IEEE-754 floating point positive
/// infinity value.
///
/// @param dst_xy Pointer to the destination storage.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_pinf(float *dst_xy);

/// Sets all elements of a vector to the IEEE-754 floating point positive
/// infinity value.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_pinf(float *dst_xyz);

/// Sets all elements of a vector to the IEEE-754 floating point positive
/// infinity value.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_pinf(float *dst_xyzw);

/// Sets the elements of a vector to the unit-length x-axis value <1,0>.
///
/// @param dst_xy Pointer to the destination storage.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_unit_x(float *dst_xy);

/// Sets the elements of a vector to the unit-length x-axis value <1,0,0>.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_unit_x(float *dst_xyz);

/// Sets the elements of a vector to the unit-length x-axis value <1,0,0,0>.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_unit_x(float *dst_xyzw);

/// Sets the elements of a vector to the unit-length y-axis value <0,1>.
///
/// @param dst_xy Pointer to the destination storage.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_set_unit_y(float *dst_xy);

/// Sets the elements of a vector to the unit-length y-axis value <0,1,0>.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_unit_y(float *dst_xyz);

/// Sets the elements of a vector to the unit-length y-axis value <0,1,0,0>.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_unit_y(float *dst_xyzw);

/// Sets the elements of a vector to the unit-length z-axis value <0,0,1>.
///
/// @param dst_xyz Pointer to the destination storage.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_set_unit_z(float *dst_xyz);

/// Sets the elements of a vector to the unit-length z-axis value <0,1,0,0>.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_unit_z(float *dst_xyzw);

/// Sets the elements of a vector to the unit-length w-axis value <0,0,0,1>.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_set_unit_w(float *dst_xyzw);

/// Compares two vector values for equality.
///
/// @param a_xy The first vector value.
/// @param b_xy The second vector value.
/// @return true if @a a_xy and @a b_xy can be considered equal.
CMN_PUBLIC bool vec2_eq(
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy);

/// Compares two vector values for equality.
///
/// @param a_xyz The first vector value.
/// @param b_xyz The second vector value.
/// @return true if @a a_xyz and @a b_xyz can be considered equal.
CMN_PUBLIC bool vec3_eq(
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz);

/// Compares two vector values for equality.
///
/// @param a_xyzw The first vector value.
/// @param b_xyzw The second vector value.
/// @return true if @a a_xyzw and @a b_xyzw can be considered equal.
CMN_PUBLIC bool vec4_eq(
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Performs component-wise addition of two vector quantities, storing the
/// result in a third, such that dst = a + b.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The first source value.
/// @param b_xy The second source value.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_add(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy);

/// Performs component-wise addition of two vector quantities, storing the
/// result in a third, such that dst = a + b.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The first source value.
/// @param b_xyz The second source value.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_add(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz);

/// Performs component-wise addition of two vector quantities, storing the
/// result in a third, such that dst = a + b.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The first source value.
/// @param b_xyzw The second source value.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_add(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Performs component-wise subtraction of two vector quantities, storing the
/// result in a third, such that dst = a - b.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The first source value.
/// @param b_xy The second source value.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_sub(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy);

/// Performs component-wise subtraction of two vector quantities, storing the
/// result in a third, such that dst = a - b.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The first source value.
/// @param b_xyz The second source value.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_sub(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz);

/// Performs component-wise subtraction of two vector quantities, storing the
/// result in a third, such that dst = a - b.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The first source value.
/// @param b_xyzw The second source value.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_sub(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Performs component-wise multiplication of two vector quantities, storing
/// the result in a third, such that dst = a * b.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The first source value.
/// @param b_xy The second source value.
/// @return The pointer to @a dst_xy.
CMN_PUBLIC float* vec2_mul(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy);

/// Performs component-wise multiplication of two vector quantities, storing
/// the result in a third, such that dst = a * b.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The first source value.
/// @param b_xyz The second source value.
/// @return The pointer to @a dst_xyz.
CMN_PUBLIC float* vec3_mul(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz);

/// Performs component-wise multiplication of two vector quantities, storing
/// the result in a third, such that dst = a * b.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The first source value.
/// @param b_xyzw The second source value.
/// @return The pointer to @a dst_xyzw.
CMN_PUBLIC float* vec4_mul(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Performs component-wise division of two vector quantities, storing the
/// result in a third, such that dst = a / b.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The first source value.
/// @param b_xy The second source value.
/// @return The pointer to @a dst_xy.
CMN_PUBLIC float* vec2_div(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy);

/// Performs component-wise division of two vector quantities, storing the
/// result in a third, such that dst = a / b.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The first source value.
/// @param b_xyz The second source value.
/// @return The pointer to @a dst_xyz.
CMN_PUBLIC float* vec3_div(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz);

/// Performs component-wise division of two vector quantities, storing the
/// result in a third, such that dst = a / b.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The first source value.
/// @param b_xyzw The second source value.
/// @return The pointer to @a dst_xyzw.
CMN_PUBLIC float* vec4_div(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Multiplies each component of a vector value by a scalar.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The source vector value.
/// @param b The scalar value.
/// @return The pointer to @a dst_xy.
CMN_PUBLIC float* vec2_scl(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float                      b);

/// Multiplies each component of a vector value by a scalar.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The source vector value.
/// @param b The scalar value.
/// @return The pointer to @a dst_xyz.
CMN_PUBLIC float* vec3_scl(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float                      b);

/// Multiplies each component of a vector value by a scalar.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The source vector value.
/// @param b The scalar value.
/// @return The pointer to @a dst_xyzw.
CMN_PUBLIC float* vec4_scl(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float                      b);

/// Multiplies each component of a vector value by a scalar. Only the first
/// three components of the vector are multiplied by the scalar value.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The source vector value.
/// @param b The scalar value.
/// @return The pointer to @a dst_xyzw.
CMN_PUBLIC float* vec4_scl3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float                      b);

/// Negates each component of a vector value, preserving the magnitude but
/// reversing the direction.
///
/// @param dst_xy Pointer to the destination storage.
/// @param src_xy The source vector value.
/// @return The pointer to @a dst_xy.
CMN_PUBLIC float* vec2_neg(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy);

/// Negates each component of a vector value, preserving the magnitude but
/// reversing the direction.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xyz The source vector value.
/// @return The pointer to @a dst_xyz.
CMN_PUBLIC float* vec3_neg(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz);

/// Negates each component of a vector value, preserving the magnitude but
/// reversing the direction.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param src_xyzw The source vector value.
/// @return The pointer to @a dst_xyzw.
CMN_PUBLIC float* vec4_neg(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw);

/// Negates each component of a vector value, preserving the magnitude but
/// reversing the direction. Only the first three components are negated.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param src_xyzw The source vector value.
/// @return The pointer to @a dst_xyzw.
CMN_PUBLIC float* vec4_neg3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw);

/// Computes the dot product of two vectors.
///
/// @param dst On return, this value is set to the dot product of a and b.
/// @param a_xy Vector value a.
/// @param b_xy Vector value b.
/// @return The dot product of the vectors.
CMN_PUBLIC float vec2_dot(
    float                      &dst,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy);

/// Computes the dot product of two vectors.
///
/// @param dst On return, this value is set to the dot product of a and b.
/// @param a_xyz Vector value a.
/// @param b_xyz Vector value b.
/// @return The dot product of the vectors.
CMN_PUBLIC float vec3_dot(
    float                      &dst,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz);

/// Computes the dot product of two vectors.
///
/// @param dst On return, this value is set to the dot product of a and b.
/// @param a_xyzw Vector value a.
/// @param b_xyzw Vector value b.
/// @return The dot product of the vectors.
CMN_PUBLIC float vec4_dot(
    float                      &dst,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Computes the dot product of two vectors, considering only the first three
/// components of each.
///
/// @param dst On return, this value is set to the dot product of a and b.
/// @param a_xyzw Vector value a.
/// @param b_xyzw Vector value b.
/// @return The dot product of the vectors.
CMN_PUBLIC float vec4_dot3(
    float                      &dst,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Calculates the magnitude (length) of a vector.
///
/// @param dst On return, this value is set to the length of the vector.
/// @param a_xy The vector value.
/// @return The magnitude (length) of vector @a a_xy.
CMN_PUBLIC float vec2_len(float &dst, float const *a_xy);

/// Calculates the magnitude (length) of a vector.
///
/// @param dst On return, this value is set to the length of the vector.
/// @param a_xyz The vector value.
/// @return The magnitude (length) of vector @a a_xyz.
CMN_PUBLIC float vec3_len(float &dst, float const *a_xyz);

/// Calculates the magnitude (length) of a vector.
///
/// @param dst On return, this value is set to the length of the vector.
/// @param a_xyzw The vector value.
/// @return The magnitude (length) of vector @a a_xyzw.
CMN_PUBLIC float vec4_len(float &dst, float const *a_xyzw);

/// Calculates the magnitude (length) of a vector. Only the first three
/// components of the vector are considered.
///
/// @param dst On return, this value is set to the length of the vector.
/// @param a_xyzw The vector value.
/// @return The magnitude (length) of vector @a a_xyzw.
CMN_PUBLIC float vec4_len3(float &dst, float const *a_xyzw);

/// Calculates the squared magnitude (length) of a vector.
///
/// @param dst On return, this value is set to the squared length of the
/// vector.
/// @param a_xy The vector value.
/// @return The squared magnitude (length) of vector @a a_xy.
CMN_PUBLIC float vec2_len_sq(float &dst, float const *a_xy);

/// Calculates the squared magnitude (length) of a vector.
///
/// @param dst On return, this value is set to the squared length of the
/// vector.
/// @param a_xyz The vector value.
/// @return The squared magnitude (length) of vector @a a_xyz.
CMN_PUBLIC float vec3_len_sq(float &dst, float const *a_xyz);

/// Calculates the squared magnitude (length) of a vector.
///
/// @param dst On return, this value is set to the squared length of the
/// vector.
/// @param a_xyzw The vector value.
/// @return The squared magnitude (length) of vector @a a_xyzw.
CMN_PUBLIC float vec4_len_sq(float &dst, float const *a_xyzw);

/// Calculates the squared magnitude (length) of a vector.
///
/// @param dst On return, this value is set to the squared length of the
/// vector.
/// @param a_xy The vector value.
/// @return The squared magnitude (length) of vector @a a_xy.
CMN_PUBLIC float vec4_len3_sq(float &dst, float const *a_xyzw);

/// Calculates the normalized (unit-length) vector for a given vector value.
/// The normalized vector has the same direction, but magnitude 1.
///
/// @param dst_xy Pointer to the destination storage.
/// @param src_xy The source vector value.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_nrm(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy);

/// Calculates the normalized (unit-length) vector for a given vector value.
/// The normalized vector has the same direction, but magnitude 1.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xyz The source vector value.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_nrm(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz);

/// Calculates the normalized (unit-length) vector for a given vector value.
/// The normalized vector has the same direction, but magnitude 1.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param src_xyzw The source vector value.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_nrm(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw);

/// Calculates the normalized (unit-length) vector for a given vector value.
/// The normalized vector has the same direction, but magnitude 1. Only the
/// first three components of @a src_xyzw are considered when computing the
/// length and normalizing the vector.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param src_xyzw The source vector value.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_nrm3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw);

/// Calculates a vector perpendicular to a given vector, but with the same
/// magnitude (length).
///
/// @param dst_xy Pointer to the destination storage.
/// @param src_xy The source vector value.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_perp(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy);

/// Calculats the cross product of two vectors, producing a third vector that
/// is orthogonal to the source vectors; the dot product of the resulting
/// vector with either of the source vectors is zero.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The first source vector.
/// @param b_xyz The second source vector.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_cross(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz);

/// Calculats the cross product of two vectors, producing a third vector that
/// is orthogonal to the source vectors; the dot product of the resulting
/// vector with either of the source vectors is zero. The w-component of the
/// resulting vector is always zero, since the cross product operation always
/// results in a vector value.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The first source vector.
/// @param b_xyzw The second source vector.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_cross(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw);

/// Performs a swizzle operation on a vector or point value to select or change
/// the order of components.
///
/// @param dst_xy Pointer to the destination storage.
/// @param src_xy The source value.
/// @param x The zero-based index of the source component that will be written
/// to the destination value at index 0.
/// @param y The zero-based index of the source component that will be written
/// to the destination value at index 1.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_swizzle(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT src_xy,
    size_t                     x,
    size_t                     y);

/// Performs a swizzle operation on a vector or point value to select or change
/// the order of components.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param src_xyz The source value.
/// @param x The zero-based index of the source component that will be written
/// to the destination value at index 0.
/// @param y The zero-based index of the source component that will be written
/// to the destination value at index 1.
/// @param z The zero-based index of the source component that will be written
/// to the destination value at index 2.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_swizzle(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT src_xyz,
    size_t                     x,
    size_t                     y,
    size_t                     z);

/// Performs a swizzle operation on a vector or point value to select or change
/// the order of components.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param src_xyzw The source value.
/// @param x The zero-based index of the source component that will be written
/// to the destination value at index 0.
/// @param y The zero-based index of the source component that will be written
/// to the destination value at index 1.
/// @param z The zero-based index of the source component that will be written
/// to the destination value at index 2.
/// @param w The zero-based index of the source component that will be written
/// to the destination value at index 3.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_swizzle(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw,
    size_t                     x,
    size_t                     y,
    size_t                     z,
    size_t                     w);

/// Performs componentwise linear interpolation between two vector or point
/// quantities.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The value at @a t = 0.
/// @param b_xy The value at @a t = 1.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_linear(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy,
    float                      t);

/// Performs componentwise linear interpolation between two vector or point
/// quantities.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The value at @a t = 0.
/// @param b_xyz The value at @a t = 1.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_linear(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz,
    float                      t);

/// Performs componentwise linear interpolation between two vector or point
/// quantities.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The value at @a t = 0.
/// @param b_xyzw The value at @a t = 1.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_linear(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float                      t);

/// Performs componentwise linear interpolation between two vector or point
/// quantities. Only the first three components are interpolated.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The value at @a t = 0.
/// @param b_xyzw The value at @a t = 1.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_linear3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float                      t);

/// Performs componentwise Bezier interpolation between two vector or point
/// quantities.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The value at @a t = 0.
/// @param b_xy The value at @a t = 1.
/// @param itan_xy The incoming tangent value.
/// @param otan_xy The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_bezier(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy,
    float const * CMN_RESTRICT itan_xy,
    float const * CMN_RESTRICT otan_xy,
    float                      t);

/// Performs componentwise Bezier interpolation between two vector or point
/// quantities.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The value at @a t = 0.
/// @param b_xyz The value at @a t = 1.
/// @param itan_xyz The incoming tangent value.
/// @param otan_xyz The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_bezier(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t);

/// Performs componentwise Bezier interpolation between two vector or point
/// quantities.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The value at @a t = 0.
/// @param b_xyzw The value at @a t = 1.
/// @param itan_xyzw The incoming tangent value.
/// @param otan_xyzw The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_bezier(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyzw,
    float const * CMN_RESTRICT otan_xyzw,
    float                      t);

/// Performs componentwise Bezier interpolation between two vector or point
/// quantities. Only the first three components are interpolated.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The value at @a t = 0.
/// @param b_xyzw The value at @a t = 1.
/// @param itan_xyzw The incoming tangent value.
/// @param otan_xyzw The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_bezier3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t);

/// Performs componentwise Hermite interpolation between two vector or point
/// quantities.
///
/// @param dst_xy Pointer to the destination storage.
/// @param a_xy The value at @a t = 0.
/// @param b_xy The value at @a t = 1.
/// @param itan_xy The incoming tangent value.
/// @param otan_xy The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xy.
CMN_PUBLIC float* vec2_hermite(
    float       * CMN_RESTRICT dst_xy,
    float const * CMN_RESTRICT a_xy,
    float const * CMN_RESTRICT b_xy,
    float const * CMN_RESTRICT itan_xy,
    float const * CMN_RESTRICT otan_xy,
    float                      t);

/// Performs componentwise Hermite interpolation between two vector or point
/// quantities.
///
/// @param dst_xyz Pointer to the destination storage.
/// @param a_xyz The value at @a t = 0.
/// @param b_xyz The value at @a t = 1.
/// @param itan_xyz The incoming tangent value.
/// @param otan_xyz The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyz.
CMN_PUBLIC float* vec3_hermite(
    float       * CMN_RESTRICT dst_xyz,
    float const * CMN_RESTRICT a_xyz,
    float const * CMN_RESTRICT b_xyz,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t);

/// Performs componentwise Hermite interpolation between two vector or point
/// quantities.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The value at @a t = 0.
/// @param b_xyzw The value at @a t = 1.
/// @param itan_xyzw The incoming tangent value.
/// @param otan_xyzw The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_hermite(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyzw,
    float const * CMN_RESTRICT otan_xyzw,
    float                      t);

/// Performs componentwise Hermite interpolation between two vector or point
/// quantities. Only the first three components are interpolated.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param a_xyzw The value at @a t = 0.
/// @param b_xyzw The value at @a t = 1.
/// @param itan_xyz The incoming tangent value.
/// @param otan_xyz The outgoing tangent value.
/// @param t A value in the range [0, 1] specifying the interpolation
/// parameter.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* vec4_hermite3(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT a_xyzw,
    float const * CMN_RESTRICT b_xyzw,
    float const * CMN_RESTRICT itan_xyz,
    float const * CMN_RESTRICT otan_xyz,
    float                      t);

/// Sets a quaternion value by explicitly providing values for the x, y, z and
/// w fields.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param x The x-component of the quaternion.
/// @param y The y-component of the quaternion.
/// @param z The z-component of the quaternion.
/// @param w The w-component of the quaternion.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* quat_set_xyzw(
    float *dst_xyzw,
    float  x,
    float  y,
    float  z,
    float  w);

/// Copies a quaternion value.
///
/// @param dst_xyzw Pointer to the destination storage.
/// @param src_xyzw Pointer to the source quaternion.
/// @return The pointer @a dst_xyzw.
CMN_PUBLIC float* quat_set_quat(
    float       * CMN_RESTRICT dst_xyzw,
    float const * CMN_RESTRICT src_xyzw);

CMN_PUBLIC float* quat_set_nan(float *dst_xyzw);
CMN_PUBLIC float* quat_set_one(float *dst_xyzw);
CMN_PUBLIC float* quat_set_zero(float *dst_xyzw);
CMN_PUBLIC float* quat_set_ninf(float *dst_xyzw);
CMN_PUBLIC float* quat_set_pinf(float *dst_xyzw);
CMN_PUBLIC float* quat_set_ident(float *dst_xyzw);
CMN_PUBLIC bool   quat_eq(float const *a_xyzw, float const *b_xyzw);
CMN_PUBLIC float* quat_add(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw);
CMN_PUBLIC float* quat_sub(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw);
CMN_PUBLIC float* quat_mul(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw);
CMN_PUBLIC float* quat_scl(float *dst_xyzw, float const *a_xyzw, float b);
CMN_PUBLIC float* quat_scl3(float *dst_xyzw, float const *a_xyzw, float b);
CMN_PUBLIC float* quat_neg(float *dst_xyzw, float const *src_xyzw);
CMN_PUBLIC float* quat_neg3(float *dst_xyzw, float const *src_xyzw);
CMN_PUBLIC float* quat_conj(float *dst_xyzw, float const *src_xyzw);
CMN_PUBLIC float  quat_dot(float const *a_xyzw, float const *b_xyzw);
CMN_PUBLIC float  quat_norm(float const *src_xyzw);
CMN_PUBLIC float  quat_len(float const *src_xyzw);
CMN_PUBLIC float  quat_len_sq(float const *src_xyzw);
CMN_PUBLIC float  quat_sel(float const *src_xyzw);
CMN_PUBLIC float* quat_inv(float *dst_xyzw, float const *src_xyzw);
CMN_PUBLIC float* quat_nrm(float *dst_xyzw, float const *src_xyzw);
CMN_PUBLIC float* quat_exp(float *dst_xyzw, float const *src_xyzw);
CMN_PUBLIC float* quat_log(float *dst_xyzw, float const *src_xyzw);
CMN_PUBLIC float* quat_closest(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw);
CMN_PUBLIC float* quat_orient_vec3(float *dst_xyzw, float const *from_xyz, float const *to_xyz);
CMN_PUBLIC float* quat_orient_vec4(float *dst_xyzw, float const *from_xyzw, float const *to_xyzw);
CMN_PUBLIC float* quat_set_mat4(float *dst_xyzw, float const *m16);
CMN_PUBLIC float* quat_set_euler_degree(float *dst_xyzw, float deg_x, float deg_y, float deg_z);
CMN_PUBLIC float* quat_set_euler_radian(float *dst_xyzw, float rad_x, float rad_y, float rad_z);
CMN_PUBLIC float* quat_set_angle_axis_degree_n(float *dst_xyzw, float angle_deg, float const *axis_xyz);
CMN_PUBLIC float* quat_set_angle_axis_radian_n(float *dst_xyzw, float angle_rad, float const *axis_xyz);
CMN_PUBLIC float* quat_set_angle_axis_degree_u(float *dst_xyzw, float angle_deg, float const *axis_xyz);
CMN_PUBLIC float* quat_set_angle_axis_degree_U(float *dst_xyzw, float angle_dad, float const *axis_xyz);
CMN_PUBLIC float* quat_linear(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw, float t);
CMN_PUBLIC float* quat_slerp(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw, float t);
CMN_PUBLIC float* quat_squad(float *dst_xyzw, float const *p_xyzw, float const *a_xyzw, float const *b_xyzw, float const *q_xyzw, float t);
CMN_PUBLIC float* quat_spline(float *dst_xyzw, float const *a_xyzw, float const *b_xyzw, float const *c_xyzw);

CMN_PUBLIC float*  mat4_set(float *dst16, float  r0c0, float r0c1, float r0c2, float r0c3, float  r1c0, float r1c1, float r1c2, float r1c3, float  r2c0, float r2c1, float r2c2, float r2c3, float  r3c0, float r3c1, float r3c2, float r3c3);
CMN_PUBLIC float*  mat4_set_mat4x4(float *dst16, float const *src16);
CMN_PUBLIC float*  mat4_set_nan(float *dst16);
CMN_PUBLIC float*  mat4_set_one(float *dst16);
CMN_PUBLIC float*  mat4_set_zero(float *dst16);
CMN_PUBLIC float*  mat4_set_negative_infinity(float *dst16);
CMN_PUBLIC float*  mat4_set_positive_infinity(float *dst16);
CMN_PUBLIC float*  mat4_set_identity(float *dst16);
CMN_PUBLIC int32_t mat4_equal(float const *a16, float const *b16);
CMN_PUBLIC int32_t mat4_is_identity(float const *src16);
CMN_PUBLIC float*  mat4_set_rows(float *dst16, float const *r0_xyzw, float const *r1_xyzw, float const *r2_xyzw, float const *r3_xyzw);
CMN_PUBLIC float*  mat4_set_columns(float *dst16, float const *c0_xyzw, float const *c1_xyzw, float const *c2_xyzw, float const *c3_xyzw);
CMN_PUBLIC float*  mat4_get_row(float *dst_xyzw, size_t rowIndex, float const *src16);
CMN_PUBLIC float*  mat4_set_row(float *dst16,   size_t rowIndex, float const *src_xyzw);
CMN_PUBLIC float*  mat4_get_column(float *dst_xyzw, size_t columnIndex, float const *src16);
CMN_PUBLIC float*  mat4_set_column(float *dst16,   size_t columnIndex, float const *src_xyzw);
CMN_PUBLIC float   mat4_trace(float const *src16);
CMN_PUBLIC float   mat4_determinant(float const *src16);
CMN_PUBLIC float*  mat4_transpose(float *dst16, float const *src16);
CMN_PUBLIC float*  mat4_concatenate(float *dst16, float const *a16, float const *b16);
CMN_PUBLIC float*  mat4_inverse_affine(float *dst16, float const *src16);
CMN_PUBLIC float*  mat4_orientation_quat(float *dst16, float const *src_xyzw);
CMN_PUBLIC float*  mat4_orientation_x_deg(float *dst16, float degX);
CMN_PUBLIC float*  mat4_orientation_x_rad(float *dst16, float radX);
CMN_PUBLIC float*  mat4_orientation_y_deg(float *dst16, float degY);
CMN_PUBLIC float*  mat4_orientation_y_rad(float *dst16, float radY);
CMN_PUBLIC float*  mat4_orientation_z_deg(float *dst16, float degZ);
CMN_PUBLIC float*  mat4_orientation_z_rad(float *dst16, float radZ);
CMN_PUBLIC float*  mat4_orientation_euler_deg(float *dst16, float degX, float degY, float degZ);
CMN_PUBLIC float*  mat4_orientation_euler_rad(float *dst16, float radX, float radY, float radZ);
CMN_PUBLIC float*  mat4_orientation_look_at(float *dst16, float const *pos_xyz, float const *target_xyz, float const *up_xyz);
CMN_PUBLIC float*  mat4_orientation_angle_axis_normalized_deg(float *dst16, float angleDeg, float const *axis_xyz);
CMN_PUBLIC float*  mat4_orientation_angle_axis_normalized_rad(float *dst16, float angleRad, float const *axis_xyz);
CMN_PUBLIC float*  mat4_orientation_angle_axis_unnormalized_deg(float *dst16, float angleDeg, float const *axis_xyz);
CMN_PUBLIC float*  mat4_orientation_angle_axis_unnormalized_rad(float *dst16, float angleRad, float const *axis_xyz);
CMN_PUBLIC float*  mat4_scale(float *dst16, float scaleX, float scaleY, float scaleZ);
CMN_PUBLIC float*  mat4_translation(float *dst16, float translationX, float translationY, float translationZ);
CMN_PUBLIC float*  mat4_orthographic(float *dst16, float left, float right, float bottom, float top, float nearClipDepth, float farClipDepth);
CMN_PUBLIC float*  mat4_perspective_deg(float *dst16, float fovDeg, float aspectRatio, float nearClipDepth, float farClipDepth);
CMN_PUBLIC float*  mat4_perspective_rad(float *dst16, float fovRad, float aspectRatio, float nearClipDepth, float farClipDepth);
CMN_PUBLIC float*  mat4_perspective_2d(float *dst16, float width, float height);
CMN_PUBLIC void    mat4_extract_frustum_normalized(float *left_xyzD, float *right_xyzD, float *top_xyzD, float *bottom_xyzD, float *near_xyzD, float *far_xyzD, float const *proj16);
CMN_PUBLIC void    mat4_extract_frustum_unnormalized(float *left_xyzD, float *right_xyzD, float *top_xyzD, float *bottom_xyzD, float *near_xyzD, float *far_xyzD, float const *proj16);
CMN_PUBLIC float*  mat4_transform_vec3(float *dst_xyz, float const *src_xyz, float const *transform16);
CMN_PUBLIC float*  mat4_transform_vec4(float *dst_xyzw, float const *src_xyzw, float const *transform16);
CMN_PUBLIC float*  mat4_transform_point(float *dst_xyz, float const *src_xyz, float const *transform16);
CMN_PUBLIC float*  mat4_transform_vector(float *dst_xyz, float const *src_xyz, float const *transform16);
CMN_PUBLIC float*  mat4_transform_array_vec3(float *dstArr_xyz, float const *srcArr_xyz, float const *transform16, size_t countVec3);
CMN_PUBLIC float*  mat4_transform_array_vec4(float *dstArr_xyzw, float const *srcArr_xyzw, float const *transform16, size_t countVec4);
CMN_PUBLIC float*  mat4_transform_array_point(float *dstArr_xyz, float const *srcArr_xyz, float const *transform16, size_t countVec3);
CMN_PUBLIC float*  mat4_transform_array_vector(float *dstArr_xyz, float const *srcArr_xyz, float const *transform16, size_t countVec3);
CMN_PUBLIC size_t  interval_intersect(float *wSE, float u0, float u1, float v0, float v1);
CMN_PUBLIC size_t  line2_intersect(float *i0_xy, float const *p0_xy, float const *d0_xy, float const *p1_xy, float const *d1_xy);
CMN_PUBLIC size_t  segment2_intersect(float *i0_xy, float *i1_xy, float const *p0_xy, float const *d0_xy, float const *p1_xy, float const *d1_xy);
CMN_PUBLIC void    segment2_parallel_offset(float *a0_xy, float *a1_xy, float *b0_xy, float *b1_xy, float const *p0_xy, float const *p1_xy, float offset);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace math */

#endif /* LIBMATH_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
