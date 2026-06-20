#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<unistd.h>

#define WIDTH 120
#define HEIGHT 40
#define FL 200.
#define X_CORRECTION 2.f
#define EPSILON 0.0001f

// Escape sequences
#define ED "\033[2J" //clear screen
#define CU "\033[H" //cursor to the top

#define ASCII_RAMP " .,:;irsXA253hMHGS#9B&@"
#define ASCII_RAMP_SIZE 23

typedef struct {
	float x, y;
} Vec2;

typedef struct {
	float x, y, z;
} Vec3;

typedef struct {
	float x, y, z, w;
} Vec4;

Vec2 vec3_extract_vec2(Vec3 v) {
	return (Vec2) {
		v.x,
		v.y
	};
}

Vec3 vec3_mult(Vec3 v, float c) {
	return (Vec3) {
		v.x*c,
		v.y*c,
		v.z*c,
	};
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
	return (Vec3) {
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	};
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
	if (mod < EPSILON) {
		return (Vec3){0};
	}
	return vec3_mult(v, 1/mod);
}

Vec3 vec4_extract_vec3(Vec4 v) {
	return (Vec3) {
		v.x,
		v.y,
		v.z,
	};
}

typedef struct {
	float m[4][4];
} Mat4;

typedef struct {
	float m[3][3];
} Mat3;

Mat3 mat3_identity(void) {
	Mat3 m = {0};
	m.m[0][0] = 1.f;
	m.m[1][1] = 1.f;
	m.m[2][2] = 1.f;
	return m;
}

Mat3 mat3_transpose(Mat3 m) {
	Mat3 n = {0};
	for (size_t i=0; i<3; i++) {
		for (size_t j=0; j<3; j++) {
			n.m[i][j] = m.m[j][i];
		}
	}
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
	if (fabsf(det) < EPSILON) {
		return (Mat3) {0};
	}
	float inv_det = 1.f/det;
	Mat3 r = {{
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
	return r;
}


Mat3 mat4_extract_mat3(Mat4 m) {
	Mat3 n = {0};

	for (size_t i=0; i<3; i++) {
		for (size_t j=0; j<3; j++) {
			n.m[i][j] = m.m[i][j];
		}
	}

	return n;
}

Mat3 mat3_mult(Mat3 a, Mat3 b) {
	Mat3 m = {0};
	for (int i=0; i<3 ; i++) {
		for (int j=0; j<3; j++) {
			float temp = 0.f;
			for (int k=0; k<3; k++) {
				temp += a.m[i][k]*b.m[k][j];
			}
			m.m[i][j] = temp;
		}
	}
	return m;
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
			for (int k=0; k<4; k++) {
				temp += a.m[i][k]*b.m[k][j];
			}
			m.m[i][j] = temp;
		}
	}
	return m;
}

typedef struct {
	int a, b, c;
} Triangle;

typedef struct {
	Vec3 a, b, c;
	Vec3 n_a, n_b, n_c;
} Surface;

typedef struct {
	Vec2 a, b, c;
	Vec3 n_a, n_b, n_c;
	float z_a, z_b, z_c;
} ScreenSurface;

typedef struct {
	Vec3 *vertices;
	Triangle *triangles;
	Vec3 *normals;
	int num_vertices;
	int num_triangles;
} Mesh;

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
	// fill default values
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

	//normalize
	for (int i=0; i < m->num_vertices; i++) {
		m->normals[i] = vec3_normalize(m->normals[i]);
	}
}

int copy_model(Mesh *dest, const Mesh *src) {
	if ((dest->num_vertices != src->num_vertices)
		|| (dest->num_triangles != src->num_triangles)) {
		fprintf(stderr, "ERROR: src model and dest model sizes do not match");
		return -1;
	}
	if (src->num_vertices) {
		memcpy(dest->vertices, src->vertices, src->num_vertices*sizeof(*src->vertices));
		memcpy(dest->normals, src->normals, src->num_vertices*sizeof(*src->normals));
	}
	if (src->num_triangles) {
		memcpy(dest->triangles, src->triangles, src->num_triangles*sizeof(*src->triangles));
	}
	dest->num_vertices = src->num_vertices;
	dest->num_triangles = src->num_triangles;
	return 0;
}

void free_model(Mesh *m) {
	free(m->vertices);
	free(m->triangles);
	m->num_vertices = 0;
	m->num_triangles = 0;
	m->vertices = NULL;
	m->triangles = NULL;
}

typedef struct {
	Vec3 direction;
	float brightness;
} Light;

typedef struct {
	Vec3 position;
	Vec3 forward;
	Vec3 up;
	float focal;
} Camera;

typedef struct {
	int width;
	int height;
	Camera *camera;
	Light *light;
} Scene;


typedef struct {
	int width;
	int height;
	char *pixels;
	float *z_buffer;
} FrameBuffer;

