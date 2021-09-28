#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys\timeb.h>

#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)

struct timeb progStart, progEnd;

cl_program load_program(cl_context context, char* filename, cl_int* errcode_ret);

char BIN_PATH[255];
int main(int argc, char *argv[])
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

    cl_int ret = 0;
    size_t global_item_size[3];
    size_t local_item_size[3];
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    ret |= clGetPlatformIDs(1, &platform_id, NULL);
    ret |= clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, NULL);
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    
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

    ftime(&progStart); 

    cl_program program = load_program(context, "kernels/main.cl", &ret);
    ret |= clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    
    size_t len = 0;
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    char *buffer = calloc(len, sizeof(char));
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
    printf("%s\n", buffer);
    
    cl_kernel kernel;

    size_t res_size = sizeof(int) * 1000000;
    int* res = malloc(res_size);
    
    cl_mem res_mem =  clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        res_size,
        NULL,
        &ret);
    
    // Run kernel > main
    kernel = clCreateKernel(program, "main", &ret);
    ret |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &res_mem);
    memcpy(global_item_size, (size_t[3]) { 1000000, 1, 1}, sizeof(global_item_size));
    memcpy(local_item_size, (size_t[3]) { 1000, 1, 1}, sizeof(local_item_size));

    ret |= clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, NULL);
    
    // Read buffers
    // cl_ulong result;
    ret |= clEnqueueReadBuffer(command_queue, res_mem, CL_TRUE, 0, res_size, res, 0, NULL, NULL);
    ret |= clReleaseMemObject(res_mem);

    clReleaseKernel(kernel);


#pragma endregion KERNEL_EXEC

#pragma region KERNEL_POST_EXEC

    int max = 0;
    for (int i=0; i<1000000; i++)
    {
        max = max(max, res[i]);
    }
    printf("Result: %d\n", max);
    
    ftime(&progEnd);
    printf("Program ended in %.3f seconds.\n", (1000 * (progEnd.time - progStart.time) + (progEnd.millitm - progStart.millitm)) / 1000.f); 

#pragma endregion KERNEL_POST_EXEC


#pragma region OPENCL_CLEAN

    printf("\n");
    if (ret == 0)
        printf("opencl success.\n");
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

