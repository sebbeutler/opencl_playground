#pragma OPENCL EXTENSION cl_khr_int64_base_atomics: enable
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics: enable

int countCollatz(int n)
{
    int cpt = 1;
    int i = n;
    while (n != 1)
    {
        cpt++;
        if (n % 2 == 0)
        {
            n /= 2;
        }
        else
        {
            n = 3 * n + 1;
        }
        //printf("Co: %d->%d\n", i, n);
    }
    // printf("CPT: %d->%d\n", i, cpt);
    return cpt;
}

kernel void main(global int* res)
{
    int i = get_global_id(0);
    //printf("%d\n", i);
    res[i] = countCollatz(i+1);
}