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
} P2;

typedef struct {
	float x;
	float y;
	float z;
} P3;

typedef struct {
	int a;
	int b;
} Line;

typedef struct {
	P3 *points;
	Line *lines;
	int num_points;
	int num_lines;
} Model;

int init_model(Model *m, int num_points, int num_lines) {
	m->points = num_points ? malloc(num_points * sizeof(*m->points)) : NULL;
	m->lines  = num_lines  ? malloc(num_lines  * sizeof(*m->lines))  : NULL;

	if ((num_points && !m->points) ||
		(num_lines  && !m->lines)) {
		free(m->points);
		free(m->lines);
		*m = (Model){0};
		return -1;
	}

	m->num_points = num_points;
	m->num_lines  = num_lines;
	return 0;
}

int copy_model(Model *dest, const Model *src) {
	if ((dest->num_points != src->num_points)
		|| (dest->num_lines != src->num_lines)) {
		fprintf(stderr, "ERROR: src model and dest model sizes do not match");
		return -1;
	}
	if (src->num_points) {
		memcpy(dest->points, src->points, src->num_points*sizeof(P3));
	}
	if (src->num_lines) {
		memcpy(dest->lines, src->lines, src->num_lines*sizeof(Line));
	}
	dest->num_points = src->num_points;
	dest->num_lines = src->num_lines;
	return 0;
}

void free_model(Model *m) {
	free(m->points);
	free(m->lines);
	m->num_points = 0;
	m->num_lines = 0;
	m->points = NULL;
	m->lines = NULL;
}

typedef struct {
	int width;
	int height;
	P2 center;
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
	out->center = (P2) {
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
	v->center = (P2) {0,0};
	v->focal = 0;
	v->background = ' ';
}

//bottom left point
int cube(P3 p, float l, Model *out) {
	float x = p.x;
	float y = p.y;
	float z = p.z;
	
	out->points[0] = (P3) {x,y,z};
	out->points[1] = (P3) {x+l,y,z};
	out->points[2] = (P3) {x+l,y+l,z};
	out->points[3] = (P3) {x,y+l,z};
	out->points[4] = (P3) {x,y,z+l};
	out->points[5] = (P3) {x+l,y,z+l};
	out->points[6] = (P3) {x+l,y+l,z+l};
	out->points[7] = (P3) {x,y+l,z+l};
	
	int counter = 0;
	for (int i=0; i < 8; i++) {
		for (int j=i+1; j < 8; j++) {
			P3 a = out->points[i];
			P3 b = out->points[j];

			if ((a.x != b.x && a.y==b.y && a.z==b.z)
				|| (a.x == b.x && a.y!=b.y && a.z==b.z)
				|| (a.x == b.x && a.y==b.y && a.z!=b.z)
			) {
				out->lines[counter++] = (Line) {i,j};
			}
		}
	}

	return 0;
}

//center + radius + density (how many points per 360°)
int sphere(P3 center, float radius, int density, Model *out) {
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
			out->points[counter++] = (P3) {
				.x = x+radius*sinf(alpha)*cosf(beta), 
				.y = y+radius*sinf(alpha)*sinf(beta), 
				.z = z+radius*cosf(alpha)
			};
		}
	}
	return 0;
}

int rotate_Y(const Model *m, float theta, P3 center, Model *rotated) {
	for (int i=0; i < m->num_points; i++) {
		//x' = x cos(theta) - z sin(theta)
		//y' = y
		//z' = x sin(theta) + z cos(theta)
		P3 p = m->points[i];
		rotated ->points[i] = (P3) {
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

P2 p3_to_p2(P3 p, float f, P2 c) {
	return (P2) {
		.x = c.x + (f*p.x/p.z)*X_CORRECTION,
		.y = c.y - (f*p.y/p.z)
	};
}

int draw_pixel(View *v, P2 p, float z, int brightness) {
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

int draw_point(View *v, P3 p, int brightness) {
	brightness = brightness > 255? 255 : brightness;
	P2 pp = p3_to_p2(p, v->focal, v->center);
		
	draw_pixel(v, pp, p.z, brightness);
	return 0;
}

int draw_line(View *v, P3 a, P3 b, int brightness) {
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
		draw_point(v, (P3){a.x+x,a.y+y,a.z+z}, brightness);
	}
	return 0;
}

void draw(View *view, Model *m) {
	clear_view(view);

	for(int i=0; i < m->num_points; i++) {
		draw_point(view, m->points[i], 255);
	}
	for(int i=0; i < m->num_lines; i++) {
		int a = m->lines[i].a;
		int b = m->lines[i].b;
		draw_line(view, m->points[a], m->points[b], 255);
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
	Model m = {0};
	Model rotated = {0};
	View v = {0};

	if (init_view(WIDTH, HEIGHT, FL, ASCII_RAMP[0], &v) != 0) {
		fprintf(stderr, "ERROR: Could not allocate view\n");
		rc = 8;
		goto cleanup;
	}
	if(init_model(&m, 8, 12) != 0 || init_model(&rotated, 8, 12) != 0){
		fprintf(stderr, "ERROR: Could not allocate model");
		rc = 8;
		goto cleanup;
	};
	cube((P3){-10,-10,200}, 20, &m);
	if (copy_model(&rotated, &m) != 0) {
		fprintf(stderr, "ERROR: Could not copy model");
		rc = 8;
		goto cleanup;
	};

	float theta = 0;
	while(1) {
		theta += 0.5*2*M_PI/60;
		rotate_Y(&m, theta, (P3){0,0,210}, &rotated);
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

