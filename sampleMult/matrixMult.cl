__kernel void mvKernel(__global float* a, const __global float* x, __global float* y, int m, int n)
{
float sum = 0.0f;
 __global float* A;
int i;
int j = 0;
int indx = get_global_id(0);
__local float xs[8192];
for(i = get_local_id(0); i < n; i+= get_local_size(0)) {
    xs[i] = x[i];
} 
mem_fence(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
A = &a[indx];
for(i = 0; i < n; i++) {
    sum += xs[i] * A[j];
    j += m;
}
y[indx] = sum;
}