int init_scene(int width, int height, float focal, Scene *out, FrameBuffer *fb, Camera *cam, Light *light) {
	char* pixels = malloc(width*height*sizeof(char));
	if (pixels == NULL) {
		return -1;
	}
	float *z_buffer = malloc(width*height*sizeof(float));
	if (z_buffer == NULL) {
		free(pixels);
		return -1;
	}
	memset(pixels, ' ', width*height);
	fb->z_buffer = z_buffer;
	fb->pixels = pixels;
	fb->width = width;
	fb->height = height;
	out->width = width;
	out->height = height;
	out->camera = cam;
	out->light = light;
	light->direction = (Vec3) {-1, -1, 1};
	light->brightness = 0.f;
	cam->position = (Vec3) {0,0,200};
	cam->forward = (Vec3) {0,0,1};
	cam->up = (Vec3) {0,1,0};
	cam->focal = focal;
	for (int i=0; i < width*height; i++) {
		fb->z_buffer[i] = INFINITY;
	}
	return 0;
}

void free_light(Light *l) {
	l->direction = (Vec3) {0};
	l->brightness = 0;
}

void free_camera(Camera *cam) {
	cam->focal = 0;
	cam->position = (Vec3) {0};
	cam->forward = (Vec3) {0};
	cam->up = (Vec3) {0};
}

void free_fb(FrameBuffer *fb) {
	fb->width = 0;
	fb->height = 0;
	free(fb->z_buffer);
	free(fb->pixels);
	fb->z_buffer = NULL;
	fb->pixels = NULL;
}

int clear_frame(FrameBuffer *fb) {
	memset(fb->pixels, ' ', fb->width*fb->height);
	for (int i=0; i < fb->width*fb->height; i++) {
		fb->z_buffer[i] = INFINITY;
	}
	return 0;
}

void free_view(Scene *v) {
	v->width = 0;
	v->height = 0;
	free_camera(v->camera);
	v->camera = NULL;
}

