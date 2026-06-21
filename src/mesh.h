#ifndef MESH_H
#define MESH_H

#include "vecmath.h"

/* Triangle indices into a mesh vertex array */
typedef struct {
	int a, b, c;
} Triangle;

/* A mesh composed of vertices, normals, and triangle indices */
typedef struct {
	Vec3 *vertices;
	Triangle *triangles;
	Vec3 *normals;
	int num_vertices;
	int num_triangles;
} Mesh;

/* A renderable object: a mesh with model and normal matrices */
typedef struct {
	Mesh *mesh;
	Mat4 model;
	Mat3 normal;
} Object;

/* Allocate vertex, normal, and triangle buffers for a mesh */
int init_mesh(Mesh *m, int num_vertices, int num_triangles);

/* Compute smooth vertex normals by accumulating face normals */
void calculate_vertices_normals(Mesh *m);

/* Free the heap memory owned by a mesh */
void free_model(Mesh *m);

/* Fill a pre-allocated 8-vertex, 12-triangle mesh with a unit cube */
int cube_mesh(Mesh *out);

/* Free all resources held by an object */
int free_obj(Object *obj);

/* Create an object with a cube mesh scaled by l and translated to center */
int obj_create_cube(Object *obj, float l, Vec3 center);

/* Rotate an object around its local X axis */
void obj_rotateX(Object *obj, float theta);

/* Rotate an object around its local Y axis */
void obj_rotateY(Object *obj, float theta);

/* Rotate an object around its local Z axis */
void obj_rotateZ(Object *obj, float theta);

#endif
