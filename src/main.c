#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<unistd.h>

#define WIDTH 120
#define HEIGHT 40
#define FL 200.
#define X_CORRECTION 2.

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

typedef struct {
	int a;
	int b;
	int c;
} Triangle;

typedef struct {
	Vec3 *vertices;
	Triangle *triangles;
	int num_vertices;
	int num_triangles;
} Mesh;

int init_mesh(Mesh *m, int num_vertices, int num_triangles) {
	m->vertices = num_vertices ? malloc(num_vertices * sizeof(*m->vertices)) : NULL;
	m->triangles  = num_triangles  ? malloc(num_triangles  * sizeof(*m->triangles))  : NULL;

	if ((num_vertices && !m->vertices) ||
		(num_triangles  && !m->triangles)) {
		free(m->vertices);
		free(m->triangles);
		*m = (Mesh){0};
		return -1;
	}

	m->num_vertices = num_vertices;
	m->num_triangles  = num_triangles;
	return 0;
}

int copy_model(Mesh *dest, const Mesh *src) {
	if ((dest->num_vertices != src->num_vertices)
		|| (dest->num_triangles != src->num_triangles)) {
		fprintf(stderr, "ERROR: src model and dest model sizes do not match");
		return -1;
	}
	if (src->num_vertices) {
		memcpy(dest->vertices, src->vertices, src->num_vertices*sizeof(Vec3));
	}
	if (src->num_triangles) {
		memcpy(dest->triangles, src->triangles, src->num_triangles*sizeof(Triangle));
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
	int width;
	int height;
	Vec2 center;
	float focal;
	char *pixels;
	char background;
	float *z_buffer;
} View;

int init_view(int width, int height, float focal, char background, View *out) {
	char* pixels = malloc(width*height*sizeof(char));
	if (pixels == NULL) {
		return -1;
	}
	float *z_buffer = malloc(width*height*sizeof(float));
	if (z_buffer == NULL) {
		free(pixels);
		return -1;
	}
	out->z_buffer = z_buffer;
	out->pixels = pixels;
	out->width = width;
	out->height = height;
	out->center = (Vec2) {
		.x = ((float)width)*0.5f,
		.y = ((float)height)*0.5f
	};
	out->focal = focal;
	out->background = background;
	memset(pixels, background, width*height);
	for (int i=0; i < width*height; i++) {
		out->z_buffer[i] = INFINITY;
	}
	return 0;
}

int clear_view(View *v) {
	memset(v->pixels, v->background, v->width*v->height);
	for (int i=0; i < v->width*v->height; i++) {
		v->z_buffer[i] = INFINITY;
	}
	return 0;
}

void free_view(View *v) {
	free(v->pixels);
	free(v->z_buffer);
	v->pixels = NULL;
	v->z_buffer = NULL;
	v->width = 0;
	v->height = 0;
	v->center = (Vec2) {0,0};
	v->focal = 0;
	v->background = ' ';
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
	
/*
 *       2-----1
 *      /|    /|
 *     3-+---0 |
 *     | 6---+-5
 *     |/    |/
 *     7-----4
 * */

	out->triangles[0] = (Triangle) {0,4,1};
	out->triangles[1] = (Triangle) {1,4,5};
	out->triangles[2] = (Triangle) {1,5,2};
	out->triangles[3] = (Triangle) {2,5,1};
	out->triangles[4] = (Triangle) {3,2,6};
	out->triangles[5] = (Triangle) {3,6,7};
	out->triangles[6] = (Triangle) {3,7,0};
	out->triangles[7] = (Triangle) {0,7,4};
	out->triangles[8] = (Triangle) {2,3,0};
	out->triangles[9] = (Triangle) {2,0,1};
	out->triangles[10] = (Triangle) {4,7,6};
	out->triangles[11] = (Triangle) {4,6,5};

	return 0;
}

//center + radius + density (how many vertices per 360°)
int sphere(Vec3 center, float radius, int density, Mesh *out) {
	float x = center.x;
	float y = center.y;
	float z = center.z;
	float step = 2*M_PI / (float)density;
	int counter = 0;
	float alpha, beta;
	for (int i = 0; i < density; i++) {
		alpha = (float)i*(step);
		for (int j = 0; j < density; j++) {
			beta = (float)j*(step);
			out->vertices[counter++] = (Vec3) {
				.x = x+radius*sinf(alpha)*cosf(beta), 
				.y = y+radius*sinf(alpha)*sinf(beta), 
				.z = z+radius*cosf(alpha)
			};
		}
	}
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


int p_to_index(View *v, int x, int y) {
	int w = v-> width;
	int h = v-> height;

	if (x < 0 || y < 0 || x >= w || y >= h) {
		return -1;
	}

	return y * w + x;
}

Vec2 p3_to_p2(Vec3 p, float f, Vec2 c) {
	return (Vec2) {
		.x = c.x + (f*p.x/p.z)*X_CORRECTION,
		.y = c.y - (f*p.y/p.z)
	};
}

int draw_pixel(View *v, Vec2 p, float z, int brightness) {
	int i = p_to_index(v, p.x, p.y);
	if (i < 0) {
		return -1;
	}
	if (z < v->z_buffer[i]) {
		v->pixels[i] = ASCII_RAMP[brightness * (ASCII_RAMP_SIZE-1) / 255];
		v->z_buffer[i] = z;
	}
	return 0;
}

int draw_point(View *v, Vec3 p, int brightness) {
	brightness = brightness > 255? 255 : brightness;
	Vec2 pp = p3_to_p2(p, v->focal, v->center);
		
	draw_pixel(v, pp, p.z, brightness);
	return 0;
}

int draw_line(View *v, Vec3 a, Vec3 b, int brightness) {
	brightness = brightness > 255? 255 : brightness;
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float dz = b.z - a.z;
	
	int step = fmaxf(fabsf(dx), fmaxf(fabsf(dy), fabsf(dz)));

	float step_x = dx / (float) step;
	float step_y = dy / (float) step;
	float step_z = dz / (float) step;
	
	float x = 0;
	float y = 0;
	float z = 0;
	
	for (int i=0; i < step; i++) {
		x += step_x;
		y += step_y;
		z += step_z;
		draw_point(v, (Vec3){a.x+x,a.y+y,a.z+z}, brightness);
	}
	return 0;
}

void draw(View *view, Mesh *m) {
	clear_view(view);
	for(int i=0; i < m->num_triangles; i++) {
		int a = m->triangles[i].a;
		int b = m->triangles[i].b;
		int c = m->triangles[i].c;
		draw_line(view, m->vertices[a], m->vertices[b], 255);
		draw_line(view, m->vertices[b], m->vertices[c], 255);
		draw_line(view, m->vertices[c], m->vertices[a], 255);
	}
}

void print(View *view) {
	printf(CU);

	int w = view -> width;
	int h = view -> height;

	for(int i=0; i < h; i++) {
		char row[w+1];
		for(int j=0; j < w; j++) {
			row[j] = view->pixels[i*w+j];
		}
		row[w] = '\0';
		printf("%s\n", row);
	}
}

/*
 * NEXT STEPS:
 * 1. Triangle data structure
 * 2. Back-face culling
 * 3. Filled triangle rasterization
 * 4. Depth interpolation
 * 5. Face normals
 * 6. Flat shading
 * 7. Camera movement
 * 8. OBJ mesh loading
 * 9. Gouraud/Phong shading
 * */

int main() {
	printf(ED);
	printf(CU);
	int rc = 0;
	Mesh m = {0};
	Mesh rotated = {0};
	View v = {0};

	if (init_view(WIDTH, HEIGHT, FL, ASCII_RAMP[0], &v) != 0) {
		fprintf(stderr, "ERROR: Could not allocate view\n");
		rc = 8;
		goto cleanup;
	}
	if(init_mesh(&m, 8, 12) != 0 || init_mesh(&rotated, 8, 12) != 0){
		fprintf(stderr, "ERROR: Could not allocate model");
		rc = 8;
		goto cleanup;
	};
	cube((Vec3){0,0,200}, 20, &m);
	if (copy_model(&rotated, &m) != 0) {
		fprintf(stderr, "ERROR: Could not copy model");
		rc = 8;
		goto cleanup;
	};

	float theta = 0;
	while(1) {
		theta += 0.5*2*M_PI/60;
		rotate_Y(&m, theta, (Vec3){0,0,200}, &rotated);
		draw(&v, &rotated);
		print(&v);
		usleep(16 * 1000);
	}

	//cleanup
	cleanup:
	free_model(&m);
	free_model(&rotated);
	free_view(&v);
	return rc;

}

