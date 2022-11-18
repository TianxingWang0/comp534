#include "utils.h"


void genetate_matrix(double*& m, int n) {
    srand48(time(NULL));
    int size = n * n;
    m = (double*)malloc(size * sizeof(double));
    for (int i = 0; i < size; i++){
        m[i] = drand48();
    }
}

void matrix_multiply(double* A, double* B, double* C, int n) {
    int index = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                C[index] += A[i * n + k] * B[k * n + j];
            }
            ++index;
        }
    }
}

double check(double *A_arr, double *B_arr, double *C_arr, int n, int plane_length, int submatrix_length) {
    double A[n][n], B[n][n], C[n][n];
    int index = 0;
    for(int submatrix_i = 0; submatrix_i < plane_length; ++submatrix_i) {
        for (int submatrix_j = 0; submatrix_j < plane_length; ++submatrix_j) {
            for (int i = submatrix_i * submatrix_length; i < (submatrix_i + 1) * submatrix_length; ++i) {
                for (int j = submatrix_j * submatrix_length; j < (submatrix_j + 1) * submatrix_length; ++j) {
                    A[i][j] = A_arr[index];
                    B[i][j] = B_arr[index];
                    C[i][j] = C_arr[index];
                    ++index;
                }
            }
        }
    }
    
    // verification
    double res = 0;
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            double tmp = 0;
            for (int k = 0; k < n; k++){
                tmp += A[i][k] * B[k][j];
            }
            res += fabs(tmp - C[i][j]);
        }
    }
    res /= (n * n);
    return res;
}