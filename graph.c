#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "dbx.h"

int _random(int min, int max)
{
	static int once;

	if (!once) {
		srand(0);
		once = 1;
	}

	return (int)transform(0, RAND_MAX, rand(), min, max);
}

float accum;
int paused = 1;
u32 prev_time;
float uptime(void)
{
	u32 m, ms = tickcount_ms();
	float f;

	if (paused)
		return accum;

	m = ms - prev_time;
	prev_time = ms;

	f = (m / 1000) + ((float)(m % 1000) / 1000.0f);
	accum += f;

	return accum;
}

int ratelimit(u32 *last, u32 ms_period)
{
	u32 ms = tickcount_ms();

	if ((ms - (*last)) < ms_period)
		return 1;

	*last = ms;
	return 0;
}

/******************************************************************************/

struct state {
	float x, y, scale;
} state = { .x = 0.0f, .y = 0.0f, .scale = 11.0f };

int z_in, z_out;
int X, Y, up, down, left, right;
char message[64];
char timestr[32];

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
//#define GRIDCLR	0x30c030
static void draw_grid(struct dbx *d)
{
	int wd = dbx_width(d);
	int ht = dbx_height(d);
	int e = expv(state.scale);
	float yscale;
	float f, step;
	float min, max;
	int x, y;

	step = powf(10, e - 1);

	min = state.x - state.scale;
	max = state.x + state.scale;
	f = (roundf(min / step) + 1) * step;
	for (; f < max; f += step) {
		x = (int)transform(min, max, f, 0, wd);
		dbx_draw_line(d, x, 0, x, ht, GRIDCLR);
	}
	if (min <= 0.0f && max >= 0.0f) {
		x = (int)transform(min, max, 0.0f, 0, wd);
		dbx_draw_line(d, x, 0, x, ht, GRIDCLR + 0x303030);
	}

	yscale = (float)ht / (float)wd;
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
	alt_dir_upd(z_in, z_out, &state.scale, -(state.scale / 10), state.scale / 10);

	alt_dir_upd(  up,  down, &state.y,  (state.scale / 20), -(state.scale / 20));
	alt_dir_upd(left, right, &state.x, -(state.scale / 20),  (state.scale / 20));
}

int inrange(float center, float radius, float v)
{
	return (v < (center - radius) || v > (center + radius)) ? 0 : 1;
}

#if 1
/******************************************************************************/

#if 1
float func1(float x)
{
	return sin(x) + 5.8f;
}

float func2(float x)
{
	return sin(1.4 * x) + 3.4f;
}

float func3(float x)
{
	return sin(x) + sin(1.4 * x);
}
#else
float func1(float x) { return sin(x); }
float func2(float x) { return 2 * cos(x + 0.3); }

float func3(float x)
{
	float t = uptime();

	if (t < 5.0f || t > 7.0f)
		return NAN;

	return 0.2f * sin(8 * x) + 1;
}
#endif
float func4(float x)
{
	float t = uptime();

	if (t < 1.0f || t > 7.0f)
		return NAN;

	if (fabs(x - t) > 0.03)
		return NAN;

	return -pow(x - 4.0f, 2) + 9.0f;
}

float func5(float x) { return tan(0.4 * x); }
float func6(float x) { return pow(0.5f * x, 3); }
float func7(float x) { return 1.0f / x; }

float func8(float x)
{
	return sin(x);
}

float factorial(int x)
{
	float v;

	for (v = 1; x; x--)
		v *= x;
	return v;
}

float taylor_sin(float x, int terms)
{
	int i, sign = -1, exp = 3;
	float y = x;

	for (i = 0; i < terms; i++, sign *= -1, exp += 2)
		y += sign * (pow(x, exp) / factorial(exp));
	return y;
}

float func8_a(float x) { return taylor_sin(x, 1); }
float func8_b(float x) { return taylor_sin(x, 2); }
float func8_c(float x) { return taylor_sin(x, 5); }
float func8_d(float x) { return taylor_sin(x, 30); }

float syncx(float x)
{
	return sin(x) / x;
}

