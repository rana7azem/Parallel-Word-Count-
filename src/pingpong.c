#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0)
            printf("Need at least 2 processes for ping-pong test.\n");
        MPI_Finalize();
        return 0;
    }

    int partner = (rank == 0) ? 1 : 0;

    int msg_sizes[] = {1, 16, 64, 256, 1024, 4096, 16384, 65536, 262144, 1048576};
    int n_sizes = sizeof(msg_sizes) / sizeof(msg_sizes[0]);

    int repetitions = 1000; 

    for (int i = 0; i < n_sizes; i++) {
        int msg_size = msg_sizes[i];
        char* buffer = (char*)malloc(msg_size);

        MPI_Barrier(MPI_COMM_WORLD); 

        double start, end;
        if (rank == 0) {
            start = MPI_Wtime();
            for (int r = 0; r < repetitions; r++) {
                MPI_Send(buffer, msg_size, MPI_BYTE, partner, 0, MPI_COMM_WORLD);
                MPI_Recv(buffer, msg_size, MPI_BYTE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            end = MPI_Wtime();

            double total_time = end - start;
            double avg_roundtrip = total_time / repetitions;
            double latency = avg_roundtrip / 2.0; 

            double bandwidth = (double)msg_size / latency; 

            printf("%d,%e,%e\n", msg_size, latency, bandwidth);
        } else {
            for (int r = 0; r < repetitions; r++) {
                MPI_Recv(buffer, msg_size, MPI_BYTE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buffer, msg_size, MPI_BYTE, partner, 0, MPI_COMM_WORLD);
            }
        }

        free(buffer);
    }

    MPI_Finalize();
    return 0;
}