//bottom left point
int cube_mesh(Mesh *out) {
	//check
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

Mat4 mat4_scale(float scaleX, float scaleY, float scaleZ) {
	Mat4 m = mat4_identity();
	m.m[0][0] = scaleX;
	m.m[1][1] = scaleY;
	m.m[2][2] = scaleZ;
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

typedef struct {
	Mesh *mesh;
	Mat4 model;
	Mat3 normal;
} Object;

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

int p_to_index(FrameBuffer *fb, int x, int y) {
	int w = fb-> width;
	int h = fb-> height;

	if (x < 0 || y < 0 || x >= w || y >= h) {
		return -1;
	}

	return y * w + x;
}

Mat4 mat4_view(Camera *cam) {
	return mat4_translate(vec3_mult(cam->position, -1.f));
}

Mat4 mat4_project(Camera *cam) {
	Mat4 p = {0};
	p.m[0][0] = cam->focal;
	p.m[1][1] = cam->focal;
	p.m[2][2] = 1.f;
	p.m[3][2] = 1.f;
	return p;
}

int draw_pixel(FrameBuffer *fb, Vec2 p, float z, float brightness) {
	Vec2 c = {fb->width/2., fb->height/2.};
	int i = p_to_index(fb, c.x+p.x*X_CORRECTION, c.y-p.y);
	if (i < 0) {
		return -1;
	}
	if (z < fb->z_buffer[i]) {
		fb->pixels[i] = ASCII_RAMP[(int)(brightness*(ASCII_RAMP_SIZE-1))];
		fb->z_buffer[i] = z;
	}
	return 0;
}

float edge_function(Vec2 a, Vec2 b, Vec2 c) {
	return  (c.x-a.x)*(c.y-b.y) - (c.y-a.y)*(c.x-b.x);
}

void draw_surface(FrameBuffer *fb, Light *light, ScreenSurface s) {
	//Centroid of surface
	Vec3 light_vector = vec3_normalize(light->direction);

	//Bounding box
	int min_x, min_y, max_x, max_y;
	min_x = fmin(s.a.x, fmin(s.b.x, s.c.x));
	min_y = fmin(s.a.y, fmin(s.b.y, s.c.y));
	max_x = fmax(s.a.x, fmax(s.b.x, s.c.x));
	max_y = fmax(s.a.y, fmax(s.b.y, s.c.y));
	for (float y=min_y; y < max_y; y += 1.) {
		for (float x=min_x; x < max_x; x += 1.) {
			Vec2 p = {x+0.5,y+0.5};
			float area = edge_function(s.a, s.b, s.c);
			float w0 = edge_function(s.b, s.c, p)/area;
			float w1 = edge_function(s.c, s.a, p)/area;
			float w2 = edge_function(s.a, s.b, p)/area;
			//Interpolate normal vector
			Vec3 norm = vec3_normalize(vec3_add(vec3_mult(s.n_a, w0),
			     vec3_add(vec3_mult(s.n_b, w1), vec3_mult(s.n_c, w2))));
			//Diffuse
			float brightness = 0.15 + 0.85*fmax(0., vec3_dot(light_vector, norm));
			//interpolate z
			float z = w0*s.z_a+w1*s.z_b+w2*s.z_c;
			if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || 
				(w0 <= 0 && w1 <= 0 && w2 <= 0)) {
				draw_pixel(fb, p, z, brightness);
			}
		}
	}
}

int back_culling(Surface s) {
	Vec3 centroid = vec3_mult(vec3_add(s.a, vec3_add(s.b, s.c)), 0.33f);
	//Camera vector
	Vec3 cam_vector = vec3_mult(centroid, -1.f);
	//Surface normal vectors
	Vec3 norm_a = vec3_normalize(vec3_cross(vec3_sub(s.b,s.a), vec3_sub(s.c,s.a)));

	//keep back-culling based on the  A-vertice
	return vec3_dot(cam_vector, norm_a) > 0 ? 1 : -1;
}

void draw_object(FrameBuffer *fb, Object *obj, Camera *cam, Light *light) {
	clear_frame(fb);
	Mesh *m = obj->mesh;
	Mat4 model = obj->model;
	Mat4 view = mat4_view(cam);
	Mat4 project = mat4_project(cam);
	Mat4 mv = mat4_mult(view, model);

	for(int i=0; i < m->num_triangles; i++) {
		Triangle t = m->triangles[i];
		Surface s = {
			mat4_mult_vec3(mv, m->vertices[t.a]),
			mat4_mult_vec3(mv, m->vertices[t.b]),
			mat4_mult_vec3(mv, m->vertices[t.c]),
			mat3_mult_dir(obj->normal, m->normals[t.a]),
			mat3_mult_dir(obj->normal, m->normals[t.b]),
			mat3_mult_dir(obj->normal, m->normals[t.c]),
		};

		//now in screen space
		//camera is at 0,0,0
		if (back_culling(s) < 0) {
			continue;
		}

		Vec4 clip_a = mat4_mult_vec4(project, (Vec4){s.a.x, s.a.y, s.a.z, 1});
		Vec4 clip_b = mat4_mult_vec4(project, (Vec4){s.b.x, s.b.y, s.b.z, 1});
		Vec4 clip_c = mat4_mult_vec4(project, (Vec4){s.c.x, s.c.y, s.c.z, 1});

		if (fabsf(clip_a.w) < EPSILON) continue;
		if (fabsf(clip_b.w) < EPSILON) continue;
		if (fabsf(clip_c.w) < EPSILON) continue;

		Vec3 ndc_a = vec3_mult(vec4_extract_vec3(clip_a), 1.f/clip_a.w);
		Vec3 ndc_b = vec3_mult(vec4_extract_vec3(clip_b), 1.f/clip_b.w);
		Vec3 ndc_c = vec3_mult(vec4_extract_vec3(clip_c), 1.f/clip_c.w);

		ScreenSurface ss = {
			vec3_extract_vec2(ndc_a),
			vec3_extract_vec2(ndc_b),
			vec3_extract_vec2(ndc_c),
			s.n_a, s.n_b, s.n_c,
			ndc_a.z, ndc_b.z, ndc_c.z
		};
		draw_surface(fb, light, ss);
	}
}

void print(FrameBuffer *fb) {
	printf(CU);

	int w = fb -> width;
	int h = fb -> height;

	for(int i=0; i < h; i++) {
		char row[w+1];
		for(int j=0; j < w; j++) {
			row[j] = fb->pixels[i*w+j];
		}
		row[w] = '\0';
		printf("%s\n", row);
	}
}

int main() {
	printf(ED);
	printf(CU);
	int rc = 0;
	Object cube = {0};
	Scene v = {0};
	FrameBuffer fb = {0};
	Camera cam = {0};
	Light light = {0};

	if (init_scene(WIDTH, HEIGHT, FL, &v, &fb, &cam, &light) != 0) {
		fprintf(stderr, "ERROR: Could not allocate view\n");
		rc = 8;
		goto cleanup;
	}
	if (obj_create_cube(&cube, 15, (Vec3){0,0,0}) != 0) {
		fprintf(stderr, "ERROR: Could not create cube\n");
		rc = 8;
		goto cleanup;
	};

	float d_theta = 0.3*2*M_PI/60;
	while(1) {
		obj_rotateY(&cube, d_theta);
		obj_rotateX(&cube, 0.5*d_theta);
		draw_object(&fb, &cube, &cam, &light);
		print(&fb);
		usleep(16 * 1000);
	}

	//cleanup
	cleanup:
	free_obj(&cube);
	free_view(&v);
	return rc;
}

