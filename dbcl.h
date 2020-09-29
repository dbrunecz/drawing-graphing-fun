
struct dbcl;

struct dbcl *dbcl_open  (const char **kernel_source);
void         dbcl_close (struct dbcl *dbcl);

int dbcl_ouptut_buffer (struct dbcl *d, unsigned int size);

struct dbcl_param {
	void *p;
	size_t sz;
};
int dbcl_parameters(struct dbcl *d, struct dbcl_param *params, int count);

int dbcl_run(struct dbcl *d, int count, void *out);
