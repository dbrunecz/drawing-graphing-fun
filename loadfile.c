/* Copyright (C) 2020 David Brunecz. Subject to GPL 2.0 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char *loadfile(char *fname)
{
	struct stat st;
	size_t sz;
	char *b;
	FILE *f;

	if (stat(fname, &st)) {
		printf("%s (%d)%s", fname, errno, strerror(errno));
		return NULL;
	}

	f = fopen(fname, "rb");
	if (!f) {
		printf("%s (%d)%s", fname, errno, strerror(errno));
		return NULL;
	}

	b = malloc(st.st_size + 1);
	if (!b)
		return NULL;

	sz = fread(b, 1, st.st_size, f);
	if (sz != st.st_size) {
		free(b);
		fclose(f);
		printf("[%d:%d] %s (%d)%s", (int)sz, (int)st.st_size,
			fname, errno, strerror(errno));
		return NULL;
	}
	b[st.st_size] = '\0';

	fclose(f);
	return b;
}
