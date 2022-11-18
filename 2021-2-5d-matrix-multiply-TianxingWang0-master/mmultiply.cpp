#include "utils.h"

#define GRID_DIM 3  // the cartisian grid dimension is 3-D

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    // read matrix length n and grid Z axis dimension size c from argv
    int n = atoi(argv[1]), c = atoi(argv[2]), verify = atoi(argv[3]);
    int size = n * n;
    int p;  // processes number
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // create caetesian topology, a 3-D grid of sqrt(p/c) * sqrt(p/c) * c
    int plane_length = (int)sqrt(p / c);
    int dimensions[3] = {plane_length, plane_length, c};
    int periodic[3] = {1, 1, 1};  // periodic on all three dimension
    MPI_Comm cart_comm;
    MPI_Cart_create(MPI_COMM_WORLD, GRID_DIM, dimensions, periodic, 1, &(cart_comm)); 
    int rank;
    MPI_Comm_rank(cart_comm, &(rank));   // set the rank after MPI_Cart_create::reorder
    int coordinates[3];  // process coordinate in the grid
    MPI_Cart_coords(cart_comm, rank, GRID_DIM, coordinates);

    // set the grid position info
    int row = coordinates[0], col = coordinates[1], k = coordinates[2];
    
    MPI_Comm plane_comm, z_comm;
    int plane_sub_dim[3] = {1, 1, 0}, z_sub_dim[3] = {0, 0, 1};
    MPI_Cart_sub(cart_comm, plane_sub_dim, &(plane_comm));
    MPI_Cart_sub(cart_comm, z_sub_dim, &(z_comm));

    // create matrix A and B on root process, who is locate at k == 0, choose coordinate {0, 0, 0} as root
    double *A, *B;
    if (rank == 0) {
        genetate_matrix(A, n);
        genetate_matrix(B, n);
    }

    /*
        allocate local A and local B for each processes, local_* and local_*_buffer enable non-blocking
        send&recv while computing
    */
    int elements_per_node = size / (p / c);  // submatrix elements number
    double *local_A = (double*) malloc(elements_per_node * sizeof(double));
    double *local_B = (double *) malloc(elements_per_node * sizeof(double));
    double *local_A_buffer = (double *) malloc(elements_per_node * sizeof(double));
    double *local_B_buffer = (double *) malloc(elements_per_node * sizeof(double));
    double start_time = MPI_Wtime();

    MPI_Barrier(plane_comm);
    // root scatters sub matrices of A and B on its plane
    if (k == 0) {
        MPI_Scatter(A, elements_per_node, MPI_DOUBLE, local_A_buffer, elements_per_node, MPI_DOUBLE, 0, plane_comm); 
        MPI_Scatter(B, elements_per_node, MPI_DOUBLE, local_B_buffer, elements_per_node, MPI_DOUBLE, 0, plane_comm);
    }
    MPI_Barrier(plane_comm);

    // plane k == 0 broadcast sub matrices along z direction
    MPI_Bcast(local_A_buffer, elements_per_node, MPI_DOUBLE, 0, z_comm);
    MPI_Bcast(local_B_buffer, elements_per_node, MPI_DOUBLE, 0, z_comm); 
    MPI_Barrier(z_comm);

    // cannon's alignment step
    int tag_A = 1, tag_B = 2;  // message passing tag for A and B
    
    int rank_source_a, rank_dest_a;  // rightwards circular shift submatrices of A
    int a_shift_distance = -row + k * (plane_length / c);
    MPI_Cart_shift(cart_comm, 1, a_shift_distance, &rank_source_a, &rank_dest_a);

    int rank_source_b, rank_dest_b;  // downwards circular shift submatrices of B
    int b_shift_distance = -col + k * (plane_length / c);
    MPI_Cart_shift(cart_comm, 0, b_shift_distance, &rank_source_b, &rank_dest_b);

    MPI_Request requests[4];

    MPI_Isend(local_A_buffer, elements_per_node, MPI_DOUBLE, rank_dest_a, tag_A, cart_comm, &requests[0]);
    MPI_Irecv(local_A, elements_per_node, MPI_DOUBLE, rank_source_a, tag_A, cart_comm, &requests[1]);

    MPI_Isend(local_B_buffer, elements_per_node, MPI_DOUBLE, rank_dest_b, tag_B, cart_comm, &requests[2]);
    MPI_Irecv(local_B, elements_per_node, MPI_DOUBLE, rank_source_b, tag_B, cart_comm, &requests[3]);

    double *local_C = (double*) malloc(elements_per_node * sizeof(double));
    std::fill_n(local_C, elements_per_node, 0.0);

    MPI_Status statuses[4];
    MPI_Waitall(4, requests, statuses);
    
    // get the rank of source and dest process in the grid
    MPI_Cart_shift(cart_comm, 1, 1, &rank_source_a, &rank_dest_a);
    MPI_Cart_shift(cart_comm, 0, 1, &rank_source_b, &rank_dest_b);

    // cannon mutiply and shift step
    int submatrix_length = n / plane_length;  // each submatrix is a submatrix_length * submatrix_length elements matrix
    for (int t = 1; t < plane_length / c; ++t) {
        // P[i][j] sends A[i][j] to local_A on P
        MPI_Request requests[4];
        MPI_Isend(local_A, elements_per_node, MPI_DOUBLE, rank_dest_a, tag_A, cart_comm, &requests[0]);
        MPI_Irecv(local_A_buffer, elements_per_node, MPI_DOUBLE, rank_source_a, tag_A, cart_comm, &requests[1]);
        
        MPI_Isend(local_B, elements_per_node, MPI_DOUBLE, rank_dest_b, tag_B, cart_comm, &requests[2]);
        MPI_Irecv(local_B_buffer, elements_per_node, MPI_DOUBLE, rank_source_b, tag_B, cart_comm, &requests[3]);

        matrix_multiply(local_A, local_B, local_C, submatrix_length);
        
        MPI_Waitall(4, requests, statuses);

        std::swap(local_A, local_A_buffer);
        std::swap(local_B, local_B_buffer);
    }
    matrix_multiply(local_A, local_B, local_C, submatrix_length);
    free(local_A);
    free(local_A_buffer);
    free(local_B);
    free(local_B_buffer);

    // gather the result on rank == 0
    double *gather_C;
    if (k == 0) {
        gather_C = (double *) malloc(elements_per_node * sizeof(double));
        std::fill_n(gather_C, elements_per_node, 0.0);
    }
    MPI_Barrier(z_comm);
    MPI_Reduce(local_C, gather_C, elements_per_node, MPI_DOUBLE, MPI_SUM, 0, z_comm);
    if (rank == 0) {
        printf("Time : %.3f\n", MPI_Wtime() - start_time);  // the algorithm ends here, collapse time
    }
    free(local_C);
    MPI_Barrier(z_comm);

    double *result_C = nullptr;
    if (rank == 0) {
        result_C = (double*) malloc(size * sizeof(double));
        std::fill_n(result_C, size, 0.0);
    }
    if (k == 0) {
        MPI_Gather(gather_C, elements_per_node, MPI_DOUBLE, result_C, elements_per_node, MPI_DOUBLE, 0, plane_comm);
    }
    // MPI_Barrier(plane_comm);  
    if (k == 0) {
        free(gather_C);
    }  

    if (rank == 0) {
        if (verify) {  // recover A B C
            printf("residual : %f\n", check(A, B, result_C, n, plane_length, submatrix_length));
        }   
        free(A);
        free(B);
        free(result_C);
    }
    MPI_Finalize();
    return 0;
}