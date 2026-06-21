#include <stdlib.h>
#include "mesh.h"

int init_mesh(Mesh *m, int num_vertices, int num_triangles) {
	m->vertices = num_vertices ? malloc(num_vertices * sizeof(*m->vertices)) : NULL;
	m->normals = num_vertices ? malloc(num_vertices * sizeof(*m->normals)) : NULL;
	m->triangles  = num_triangles  ? malloc(num_triangles  * sizeof(*m->triangles))  : NULL;

	if ((num_vertices && !m->vertices) ||
		(num_vertices && !m->normals) ||
		(num_triangles  && !m->triangles)) {
		free(m->vertices);
		free(m->normals);
		free(m->triangles);
		*m = (Mesh){0};
		return -1;
	}

	m->num_vertices = num_vertices;
	m->num_triangles  = num_triangles;
	return 0;
}

void calculate_vertices_normals(Mesh *m) {
	for (int i=0; i < m->num_vertices; i++) {
		m->normals[i] = (Vec3) {0, 0, 0};
	}

	for (int i=0; i < m->num_triangles; i++) {
		Triangle t = m->triangles[i];

		Vec3 a = m->vertices[t.a];
		Vec3 b = m->vertices[t.b];
		Vec3 c = m->vertices[t.c];

		Vec3 n = vec3_normalize(vec3_cross(vec3_sub(b,a), vec3_sub(c,a)));

		m->normals[t.a] = vec3_add(m->normals[t.a], n);
		m->normals[t.b] = vec3_add(m->normals[t.b], n);
		m->normals[t.c] = vec3_add(m->normals[t.c], n);
	}

	for (int i=0; i < m->num_vertices; i++) {
		m->normals[i] = vec3_normalize(m->normals[i]);
	}
}

void free_model(Mesh *m) {
	free(m->vertices);
	free(m->triangles);
	m->num_vertices = 0;
	m->num_triangles = 0;
	m->vertices = NULL;
	m->triangles = NULL;
}

int cube_mesh(Mesh *out) {
	if (out->num_vertices != 8 ||
		out->num_triangles != 12) {
		return -1;
	}
	float h = 0.5f;

	out->vertices[0] = (Vec3) {+h,+h,+h};
	out->vertices[1] = (Vec3) {+h,+h,-h};
	out->vertices[2] = (Vec3) {-h,+h,-h};
	out->vertices[3] = (Vec3) {-h,+h,+h};
	out->vertices[4] = (Vec3) {+h,-h,+h};
	out->vertices[5] = (Vec3) {+h,-h,-h};
	out->vertices[6] = (Vec3) {-h,-h,-h};
	out->vertices[7] = (Vec3) {-h,-h,+h};

	out->triangles[0] = (Triangle) {0,4,1};
	out->triangles[1] = (Triangle) {1,4,5};
	out->triangles[2] = (Triangle) {1,5,2};
	out->triangles[3] = (Triangle) {2,5,6};
	out->triangles[4] = (Triangle) {3,2,6};
	out->triangles[5] = (Triangle) {3,6,7};
	out->triangles[6] = (Triangle) {3,7,0};
	out->triangles[7] = (Triangle) {0,7,4};
	out->triangles[8] = (Triangle) {2,3,0};
	out->triangles[9] = (Triangle) {2,0,1};
	out->triangles[10] = (Triangle) {4,7,6};
	out->triangles[11] = (Triangle) {4,6,5};

	calculate_vertices_normals(out);
	return 0;
}

int free_obj(Object *obj) {
	obj->model = (Mat4) {0};
	obj->normal = (Mat3) {0};
	free_model(obj->mesh);
	return 0;
}

int obj_create_cube(Object *obj, float l, Vec3 center) {
	obj->mesh = malloc(sizeof(*obj->mesh));
	if (init_mesh(obj->mesh, 8, 12) < 0) {
		free_model(obj->mesh);
		return -1;
	};
	cube_mesh(obj->mesh);
	obj->model = mat4_mult(mat4_translate(center), mat4_mult(mat4_scale(l, l, l), mat4_identity()));
	obj->normal = mat3_inverse(mat3_transpose(mat4_extract_mat3(obj->model)));
	return 0;
}

void obj_rotateX(Object *obj, float theta) {
	obj->model = mat4_mult(mat4_rotate_X(theta), obj->model);
	obj->normal = mat3_inverse(mat3_transpose(mat4_extract_mat3(obj->model)));
}

void obj_rotateY(Object *obj, float theta) {
	obj->model = mat4_mult(mat4_rotate_Y(theta), obj->model);
	obj->normal = mat3_inverse(mat3_transpose(mat4_extract_mat3(obj->model)));
}

void obj_rotateZ(Object *obj, float theta) {
	obj->model = mat4_mult(mat4_rotate_Z(theta), obj->model);
	obj->normal = mat3_inverse(mat3_transpose(mat4_extract_mat3(obj->model)));
}
