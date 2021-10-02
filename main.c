#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include <CL/cl.h>
#include <SDL.h>

#include "data_structures/clist.h"
#include "ui.h"

#include <sys\timeb.h> 

#define MAX_SOURCE_SIZE (0x100000)

void onStart(int argc, char* argv[]);
void onExit(void);
cl_program load_program(cl_context context, char* filename, cl_int* errcode_ret);
bool draw();
void event(SDL_Event* e);

cl_int ret = 0;
cl_program program;
cl_command_queue command_queue;
cl_kernel kernel;
size_t global_item_size[3];
size_t local_item_size[3];

typedef struct Particle
{
    float x;
    float y;
    float vX;
    float vY;
} Particle;

#define PARTICLE_COUNT 200000

Particle particles[PARTICLE_COUNT];
cl_mem particles_mem;
struct timeb deltaTimeStart, deltaTimeEnd;

char BIN_PATH[255];
int main(int argc, char* argv[])
{
    strcpy(BIN_PATH, argv[0]);
    for (size_t i = strlen(BIN_PATH) - 1; i >= 0; i--)
    {
        if (BIN_PATH[i] == '/' || BIN_PATH[i] == '\\')
        {
            BIN_PATH[i + 1] = '\0';
            break;
        }
    }

#pragma region OPENCL_INIT

    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    ret |= clGetPlatformIDs(1, &platform_id, NULL);
    ret |= clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, NULL);
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    
    size_t info;
    clGetDeviceInfo(
    device_id,
    CL_DEVICE_MAX_WORK_GROUP_SIZE,
    sizeof(info),
    &info,
    NULL);

    // for (int i=0; i < 3; i++)
    //     printf("Info: %llu\n", info[i]);

    // printf("Info: %llu\n", info);

#pragma endregion OPENCL_INIT

#pragma region KERNEL_EXEC 

    program = load_program(context, "main.cl", &ret);
    ret |= clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    
    size_t len = 0;
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    char *buffer = calloc(len, sizeof(char));
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
    printf("%s\n", buffer);

    int width = sqrt(PARTICLE_COUNT);
    int offsetX = 100;
    int offsetY = 150;
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < width; j++)
        {
            particles[i * width + j ] = (Particle) { offsetX + j, offsetY + i, 0, 0 };
        }   
    }

    particles_mem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, sizeof(particles), particles, &ret);
    // clEnqueueWriteBuffer(command_queue, particles_mem, CL_TRUE, 0, sizeof(particles), particles, 0, NULL, NULL);
    
    // Run kernel > main
    kernel = clCreateKernel(program, "main", &ret);
    global_item_size[0] = PARTICLE_COUNT;
    local_item_size[0] = 1000;
    ftime(&deltaTimeStart);

#pragma endregion KERNEL_EXEC

#pragma region KERNEL_POST_EXEC

    ui_init();
    ui_run(&draw, &event);

#pragma endregion KERNEL_POST_EXEC


#pragma region OPENCL_CLEAN

    printf("\n");
    if (ret == 0)
        printf("opencl success.\n");
    else
        printf("opencl error: [%d]\n", ret);

    // clean
    clReleaseMemObject(particles_mem);
    clReleaseKernel(kernel);
    clFlush(command_queue);
    clFinish(command_queue);
    clReleaseCommandQueue(command_queue);
    clReleaseProgram(program);    
    clReleaseContext(context);

#pragma endregion OPENCL_CLEAN
    return 0;
}

float speed = 1;
bool applyForce = false;
int mx = 0, my = 0;
bool draw()
{
    ftime(&deltaTimeEnd);
    double dT = (1000 * (deltaTimeEnd.time - deltaTimeStart.time) + (deltaTimeEnd.millitm - deltaTimeStart.millitm)) / 1000.f;

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &particles_mem);
    clSetKernelArg(kernel, 1, sizeof(double), &dT);
    clSetKernelArg(kernel, 2, sizeof(int), &mx);
    clSetKernelArg(kernel, 3, sizeof(int), &my);
    ret |= clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_item_size, local_item_size, 0, NULL, NULL);
    clEnqueueReadBuffer(command_queue, particles_mem, CL_TRUE, 0, sizeof(particles), particles, 0, NULL, NULL);
    ftime(&deltaTimeStart);
    
    SDL_SetRenderDrawColor( ui_renderer, 235, 203, 139, 255 );
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        SDL_RenderDrawPoint(ui_renderer, particles[i].x, particles[i].y);
    }
    return true;
}

void event(SDL_Event* e)
{
    switch ( e->type ) {
        case SDL_MOUSEBUTTONDOWN:
            applyForce = true;
            break;
        case SDL_MOUSEBUTTONUP:
            applyForce = false; 
            break;
        case SDL_MOUSEMOTION:
            mx = e->button.x;
            my = e->button.y;
            break;
    }
}

cl_program load_program(cl_context context, char* filename, cl_int* errcode_ret)
{
    FILE *fp;
    char *source_str;
    size_t source_size;


    char kernel_path[255];
    strcpy(kernel_path, BIN_PATH);
    strcat(kernel_path, filename);
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    if (!source_str)
    {
        *errcode_ret |= -4;
        return 0;
    }
    fp = fopen(kernel_path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    return clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, errcode_ret);
}

#ifdef _WIN32
#include <sys\timeb.h> 
struct timeb progStart, progEnd;
#endif // !_WIN32

void onStart(int argc, char* argv[])
{
    printf(
" _                                   \n"
"| |                                  \n"
"| | ___ __ ___   ___  __ _ _ __  ___ \n"
"| |/ / '_ ` _ \\ / _ \\/ _` | '_ \\/ __|\n"
"|   <| | | | | |  __/ (_| | | | \\__ \\\n"
"|_|\\_\\_| |_| |_|\\___|\\__,_|_| |_|___/\n");
    // Add function on program exit event
    atexit(&onExit);
    
#ifdef _WIN32
    // Save the time when the program starts to get the execution time later
    ftime(&progStart);
#endif // !_WIN32
}

void onExit(void)
{

#ifdef _WIN32

    ftime(&progEnd);
    
    printf("\n\n---------------------------------------------------------------------------------------------------------------------\n");
    printf("Program ended in %.3f seconds.\n", (1000 * (progEnd.time - progStart.time) + (progEnd.millitm - progStart.millitm)) / 1000.f);
    //system("pause");
#endif // !_WIN32
}