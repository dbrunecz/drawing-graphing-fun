/* Copyright (C) 2020 David Brunecz. Subject to GPL 2.0 */

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

int paused;
int pmode;

struct score {
	int p1, p2;
} score;

char msg[128];

#define TRANS_RATE	8

int wall_collision_check(int r, int *p, int *v, int max)
{
	if ((*p - r) <= 0) {
		*v *= -1;
		*p = 0 + r;
		return pmode ? 0 : -1;
	}
	if ((*p + r) >= max) {
		*v *= -1;
		*p = max - r;
		return pmode ? 0 : 1;
	}
	return 0;
}

struct player_ctrl {
	int up, dn;
} p1, p2;

struct sprite {
	int x, y;
	int xv, yv;
	u32 clr;
	u32 type;
	int (*update)(struct dbx *, struct sprite *, float );
	struct player_ctrl *ctrl;
	union {
		u32 dia;
		struct {
			u32 wd;
			u32 ht;
		} r;
	} u;
};

int paddle_update(struct dbx *d, struct sprite *s, float t)
{
	int ht = dbx_height(d);

	if (s->ctrl->up && !s->ctrl->dn)
		s->yv -= 1;
	if (!s->ctrl->up && s->ctrl->dn)
		s->yv += 1;

	s->y += s->yv;

	if (s->y < 0) {
		s->y = 0;
		s->yv = 0;
	}
	if ((s->y + s->u.r.ht) >= ht) {
		s->y = ht - s->u.r.ht;
		s->yv = 0;
	}
	return 0;
}

int _paddle_collision_check(struct dbx *d, struct sprite *s, struct sprite *p)
{
	int wd = dbx_width(d);
	int rnl = p->x > wd / 2;
	int r = s->u.dia / 2;

	if (s->y < p->y || s->y > (p->y + p->u.r.ht))
		return 0;

	if (s->xv < 0 && !rnl) {
		if ((s->x - r) <= (p->x + p->u.r.wd)) {
			s->x = p->x + p->u.r.wd + r;
			s->xv = -s->xv;
			s->yv += p->yv / 3;
		}
		return 0;
	}

	if (s->xv > 0 && rnl) {
		if (s->x + r >= p->x) {
			s->x = p->x - r;
			s->xv = -s->xv;
		}
		return 0;
	}
	return 0;
}

int last_score = 1;
int paddle_collision_check(struct dbx *d, struct sprite *s);
static int init(struct dbx *d);

int ball_update(struct dbx *d, struct sprite *s, float t)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	int r = s->u.dia / 2;

	s->x += s->xv;
	s->y += s->yv;

	wall_collision_check(r, &s->y, &s->yv, ht);

	paddle_collision_check(d, s);

	switch (wall_collision_check(r, &s->x, &s->xv, wd)) {
	case  1: score.p1++; last_score = 1; init(d); break;
	case -1: score.p2++; last_score = 2; init(d); break;
	}
	return 0;
}

int paddle_sprite_init(struct sprite *s, int x, int y)
{
	s->x = x;
	s->y = y;
	s->xv = 0;
	s->yv = 0;
	return 0;
}

#define BALL_DIA	30
#define R_WD		10
#define R_HT		60

#define PLAYER1_PADDLE_SPRITE	0
#define PLAYER2_PADDLE_SPRITE	1
#define BALL_SPRITE		2

#define ST_CIRC		0
#define ST_RECT		1
struct sprite sprites[] = {
	{ 0, 0, 0, 0, 0x3030c0, ST_RECT, paddle_update, &p1, .u.r.wd = R_WD, .u.r.ht = R_HT },
	{ 0, 0, 0, 0, 0x3030c0, ST_RECT, paddle_update, &p2, .u.r.wd = R_WD, .u.r.ht = R_HT },
	{ 0, 0, 0, 0, 0x10e010, ST_CIRC, ball_update, NULL, .u.dia = BALL_DIA },
};

