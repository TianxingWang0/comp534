#pragma once

#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string.h>
#include <omp.h>
#include <stdlib.h>
#include <numa.h>

// print out the usage of program
void usage(const char *name);

// print out the matrix for debugging
void print_matrix(char name[], double** A, int n);

// print the thread and cpu index
void print_thread_cpu(int step, int row = -1);

// print out the compact matrix pi for debugging
void print_matrix(int* pi, int n);

// init A with random value and L U 
void init(double** A, double** L, double** U, int* pi, int n, int nworkers);

// deep copy matrix A for norm check
double** deep_copy(double** A, int n);

// apply L2,1 normalization of Pi@A - L@U
double norm(int n, int* pi, double** A, double** L, double** U);

// matrix mutiply of row L and U[j]
double multiply(double* L, double** U, int j, int length);

// euclidean norm of A's column j
double euclidean(int n, double** A, int j);