/*
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */



//Based on http://www.iti.fh-flensburg.de/lang/algorithmen/sortieren/bitonic/bitonicen.htm



#include <assert.h>
#include <cooperative_groups.h>

namespace cg = cooperative_groups;
#include <helper_cuda.h>
#include "sortingNetworks_common.h"
#include "sortingNetworks_common.cuh"



////////////////////////////////////////////////////////////////////////////////
// Monolithic bitonic sort kernel for short arrays fitting into shared memory
////////////////////////////////////////////////////////////////////////////////
__global__ void bitonicSortShared(
    uint *d_DstKey,
    uint *d_SrcKey,
    uint arrayLength,
    uint dir,
    uint small_size
)
{
    // Handle to thread block group
    cg::thread_block cta = cg::this_thread_block();
    //Shared memory storage for one or more short vectors
    __shared__ uint s_key[SHARED_SIZE_LIMIT];

    //Offset to the beginning of subbatch and load data
    d_SrcKey += blockIdx.x * small_size + threadIdx.x;
    d_DstKey += blockIdx.x * small_size + threadIdx.x;
    s_key[threadIdx.x +                0] = d_SrcKey[               0];
    s_key[threadIdx.x + (small_size / 2)] = d_SrcKey[(small_size / 2)];

    for (uint size = 2; size < arrayLength; size <<= 1)
    {
        //Bitonic merge
        uint ddd = dir ^ ((threadIdx.x & (size / 2)) != 0);

        for (uint stride = size / 2; stride > 0; stride >>= 1)
        {
            cg::sync(cta);
            uint pos = 2 * threadIdx.x - (threadIdx.x & (stride - 1));
            Comparator(
                s_key[pos +      0],
                s_key[pos + stride],
                ddd
            );
        }
    }

    //ddd == dir for the last bitonic merge step
    {
        for (uint stride = arrayLength / 2; stride > 0; stride >>= 1)
        {
            cg::sync(cta);
            uint pos = 2 * threadIdx.x - (threadIdx.x & (stride - 1));
            Comparator(
                s_key[pos +      0],
                s_key[pos + stride],
                dir
            );
        }
    }

    cg::sync(cta);
    d_DstKey[               0] = s_key[threadIdx.x +                0];
    d_DstKey[(small_size / 2)] = s_key[threadIdx.x + (small_size / 2)];
}



////////////////////////////////////////////////////////////////////////////////
// Bitonic sort kernel for large arrays (not fitting into shared memory)
////////////////////////////////////////////////////////////////////////////////
//Bottom-level bitonic sort
//Almost the same as bitonicSortShared with the exception of
//even / odd subarrays being sorted in opposite directions
//Bitonic merge accepts both
//Ascending | descending or descending | ascending sorted pairs
__global__ void bitonicSortShared1(
    uint *d_DstKey,
    uint *d_SrcKey
)
{
    // Handle to thread block group
    cg::thread_block cta = cg::this_thread_block();
    //Shared memory storage for current subarray
    __shared__ uint s_key[SHARED_SIZE_LIMIT];

    //Offset to the beginning of subarray and load data
    d_SrcKey += blockIdx.x * SHARED_SIZE_LIMIT + threadIdx.x;
    d_DstKey += blockIdx.x * SHARED_SIZE_LIMIT + threadIdx.x;
    s_key[threadIdx.x +                       0] = d_SrcKey[                      0];
    s_key[threadIdx.x + (SHARED_SIZE_LIMIT / 2)] = d_SrcKey[(SHARED_SIZE_LIMIT / 2)];

    for (uint size = 2; size < SHARED_SIZE_LIMIT; size <<= 1)
    {
        //Bitonic merge
        uint ddd = (threadIdx.x & (size / 2)) != 0;

        for (uint stride = size / 2; stride > 0; stride >>= 1)
        {
            cg::sync(cta);
            uint pos = 2 * threadIdx.x - (threadIdx.x & (stride - 1));
            Comparator(
                s_key[pos +      0],
                s_key[pos + stride],
                ddd
            );
        }
    }

    //Odd / even arrays of SHARED_SIZE_LIMIT elements
    //sorted in opposite directions
    uint ddd = blockIdx.x & 1;
    {
        for (uint stride = SHARED_SIZE_LIMIT / 2; stride > 0; stride >>= 1)
        {
            cg::sync(cta);
            uint pos = 2 * threadIdx.x - (threadIdx.x & (stride - 1));
            Comparator(
                s_key[pos +      0],
                s_key[pos + stride],
                ddd
            );
        }
    }


    cg::sync(cta);
    d_DstKey[                      0] = s_key[threadIdx.x +                       0];
    d_DstKey[(SHARED_SIZE_LIMIT / 2)] = s_key[threadIdx.x + (SHARED_SIZE_LIMIT / 2)];
}

//Bitonic merge iteration for stride >= SHARED_SIZE_LIMIT
__global__ void bitonicMergeGlobal(
    uint *d_DstKey,
    uint *d_SrcKey,
    uint arrayLength,
    uint size,
    uint stride,
    uint dir
)
{
    uint global_comparatorI = blockIdx.x * blockDim.x + threadIdx.x;
    uint        comparatorI = global_comparatorI & (arrayLength / 2 - 1);

    //Bitonic merge
    uint ddd = dir ^ ((comparatorI & (size / 2)) != 0);
    uint pos = 2 * global_comparatorI - (global_comparatorI & (stride - 1));

    uint keyA = d_SrcKey[pos +      0];
    uint keyB = d_SrcKey[pos + stride];

    Comparator(
        keyA,
        keyB,
        ddd
    );

    d_DstKey[pos +      0] = keyA;
    d_DstKey[pos + stride] = keyB;
}