/******************************************************************************/

struct func {
	u32 clr;
	float (*func)(float);
} funcs[] = {
	{0x00e0e0, syncx },
#if 0
	//{ 0x909090, func8 },
	{ 0xf06060, func8_a },
	{ 0x60f060, func8_b },
	{ 0x6060f0, func8_c },
	{ 0xe0e0e0, func8_d },
#endif
#if 0
	{ 0xf0f000, func1 },
	{ 0x00f0f0, func2 },
	{ 0xf000f0, func3 },
#endif
#if 0
	{ 0x30e080, func4 },
	{ 0xe05060, func5 },
	{ 0x80e080, func6 },
	{ 0xa0f0a0, func7 },
#endif
};

void graph(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	int px, py, x, y, i;
	float fx, fy, yscale;

	for (i = 0; i < ARRAY_SIZE(funcs); i++) {
		px = -1;
		for (x = 0; x < wd; x++) {
			fx = transform(0, wd, x, state.x - state.scale, state.x + state.scale);

			fy = funcs[i].func(fx);

			if (isnan(fy) || !inrange(state.y, state.scale, fy))
				continue;
			yscale = (float)ht / (float)wd;
			y = transform(state.y - state.scale * yscale,
				      state.y + state.scale * yscale, fy, ht, 0);
			if (px < 0)
				dbx_draw_point(d, x, y, funcs[i].clr);
			else
				dbx_draw_line(d, px, py, x, y, funcs[i].clr);
			px = x;
			py = y;
		}
	}
}
#else
void graph(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	int px, py, x, y, i;
	float fx, fy, yscale;
	u32 clr;

	for (i = 0; i < 10; i++) {
		px = -1;
		for (x = 0; x < wd; x++) {
			fx = transform(0, wd, x, state.x - state.scale,
						 state.x + state.scale);

			fy = (i * 0.6f * sin(fx)) / fx - 3;

			if (isnan(fy) || !inrange(state.y, state.scale, fy))
				continue;
			yscale = (float)ht / (float)wd;
			y = transform(state.y - state.scale * yscale,
				      state.y + state.scale * yscale, fy, ht, 0);
			clr = 0x909030 + i * 0x040404;
			//clr = 0x609050 + i * 0x040404;
			if (px < 0)
				dbx_draw_point(d, x, y, clr);
			else
				dbx_draw_line(d, px, py, x, y, clr);
			px = x;
			py = y;
		}
	}
}
#endif

static int update_display(struct dbx *d)
{
	int wd = dbx_width(d);

	dbx_blank_pixmap(d);

	update_state();
	mouse_coord(d);

	draw_grid(d);

	graph(d);

	dbx_draw_string(d, 20, 20, message, strlen(message), 0xf0ff00);
	snprintf(timestr, sizeof(timestr) - 1, "%2.3f", uptime());
	dbx_draw_string(d, wd - 90, 20, timestr, strlen(timestr), 0xf0ff00);

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
	case RIGHT: right = press; break;
	case LEFT:   left = press; break;
	case DOWN:   down = press; break;
	case UP:       up = press; break;
	case '-':   z_out = press; break;
	case '+':    z_in = press; break;

	case '=':
		z_in = 0;
		break;
	case 'r':
		accum = 0.0f;
		prev_time = tickcount_ms();
		break;
	case ' ':
		if (press) {
			prev_time = tickcount_ms();
			paused = !paused;
		}
		break;
	default:
		printf("%c %d\n", key, press);
		break;
	}
	return key != 'q' ? 0 : (press ? -1 : 0);
}

static int button(struct dbx *d, int button, int x, int y, int press)
{
	printf("%d [%d %d] %d\n", button, x, y, press);
	return 0;
}

#define UPDATE_PERIOD_MS	30
int main(int argc, char *argv[])
{
	struct dbx_ops ops = {
		.update = update_display,
		.motion = motion,
		.button = button,
		.key = key,
		.configure = NULL,
	};

	dbx_run(argc, argv, &ops, UPDATE_PERIOD_MS);
	return EXIT_SUCCESS;
}
