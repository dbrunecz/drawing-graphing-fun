#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "dbx.h"

int ratelimit(u32 *last, u32 ms_period)
{
	u32 ms = tickcount_ms();

	if ((ms - (*last)) < ms_period)
		return 1;

	*last = ms;
	return 0;
}

float uptime(void)
{
	u32 ms = tickcount_ms();
	static u32 base;
	float f;

	if (!base)
		base = ms;
	ms -= base;

	f = (ms / 1000) + ((float)(ms % 1000) / 1000.0f);
	return f;
}

#if 0

____________LINE___________________________________________________
direction vector d = (l, m, n), passing through (x1, y1, z1)
line:    (x - x1) / l = (y - y1) / m = (z - z1) / n

____________PLANE__________________________________________________
ax + by + cz + d = 0


#endif

#if 0
#if 0

            ^
           / \
          / | \
         /  |  \
        /   |   \
       /____|____\

#endif

struct vec { float x, y, z; };

struct camera {
	struct vec pos, ang;
	struct aperature { float x, y; } a;
} cam = { .pos = { 0.0f, -1.0f, 0.0f },
	  .ang = { 0.0f,  1.0f, 0.0f },
	  .a.x = 1.0f };

/******************************************************************************/
#endif

float viewing_angle(float aperature, float distance)
{
	return atan((aperature / 2.0f) / distance) * 2.0f;
}

struct state {
	float x, y, scale;
} state = { .x = 0.0f, .y = 0.0f, .scale = 6.0f };

int z_in, z_out;
int X, Y, up, down, left, right;
char message[64];

void screen2coord(struct dbx *d, int x, int y, float *fx, float *fy)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	float yscale = (float)ht / (float)wd;
	float min, max;

	min = state.x - state.scale;
	max = state.x + state.scale;
	*fx = transform(0, wd, x, min, max);

	min = state.y - state.scale * yscale;
	max = state.y + state.scale * yscale;
	*fy = transform(0, ht, y, max, min);
}

void coord2screen(struct dbx *d, float fx, float fy, int *x, int *y)
{
	int wd = dbx_width(d);
	int ht = dbx_height(d);
	float yscale = (float)ht / (float)wd;
	float min, max;

	min = state.x - state.scale;
	max = state.x + state.scale;
	*x = (int)transform(min, max, fx,  0, wd);

	min = state.y - state.scale * yscale;
	max = state.y + state.scale * yscale;
	*y = (int)transform(min, max, fy, ht, 0);
}

int expv(float v)
{
	float f;
	int i;

	if (v >= 1.0f) {
		for (i = 0, f = 1.0f; f <= v; f *= 10.0f, i++)
			;
		return i - 1;
	} else {
		for (i = -1, f = 0.1f; f >= v; f /= 10.0f, i--)
			;
		return i + 1;
	}
}

#define GRIDCLR	0x204020
static void draw_grid(struct dbx *d)
{
	float min = state.x - state.scale;
	float max = state.x + state.scale;
	int e = expv(state.scale);
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	float yscale = (float)ht / (float)wd;
	float f, step;
	int x, y;

	step = powf(10, e - 1);
	if ((max - min) / step > 30)
		step = powf(10, e);

	f = (roundf(min / step) + 1) * step;
	for (; f < max; f += step) {
		x = (int)transform(min, max, f, 0, wd);
		dbx_draw_line(d, x, 0, x, ht, GRIDCLR);
	}
	if (min <= 0.0f && max >= 0.0f) {
		x = (int)transform(min, max, 0.0f, 0, wd);
		dbx_draw_line(d, x, 0, x, ht, GRIDCLR + 0x303030);
	}

	min = state.y - state.scale * yscale;
	max = state.y + state.scale * yscale;
	f = (roundf(min / step) + 1) * step;
	for (; f < max; f += step) {
		y = (int)transform(min, max, f, ht, 0);
		dbx_draw_line(d, 0, y, wd, y, GRIDCLR);
	}
	if (min <= 0.0f && max >= 0.0f) {
		y = (int)transform(min, max, 0.0f, ht, 0);
		dbx_draw_line(d, 0, y, wd, y, GRIDCLR + 0x303030);
	}
}

