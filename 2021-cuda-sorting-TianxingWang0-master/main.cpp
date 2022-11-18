/**
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/**
 * This sample implements bitonic sort and odd-even merge sort, algorithms
 * belonging to the class of sorting networks.
 * While generally subefficient on large sequences
 * compared to algorithms with better asymptotic algorithmic complexity
 * (i.e. merge sort or radix sort), may be the algorithms of choice for sorting
 * batches of short- or mid-sized arrays.
 * Refer to the excellent tutorial by H. W. Lang:
 * http://www.iti.fh-flensburg.de/lang/algorithmen/sortieren/networks/indexen.htm
 *
 * Victor Podlozhnyuk, 07/09/2009
 */

// CUDA Runtime
#include <cuda_runtime.h>

// Utilities and system includes
#include <helper_cuda.h>
#include <helper_timer.h>
#include <math.h>
#include <stdlib.h>
#include "sortingNetworks_common.h"

////////////////////////////////////////////////////////////////////////////////
// Test driver
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    cudaError_t error;
    printf("%s Starting...\n\n", argv[0]);
    // the array length is 2^k
    uint kexp = atoi(argv[1]);

    printf("Starting up CUDA context...\n");
    int dev = findCudaDevice(argc, (const char **)argv);

    // sort an array of uint
    uint *h_InputKey, *h_OutputKeyGPU;
    uint *d_InputKey, *d_OutputKey;
    uint *t_InputKey, *t_OutputKey;  // tmp pointer for better code format
    StopWatchInterface *hTimer = NULL;

    const uint             N = pow(2, kexp);
    uint         arrayLength;
    uint                 DIR = 0;
    uint         threadCount = 0;
    const uint     numValues = 65536;
    const uint GPUMem_length = 1048576;     // the maximum length of array to fit into GPU memory
    const uint  chunk_length = GPUMem_length / 2;  // two chunks fit right into GPU mem

    printf("Allocating and initializing host arrays...\n\n");
    sdkCreateTimer(&hTimer);
    h_InputKey     = (uint *)malloc(N * sizeof(uint));
    h_OutputKeyGPU = (uint *)malloc(N * sizeof(uint));
    srand(2001);

    for (uint i = 0; i < N; i++)
    {
        h_InputKey[i] = rand() % numValues;
    }

    int flag = 1;

    if (kexp <= 20)  // the input data can all fit in GPU 
    {
        printf("Allocating and initializing CUDA arrays...\n\n");
        error = cudaMalloc((void **)&d_InputKey,  N * sizeof(uint));
        checkCudaErrors(error);
        error = cudaMalloc((void **)&d_OutputKey, N * sizeof(uint));
        checkCudaErrors(error);
        error = cudaMemcpy(d_InputKey, h_InputKey, N * sizeof(uint), cudaMemcpyHostToDevice);
        checkCudaErrors(error);

        arrayLength = N;
        printf("Testing array length %u (%u arrays per batch)...\n", arrayLength, N / arrayLength);
        error = cudaDeviceSynchronize();
        checkCudaErrors(error);

        sdkResetTimer(&hTimer);
        sdkStartTimer(&hTimer);

        threadCount = bitonicSort(
                            d_OutputKey,
                            d_InputKey,
                            N / arrayLength,
                            arrayLength,
                            DIR
                        );
        error = cudaDeviceSynchronize();
        checkCudaErrors(error);
    }
    else  // kexp > 20, need external bitonic sort
    {
        printf("Allocating and initializing CUDA arrays...\n\n");
        // For this part, the loading data into the GPU cannot exceed 1M, so we choose the largest one;
        error = cudaMalloc((void **)&d_InputKey, GPUMem_length * sizeof(uint));
        checkCudaErrors(error);
        error = cudaMalloc((void **)&d_OutputKey, GPUMem_length * sizeof(uint));
        checkCudaErrors(error);

        arrayLength = GPUMem_length;
        uint chunk_number = N / chunk_length;  // the number of chunks in the array

        printf("Testing array length %u (%u arrays per batch)...\n", arrayLength, GPUMem_length / arrayLength);

        // apply CBM[M], M = 2..chunk_number
        for (uint M = 2; M <= chunk_number; M <<= 1) {
            uint group_number = chunk_number / M;  // the total group number
            for (uint stride = M / 2; stride > 0; stride >>= 1) {
                // this phase upper chunk and lower chunk with distance of chunk count of 'stride'
                DIR = 0;  // the first group's sort direction is 0
                for (uint group_index = 0; group_index < group_number; ++group_index) {
                    uint upper_chunk_index = group_index * M;
                    uint lower_chunk_index = upper_chunk_index + stride;
                    for (uint shift = 0; shift < stride / 2; ++shift) {
                        // load upper chunk to device
                        t_InputKey = h_InputKey + chunk_length * upper_chunk_index;
                        error = cudaMemcpy(d_InputKey, t_InputKey, chunk_length * sizeof(uint), cudaMemcpyHostToDevice);
                        checkCudaErrors(error);

                        // load lower chunk to device
                        t_InputKey = h_InputKey + chunk_length * lower_chunk_index;
                        d_InputKey += chunk_length;
                        error = cudaMemcpy(d_InputKey, t_InputKey, chunk_length * sizeof(uint), cudaMemcpyHostToDevice);
                        d_InputKey -= chunk_length;
                        checkCudaErrors(error);

                        // printf("M : %u, stride : %u, group_index : %u, upper_index : %u, lower_index : %u, DIR, : %u\n",
                        //            M, stride, group_index, upper_chunk_index, lower_chunk_index, DIR);

                        if (stride == 1) {  // do neighboring pair chunk bitonic sort in the rightmost phase
                            threadCount = bitonicSort(
                                                d_OutputKey,
                                                d_InputKey,
                                                GPUMem_length / arrayLength,
                                                arrayLength,
                                                DIR
                                            );
                        } else {
                            threadCount = chunkBitonicMerge(
                                                d_OutputKey,
                                                d_InputKey,
                                                GPUMem_length / arrayLength,
                                                arrayLength,
                                                DIR
                                                );
                        }
    
                        error = cudaDeviceSynchronize();
                        checkCudaErrors(error);

                        // copy the d_array back into the h_array;
                        t_OutputKey = h_OutputKeyGPU + chunk_length * upper_chunk_index;
                        error = cudaMemcpy(t_OutputKey, d_OutputKey, chunk_length * sizeof(uint), cudaMemcpyDeviceToHost);
                        checkCudaErrors(error);

                        t_OutputKey = h_OutputKeyGPU + chunk_length * lower_chunk_index;
                        d_OutputKey += chunk_length;
                        error = cudaMemcpy(t_OutputKey, d_OutputKey, chunk_length * sizeof(uint), cudaMemcpyDeviceToHost);
                        d_OutputKey -= chunk_length;
                        checkCudaErrors(error);

                        // shift upper chunk index and lower chunk index downward for next iteration
                        ++upper_chunk_index;
                        ++lower_chunk_index;
                    }
                    DIR = !(DIR);  // alternate the direction of neighboring group
                }
            }
            // finish a CBM, copy back to h_InputKey for next CBM
            memcpy(h_InputKey, h_OutputKeyGPU, N * sizeof(uint));
        }
    } 

    sdkStopTimer(&hTimer);
    printf("Average time: %f ms\n\n", sdkGetTimerValue(&hTimer));

    double dTimeSecs = 1.0e-3 * sdkGetTimerValue(&hTimer);
    printf("sortingNetworks-bitonic, Throughput = %.4f MElements/s, Time = %.5f s, Size = %u elements, NumDevsUsed = %u, Workgroup = %u\n",
            (1.0e-6 * (double)arrayLength / dTimeSecs), dTimeSecs, arrayLength, 1, threadCount);

    printf("\nValidating the results...\n");
    if (kexp <= 20) 
    {
        printf("...reading back GPU results\n");
        error = cudaMemcpy(h_OutputKeyGPU, d_OutputKey, N * sizeof(uint), cudaMemcpyDeviceToHost);
        checkCudaErrors(error);
    }

    int keysFlag = validateSortedKeys(h_OutputKeyGPU, h_InputKey, N / arrayLength, arrayLength, numValues, DIR);
    flag = flag && keysFlag;

    printf("\n");

    printf("Shutting down...\n");
    sdkDeleteTimer(&hTimer);
    cudaFree(d_OutputKey);
    cudaFree(d_InputKey);
    free(h_OutputKeyGPU);
    free(h_InputKey);

    exit(flag ? EXIT_SUCCESS : EXIT_FAILURE);
}
