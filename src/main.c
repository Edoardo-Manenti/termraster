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
	float x;
	float y;
} Vec2;

typedef struct {
	float x;
	float y;
	float z;
} Vec3;

Vec3 mult(Vec3 v, float c) {
	return (Vec3) {
		v.x*c,
		v.y*c,
		v.z*c,
	};
}

Vec3 add(Vec3 a, Vec3 b) {
	return (Vec3) {
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	};
}

Vec3 sub(Vec3 a, Vec3 b) {
	return add(a, mult(b, -1.));
}


Vec3 cross(Vec3 a, Vec3 b) {
	return (Vec3) {
		a.y*b.z-b.y*a.z,
		a.z*b.x-b.z*a.x,
		a.x*b.y-b.x*a.y
	};
}

float dot(Vec3 a, Vec3 b) {
	return a.x*b.x+a.y*b.y+a.z*b.z;
}

Vec3 normalize(Vec3 v) {
	float mod = sqrtf(dot(v, v));
	if (mod < EPSILON) {
		return (Vec3){0};
	}
	return mult(v, 1/mod);
}

typedef struct {
	int a, b, c;
} Triangle;

typedef struct {
	Vec3 a, b, c;
	Vec3 n_a, n_b, n_c;
} Surface;

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

		Vec3 n = normalize(cross(sub(b,a), sub(c,a)));

		m->normals[t.a] = add(m->normals[t.a], n);
		m->normals[t.b] = add(m->normals[t.b], n);
		m->normals[t.c] = add(m->normals[t.c], n);
	}

	//normalize
	for (int i=0; i < m->num_vertices; i++) {
		m->normals[i] = normalize(m->normals[i]);
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
	cam->position = (Vec3) {0,0,0};
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
int cube(Vec3 center, float l, Mesh *out) {
	//check
	if (out->num_vertices != 8 ||
		out->num_triangles != 12) {
		return -1;
	}

	float x = center.x;
	float y = center.y;
	float z = center.z;
	float h = l*0.5f;
	
	out->vertices[0] = (Vec3) {x+h,y+h,z+h};
	out->vertices[1] = (Vec3) {x+h,y+h,z-h};
	out->vertices[2] = (Vec3) {x-h,y+h,z-h};
	out->vertices[3] = (Vec3) {x-h,y+h,z+h};
	out->vertices[4] = (Vec3) {x+h,y-h,z+h};
	out->vertices[5] = (Vec3) {x+h,y-h,z-h};
	out->vertices[6] = (Vec3) {x-h,y-h,z-h};
	out->vertices[7] = (Vec3) {x-h,y-h,z+h};
	
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

int rotate_Y(const Mesh *m, float theta, Vec3 center, Mesh *rotated) {
	for (int i=0; i < m->num_vertices; i++) {
		//x' = x cos(theta) - z sin(theta)
		//y' = y
		//z' = x sin(theta) + z cos(theta)
		Vec3 p = m->vertices[i];
		rotated ->vertices[i] = (Vec3) {
			.x = (p.x - center.x)*cosf(theta) - (p.z - center.z)*sinf(theta) + center.x,
			.y = p.y,
			.z = (p.x - center.x)*sinf(theta) + (p.z - center.z)*cosf(theta) + center.z
		};
	}
	return 0;
}

int rotate_X(const Mesh *m, float theta, Vec3 center, Mesh *rotated) {
	for (int i=0; i < m->num_vertices; i++) {
		//x' = x
		//y' = y cos(theta) - z sin(theta)
		//z' = x sin(theta) + z cos(theta)
		Vec3 p = m->vertices[i];
		rotated ->vertices[i] = (Vec3) {
			.x = p.x,
			.y = (p.y - center.y)*cosf(theta) - (p.z - center.z)*sinf(theta) + center.y,
			.z = (p.y - center.y)*sinf(theta) + (p.z - center.z)*cosf(theta) + center.z
		};
	}
	return 0;
}

int p_to_index(FrameBuffer *fb, int x, int y) {
	int w = fb-> width;
	int h = fb-> height;

	if (x < 0 || y < 0 || x >= w || y >= h) {
		return -1;
	}

	return y * w + x;
}

Vec2 project_point(Vec3 p, float f, Vec2 c) {
	return (Vec2) {
		.x = c.x + (f*p.x/p.z)*X_CORRECTION,
		.y = c.y - (f*p.y/p.z)
	};
}

int draw_pixel(FrameBuffer *fb, Vec2 p, float z, float brightness) {
	int i = p_to_index(fb, p.x, p.y);
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
	return  (c.x-a.x)*(c.y-b.y) - (c.y-a.y)*(c.x - b.x);
}

void draw_surface(FrameBuffer *fb, Camera *cam, Light *light, Surface s) {
	//Centroid of surface
	Vec3 centroid = mult(add(s.a, add(s.b, s.c)), 0.33f);
	//Camera vector
	Vec3 cam_vector = sub(centroid, cam->position);
	Vec3 light_vector = normalize(mult(light->direction, -1.));
	//Surface normal vectors
	Vec3 norm_a = normalize(cross(sub(s.b,s.a), sub(s.c,s.a)));

	//keep back-culling based on the  A-vertice
	if (dot(cam_vector, norm_a)>0) {
		return;
	}

	//Fb center
	Vec2 sc = {(float)fb->width/2, (float)fb->height/2};

	//Project
	Vec2 aa = project_point(s.a, cam->focal, sc);
	Vec2 bb = project_point(s.b, cam->focal, sc);
	Vec2 cc = project_point(s.c, cam->focal, sc);

	//Bounding box
	int min_x, min_y, max_x, max_y;
	min_x = fmin(aa.x, fmin(bb.x, cc.x));
	min_y = fmin(aa.y, fmin(bb.y, cc.y));
	max_x = fmax(aa.x, fmax(bb.x, cc.x));
	max_y = fmax(aa.y, fmax(bb.y, cc.y));
	for (float y=min_y; y < max_y; y += 1.) {
		for (float x=min_x; x < max_x; x += 1.) {
			Vec2 p = {x+0.5,y+0.5};
			float area = edge_function(aa, bb, cc);
			float w0 = edge_function(aa, bb, p)/area;
			float w1 = edge_function(bb, cc, p)/area;
			float w2 = edge_function(cc, aa, p)/area;
			//Interpolate normal vector
			Vec3 norm = normalize(add(mult(s.n_a, w1),
			     add(mult(s.n_b, w2), mult(s.n_c, w0))));
			//Diffuse
			float brightness = 0.15 + 0.85*fmax(0., dot(light_vector, norm));
			//interpolate z
			float z = w0*s.a.z+w1*s.b.z+w2*s.c.z;
			if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || 
				(w0 <= 0 && w1 <= 0 && w2 <= 0)) {
				draw_pixel(fb, p, z, brightness);
			}
		}
	}
}

void draw(FrameBuffer *fb, Camera *cam, Light *light, Mesh *m) {
	clear_frame(fb);
	calculate_vertices_normals(m);
	for(int i=0; i < m->num_triangles; i++) {
		Triangle t = m->triangles[i];
		Surface s = {
			m->vertices[t.a],
			m->vertices[t.b],
			m->vertices[t.c],
			m->normals[t.a],
			m->normals[t.b],
			m->normals[t.c]
		};
		draw_surface(fb, cam, light, s);
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

/*
 * NEXT STEPS:
 * Rasterization ✓
 * Phong normals ✓
 * Object/Transform abstraction
 * Matrix math
 * Renderer abstraction
 * OBJ loading
 * Modularization
 * Camera movement
 * Textures
 * Perspective-correct interpolation
 * */

int main() {
	printf(ED);
	printf(CU);
	int rc = 0;
	Mesh m = {0};
	Mesh rotated = {0};
	Scene v = {0};
	FrameBuffer fb = {0};
	Camera cam = {0};
	Light light = {0};

	if (init_scene(WIDTH, HEIGHT, FL, &v, &fb, &cam, &light) != 0) {
		fprintf(stderr, "ERROR: Could not allocate view\n");
		rc = 8;
		goto cleanup;
	}
	if(init_mesh(&m, 8, 12) != 0 || init_mesh(&rotated, 8, 12) != 0){
		fprintf(stderr, "ERROR: Could not allocate model");
		rc = 8;
		goto cleanup;
	};
	cube((Vec3){0,0,200}, 15, &m);
	if (copy_model(&rotated, &m) != 0) {
		fprintf(stderr, "ERROR: Could not copy model");
		rc = 8;
		goto cleanup;
	};

	float theta = 0;
	while(1) {
		theta += 0.3*2*M_PI/60;
		rotate_Y(&m, theta, (Vec3){0,0,200}, &rotated);
		rotate_X(&rotated, 0.6*theta, (Vec3){0,0,200},&rotated);
		draw(&fb, &cam, &light, &rotated);
		print(&fb);
		usleep(16 * 1000);
	}

	//cleanup
	cleanup:
	free_model(&m);
	free_model(&rotated);
	free_view(&v);
	return rc;
}

