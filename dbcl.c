#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/opencl.h>

#include "dbcl.h"

struct dbcl {
	size_t global;
	size_t local;
	cl_device_id device_id;
	cl_context context;
	cl_command_queue commands;
	cl_program program;
	cl_kernel kernel;
	cl_mem output;
	size_t output_size;
};

void dbcl_close(struct dbcl *d)
{
	if (!d)
		return;

	if (d->program)
		clReleaseProgram(d->program);
	if (d->kernel)
		clReleaseKernel(d->kernel);
	if (d->commands)
		clReleaseCommandQueue(d->commands);
	if (d->context)
		clReleaseContext(d->context);
	free(d);
}

struct dbcl *dbcl_open(const char **kernel_source)
{
	struct dbcl *d = malloc(sizeof(*d));
	char buf[2048];
	size_t len;
	int err;

	if (!d)
		return NULL;
	memset(d, 0, sizeof(*d));

	err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &d->device_id, NULL);
	if (err != CL_SUCCESS)
		goto exit_error;

	d->context = clCreateContext(0, 1, &d->device_id, NULL, NULL, &err);
	if (!d->context)
		goto exit_error;

	d->commands = clCreateCommandQueueWithProperties(d->context, d->device_id,
							 NULL, &err);
	if (!d->commands)
		goto exit_error;

	d->program = clCreateProgramWithSource(d->context, 1, kernel_source,
						NULL, &err);
	if (!d->program)
		goto exit_error;

	err = clBuildProgram(d->program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		clGetProgramBuildInfo(d->program, d->device_id, CL_PROGRAM_BUILD_LOG,
					sizeof(buf), buf, &len);
		printf("Error: Failed to build program executable!\n%s\n", buf);
		goto exit_error;
	}

	d->kernel = clCreateKernel(d->program, "square", &err);
	if (!d->kernel || err != CL_SUCCESS)
		goto exit_error;

	return d;

exit_error:
	printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);
	dbcl_close(d);
	return NULL;
}

int dbcl_ouptut_buffer(struct dbcl *d, unsigned int size)
{
	d->output_size = size;
	d->output = clCreateBuffer(d->context, CL_MEM_WRITE_ONLY, size,
					NULL, NULL);
	return d->output ? 0 : -1;
}

int dbcl_parameters(struct dbcl *d, struct dbcl_param *params, int count)
{
	struct dbcl_param *p;
	int i, n = 0;
	int err;

	if (d->output)
		if (clSetKernelArg(d->kernel, n++, sizeof(cl_mem), &d->output)) {
			printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);
			return -1;
		}

	for (i = 0; i < count; i++) {
		p = &params[i];
		err = clSetKernelArg(d->kernel, n++, p->sz, p->p);
		if (err) {
			printf("%s:%d %s() %d\n", __FILE__, __LINE__, __func__, err);
			return -1;
		}
	}

	return 0;
}

int dbcl_run(struct dbcl *d, int count, void *out)
{
	size_t global;
	//size_t local;
	int err;
/*
	err = clGetKernelWorkGroupInfo(d->kernel, d->device_id,
					CL_KERNEL_WORK_GROUP_SIZE,
					sizeof(local), &local, NULL);
	if (err != CL_SUCCESS) {
		printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);
		return -1;
	}
	printf("%s() %lu\n", __func__, local);
*/
	global = count;
	//err = clEnqueueNDRangeKernel(d->commands, d->kernel, 1, NULL, &global, &local,
	err = clEnqueueNDRangeKernel(d->commands, d->kernel, 1, NULL, &global, NULL,
					0, NULL, NULL);
	if (err) {
		printf("%s:%d %s() %d\n", __FILE__, __LINE__, __func__, err);
		return -1;
	}

	clFinish(d->commands);

	if (d->output) {
		err = clEnqueueReadBuffer(d->commands, d->output, CL_TRUE, 0,
					d->output_size, out, 0, NULL, NULL);
		if (err != CL_SUCCESS) {
			printf("%s:%d %s()\n", __FILE__, __LINE__, __func__);
			return -1;
		}
	}
	return 0;
}
