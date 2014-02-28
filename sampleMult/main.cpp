#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <math.h>
#include <string.h>

char* readSource(const char* sourceFilename);

double execution_time(cl_event &event)
{
  cl_ulong start, end;

  clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
  clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);

  return (double)1.0e-9 * (end - start);
}


void randomInit(float* data, int size)
{
int i =0;
for(i; i < size; i++)
    data[i] = (rand()/(float)RAND_MAX) * 10;
}

void cpuMV (float* y, float* A, float* X, int M, int N)
{
for(int i = 0; i< M; i++) {
    double sum = 0;
    y[i] = 0;
    for(int k = 0; k < N; k++) {
        double a = A[i + k* M];
        double x = X[k];
        sum += a * x;
    }
    y[i] = (float) sum;
 }
}

int main( int argc, char ** argv) {
printf("this is a test\n");
int M = 1024*16;
int N = 8192;
float *A, *x;
float *y;
A = (float *)malloc(sizeof(float) * M * N);
x = (float *)malloc(sizeof(float) * N);
y = (float *)malloc(sizeof(float) * M);
randomInit(A, M * N);
randomInit(x, N);
int wrong;
wrong = 0;  
cl_int err;
cl_uint numPlatforms;
cl_platform_id *platforms;

err = clGetPlatformIDs(0, NULL, &numPlatforms);
if (err != CL_SUCCESS) {
    printf("clGetPlatformIDs failed\n");
    exit(-1);
}

if(numPlatforms == 0) {
    printf("No platforms detected.\n");
    exit(-1);   
}
platforms = (cl_platform_id*)malloc(numPlatforms*sizeof(cl_platform_id));

clGetPlatformIDs(numPlatforms, platforms, NULL);

printf("%u platforms found\n", numPlatforms);
for(int i =0; i < numPlatforms; i++) {
    char buff[100];
    printf("Platform %u:\n", i);
    err = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(buff), buff, NULL);
    printf("\tVendor: %s\n", buff);
    err = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(buff), buff, NULL);
    printf("\tName: %s\n", buff);
    if (err != CL_SUCCESS) {
        printf("clGetPlatformInfo failed\n");
        exit(-1);
    }
}
printf("\n");

cl_uint numDevices = 0;
cl_device_id *devices;
err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
if(err != CL_SUCCESS) {
    printf("clGetDeviceIDs failed\n");
    exit(-1);
}
if (numDevices == 0){
    printf("No devices found\n");
    exit(-1);
}
devices = (cl_device_id*)malloc(numDevices*sizeof(cl_device_id));
err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
printf("%u devices found\n", numDevices);
for(int i =0; i < numDevices; i++) {
    char buff[100];
    printf("Device %u:\n", i);
    err = clGetDeviceInfo(devices[i], CL_DEVICE_VENDOR, sizeof(buff), buff, NULL);
    printf("\tVendor: %s\n", buff);
    err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(buff), buff, NULL);
    printf("\tName: %s\n", buff);
    if (err != CL_SUCCESS) {
        printf("clGetDeviceInfo failed\n");
        exit(-1);
    }
}
cl_context context;
context = clCreateContext(NULL, numDevices,devices, NULL, NULL, &err);
if(err != CL_SUCCESS){
    printf("clCreateContext failed\n");
    exit(-1);
}

cl_command_queue cmdQueue;
cmdQueue = clCreateCommandQueue(context, devices[0], 0, &err);
if(err != CL_SUCCESS) { 
    printf("clCreateCommandQueue failed\n");
    exit(-1);
}

