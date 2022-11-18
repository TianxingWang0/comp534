#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <cmath>
#include <algorithm>
#include <string.h>
#include <time.h>

// randomly generate n * n matrix 
void genetate_matrix(double*& m, int n);

// calculate C += A * B
void matrix_multiply(double* A, double* B, double* C, int n);

// wait mpi message send&recv for A and B
// void mpi_wait(MPI_Request* request_b_send, request_b_recv, )

// check the multiply result
double check(double *A_arr, double *B_arr, double *C_arr, int n, int plane_length, int submatrix_length);

// 2.5D cannon send and recieve step
// void send_and_recv()