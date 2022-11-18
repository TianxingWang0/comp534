#include "utils.h"

void usage(const char *name) {
  std::cout << "usage: " << name
            << " matrix-size nworkers"
            << std::endl;
  exit(-1);
}

void print_thread_cpu(int step, int row) {
  int cpu = sched_getcpu();
  if (row == -1) {
    printf("step %d with thread : %d, on cpu : %d, on node : %d \n", step, omp_get_thread_num(), cpu, numa_node_of_cpu(cpu));
  } else {
    printf("step %d on row %d with thread : %d, on cpu : %d, on node : %d \n", step, row, omp_get_thread_num(), cpu, numa_node_of_cpu(cpu));
  }
}

void print_matrix(char name[], double** A, int n) {
  printf(name);
  printf("\n[");
  for (int i = 0; i < n; ++i) {
    printf("[");
    for (int j = 0; j < n; ++j) {
      printf("%f\t", A[i][j]);
    }
    printf("],\n");
  }
  printf("]\n");
}

void print_matrix(int* pi, int n) {
  printf("Pi:\n");
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      if (j == pi[i]) {
        printf("1\t");
      } else {
        printf("0\t");
      }
    }
    printf("\n");
  }
  printf("\n");
}

void init(double** A, double** L, double** U, int* pi, int n, int nworkers) {
  # pragma omp parallel for schedule(static, 1) default(none) shared(A, L, U, pi, n) 
  for (int i = n - 1; i >= 0; --i) {
    // print_thread_cpu(0, i);
    int size_alloc = sizeof(double) * n;
    A[i] = (double*)numa_alloc_local(size_alloc);
    srand48(i);
    for (int j = 0; j < n; ++j) {
      A[i][j] = drand48();
    }

    pi[i] = i;
    L[i] = (double*)numa_alloc_local(size_alloc);
    U[i] = (double*)numa_alloc_local(size_alloc);
    memset(L[i], 0.0, size_alloc);
    memset(U[i], 0.0, size_alloc);
    L[i][i] = 1.0;
  }
}

double** deep_copy(double** A, int n) {
  double** ans = new double*[n];
  for (int i = 0; i < n; ++i) {
    ans[i] = new double[n];
    memcpy(ans[i], A[i], sizeof(double) * n);
  }
  return ans;
}

double multiply(double* L, double** U, int j, int length) {
  double ans = 0.0;
  for (int i = 0; i <= length; ++i) {
    ans += L[i] * U[i][j];
  }
  return ans;
}

double euclidean(int n, double** A, int j) {
  double ans = 0.0; 
  for (int i = 0; i < n; ++i) {
    ans += pow(A[i][j], 2);
  }
  return sqrt(ans);
}

double norm(int n, int* pi, double** A, double** L, double** U) {
  double** res = new double*[n];
  double ans = 0.0;
  for (int i = 0; i < n; ++i) {
    res[i] = new double[n];
    for (int j = 0; j < n; ++j) {
      res[i][j] = A[pi[i]][j] - multiply(L[i], U, j, std::min(i, j));
    }
  }
  for (int j = 0; j < n; ++j) {
    ans += euclidean(n, res, j);
  }
  return ans;
}