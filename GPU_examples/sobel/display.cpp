#define NOMINMAX // so that windows.h does not define min/max macros

#include "freeglut.h"
#include <CL/cl.h>
#include <algorithm>
#include <iostream>
#include "parse_ppm.h"
#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

unsigned int rows = 1080;
unsigned int cols = 1920;
#define REFRESH_DELAY 10 //ms

int glutWindowHandle;
int graphicsWinWidth = 1024;
int graphicsWinHeight = (int)(((float) ROWS / (float) COLS) * graphicsWinWidth);
float fZoom = 1024;
bool useFilter = false;
unsigned int thresh = 128;
int fps_raw = 0;

cl_uint *input = NULL;
cl_uint *output = NULL;

cl_uint num_platforms;
cl_platform_id platform;
cl_uint num_devices;
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_program program;
cl_kernel kernel;
cl_mem in_buffer, out_buffer;
const char* source_file = "sobel.cl";

std::string imageFilename;
std::string deviceInfo;

void keyboard(unsigned char, int, int);
void timerEvent(int value);
void initGL(int argc, char **argv);
void display();
void reshape(int w, int h);
void idle(void);
void teardown(int exit_status = 1);
void initCL();

void filter(unsigned int *output)
{
    size_t sobelSize = 1;
    cl_int status;

    status = clEnqueueWriteBuffer(queue, in_buffer, CL_FALSE, 0, sizeof(unsigned int) * rows * cols, input, 0, NULL, NULL);
    if (status != CL_SUCCESS) std::cout << "Error: could not copy data into device" << std::endl;

    status = clFinish(queue);
    if (status != CL_SUCCESS) std::cout << "Error: could not finish successfully" << std::endl;

    status = clSetKernelArg(kernel, 3, sizeof(unsigned int), &thresh);
    if (status != CL_SUCCESS) std::cout << "Error: could not set sobel threshold" << std::endl;

    cl_event event;
    status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &sobelSize, &sobelSize, 0, NULL, &event);
    if (status != CL_SUCCESS) std::cout << "Error: could not enqueue sobel filter" << std::endl;

    status  = clFinish(queue);
    if (status != CL_SUCCESS) std::cout << "Error: could not finish successfully" << std::endl;

    cl_ulong start, end;
    status  = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    status |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    if (status != CL_SUCCESS) std::cout << "Error: could not get profile information" << std::endl;
    clReleaseEvent(event);

    fps_raw = (int)(1.0f / ((end - start) * 1e-9f));

    status = clEnqueueReadBuffer(queue, out_buffer, CL_FALSE, 0, sizeof(unsigned int) * rows * cols, output, 0, NULL, NULL);
    if (status != CL_SUCCESS) std::cout << "Error: could not copy data from device" << std::endl;

    status = clFinish(queue);
    if (status != CL_SUCCESS) std::cout << "Error: could not successfully finish copy" << std::endl;
}

char* readSourceFile(const char *sourceFilename) {
  FILE *fp;
  int errs;
  int size;
  char *source;

#ifdef _WIN32
  if (fopen_s(&fp, sourceFilename, "r") != 0) {
    fprintf(stderr, "ERROR: failed to open %s.\n", sourceFilename);
    exit(-1);
  }
#else
  fp = fopen(sourceFilename, "r");
  if (fp == 0) {
    fprintf(stderr, "ERROR: failed to open %s.\n", sourceFilename);
    exit(-1);
  }
#endif

  errs = fseek(fp, 0, SEEK_END);
  if (errs != 0) {
    std::cout << "Error seeking to end of file" << std::endl;
    exit(-1);
  }
  size = ftell(fp);
  if (size < 0) {
    std::cout << "Errror getting file position" << std::endl;
    exit(-1);
  }
  errs = fseek(fp, 0, SEEK_SET);
  if (errs != 0){
    std::cout << "Error seeking to start of file\n" << std::endl;
    exit(-1);
  }
  source = (char *) malloc(size + 1);
  errs = fread(source, 1, size, fp);
  if (errs != size) {
    printf("only read %d bytes\n", errs);
//    exit(0);
  }
  source[errs]= '\0';
  fclose(fp);
  return source;
}

int main(int argc, char **argv)
{
    imageFilename = (argc > 1) ? argv[1] : "butterflies.ppm";


    initGL(argc, argv);
    initCL();

    input = (cl_uint*)malloc(sizeof(unsigned int) * rows * cols);
    output = (cl_uint*)malloc(sizeof(unsigned int) * rows * cols);

    // Read the image
    if (!parse_ppm(imageFilename.c_str(), cols, rows, (unsigned char *)input)) {
        std::cerr << "Error: could not load " << argv[1] << std::endl;
        teardown();
    }

    std::cout << "Commands:" << std::endl;
    std::cout << " <space>  Toggle filter on or off" << std::endl;
    std::cout << " -" << std::endl
              << "    Reduce filter threshold" << std::endl;
    std::cout << " +" << std::endl
              << "    Increase filter threshold" << std::endl;
    std::cout << " =" << std::endl
              << "    Reset filter threshold to default" << std::endl;
    std::cout << " q/<enter>/<esc>" << std::endl
              << "    Quit the program" << std::endl;

    glutMainLoop();

    teardown(0);
}