void mouse_coord(struct dbx *d)
{
	float x, y;

	screen2coord(d, X, Y, &x, &y);
	snprintf(message, 32, "%5.3f %5.3f", x, y);
	dbx_draw_string(d, 20, 20, message, strlen(message), 0xf0ff00);
}

void alt_dir_upd(int a, int b, float *v, float a_delta, float b_delta)
{
	if ((a && b) || (!a && !b))
		return;
	if (a)
		*v += a_delta;
	else
		*v += b_delta;
}

void update_state(void)
{
	alt_dir_upd(z_in, z_out, &state.scale, -(state.scale / 15), state.scale / 15);

	alt_dir_upd(  up,  down, &state.y,  (state.scale / 40), -(state.scale / 40));
	alt_dir_upd(left, right, &state.x, -(state.scale / 40),  (state.scale / 40));
}

struct mat {
	float x[2][2];
};

struct mat rot = { { { 0.0f, -1.0f }, { 1.0f, 0.0f } } };

void rotation(struct mat *m, float t, float w)
{
	float x = w * t;

	m->x[0][0] = cos(x);
	m->x[0][1] = -sin(x);

	m->x[1][0] = sin(x);
	m->x[1][1] = cos(x);
}

struct vec { float x[2]; };

struct vec3 { float x[3]; };
struct line { struct vec3 p, d; };

int float_cmp(float a, float b, float range)
{
	return fabs(a - b) < fabs(range) ? 1 : 0;
}

int z_line_intersect(struct line *l, float z, float *x, float *y)
{
	if (float_cmp(l->d.x[2], 0.0f, 0.0001f))
		return 0;

	*x = ((z - l->p.x[2]) * l->d.x[0]) / l->d.x[2] + l->p.x[0];
	*y = ((z - l->p.x[2]) * l->d.x[1]) / l->d.x[2] + l->p.x[1];

	if ((l->d.x[0] > 0.0f && *x < l->p.x[0]) ||
	    (l->d.x[0] < 0.0f && *x > l->p.x[0]))
		return -1;

	if ((l->d.x[1] > 0.0f && *y < l->p.x[1]) ||
	    (l->d.x[1] < 0.0f && *y > l->p.x[1]))
		return -1;

	return 1;
}

void lp_intersect_test(void)
{
	//float x, y, z, xi, yi;
	struct line l = { .p.x = { 1.0f, 1.0f, 1.0f },
			  //.d.x = { 1.0f, 1.0f, 1.0f } };
			  .d.x = { -1.0f, -1.0f, -1.0f } };
	float x, y;
	int r;

	r = z_line_intersect(&l, 0.0f, &x, &y);
	printf("%4.2f %4.2f (%d)\n", x, y, r);
	//r = z_line_intersect(&l, 1.0f, &x, &y);
	//printf("%4.2f %4.2f (%d)\n", x, y, r);
	exit(0);
}

void print_vec(struct vec *v)
{
	int i;

	for (i = 0; i < 2; i++)
		printf("%2.2f ", v->x[i]);
	printf("\n");
}

struct vector {
	struct vec o, v;
	u32 clr;
	float w;
};

void mat_mult(struct mat *m, struct vec *v_in, struct vec *v_out)
{
	int i, j;

	for (i = 0; i < 2; i++) {
		v_out->x[i] = 0.0f;
		for (j = 0; j < 2; j++)
			v_out->x[i] += m->x[i][j] * v_in->x[j];
	}
	//print_vec(v_out);
}

void draw_vec(struct dbx *d, struct vector *v, float t)
{
	int x1, y1, x2, y2;
	struct vec n;

	rotation(&rot, t, v->w);
	mat_mult(&rot, &v->v, &n);

	coord2screen(d, v->o.x[0],          v->o.x[1],          &x1, &y1);
	coord2screen(d, v->o.x[0] + n.x[0], v->o.x[1] + n.x[1], &x2, &y2);

	dbx_draw_line(d, x1, y1, x2, y2, v->clr);
	dbx_fill_circle(d, x1 - 3, y1 - 3, 6, v->clr);
}

