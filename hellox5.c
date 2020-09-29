#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "dbx.h"

static void modv(int *v, int min, int max, int m)
{
	*v += m;
	if (*v < min)
		*v = min;
	if (*v > max)
		*v = max;
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

int ratelimit(u32 *last, u32 ms_period)
{
	u32 ms = tickcount_ms();

	if ((ms - (*last)) < ms_period)
		return 1;

	*last = ms;
	return 0;
}

/******************************************************************************/

struct xp {
	u32 clr;
	u32 x;
	u32 y;
} xp[8];
u32 xpcnt;

#define R_WD	40
#define R_HT	50

char message[32];
int X, Y;
int up, right, left, down;

static int motion(struct dbx *d, XMotionEvent *e)
{
	snprintf(message, 32, "%4d %4d", e->x, e->y);

	X = e->x;
	Y = e->y;
	return 0;
}

void update_color_component(u32 *c, int offset, int delta)
{
	int v = (*c >> offset) & 0xff;
	*c &= ~(0xff << offset);
	int_mod(&v, 0, 0xff, delta);
	*c |= v << offset;
}

#define XP_STEP		(255 / (200 / 30))
static void do_xps(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	int i, flash = 0, sz;

	for (i = 0; i < ARRAY_SIZE(xp); i++) {
		if (!xp[i].clr)
			continue;

		if (xp[i].clr == 0xffffff) {
			flash = 1;
			xp[i].clr = 0xffc0c0;
			continue;
		}

		if (!flash) {
			sz = 300 - (xp[i].clr >> 16) / 2;
			dbx_fill_circle(d, xp[i].x - sz / 2, xp[i].y - sz / 2, sz,
					xp[i].clr);
		}
		update_color_component(&xp[i].clr, 16, -XP_STEP);
		update_color_component(&xp[i].clr, 8, -XP_STEP - 1);
		update_color_component(&xp[i].clr, 0, -XP_STEP - 1);
	}

	if (flash)
		dbx_fill_rectangle(d, 0, 0, wd, ht, 0xffffff);
}

/******************************************************************************/

struct sprite {
	int x, y;
	u32 clr;
	u32 type;
	int (*update)(struct dbx *, struct sprite *, float );
	union {
		u32 rad;
		struct {
			u32 wd;
			u32 ht;
		} r;
	} u;
};

#define TRANS_RATE	8
int user_update(struct dbx *d, struct sprite *s, float t)
{
	int wd = dbx_width(d);
	int ht = dbx_height(d);

	if (left && !right)
		modv(&s->x, 0, wd, -TRANS_RATE);
	if (right && !left)
		modv(&s->x, 0, wd - s->u.r.wd, TRANS_RATE);

	if (up && !down)
		modv(&s->y, 0, ht, -TRANS_RATE);
	if (down && !up)
		modv(&s->y, 0, ht - s->u.r.ht, TRANS_RATE);
	return 0;
}

#define HZ		(0.3f)
int rect1_update(struct dbx *d, struct sprite *s, float t)
{
	float fx = sinf(2.0f * 3.14159f * HZ * t);
	int ht = dbx_height(d);
	int wd = dbx_width(d);

	s->x = (int)transform(-1.0f, 1.0f, fx, 0, wd - s->u.r.wd);
	s->y = 2 * ht / 3;
	return 0;
}

int rect2_update(struct dbx *d, struct sprite *s, float t)
{
	float fy = cosf(2.0f * 3.14159f * HZ * t);
	int ht = dbx_height(d);
	int wd = dbx_width(d);

	s->x = wd / 2 - s->u.r.wd / 2;
	s->y = (int)transform(-1.0f, 1.0f, fy, 0, ht - s->u.r.ht);

	return 0;
}

int circ1_update(struct dbx *d, struct sprite *s, float t)
{
	float fx = sinf(2.0f * 3.14159f * HZ * t);
	float fy = cosf(2.0f * 3.14159f * HZ * t);
	int ht = dbx_height(d);
	int wd = dbx_width(d);

	s->x = (int)transform(-1.0f, 1.0f, fx, 50, wd - s->u.rad - 50);
	s->y = (int)transform(-1.0f, 1.0f, fy, 50, ht - s->u.rad - 50);
	return 0;
}

int circ2_update(struct dbx *d, struct sprite *s, float t)
{
	float fx = cosf(2.0f * 3.14159f * (2 * HZ) * t);
	float fy = sinf(2.0f * 3.14159f * (2 * HZ) * t);
	int ht = dbx_height(d);
	int wd = dbx_width(d);

	s->x = (int)transform(-1.0f, 1.0f, fx, 20, wd - s->u.rad - 20);
	s->y = (int)transform(-1.0f, 1.0f, fy, 20, ht - s->u.rad - 20);
	return 0;
}

int circ3_update(struct dbx *d, struct sprite *s, float t)
{
	s->x = X;
	s->y = Y;
	return 0;
}

/******************************************************************************/

#define RAD		70

#define ST_CIRC		0
#define ST_RECT		1
struct sprite sprites[] = {
	{ 0, 0, 0x10b010, ST_RECT, user_update, .u.r.wd = R_WD, .u.r.ht = R_HT },
	{ 0, 0, 0x9090ff, ST_RECT, rect1_update, .u.r.wd = R_WD, .u.r.ht = R_HT },
	{ 0, 0, 0x303030, ST_RECT, rect2_update, .u.r.wd = R_WD, .u.r.ht = R_HT },
	{ 0, 0, 0xff4040, ST_CIRC, circ1_update, .u.rad = RAD },
	{ 0, 0, 0x503030, ST_CIRC, circ2_update, .u.rad = RAD },
	{ 0, 0, 0xf030f0, ST_CIRC, circ3_update, .u.rad = RAD },
};

void box_init(struct sprite *s, int *left, int *right, int *top, int *bottom)
{
	int hr;

	switch (s->type) {
	case ST_RECT:
		*left = s->x;
		*right = s->x + s->u.r.wd;
		*top = s->y;
		*bottom = s->y + s->u.r.ht;
		break;
	case ST_CIRC:
		hr = s->u.rad / 2;
		*left = s->x - hr;
		*right = s->x + hr;
		*top = s->y - hr;
		*bottom = s->y + hr;
		break;
	}
}

struct rect {
	int l, r, t, b;
};

void intersection(int al, int ah, int bl, int bh, int *il, int *ih)
{
	*ih = MIN(ah, bh);
	*il = MAX(al, bl);
}

struct rect *check_collision(struct sprite *a, struct sprite *b)
{
	static struct rect r;
	int al, ar, at, ab;
	int bl, br, bt, bb;

	box_init(a, &al, &ar, &at, &ab);
	box_init(b, &bl, &br, &bt, &bb);

	if (al > br || ar < bl)
		return NULL;
	if (at > bb || ab < bt)
		return NULL;

	intersection(al, ar, bl, br, &r.l, &r.r);
	intersection(at, ab, bt, bb, &r.t, &r.b);

	return &r;
}

#define WHITE1	0xc0c0c0
void collision_detect(struct dbx *d)
{
	struct sprite *a, *b;
	struct rect *r;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(sprites); i++) {
		a = &sprites[i];
		for (j = i + 1; j < ARRAY_SIZE(sprites); j++) {
			b = &sprites[j];
			r = check_collision(a, b);
			if (!r)
				continue;
			dbx_draw_rectangle(d, r->l, r->t, r->r - r->l, r->b - r->t,
						WHITE1);
		}
	}
}

