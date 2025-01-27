#pragma once

#include <cuda_runtime.h>
#include <math.h>

inline dim3 getGridSize(dim3 taskSize, dim3 blockSize)
{
    return dim3((taskSize.x + blockSize.x - 1) / blockSize.x, (taskSize.y + blockSize.y - 1) / blockSize.y, (taskSize.z + blockSize.z - 1) / blockSize.z);
}

inline dim3 getGridSize(Vector2i taskSize, dim3 blockSize)
{
    return getGridSize(dim3(taskSize.x, taskSize.y), blockSize);
}

/// Classes extending this must be head allocated
struct Managed {
    void *operator new(size_t len){
        void *ptr;
        cudaMallocManaged(&ptr, len); // did some earlier kernel throw an assert?
        cudaDeviceSynchronize();
        return ptr;
    }

    void operator delete(void *ptr) {
        cudaDeviceSynchronize();  // did some earlier kernel throw an assert?
        cudaFree(ptr);
    }
};

/// ! Must be run by a single warp (32 threads) simultaneously.
inline __device__ void warpReduce(volatile float* sdata, int tid) {
    // Ignore the fact that we compute some unnecessary sums.
	sdata[tid] += sdata[tid + 32];
	sdata[tid] += sdata[tid + 16];
	sdata[tid] += sdata[tid + 8];
	sdata[tid] += sdata[tid + 4];
	sdata[tid] += sdata[tid + 2];
	sdata[tid] += sdata[tid + 1];
}


template<typename T //!< int or float
>
inline __device__ void warpReduce256(
float localValue,
volatile float* dim_shared1,
int locId_local,
T* outTotal) {
    dim_shared1[locId_local] = localValue;
    __syncthreads();

    if (locId_local < 128) dim_shared1[locId_local] += dim_shared1[locId_local + 128];
    __syncthreads();
    if (locId_local < 64) dim_shared1[locId_local] += dim_shared1[locId_local + 64];
    __syncthreads();

    if (locId_local < 32) warpReduce(dim_shared1, locId_local);

    if (locId_local == 0) atomicAdd(outTotal, (T)dim_shared1[locId_local]);
}

template <typename T> 
__device__ int computePrefixSum_device(uint element, T *sum, int localSize, int localId)
{
	// TODO: should be localSize...
	__shared__ uint prefixBuffer[16 * 16];
	__shared__ uint groupOffset;

	prefixBuffer[localId] = element;
	__syncthreads();

	int s1, s2;

	for (s1 = 1, s2 = 1; s1 < localSize; s1 <<= 1)
	{
		s2 |= s1;
		if ((localId & s2) == s2) prefixBuffer[localId] += prefixBuffer[localId - s1];
		__syncthreads();
	}

	for (s1 >>= 2, s2 >>= 1; s1 >= 1; s1 >>= 1, s2 >>= 1)
	{
		if (localId != localSize - 1 && (localId & s2) == s2) prefixBuffer[localId + s1] += prefixBuffer[localId];
		__syncthreads();
	}

	if (localId == 0 && prefixBuffer[localSize - 1] > 0) groupOffset = atomicAdd(sum, prefixBuffer[localSize - 1]);
	__syncthreads();

	int offset;// = groupOffset + prefixBuffer[localId] - 1;
	if (localId == 0) {
		if (prefixBuffer[localId] == 0) offset = -1;
		else offset = groupOffset;
	} else {
		if (prefixBuffer[localId] == prefixBuffer[localId - 1]) offset = -1;
		else offset = groupOffset + prefixBuffer[localId-1];
	}

	return offset;
}

__device__ static inline void atomicMin(float* address, float val)
{
	int* address_as_i = (int*)address;
	int old = *address_as_i, assumed;
	do {
		assumed = old;
		old = ::atomicCAS(address_as_i, assumed,
			__float_as_int(::fminf(val, __int_as_float(assumed))));
	} while (assumed != old);
}

__device__ static inline void atomicMax(float* address, float val)
{
	int* address_as_i = (int*)address;
	int old = *address_as_i, assumed;
	do {
		assumed = old;
		old = ::atomicCAS(address_as_i, assumed,
			__float_as_int(::fmaxf(val, __int_as_float(assumed))));
	} while (assumed != old);
}

template<typename T>
__global__ void memsetKernel_device(T *devPtr, const T val, size_t nwords)
{
	size_t offset = threadIdx.x + blockDim.x * blockIdx.x;
	if (offset >= nwords) return;
	devPtr[offset] = val;
}

template<typename T>
__global__ void memsetKernelLarge_device(T *devPtr, const T val, size_t nwords)
{
	size_t offset = threadIdx.x + blockDim.x * (blockIdx.x + blockIdx.y * gridDim.x);
	if (offset >= nwords) return;
	devPtr[offset] = val;
}

/// Sets nwords many val in devPtr
/// Note that this gives the amount of elements, not of bytes
template<typename T>
inline void memsetKernel(T *devPtr, const T val, size_t nwords)
{
	dim3 blockSize(256);
	dim3 gridSize((int)ceil((float)nwords / (float)blockSize.x));
	if (gridSize.x <= 65535) {
		memsetKernel_device<T> <<<gridSize,blockSize>>>(devPtr, val, nwords);
	} else {
		gridSize.x = (int)ceil(sqrt((float)gridSize.x));
		gridSize.y = (int)ceil((float)nwords / (float)(blockSize.x * gridSize.x));
		memsetKernelLarge_device<T> <<<gridSize,blockSize>>>(devPtr, val, nwords);
	}
}

template<typename T>
__global__ void fillArrayKernel_device(T *devPtr, size_t nwords)
{
	size_t offset = threadIdx.x + blockDim.x * blockIdx.x;
	if (offset >= nwords) return;
	devPtr[offset] = offset;
}

/// Fill array at devPtr with 0:nwords-1
/// T must be of number type (convertible from size_t)
template<typename T>
inline void fillArrayKernel(T *devPtr, size_t nwords)
{
	dim3 blockSize(256);
	dim3 gridSize((int)ceil((float)nwords / (float)blockSize.x));
	fillArrayKernel_device<T> <<<gridSize,blockSize>>>(devPtr, nwords);
}