float semi_circ(float r, float px, float py, float x)
{
	return sqrt(pow(r, 2) - pow(x - px, 2)) + py;
}

void perpendicular(struct vec *in, struct vec *out)
{
	out->x[0] = in->x[1];
	out->x[1] = -1.0f * in->x[0];
}

#define HZ(w)		((w) * 2.0f * 3.14159f)
struct vector vs[] = {
	{ .o.x = { 1.0f, 0.0f }, .v.x = { 0.5f, 0.0f }, .w = HZ(1.0f), .clr = 0x6060d0 },
	{ .o.x = { -1.0f, 0.0f }, .v.x = { 1.5f, 0.0f }, .w = HZ(-2.0f), .clr = 0xe06050 },
};

void ray(struct dbx *d)
{
	float s, a, va = viewing_angle(1.0f, 0.6f);
	struct vector v = { .o.x = { 2.0f, 2.0f },
			    .v.x = { 1.4f, 1.4f },
			    .w = 1.0f, .clr = 0xe0c000 };
	int wd = dbx_width(d);
	//struct vec n;
	int i;

	s = va / wd;
	a = -1.0f * va / 2.0f;
	for (i = 0; i < wd; i++, a += s) {
		draw_vec(d, &v, a);

		//rotation(&rot, a, v.w);
		//mat_mult(&rot, &v.v, &n);

		//n.x[0] += v->o.x[0];
		//n.x[1] += v->o.x[1];
	}
}

void scan_window(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	float x_aper = 1.0f;
	float y_aper = 1.0f * ht / wd;
	float xa, xs, xva = viewing_angle(x_aper, 0.6f);
	float ya, ys, yva = viewing_angle(y_aper, 0.6f);
	int x, y;

	ys = yva / ht;
	xs = xva / wd;

	ya = -1.0f * yva / 2.0f;
	xa = -1.0f * xva / 2.0f;

	printf("X %5.3f %5.3f %5.3f\n", x_aper, xva, xs);
	printf("Y %5.3f %5.3f %5.3f\n", y_aper, yva, ys);

	for (y = 0; y < ht; y++, ya += ys) {
		for (x = 0; x < wd; x++, xa += xs) {
			printf("%4d %4d %5.3f %5.3f\n", y, x, ya, xa);
		}
	}
}

void graph(struct dbx *d)
{
	float t = uptime();
	int i;

	ray(d);

	for (i = 0; i < ARRAY_SIZE(vs); i++)
		draw_vec(d, &vs[i], t);
}

static int update(struct dbx *d)
{
	dbx_blank_pixmap(d);

	update_state();

	draw_grid(d);

	graph(d);

	mouse_coord(d);
	return 0;
}

static int motion(struct dbx *d, XMotionEvent *e)
{
	X = e->x;
	Y = e->y;
	return 0;
}

#define LEFT	0xff51
#define UP	0xff52
#define RIGHT	0xff53
#define DOWN	0xff54

static int key(struct dbx *d, int code, int key, int press)
{
	switch (key) {
	case UP:       up = press; break;
	case DOWN:   down = press; break;
	case LEFT:   left = press; break;
	case RIGHT: right = press; break;
	case '-':   z_out = press; break;
	case '+':    z_in = press; break;

	case '=':
		scan_window(d);
		z_in = 0;
		break;
	}
	return key != 'q' ? 0 : (press ? -1 : 0);
}

#define UPDATE_PERIOD_MS	30
int main(int argc, char *argv[])
{
	struct dbx_ops ops = {
		.update = update,
		.motion = motion,
		.key = key,
	};

	dbx_run(argc, argv, &ops, UPDATE_PERIOD_MS);

	//printf("%f", cam.pos.x);

	return EXIT_SUCCESS;
}
