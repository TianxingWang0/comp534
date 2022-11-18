#include "utils.h"
#include "timer.h"


int main(int argc, char **argv) {
  const char *name = argv[0];
  if (argc < 3) {
    usage(name);
  }
  int n = std::atoi(argv[1]);
  int nworkers = atoi(argv[2]);
  // std::cout << name << ": "
  //           << n << " " << nworkers
  //           << std::endl;
  omp_set_num_threads(nworkers);

  double **A = new double*[n], **L = new double*[n], **U = new double*[n];
  int* pi = new int[n];
  init(A, L, U, pi, n, nworkers);
  // double** A_origin = deep_copy(A, n);
  // print_matrix("A", A, n);
  // print_matrix("L", L, n);
  // print_matrix("U", U, n);

  struct Compare { double val = 0.0; int index; };    
  #pragma omp declare reduction(maximum : struct Compare : omp_out = omp_in.val > omp_out.val ? omp_in : omp_out)

  // parallel LU phrase
  timer_start();

  for (int k = 0; k < n; ++k) {
    double max_val = 0.0;
    int k_;
    for (int i = k; i < n; ++i) {
      if (max_val < fabs(A[i][k])) {
        max_val = fabs(A[i][k]);
        k_ = i;
      }
    }

    // Compare max_val;
    // #pragma omp parallel for default(none) schedule(static, 0) reduction(maximum : max_val) num_threads(nworkers) shared(A, k, n)
    // for (int i = n - 1; i > k; --i) {
    //   // printf("step 1 row %d with thread : %d \n", i, omp_get_thread_num());
    //   double abs_val = fabs(A[i][k]);
    //   if (max_val.val < abs_val) {
    //     max_val.val = abs_val;
    //     max_val.index = i;
    //   }
    // }

    // int k_ = max_val.index;

    // std::swap(pi[k], pi[k_]);
    // step 1 : master thread
    std::swap(A[k], A[k_]);

    // step 2 ~ 4 can all be cuncurrent

    # pragma omp parallel default(none) shared(k, k_, n, A, L, U) num_threads(nworkers)
    {
      // step 2 : swap(L[k, :k], L[k’, :k])
      # pragma omp single nowait
      {
        // print_thread_cpu(2);
        std::swap_ranges(L[k], L[k] + k, L[k_]);
        //std::swap_ranges(L[pi[k]], L[pi[k]] + k, L[k_]);
        // for (int j = 0; j < k; ++j) {
        //   std::swap(L[k][j], L[k_][j]);
        // }
      }
      // step 3 : assign A[k, k:] to U[k, k:]
      # pragma omp single nowait
      {
        // print_thread_cpu(3, k);
        memcpy(&U[k][k], &A[k][k], sizeof(double) * (n - k));
        // for (int i = k; i < n; ++i) {
        //   U[k][i] = A[k][i];
        // }
      }

      // step 4 : assign A[k:, k] / A[k][k] to L[k:, k] then A[i][j] -= L[i][k] * U[k][j], i and j in [k + 1, n]
      # pragma omp for nowait schedule(static, 1)
      for (int i = n - 1; i > k; --i) {
        // print_thread_cpu(4, i);
        double tmp = A[i][k] / A[k][k];
        L[i][k] = tmp;  // step 4
        for (int j = k + 1; j < n; ++j) {
          A[i][j] -= tmp * A[k][j];
        }
      }
    }
    // printf("barrier\n");
  }

  // LU ends
  double seconds = timer_elapsed();
  printf(", %.3f", seconds);
  // printf("Norm of PA - LU = %f.\n", norm(n, pi, A_origin, L, U));
  return 0;
}
