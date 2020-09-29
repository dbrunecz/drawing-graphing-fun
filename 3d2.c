/* Copyright (C) 2020 David Brunecz. Subject to GPL 2.0 */

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "dbx.h"


#if 0
____________LINE___________________________________________________
direction vector d = (l, m, n), passing through (x1, y1, z1)
line:    (x - x1) / l = (y - y1) / m = (z - z1) / n

____________PLANE__________________________________________________
ax + by + cz + d = 0
#endif

#if 0

            ^      -|
           / \      |
          / | \     | d
         /  |  \    |
        /   |   \   |
       /____|____\ -|
       |----a----|

#endif
float viewing_angle(float aperature, float distance)
{
	return atan((aperature / 2.0f) / distance) * 2.0f;
}

struct vec { float x[2]; };

struct vec3 { float x[3]; };
struct line { struct vec3 p, d; };

void angle2vector(struct vec3 *d, float theta, float phi, int dbg)
{
	d->x[0] = cos(theta) * cos(phi);
	d->x[1] = sin(theta) * cos(phi);
	d->x[2] = sin(phi);
}

void vec3_sum_scale(struct vec3 *dst, struct vec3 *src, float scale)
{
	int i;

	for (i = 0; i < 3; i++)
		dst->x[i] += scale * src->x[i];
}

struct mat {
	float x[2][2];
};

struct mat rot;

void rotation(struct mat *m, float t, float w)
{
	float x = w * t;

	m->x[0][0] = cos(x);
	m->x[0][1] = -sin(x);

	m->x[1][0] = sin(x);
	m->x[1][1] = cos(x);
}

int float_cmp(float a, float range)
{
	return fabs(a) < fabs(range) ? 1 : 0;
}

int z_line_intersect(struct line *l, float z, float *x, float *y)
{
	if (float_cmp(l->d.x[2] - 0.0f, 0.0001f))
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

void mat_mult(struct mat *m, struct vec *v_in, struct vec *v_out)
{
	int i, j;

	for (i = 0; i < 2; i++) {
		v_out->x[i] = 0.0f;
		for (j = 0; j < 2; j++)
			v_out->x[i] += m->x[i][j] * v_in->x[j];
	}
}

/******************************************************************************/

int up, down, left, right, fwd, rev, hi, lo;
struct state {
	struct vec3 p;
	float theta, phi;
} state = { .p = { .x = { 0.0f, 0.0f, 350.0f } }, .theta = 60.0f, .phi = -0.12f };

void update_state(void)
{
	struct vec3 d;

	if (up && !down)
		state.phi += 0.01f;
	else if (!up && down)
		state.phi -= 0.01f;

	if (left && !right)
		state.theta -= 0.05f;
	else if (!left && right)
		state.theta += 0.05f;

	if (hi && !lo)
		state.p.x[2] += 5.0f;
	else if (!hi && lo) {
		state.p.x[2] -= 5.0f;
		if (state.p.x[2] < 5.0f)
			state.p.x[2] = 5.0f;
	}

	angle2vector(&d, state.theta, 0, 0);
	if (fwd && !rev)
		vec3_sum_scale(&state.p, &d, 65.5f);
	else if (!fwd && rev)
		vec3_sum_scale(&state.p, &d, -65.5f);
}

#if 1
#define GNCLR	0x202040
#define GDCLR	0xb0
#define GSHFT	0
#define GSCL	1000.0f
#else
#define GNCLR	0x204020
#define GDCLR	0xb0
#define GSHFT	8
#define GSCL	600.0f
#endif

#if 0
#define GMOD	200.0f
#define GDELT	5.0f
#else
#define GMOD	150.0f
#define GDELT	2.0f
#endif

u32 ground_clr(struct line *l, float x, float y)
{

	float d = GSCL / sqrt(pow(x - l->p.x[0], 2) + pow(y - l->p.x[1], 2));
	float gmod = GMOD;

	if (d > 1.0f)
		d = 1.0f;

	if (sqrt(pow(x, 2) + pow(y, 2)) < 700.0f)
		return 0x400000;
	//if (!float_cmp(d, 0.001))
	//printf("%4.3f\n", d);
	//for (i = 0.0f; i < d; i += 0.01f)
	//	gmod += 0.1f;

	x = fmod(x, gmod);
	y = fmod(y, gmod);

	if (float_cmp(x, GDELT))
		return GNCLR + (((u32)(GDCLR * d) & 0xff) << GSHFT);
	if (float_cmp(y, GDELT))
		return GNCLR + (((u32)(GDCLR * d) & 0xff) << GSHFT);

	return GNCLR;
}

int ground_intersect(struct line *l, float *x, float *y)
{
	return z_line_intersect(l, 0.0f, x, y);
}

u32 sphere_clr(struct line *l, float x, float y)
{
	return 0x008000;
}

int sphere_intersect(struct line *l, float *x, float *y)
{
	// perpendicular
	return -1;
}

char msg[256];
void ray_trace(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	float x_aper = 1.0f;
	float y_aper = 1.0f * ht / wd;
	float xa, xs, xva = viewing_angle(x_aper, 1.6f);
	float ya, ys, yva = viewing_angle(y_aper, 1.6f);
	int x, y;
	float xi, yi;
	struct line l = { .p = state.p };
	int ret;

	ys = yva / ht;
	xs = xva / wd;

	ya = -1.0f * yva / 2.0f;
	for (y = ht; y >= 0; y--, ya += ys) {
		xa = -1.0f * xva / 2.0f;
		for (x = 0; x < wd; x++, xa += xs) {
			angle2vector(&l.d, state.theta + xa, state.phi + ya, 1);
#if 0
			ret = sphere_intersect(&l, &xi, &yi);
			if (ret <= 0)
				dbx_draw_point(d, x, y, 0);
			else
				dbx_draw_point(d, x, y, sphere_clr(&l, xi, yi));
#endif
			ret = ground_intersect(&l, &xi, &yi);
			if (ret <= 0) {
				if (ret < 0)
					printf("%d %d %d\n", x, y, x * y);
				dbx_draw_point(d, x, y, 0);
			}
			else
				dbx_draw_point(d, x, y, ground_clr(&l, xi, yi));
		}
	}

	snprintf(msg, sizeof(msg), "(%4.2f, %4.2f, %4.2f) <%4.2f, %4.2f>",
		state.p.x[0], state.p.x[1], state.p.x[2],
		state.theta, state.phi);
	dbx_draw_string(d, 20, 20, msg, strlen(msg), 0xf0f000);
}

static int update(struct dbx *d)
{
	dbx_blank_pixmap(d);
	update_state();
	ray_trace(d);
	return 0;
}

#define LEFT	0xff51
#define UP	0xff52
#define RIGHT	0xff53
#define DOWN	0xff54
static int key(struct dbx *d, int code, int key, int press)
{
	switch (key) {
	case LEFT:   left = press; break;
	case UP:       up = press; break;
	case RIGHT: right = press; break;
	case DOWN:   down = press; break;
	case 'f':     fwd = press; break;
	case 'r':     rev = press; break;
	case 'h':      hi = press; break;
	case 'l':      lo = press; break;
	}
	return key != 'q' ? 0 : -1;
}

#define UPDATE_PERIOD_MS	30
int main(int argc, char *argv[])
{
	struct dbx_ops ops = { .update = update, .key = key, };

	dbx_run(argc, argv, &ops, UPDATE_PERIOD_MS);
	return EXIT_SUCCESS;
}