int paddle_collision_check(struct dbx *d, struct sprite *s)
{
	_paddle_collision_check(d, s, &sprites[PLAYER1_PADDLE_SPRITE]);
	_paddle_collision_check(d, s, &sprites[PLAYER2_PADDLE_SPRITE]);
	return 0;
}

void display_score(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);

	snprintf(msg, sizeof(msg), "P1: %d    P2:%d", score.p1, score.p2);
	dbx_draw_string(d, wd / 2 - 40, ht - 30, msg, strlen(msg), 0xf0ff00);
}

static int update_pixmap(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	float t = uptime();
	struct sprite *s;
	int i, r;

	if (paused) {
		snprintf(msg, sizeof(msg), "PRESS SPACE TO TOGGLE PAUSE");
		dbx_draw_string(d, 100, 100, msg, strlen(msg), 0xf0ff00);
		return 0;
	}

	dbx_blank_pixmap(d);

	for (i = 0; i < ARRAY_SIZE(sprites); i++) {
		s = &sprites[i];
		s->update(d, s, t);
	}

	for (i = 0; i < ARRAY_SIZE(sprites); i++) {
		s = &sprites[i];
		switch (s->type) {
		case ST_RECT:
			dbx_fill_rectangle(d, s->x, s->y, s->u.r.wd, s->u.r.ht, s->clr);
			break;
		case ST_CIRC:
			r = s->u.dia / 2;
			dbx_fill_circle(d, s->x - r, s->y - r, s->u.dia, s->clr);
			break;
		}
	}

	dbx_draw_line(d, 0, 5, 0, ht - 5, 0xf04050);
	dbx_draw_line(d, wd - 1, 5, wd - 1, ht - 5, 0xf04050);

	dbx_draw_line(d, 5, 1, wd - 5, 1, 0xd0d0d0);
	dbx_draw_line(d, 5, ht - 2, wd - 5, ht - 2, 0xd0d0d0);
	display_score(d);

	return 0;
}

#define UP	0xff52
#define DOWN	0xff54

static int key(struct dbx *d, int code, int key, int press)
{
	switch (key) {
	case 'a':  p1.up = press; paused = 0; break;
	case 'z':  p1.dn = press; paused = 0; break;
	case DOWN: p2.dn = press; paused = 0; break;
	case UP:   p2.up = press; paused = 0; break;
	case 'r':
		pmode = 0;
		score.p1 = score.p2 = 0;
		last_score = 1;
		init(d);
		break;
	case 'p':
		pmode = press ? !pmode : pmode;
		break;
	case ' ':
		paused = press ? !paused : paused;
		break;
	}
	//printf("%x %c %08x\n", key, isprint(key) ? key : '?', code);
	return key != 'q' ? 0 : (press ? -1 : 0);
}

void ball_init(struct dbx *d)
{
	struct sprite *b = &sprites[BALL_SPRITE];
	int ht = dbx_height(d);
	int wd = dbx_width(d);
	static int cnt;

	b->y = ht / 2;
	b->yv = 0;

	if (last_score == 1) {
		b->xv = 16 + cnt++ / 3;
		b->x = 30;
	} else {
		b->xv = -1 * (16 + cnt++ / 3);
		b->x = wd - 40;
	}
}

static int init(struct dbx *d)
{
	int ht = dbx_height(d);
	int wd = dbx_width(d);

	ball_init(d);

	paddle_sprite_init(&sprites[PLAYER1_PADDLE_SPRITE],
					10, (ht - R_HT) / 2);
	paddle_sprite_init(&sprites[PLAYER2_PADDLE_SPRITE],
					wd - R_WD - 10, (ht - R_HT) / 2);

	update_pixmap(d);
	paused = 1;
	return 0;
}

#define UPDATE_PERIOD_MS	30
int main(int argc, char *argv[])
{
	struct dbx_ops ops = {
		.update = update_pixmap,
		.configure = NULL,
		.button = NULL,
		.motion = NULL,
		.init = init,
		.key = key,
	};

	dbx_run(argc, argv, &ops, UPDATE_PERIOD_MS);
	return EXIT_SUCCESS;
}
