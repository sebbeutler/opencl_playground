#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)

cl_program load_program(cl_context context, char* filename, cl_int* errcode_ret);

char BIN_PATH[255];
int main(int argc, char *argv[])
{
    strcpy(BIN_PATH, argv[0]);
    for (int i = strlen(BIN_PATH) - 1; i >= 0; i--)
    {
        if (BIN_PATH[i] == '/' || BIN_PATH[i] == '\\')
        {
            BIN_PATH[i + 1] = '\0';
            break;
        }
    }

#pragma region OPENCL_INIT

    cl_int ret = 0;
    size_t global_item_size[3];
    size_t local_item_size[3];
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    ret |= clGetPlatformIDs(1, &platform_id, NULL);
    ret |= clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, NULL);
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

#pragma endregion OPENCL_INIT

#pragma region KERNEL_EXEC

    cl_program program = load_program(context, "kernels/main.cl", &ret);
    ret |= clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    
    size_t len = 0;
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    char *buffer = calloc(len, sizeof(char));
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
    printf("%s\n", buffer);
    
    cl_kernel kernel;

    // cl_mem pop_mem =  clCreateBuffer(
    //     context,
    //     CL_MEM_COPY_HOST_PTR,
    //     SIZE,
    //     HOST_PTR,
    //     &ret);
    
    // Run kernel > main
    kernel = clCreateKernel(program, "main", &ret);
    // ret |= clSetKernelArg(kernel, 0, sizeof(unsigned long), &initstate);
    global_item_size[0] = 1;
    local_item_size[0] = 1;
    ret |= clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_item_size, local_item_size, 0, NULL, NULL);
    
    
    // Read buffers
    // ret |= clEnqueueReadBuffer(command_queue, pop_mem, CL_TRUE, 0, pop_size, seq_global, 0, NULL, NULL);
    // ret |= clReleaseMemObject(pop_mem);

    clReleaseKernel(kernel);


#pragma endregion KERNEL_EXEC

#pragma region KERNEL_POST_EXEC

#pragma endregion KERNEL_POST_EXEC


#pragma region OPENCL_CLEAN

    if (ret == 0)
        printf("opencl success.\n", ret);
    else
        printf("opencl error: [%d]\n", ret);

    // clean
    clFlush(command_queue);
    clFinish(command_queue);
    clReleaseCommandQueue(command_queue);
    clReleaseProgram(program);    
    clReleaseContext(context);

#pragma endregion OPENCL_CLEAN

    return 0;
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