//Combined bitonic merge steps for
//size > SHARED_SIZE_LIMIT and stride = [1 .. SHARED_SIZE_LIMIT / 2]
__global__ void bitonicMergeShared(
    uint *d_DstKey,
    uint *d_SrcKey,
    uint arrayLength,
    uint size,
    uint dir
)
{
    // Handle to thread block group
    cg::thread_block cta = cg::this_thread_block();
    //Shared memory storage for current subarray
    __shared__ uint s_key[SHARED_SIZE_LIMIT];

    d_SrcKey += blockIdx.x * SHARED_SIZE_LIMIT + threadIdx.x;
    d_DstKey += blockIdx.x * SHARED_SIZE_LIMIT + threadIdx.x;
    s_key[threadIdx.x +                       0] = d_SrcKey[                      0];
    s_key[threadIdx.x + (SHARED_SIZE_LIMIT / 2)] = d_SrcKey[(SHARED_SIZE_LIMIT / 2)];

    //Bitonic merge
    uint comparatorI = UMAD(blockIdx.x, blockDim.x, threadIdx.x) & ((arrayLength / 2) - 1);
    uint ddd = dir ^ ((comparatorI & (size / 2)) != 0);

    for (uint stride = SHARED_SIZE_LIMIT / 2; stride > 0; stride >>= 1)
    {
        cg::sync(cta);
        uint pos = 2 * threadIdx.x - (threadIdx.x & (stride - 1));
        Comparator(
            s_key[pos +      0],
            s_key[pos + stride],
            ddd
        );
    }

    cg::sync(cta);
    d_DstKey[                      0] = s_key[threadIdx.x +                       0];
    d_DstKey[(SHARED_SIZE_LIMIT / 2)] = s_key[threadIdx.x + (SHARED_SIZE_LIMIT / 2)];
}



////////////////////////////////////////////////////////////////////////////////
// Interface function
////////////////////////////////////////////////////////////////////////////////
//Helper function (also used by odd-even merge sort)
extern "C" uint factorRadix2(uint *log2L, uint L)
{
    if (!L)
    {
        *log2L = 0;
        return 0;
    }
    else
    {
        for (*log2L = 0; (L & 1) == 0; L >>= 1, *log2L++);

        return L;
    }
}

// bitonic sort on array of uint
extern "C" uint bitonicSort(
    uint *d_DstKey,
    uint *d_SrcKey,
    uint batchSize,
    uint arrayLength,
    uint dir
)
{
    //Nothing to sort
    if (arrayLength < 2)
        return 0;

    //Only power-of-two array lengths are supported by this implementation
    uint log2L;
    uint factorizationRemainder = factorRadix2(&log2L, arrayLength);
    assert(factorizationRemainder == 1);

    dir = (dir != 0);

    uint  blockCount = batchSize * arrayLength / SHARED_SIZE_LIMIT;
    uint threadCount = SHARED_SIZE_LIMIT / 2;

    if (arrayLength <= SHARED_SIZE_LIMIT)
    {
        if ((batchSize * arrayLength) % SHARED_SIZE_LIMIT == 0)
        {
            bitonicSortShared<<<blockCount, threadCount>>>(d_DstKey, d_SrcKey, arrayLength, dir, SHARED_SIZE_LIMIT);
        }
        else  // The kexp is smaller than 10;
        {
            uint small_size = batchSize * arrayLength;
            blockCount = 1;
            threadCount = batchSize * arrayLength / 2;
            bitonicSortShared<<<blockCount, threadCount>>>(d_DstKey, d_SrcKey, arrayLength, dir, small_size);
        }  
    }
    else
    {
        bitonicSortShared1<<<blockCount, threadCount>>>(d_DstKey, d_SrcKey);

        for (uint size = 2 * SHARED_SIZE_LIMIT; size <= arrayLength; size <<= 1)
            for (unsigned stride = size / 2; stride > 0; stride >>= 1)
                if (stride >= SHARED_SIZE_LIMIT)
                {
                    bitonicMergeGlobal<<<(batchSize * arrayLength) / 512, 256>>>(d_DstKey, d_DstKey, arrayLength, size, stride, dir);
                }
                else
                {
                    bitonicMergeShared<<<blockCount, threadCount>>>(d_DstKey, d_DstKey, arrayLength, size, dir);
                    break;
                }
    }

    return threadCount;
}


// chunk bitonic merge, two pointers compare and swap on upper and lower chunck
extern "C" uint chunkBitonicMerge(
    uint *d_DstKey,
    uint *d_SrcKey,
    uint batchSize,
    uint arrayLength,
    uint dir
)
{
    //Nothing to sort
    if (arrayLength < 2)
        return 0;

    //Only power-of-two array lengths are supported by this implementation
    uint log2L;
    uint factorizationRemainder = factorRadix2(&log2L, arrayLength);
    assert(factorizationRemainder == 1);

    dir = (dir != 0);

    uint threadCount = SHARED_SIZE_LIMIT / 2;

    uint size = arrayLength;
    uint stride = size / 2;
    bitonicMergeGlobal<<<(batchSize * arrayLength) / 512, 256>>>(d_DstKey, d_DstKey, arrayLength, size, stride, dir);

    return threadCount;
}