cl_mem d_A, d_x;
cl_mem d_y;
d_A = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, M * N * sizeof(float), A, &err);
if (err != CL_SUCCESS) {
    printf("clCreateBuffer for A failed\n");
    exit(-1);
}
d_x = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,  N * sizeof(float), x, &err);
if (err != CL_SUCCESS) {
    printf("clCreateBuffer for x failed\n");
    exit(-1);
}
d_y = clCreateBuffer(context, CL_MEM_READ_WRITE, M * sizeof(float), NULL, &err);
if (err != CL_SUCCESS) {
    printf("clCreateBuffer for y failed\n");
    exit(-1);
}
cl_program program;
char* source;
const char *sourceFile = "matrixMult.cl";
source = readSource(sourceFile);
program = clCreateProgramWithSource(context, 1, (const char**) &source, NULL, &err);
if (err != CL_SUCCESS) {
    printf("clCreateProgramFailed");
    exit(-1);
}
cl_int buildErr;
buildErr = clBuildProgram(program, 1, devices, "-cl-fast-relaxed-math", NULL, NULL);
if (buildErr != CL_SUCCESS) {
    printf("Program failed to build,\n");
    cl_build_status buildStatus;
    for(int i = 0; i < numDevices; i++) {
        clGetProgramBuildInfo(program, devices[i], CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &buildStatus, NULL);
        if(buildStatus == CL_SUCCESS) {
            continue;
        }
        char *buildLog;
        size_t buildLogSize;
        clGetProgramBuildInfo(program, devices[i], CL_PROGRAM_BUILD_LOG, 0, NULL, &buildLogSize);
        buildLog = (char *)malloc(buildLogSize);
        clGetProgramBuildInfo(program, devices[i], CL_PROGRAM_BUILD_LOG,buildLogSize, buildLog, NULL);
        buildLog[buildLogSize -1] = '\0';
        printf("Device %u Build Log:\n%s\n", i, buildLog);
        free(buildLog);
    }
    exit(0);
}
else {
    printf("No build errors\n");
}

cl_kernel kernel;
kernel = clCreateKernel(program, "mvKernel", &err);
if(err != CL_SUCCESS) {
    printf("clCreateKernel failed\n");
    exit(-1);
}
err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_A);
err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_x);
err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_y);
err |= clSetKernelArg(kernel, 3, sizeof(int), &M);
err |= clSetKernelArg(kernel, 4, sizeof(int), &N);

size_t globalWorkSize[1];
globalWorkSize[0] = M;
size_t localWorkSize[1];
localWorkSize[0] = 256;
cl_event kernel_event;

err = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, &kernel_event);
clEnqueueReadBuffer(cmdQueue, d_y, CL_TRUE, 0, M * sizeof(float), y, 0, NULL, NULL);
clFlush(cmdQueue);
err = clFinish(cmdQueue);
if(err != CL_SUCCESS) {
    printf("ERROR!!");
    exit(-1);
}

double dSeconds = execution_time(kernel_event);
double dNumOps = 2.0* (double)M*(double)N;
  double gflops = 1.0e-9 * dNumOps/dSeconds;

  printf("Performance Results:\n-----------------------------\n");
  printf("Throughput = %.4f GFLOPs\n", gflops);
  printf("Time = %.5f s\n", dSeconds);
  printf("Size = %.0f\n", dNumOps);
  printf("Workgroup size = %i\n", localWorkSize[0]);

clReleaseKernel(kernel);
clReleaseProgram(program);
clReleaseCommandQueue(cmdQueue);
clReleaseMemObject(d_A);
clReleaseMemObject(d_x);
clReleaseMemObject(d_y);
clReleaseContext(context);
for(int i=0; i < (M <10 ? M : 10); i++)
    printf("vector y = %f\n", y[i]);
float* refY;
refY = (float*)malloc(M*sizeof(float));
cpuMV(refY, A, x, M, N);
for (int i = 0; i < M; ++i) {
    float diff = refY[i] - y[i];
    if (fabsf(diff)/ refY[i] > 1e-4)
        wrong++;
}
printf("There were %d errors!!\n", wrong);
free(A);
free(y);
free(x);
free(source);
free(platforms);    
free(devices);
}

char* readSource(const char *sourceFilename) {
FILE *fp;
int errs;
int size;
char *source;
fp = fopen(sourceFilename, "rb");
errs = fseek(fp, 0, SEEK_END);
if(errs != 0) {
    printf("Error seeking to end of file");
    exit(-1);
}
size = ftell(fp);
if(size<0) {
    printf("Errror getting file position");
    exit(-1);
}
errs = fseek(fp, 0, SEEK_SET);
if(errs != 0){
    printf("Error seeking to start of file\n");
    exit(-1);
}
source = (char*)malloc(size +1);
errs = fread(source, 1, size, fp);
if(errs != size) {
    printf("only read %d bytes\n", errs);
    exit(0);
}
source[size]= '\0';
return source;
}
