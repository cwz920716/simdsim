#pragma once

#include <stdio.h>
#include <cuda.h>
#include <time.h>
#include <fstream>
#include <string>
#include <iostream>
#include <limits>
#include <string.h>

#include <unistd.h>
#include <cassert>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <inttypes.h>

#ifndef LSGDEBUG
#define LSGDEBUG 0
#endif 

#define dprintf	if (debug) printf
unsigned const debug = LSGDEBUG;

typedef unsigned foru;
//typedef float foru;

double rtclock()
{
    struct timezone Tzp;
    struct timeval Tp;
    int stat;
    stat = gettimeofday (&Tp, &Tzp);
    if (stat != 0) printf("Error return from gettimeofday: %d",stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
}


__device__ 
void global_sync(unsigned goalVal, volatile unsigned *Arrayin, volatile unsigned *Arrayout) {
	// thread ID in a block
	unsigned tid_in_blk = threadIdx.x * blockDim.y + threadIdx.y;
	unsigned nBlockNum = gridDim.x * gridDim.y;
	unsigned bid = blockIdx.x * gridDim.y + blockIdx.y;
	// only thread 0 is used for synchronization
	if (tid_in_blk == 0) {
		Arrayin[bid] = goalVal;
		__threadfence();
	}
	if (bid == 0) {
		if (tid_in_blk < nBlockNum) {
			while (Arrayin[tid_in_blk] != goalVal){
				//Do nothing here
			}
		}
		__syncthreads();
		if (tid_in_blk < nBlockNum) {
			Arrayout[tid_in_blk] = goalVal;
			__threadfence();
		}
	}
	if (tid_in_blk == 0) {
		while (Arrayout[bid] != goalVal) {
			//Do nothing here
		}
	}
	__syncthreads();
}

static unsigned CudaTest(char *msg)
{
  cudaError_t e;

  cudaThreadSynchronize();
  if (cudaSuccess != (e = cudaGetLastError())) {
    fprintf(stderr, "%s: %d\n", msg, e);
    fprintf(stderr, "%s\n", cudaGetErrorString(e));
    exit(-1);
    //return 1;
  }
  return 0;
}
// from CUDA SDK.
inline int ConvertSMVer2Cores(int major, int minor)
{
        // Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
        typedef struct {
                int SM; // 0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version
                int Cores;
        } sSMtoCores;

        sSMtoCores nGpuArchCoresPerSM[] =
        { { 0x10,  8 }, // Tesla Generation (SM 1.0) G80 class
          { 0x11,  8 }, // Tesla Generation (SM 1.1) G8x class
          { 0x12,  8 }, // Tesla Generation (SM 1.2) G9x class
          { 0x13,  8 }, // Tesla Generation (SM 1.3) GT200 class
          { 0x20, 32 }, // Fermi Generation (SM 2.0) GF100 class
          { 0x21, 48 }, // Fermi Generation (SM 2.1) GF10x class
          { 0x30, 192}, // Fermi Generation (SM 3.0) GK10x class
	  { 0x35, 192}, // Kepler Generation (SM 3.5) GK110 class
          {   -1, -1 }
        };

        int index = 0;
        while (nGpuArchCoresPerSM[index].SM != -1) {
                if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor) ) {
                        return nGpuArchCoresPerSM[index].Cores;
                }
                index++;
        }
        printf("MapSMtoCores SM %d.%d is undefined (please update to the latest SDK)!\n", major, minor);
        return -1;
}

#  define CUDA_SAFE_CALL_NO_SYNC( call) {                                    \
    cudaError err = call;                                                    \
    if( cudaSuccess != err) {                                                \
        fprintf(stderr, "Cuda error in file '%s' in line %i : %s.\n",        \
                __FILE__, __LINE__, cudaGetErrorString( err) );              \
        exit(EXIT_FAILURE);                                                  \
    } }

#  define CUDA_SAFE_CALL( call)     CUDA_SAFE_CALL_NO_SYNC(call);                                            \

#  define CUDA_SAFE_THREAD_SYNC( ) {                                         \
    cudaError err = CUT_DEVICE_SYNCHRONIZE();                                 \
    if ( cudaSuccess != err) {                                               \
        fprintf(stderr, "Cuda error in file '%s' in line %i : %s.\n",        \
                __FILE__, __LINE__, cudaGetErrorString( err) );              \
    } }

// from http://forums.nvidia.com/index.php?showtopic=186669
static __device__ uint get_smid(void) {
     uint ret;
     asm("mov.u32 %0, %smid;" : "=r"(ret) );
     return ret;
}
