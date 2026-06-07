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

P2 add(P2 a, P2 b) {
	return (P2) {a.x+a.x, a.y+b.y};
}

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

void init_model(Model *m, int num_points, int num_lines) {
	if (num_points) {
		m->points = malloc(num_points * sizeof(P3));
	}
	if (num_lines) {
		m->lines = malloc(num_lines * sizeof(Line));
	}
	m->num_points = num_points;
	m->num_lines = num_lines;
}

void copy_model(Model *out, Model *m) {
	if (m->num_points) {
		out->points = malloc(m->num_points * sizeof(P3));
		memcpy(out->points, m->points, m->num_points*sizeof(P3));
	}
	if (m->num_lines) {
		out->lines = malloc(m->num_lines * sizeof(Line));
		memcpy(out->lines, m->lines, m->num_lines*sizeof(Line));
	}
	out->num_points = m->num_points;
	out->num_lines = m->num_lines;
}

void free_model(Model *m) {
	free(m->points);
	free(m->lines);
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
	out->width = width;
	out->height = height;
	out->pixels = malloc(width*height*sizeof(char));
	out->z_buffer = malloc(width*height*sizeof(float));
	out->center = (P2) {
		.x = ((float)width)*0.5f,
		.y = ((float)height)*0.5f
	};
	out->focal = focal;
	out->background = background;
	memset(out->pixels, background, width*height);
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
}

//bottom left point
int cube(P3 p, float l, Model *out) {
	init_model(out, 8, 12);
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
	init_model(out, density*density, 0);
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

int rotate_Y(Model *m, float theta, P3 center, Model *rotated) {
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

	View v;
	init_view(WIDTH, HEIGHT, FL, ASCII_RAMP[0], &v);

	Model m;
	Model rotated;
	cube((P3){-10,-10,200}, 20, &m);
	copy_model(&rotated, &m);

	float theta = 0;
	while(1) {
		theta += 0.5*2*M_PI/60;
		rotate_Y(&m, theta, (P3){0,0,210}, &rotated);
		draw(&v, &rotated);
		print(&v);
		usleep(16 * 1000);
	}
	free_model(&m);
	free_model(&rotated);
	free_view(&v);
	return 0;
}
