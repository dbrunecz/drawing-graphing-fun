/* Copyright (C) 2020 David Brunecz. Subject to GPL 2.0 */

int float_cmp(float a, float range)
{
	return fabs(a) < fabs(range) ? 1 : 0;
}

float z_line_intersect(float px1, float px2, float dx1, float dx2, float z)
{
	return ((z - px2) * dx1) / dx2 + px1;
}

float angle2vecx(float theta, float phi)
{
	return cos(theta) * cos(phi);
}

float angle2vecy(float theta, float phi)
{
	return sin(theta) * cos(phi);
}

float angle2vecz(float phi)
{
	return sin(phi);
}

#define GNCLR	0x204020
#define GDCLR	0xb0
#define GSHFT	8
#define GSCL	1000.0f

#define GMOD	250.0f
#define GDELT	2.0f

unsigned int ground_clr(float x1, float y1, float x, float y)
{

	float d = GSCL / sqrt(pow(x - x1, 2) + pow(y - y1, 2));
	float gmod = GMOD;

	if (d > 1.0f)
		d = 1.0f;

	x = fmod(x, gmod);
	y = fmod(y, gmod);

	if (float_cmp(x, GDELT))
		return GNCLR + (((unsigned int)(GDCLR * d) & 0xff) << GSHFT);
	if (float_cmp(y, GDELT))
		return GNCLR + (((unsigned int)(GDCLR * d) & 0xff) << GSHFT);

	return GNCLR;
}

__kernel void square(__global unsigned int* output,
			const float x,
			const float y,
			const float z,
			const float theta,
			const float phi,
			const float yva,
			const float xva,
			const float ystep,
			const float xstep,
			const unsigned int wd,
			const unsigned int ht)
{
	const unsigned int count = wd * ht;
	int i = get_global_id(0);
	float dx, dy, dz;
	float xa, ya;
	int h, w;
	float xi, yi;

	if (i >= count)
		return;

	h = ht - 1 - (i / wd);
	w = i % wd;

	ya = (-1.0f * yva / 2.0f) + h * ystep;
	xa = (-1.0f * xva / 2.0f) + w * xstep;

	dz = angle2vecz(phi + ya);
	if (float_cmp(dz - 0.0f, 0.0001f)) {
		output[i] = 0;
		return;
	}

	dx = angle2vecx(theta + xa, phi + ya);
	dy = angle2vecy(theta + xa, phi + ya);

	xi = z_line_intersect(x, z, dx, dz, 0.0f);
	if ((dx > 0.0f && xi < x) || (dx < 0.0f && xi > x)) {
		output[i] = 0;
		return;
	}

	yi = z_line_intersect(y, z, dy, dz, 0.0f);
	if ((dy > 0.0f && yi < y) || (dy < 0.0f && yi > y)) {
		output[i] = 0;
		return;
	}

	output[i] = ground_clr(x, y, xi, yi);
}
