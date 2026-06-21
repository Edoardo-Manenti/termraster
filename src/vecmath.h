#ifndef VECMATH_H
#define VECMATH_H

#include <stddef.h>
#include <math.h>

#define EPSILON 0.0001f

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;
typedef struct { float m[4][4]; } Mat4;
typedef struct { float m[3][3]; } Mat3;

/* Extract xy components from a 3D vector */
Vec2 vec3_extract_vec2(Vec3 v);

/* Multiply a 3D vector by a scalar */
Vec3 vec3_mult(Vec3 v, float c);

/* Add two 3D vectors */
Vec3 vec3_add(Vec3 a, Vec3 b);

/* Subtract two 3D vectors */
Vec3 vec3_sub(Vec3 a, Vec3 b);

/* Cross product of two 3D vectors */
Vec3 vec3_cross(Vec3 a, Vec3 b);

/* Dot product of two 3D vectors */
float vec3_dot(Vec3 a, Vec3 b);

/* Normalize a 3D vector to unit length */
Vec3 vec3_normalize(Vec3 v);

/* Extract xyz components from a 4D vector */
Vec3 vec4_extract_vec3(Vec4 v);

/* Transpose a 3x3 matrix */
Mat3 mat3_transpose(Mat3 m);

/* Determinant of a 3x3 matrix */
float mat3_determinant(Mat3 m);

/* Inverse of a 3x3 matrix */
Mat3 mat3_inverse(Mat3 m);

/* Extract the upper-left 3x3 submatrix from a 4x4 matrix */
Mat3 mat4_extract_mat3(Mat4 m);

/* Multiply a 3x3 matrix by a 3D direction vector */
Vec3 mat3_mult_dir(Mat3 m, Vec3 p);

/* Create a 4x4 identity matrix */
Mat4 mat4_identity(void);

/* Multiply two 4x4 matrices */
Mat4 mat4_mult(Mat4 a, Mat4 b);

/* Create a 4x4 rotation matrix around the X axis */
Mat4 mat4_rotate_X(float theta);

/* Create a 4x4 rotation matrix around the Y axis */
Mat4 mat4_rotate_Y(float theta);

/* Create a 4x4 rotation matrix around the Z axis */
Mat4 mat4_rotate_Z(float theta);

/* Create a 4x4 translation matrix */
Mat4 mat4_translate(Vec3 t);

/* Create a 4x4 scaling matrix */
Mat4 mat4_scale(float x, float y, float z);

/* Multiply a 4x4 matrix by a 3D vector (treats vector as homogeneous with w=1) */
Vec3 mat4_mult_vec3(Mat4 m, Vec3 p);

/* Multiply a 4x4 matrix by a 4D vector */
Vec4 mat4_mult_vec4(Mat4 m, Vec4 p);

#endif
