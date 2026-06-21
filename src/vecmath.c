#include "vecmath.h"

Vec2 vec3_extract_vec2(Vec3 v) {
	return (Vec2) {v.x, v.y};
}

Vec3 vec3_mult(Vec3 v, float c) {
	return (Vec3) {v.x*c, v.y*c, v.z*c};
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
	return (Vec3) {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
	return vec3_add(a, vec3_mult(b, -1.));
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
	return (Vec3) {
		a.y*b.z-b.y*a.z,
		a.z*b.x-b.z*a.x,
		a.x*b.y-b.x*a.y
	};
}

float vec3_dot(Vec3 a, Vec3 b) {
	return a.x*b.x+a.y*b.y+a.z*b.z;
}

Vec3 vec3_normalize(Vec3 v) {
	float mod = sqrtf(vec3_dot(v, v));
	if (mod < EPSILON) return (Vec3){0};
	return vec3_mult(v, 1/mod);
}

Vec3 vec4_extract_vec3(Vec4 v) {
	return (Vec3) {v.x, v.y, v.z};
}

Mat3 mat3_transpose(Mat3 m) {
	Mat3 n = {0};
	for (size_t i=0; i<3; i++)
		for (size_t j=0; j<3; j++)
			n.m[i][j] = m.m[j][i];
	return n;
}

float mat3_determinant(Mat3 m) {
	return
	m.m[0][0] * (m.m[1][1]*m.m[2][2] - m.m[1][2]*m.m[2][1]) -
	m.m[0][1] * (m.m[1][0]*m.m[2][2] - m.m[1][2]*m.m[2][0]) +
	m.m[0][2] * (m.m[1][0]*m.m[2][1] - m.m[1][1]*m.m[2][0]);
}

Mat3 mat3_inverse(Mat3 m) {
	float det = mat3_determinant(m);
	if (fabsf(det) < EPSILON) return (Mat3) {0};
	float inv_det = 1.f/det;
	return (Mat3) {{
		{
			(m.m[1][1]*m.m[2][2] - m.m[1][2]*m.m[2][1]) * inv_det,
			(m.m[0][2]*m.m[2][1] - m.m[0][1]*m.m[2][2]) * inv_det,
			(m.m[0][1]*m.m[1][2] - m.m[0][2]*m.m[1][1]) * inv_det
		},
		{
			(m.m[1][2]*m.m[2][0] - m.m[1][0]*m.m[2][2]) * inv_det,
			(m.m[0][0]*m.m[2][2] - m.m[0][2]*m.m[2][0]) * inv_det,
			(m.m[0][2]*m.m[1][0] - m.m[0][0]*m.m[1][2]) * inv_det
		},
		{
			(m.m[1][0]*m.m[2][1] - m.m[1][1]*m.m[2][0]) * inv_det,
			(m.m[0][1]*m.m[2][0] - m.m[0][0]*m.m[2][1]) * inv_det,
			(m.m[0][0]*m.m[1][1] - m.m[0][1]*m.m[1][0]) * inv_det
		}
	}};
}

Mat3 mat4_extract_mat3(Mat4 m) {
	Mat3 n = {0};
	for (size_t i=0; i<3; i++)
		for (size_t j=0; j<3; j++)
			n.m[i][j] = m.m[i][j];
	return n;
}

Vec3 mat3_mult_dir(Mat3 m, Vec3 p) {
	Vec3 out = {0};
	out.x = m.m[0][0]*p.x+m.m[0][1]*p.y+m.m[0][2]*p.z;
	out.y = m.m[1][0]*p.x+m.m[1][1]*p.y+m.m[1][2]*p.z;
	out.z = m.m[2][0]*p.x+m.m[2][1]*p.y+m.m[2][2]*p.z;
	return out;
}

Mat4 mat4_identity(void) {
	Mat4 m = {0};
	m.m[0][0] = 1.f;
	m.m[1][1] = 1.f;
	m.m[2][2] = 1.f;
	m.m[3][3] = 1.f;
	return m;
}

Mat4 mat4_mult(Mat4 a, Mat4 b) {
	Mat4 m = {0};
	for (int i=0; i<4 ; i++) {
		for (int j=0; j<4; j++) {
			float temp = 0.f;
			for (int k=0; k<4; k++)
				temp += a.m[i][k]*b.m[k][j];
			m.m[i][j] = temp;
		}
	}
	return m;
}

Mat4 mat4_rotate_X(float theta) {
	Mat4 m = mat4_identity();
	m.m[1][1] = cosf(theta);
	m.m[1][2] = -sinf(theta);
	m.m[2][1] = sinf(theta);
	m.m[2][2] = cosf(theta);
	return m;
}

Mat4 mat4_rotate_Y(float theta) {
	Mat4 m = mat4_identity();
	m.m[0][0] = cosf(theta);
	m.m[0][2] = -sinf(theta);
	m.m[2][0] = sinf(theta);
	m.m[2][2] = cosf(theta);
	return m;
}

Mat4 mat4_rotate_Z(float theta) {
	Mat4 m = mat4_identity();
	m.m[0][0] = cosf(theta);
	m.m[0][1] = -sinf(theta);
	m.m[1][0] = sinf(theta);
	m.m[1][1] = cosf(theta);
	return m;
}

Mat4 mat4_translate(Vec3 t) {
	Mat4 m = mat4_identity();
	m.m[0][3] = t.x;
	m.m[1][3] = t.y;
	m.m[2][3] = t.z;
	return m;
}

Mat4 mat4_scale(float x, float y, float z) {
	Mat4 m = mat4_identity();
	m.m[0][0] = x;
	m.m[1][1] = y;
	m.m[2][2] = z;
	return m;
}

Vec3 mat4_mult_vec3(Mat4 m, Vec3 p) {
	Vec3 out = {0};
	out.x = m.m[0][0]*p.x+m.m[0][1]*p.y+m.m[0][2]*p.z+m.m[0][3];
	out.y = m.m[1][0]*p.x+m.m[1][1]*p.y+m.m[1][2]*p.z+m.m[1][3];
	out.z = m.m[2][0]*p.x+m.m[2][1]*p.y+m.m[2][2]*p.z+m.m[2][3];
	return out;
}

Vec4 mat4_mult_vec4(Mat4 m, Vec4 p) {
	Vec4 out = {0};
	out.x = m.m[0][0]*p.x+m.m[0][1]*p.y+m.m[0][2]*p.z+m.m[0][3]*p.w;
	out.y = m.m[1][0]*p.x+m.m[1][1]*p.y+m.m[1][2]*p.z+m.m[1][3]*p.w;
	out.z = m.m[2][0]*p.x+m.m[2][1]*p.y+m.m[2][2]*p.z+m.m[2][3]*p.w;
	out.w = m.m[3][0]*p.x+m.m[3][1]*p.y+m.m[3][2]*p.z+m.m[3][3]*p.w;
	return out;
}