void initCL()
{
    cl_int status;

    cl_uint num_platforms;
    status = clGetPlatformIDs(1, &platform, &num_platforms);

    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (status != CL_SUCCESS) std::cout << "Error: could not query devices" << std::endl;
    num_devices = 1; // always only using one device

    char info[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(info), info, NULL);
    deviceInfo = info;

    context = clCreateContext(0, num_devices, &device, NULL, NULL, &status);
    if (status != CL_SUCCESS) std::cout << "Error: could not create OpenCL context" << std::endl;

    queue = clCreateCommandQueue(context, device, 0, &status);
    if (status != CL_SUCCESS) std::cout << "Error: could not create command queue" << std::endl;

    char *source_str = readSourceFile(source_file);

    program = clCreateProgramWithSource(context, 1, (const char **) &source_str, NULL, &status);

    status = clBuildProgram(program, num_devices, &device, "", NULL, NULL);
    if (status != CL_SUCCESS) std::cout << "Error: could not build program" << std::endl;

    kernel = clCreateKernel(program, "sobel", &status);
    if (status != CL_SUCCESS) std::cout << "Error: could not create sobel kernel" << std::endl;

    in_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned int) * rows * cols, NULL, &status);
    if (status != CL_SUCCESS) std::cout << "Error: could not create device buffer" << std::endl;

    out_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * rows * cols, NULL, &status);
    if (status != CL_SUCCESS) std::cout << "Error: could not create output buffer" << std::endl;

    int pixels = cols * rows;

    status  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &in_buffer);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &out_buffer);
    status |= clSetKernelArg(kernel, 2, sizeof(int), &pixels);
    if (status != CL_SUCCESS) std::cout << "Error: could not set sobel args" << std::endl;
}

void initGL(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH)/2 - graphicsWinWidth/2,
                           glutGet(GLUT_SCREEN_HEIGHT)/2 - graphicsWinHeight/2);
    glutInitWindowSize(graphicsWinWidth, graphicsWinHeight);
    glutWindowHandle = glutCreateWindow("Filter");

    glutKeyboardFunc(keyboard);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutTimerFunc(REFRESH_DELAY, timerEvent, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    float fAspects[2] = {(float)glutGet(GLUT_WINDOW_WIDTH)/(float)COLS, (float)glutGet(GLUT_WINDOW_HEIGHT)/(float)ROWS};
    fZoom = fAspects[0] > fAspects[1] ? fAspects[1] : fAspects[0];
    glPixelZoom(fZoom, fZoom);

}


void keyboard(unsigned char key, int, int)
{
    switch (key) {
        case ' ':
            useFilter = !useFilter;
            break;

        case '=':
            thresh = 128;
            break;
        case '-':
            thresh = std::max(thresh - 10, 16u);
            break;
        case '+':
            thresh = std::min(thresh + 10, 255u);
            break;

        case '\033':
        case '\015':
        case 'Q':
        case 'q':
            teardown(0);
            break;
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    if (useFilter) {
        filter(output);
        glDrawPixels(cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, output);
    } else {
        glDrawPixels(cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, input);
    }

    glutSwapBuffers();

    char title[256];
    if (useFilter) {
#ifdef _WIN32
        sprintf_s(title, 256, "Sobel Filter ON %s 1920 x 1080, %d fps (kernel)", deviceInfo.c_str(), fps_raw);
#else
        sprintf(title, "Sobel Filter ON %s 1920 x 1080, %d fps (kernel)", deviceInfo.c_str(), fps_raw);
#endif
    } else {
#ifdef _WIN32
        sprintf_s(title, 256, "Sobel Filter OFF 1920 x 1080");
#else
        sprintf(title, "Sobel Filter OFF 1920 x 1080");
#endif
    }
    glutSetWindowTitle(title);
}

void reshape(int w, int h)
{
    glPixelZoom((float)w/COLS, (float)h/ROWS);
}

void timerEvent(int value)
{
    glutPostRedisplay();
    glutTimerFunc(REFRESH_DELAY, timerEvent, 0);
}

void idle(void)
{ }

void cleanup()
{
  // Called from aocl_utils::check_error, so there's an error.
  teardown(-1);
}

void teardown(int exit_status)
{
    if (input) free(input);
    if (output) free(output);
    if (in_buffer) clReleaseMemObject(in_buffer);
    if (out_buffer) clReleaseMemObject(out_buffer);
    if (kernel) clReleaseKernel(kernel);
    if (program) clReleaseProgram(program);
    if (queue) clReleaseCommandQueue(queue);
    if (context) clReleaseContext(context);

    exit(exit_status);
}