static int update_pixmap(struct dbx *d)
{
	float t = uptime();
	struct sprite *s;
	int i, r;

	dbx_blank_pixmap(d);

	for (i = 0; i < ARRAY_SIZE(sprites); i++)
		sprites[i].update(d, &sprites[i], t);

	for (i = 0; i < ARRAY_SIZE(sprites); i++) {
		s = &sprites[i];
		switch (s->type) {
		case ST_RECT:
			dbx_fill_rectangle(d, s->x, s->y, s->u.r.wd, s->u.r.ht, s->clr);
			break;
		case ST_CIRC:
			r = s->u.rad;
			dbx_fill_circle(d, s->x - r / 2, s->y - r / 2, r, s->clr);
			break;
		}
	}

	collision_detect(d);

	dbx_draw_string(d, 20, 20, message, strlen(message), 0xf0ff00);

	do_xps(d);

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
	}
	//printf("%x %c %08x\n", key, isprint(key) ? key : '?', code);
	return key != 'q' ? 0 : -1;
}

static int button(struct dbx *d, int button, int x, int y, int press)
{
	xp[xpcnt].clr = 0xffffff;
	xp[xpcnt].x = x;
	xp[xpcnt].y = y;
	xpcnt = (xpcnt + 1) & 7;

	printf("%d [%d %d] %d\n", button, x, y, press);
	return 0;
}

#define UPDATE_PERIOD_MS	30
int main(int argc, char *argv[])
{
	struct dbx_ops ops = {
		.update = update_pixmap,
		.motion = motion,
		.key = key,
		.configure = NULL,
		.button = button,
	};

	dbx_run(argc, argv, &ops, UPDATE_PERIOD_MS);
	return EXIT_SUCCESS;
}
