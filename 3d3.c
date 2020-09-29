#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "dbx.h"
#include "dbcl.h"
#include "loadfile.h"

float viewing_angle(float aperature, float distance)
{
	return atan((aperature / 2.0f) / distance) * 2.0f;
}

struct vec3 { float x[3]; };

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
		state.theta -= 0.01f;
	else if (!left && right)
		state.theta += 0.01f;

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

char msg[256];

struct dbcl *dbcl;

u32 *dat;
u32 dat_sz;

float prm_x;
float prm_y;
float prm_z;
float prm_theta;
float prm_phi;
float prm_yva;
float prm_xva;
float prm_ystep;
float prm_xstep;
u32 prm_wd;
u32 prm_ht;

#define CL_PRM(x)	{ &(x), sizeof(x) }

struct dbcl_param prms[] = {
	CL_PRM(prm_x),
	CL_PRM(prm_y),
	CL_PRM(prm_z),
	CL_PRM(prm_theta),
	CL_PRM(prm_phi),
	CL_PRM(prm_yva),
	CL_PRM(prm_xva),
	CL_PRM(prm_ystep),
	CL_PRM(prm_xstep),
	CL_PRM(prm_wd),
	CL_PRM(prm_ht),
};

void ray_trace(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	float x_aper = 1.0f;
	float y_aper = 1.0f * ht / wd;
	float xva = viewing_angle(x_aper, 1.9f);
	float yva = viewing_angle(y_aper, 1.9f);
	float xs = xva / wd;
	float ys = yva / ht;
	int i;
	//u64 us;

	if (!dat) {
		dat_sz = sizeof(*dat) * wd * ht;
		dat = malloc(dat_sz);
		if (dbcl_ouptut_buffer(dbcl, dat_sz)) {
			printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);
			exit(0);
		}

	}
	if (!dat)
		return;

	prm_x = state.p.x[0];
	prm_y = state.p.x[1];
	prm_z = state.p.x[2];
	prm_theta = state.theta;
	prm_phi = state.phi;
	prm_yva = yva;
	prm_xva = xva;
	prm_ystep = ys;
	prm_xstep = xs;
	prm_wd = wd;
	prm_ht = ht;

	if (dbcl_parameters(dbcl, prms, ARRAY_SIZE(prms)))
		printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);

	if (dbcl_run(dbcl, wd * ht, dat))
		printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);

	//us = tickcount_us();
	for (i = 0; i < wd * ht; i++)
		dbx_draw_point(d, i % wd, i / wd, dat[i]);
	//us = tickcount_us() - us;
	//printf("%" PRIu64 " us  (%u, %u)\n", us, wd, ht);

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
	return key != 'q' ? 0 : (press ? -1 : 0);
}

#define UPDATE_PERIOD_MS	30
int main(int argc, char *argv[])
{
	struct dbx_ops ops = { .update = update, .key = key, };
	const char *kernel;

	kernel = loadfile(argv[1]);
	if (!kernel) {
		printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);
		return EXIT_FAILURE;
	}

	dbcl = dbcl_open(&kernel);
	if (!dbcl) {
		free((char *)kernel);
		printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);
		return EXIT_FAILURE;
	}

	dbx_run(argc, argv, &ops, UPDATE_PERIOD_MS);

	dbcl_close(dbcl);
	free((char *)kernel);

	return EXIT_SUCCESS;
}
