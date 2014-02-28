// Copyright (C) 2013 Altera Corporation, San Jose, California, USA. All rights reserved. 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this 
// software and associated documentation files (the "Software"), to deal in the Software 
// without restriction, including without limitation the rights to use, copy, modify, merge, 
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to 
// whom the Software is furnished to do so, subject to the following conditions: 
// The above copyright notice and this permission notice shall be included in all copies or 
// substantial portions of the Software. 
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE. 
//  
// This agreement shall be governed in all respects by the laws of the State of California and 
// by the laws of the United States of America. 

#define NOMINMAX // so that windows.h does not define min/max macros

#include <GL/freeglut.h>
#include <CL/opencl.h>
#include <algorithm>
#include <iostream>
#include "parse_ppm.h"
#include "defines.h"
#include "AOCL_Utils.h"

using namespace aocl_utils;

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

std::string imageFilename;
std::string aocxFilename;
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
    checkError(status, "Error: could not copy data into device");

    status = clFinish(queue);
    checkError(status, "Error: could not finish successfully");

    status = clSetKernelArg(kernel, 3, sizeof(unsigned int), &thresh);
    checkError(status, "Error: could not set sobel threshold");

    cl_event event;
    status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &sobelSize, &sobelSize, 0, NULL, &event);
    checkError(status, "Error: could not enqueue sobel filter");

    status  = clFinish(queue);
    checkError(status, "Error: could not finish successfully");

    cl_ulong start, end;
    status  = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    status |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    checkError(status, "Error: could not get profile information");
    clReleaseEvent(event);

    fps_raw = (int)(1.0f / ((end - start) * 1e-9f));

    status = clEnqueueReadBuffer(queue, out_buffer, CL_FALSE, 0, sizeof(unsigned int) * rows * cols, output, 0, NULL, NULL);
    checkError(status, "Error: could not copy data from device");

    status = clFinish(queue);
    checkError(status, "Error: could not successfully finish copy");
}

int main(int argc, char **argv)
{
    imageFilename = (argc > 1) ? argv[1] : "butterflies.ppm";

    initGL(argc, argv);
    initCL();

    input = (cl_uint*)alignedMalloc(sizeof(unsigned int) * rows * cols);
    output = (cl_uint*)alignedMalloc(sizeof(unsigned int) * rows * cols);

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

    if (!setCwdToExeDir()) {
        teardown();
    }

    platform = findPlatform("Altera");
    if (platform == NULL) {
        teardown();
    }

    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    checkError (status, "Error: could not query devices");
    num_devices = 1; // always only using one device

    char info[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(info), info, NULL);
    deviceInfo = info;

    context = clCreateContext(0, num_devices, &device, NULL, NULL, &status);
    checkError(status, "Error: could not create OpenCL context");

    queue = clCreateCommandQueue(context, device, 0, &status);
    checkError(status, "Error: could not create command queue");

    std::string binary_file = getBoardBinaryFile("sobel", device);
    std::cout << "Using AOCX: " << binary_file << "\n";
    program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);

    status = clBuildProgram(program, num_devices, &device, "", NULL, NULL);
    checkError(status, "Error: could not build program");

    kernel = clCreateKernel(program, "sobel", &status);
    checkError(status, "Error: could not create sobel kernel");

    in_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned int) * rows * cols, NULL, &status);
    checkError(status, "Error: could not create device buffer");

    out_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * rows * cols, NULL, &status);
    checkError(status, "Error: could not create output buffer");

    int pixels = cols * rows;
    status  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &in_buffer);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &out_buffer);
    status |= clSetKernelArg(kernel, 2, sizeof(int), &pixels);
    checkError(status, "Error: could not set sobel args");
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
    if (input) alignedFree(input);
    if (output) alignedFree(output);
    if (in_buffer) clReleaseMemObject(in_buffer);
    if (out_buffer) clReleaseMemObject(out_buffer);
    if (kernel) clReleaseKernel(kernel);
    if (program) clReleaseProgram(program);
    if (queue) clReleaseCommandQueue(queue);
    if (context) clReleaseContext(context);

    exit(exit_status);
}

