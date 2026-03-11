#include "custom_collectives.h"

#include <mpi.h>
#include <iostream>

void Custom_Scatter(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                    int* recvbuf, int recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int problem_size = sendcount * size;
    int d = 0;
    int temp = size - 1;
    while (temp > 0) {
        d++;
        temp >>= 1;
    }

    int *buf;
    if (rank == root) {
        buf = sendbuf;
    } else {
        buf = (int*) malloc(problem_size * sizeof(int));
    }

    int round = 1;
    int flip = 1 << (d - 1);
    int mask = flip - 1;
    for (int j = d - 1; j >= 0; --j) {
        if (((rank ^ root) & mask) == 0) {
            int half = problem_size >> round;
            if (((rank ^ root) & flip) == 0) {
                MPI_Send(buf + half, half, sendtype, rank ^ flip, 0, comm);
            } else {
                MPI_Recv(buf, half, recvtype, rank ^ flip, 0, comm, NULL);
            }
        }

        mask >>= 1;
        flip >>= 1;
        ++round;
    }

    memcpy(recvbuf, buf, recvcount * sizeof(int));
    if (rank != root) {
        free(buf);
    }

    ////////////////////////////////////////
}

void Custom_Scatter_DEBUG(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                    int* recvbuf, int recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int problem_size = sendcount * size;

    int d = 0;
    int temp = size - 1;
    while (temp > 0) {
        d++;
        temp >>= 1;
    }

    int flip = 1 << (d - 1);
    int mask = flip - 1;

    int round = 1;

    for (int j = d - 1; j >= 0; --j) {
        // debug
        MPI_Barrier(comm);
        if (rank == root) {
            std::cout << "Step " << (d-j) << std::endl;
        }
        MPI_Barrier(comm);
        // debug

        if (((rank ^ root) & mask) == 0) {
            int half = problem_size >> round;
            if (((rank ^ root) & flip) == 0) {

                // debug
                std::cout << "Rank " << rank << " sends to rank " << (rank ^ flip) << ": ";
                for (int i = 0; i < half; ++i) {
                    std::cout << sendbuf[half + i] << " ";
                }
                std::cout << std::endl;
                // debug

                MPI_Send(sendbuf + half, half, sendtype, rank ^ flip, 0, comm);
            } else {
                MPI_Recv(sendbuf, half, recvtype, rank ^ flip, 0, comm, NULL);
                // for (int i = 0; i < recvcount; ++i) {
                //     recvbuf[i] = sendbuf[i];
                // }
            }
        }

        for (int i = 0; i < recvcount; ++i) {
            recvbuf[i] = sendbuf[i];
        }

        mask >>= 1;
        flip >>= 1;

        // debug
        MPI_Barrier(comm);
        if (rank == root) {
            std::cout << "Step " << (d-j) << " complete" << std::endl << std::endl;
        }
        // debug

        ++round;
    }

    // debug
    MPI_Barrier(comm);
    // debug

    ////////////////////////////////////////
}

void Custom_Allgather(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                      int* recvbuf, int recvcount, MPI_Datatype recvtype,
                      MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////
    


    ////////////////////////////////////////
}

void Custom_Allreduce(int* sendbuf, int* recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////
    


    ////////////////////////////////////////
}

void Custom_Alltoall_Hypercube(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                               int* recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////
    


    ////////////////////////////////////////
}

void Custom_Alltoall_Arbitrary(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                               int* recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////
    


    ////////////////////////////////////////
}
