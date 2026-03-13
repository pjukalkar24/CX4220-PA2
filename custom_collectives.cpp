#include "custom_collectives.h"

#include <mpi.h>
#include <iostream>
#include <cstring>

// best implementation involves reordering data at the root
void Custom_Scatter(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                    int* recvbuf, int recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////

    // intiialize rank and size
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    // initialize problem size, d = log2(size)
    int problem_size = sendcount * size;
    int d = 0;
    int temp = size - 1;
    while (temp > 0) {
        d++;
        temp >>= 1;
    }

    // create a buffer to hold reordered data at the root
    int *buf = (int*) malloc(problem_size * sizeof(int));
    if (root != 0 && rank == root) {
        for (int v = 0; v < size; ++v) {
            // buf[v] = sendbuf[v ^ root]
            memcpy(buf + v * sendcount, sendbuf + (v ^ root) * sendcount, sendcount * sizeof(int));
        }
    } else if (root == 0 && rank == root) {
        // if root is 0, we do't need to reorder
        free(buf);
        buf = sendbuf;
    }

    // main algorithm
    int round = 1;
    int flip = 1 << (d - 1);
    int mask = flip - 1;
    for (int j = d - 1; j >= 0; --j) {
        if (((rank ^ root) & mask) == 0) {
            // send / recv half of the data
            int half = problem_size >> round;
            if (((rank ^ root) & flip) == 0) {
                MPI_Send(buf + half, half, sendtype, rank ^ flip, 0, comm);
            } else {
                MPI_Recv(buf, half, recvtype, rank ^ flip, 0, comm, MPI_STATUS_IGNORE);
            }
        }

        mask >>= 1;
        flip >>= 1;
        ++round;
    }

    // copy into recvbuf
    memcpy(recvbuf, buf, recvcount * sizeof(int));
    if (root == 0 && rank == root) {
        return;
    }
    free(buf);

    ////////////////////////////////////////
}

void Custom_Allgather(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                      int* recvbuf, int recvcount, MPI_Datatype recvtype,
                      MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////
    
    // copy sendbuf data in the correct position in recvbuf
    // communicate

    // init rank and size
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    // get d = log2(size)
    int d = 0;
    int temp = size - 1;
    while (temp > 0) {
        d++;
        temp >>= 1;
    }

    // copy local data into recvbuf at the correct position based on rank
    for (int i = 0; i < sendcount; ++i) {
        recvbuf[rank * sendcount + i] = sendbuf[i];
    }

    for (int j = 0; j < d; ++j) {
        // get the partner and current data size to send/recv
        int partner = rank ^ (1 << j);
        int data_size = sendcount * (1 << j);

        // send and receieve data with partner, copying into correct position of recvbuf
        int send_offset = (rank >> j) << j;
        int recv_offset = (partner >> j) << j;
        MPI_Sendrecv(
            recvbuf + (send_offset * sendcount), data_size, sendtype, partner, 0,
            recvbuf + (recv_offset * sendcount), data_size, recvtype, partner, 0,
            comm, MPI_STATUS_IGNORE
        );
    }

    ////////////////////////////////////////
}

void Custom_Allreduce(int* sendbuf, int* recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////

    // init rank and size
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    // get d = log2(size)
    int d = 0;
    int temp = size - 1;
    while (temp > 0) {
        d++;
        temp >>= 1;
    }

    // copy sendbuf into recvbuf
    memcpy(recvbuf, sendbuf, count * sizeof(int));

    int *tmpbuf = (int*) malloc(count * sizeof(int));
    for (int j = 0; j < d; ++j) {
        // get partner and send/recv local data
        int partner = rank ^ (1 << j);
        MPI_Sendrecv(
            recvbuf, count, datatype, partner, 0,
            tmpbuf, count, datatype, partner, 0, comm, MPI_STATUS_IGNORE
        );

        // update this rank's data with partner rank's data
        for (int i = 0; i < count; ++i) {
            recvbuf[i] += tmpbuf[i];
        }   
    }
    free(tmpbuf);

    ////////////////////////////////////////
}

void Custom_Alltoall_Hypercube(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                               int* recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////

    // init rank and size
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    // get d = log2(size)
    int d = 0;
    int temp = size - 1;
    while (temp > 0) {
        d++;
        temp >>= 1;
    }

    int half = (sendcount * size) / 2; // get halfway point of data
    int *buf = (int*) malloc(sendcount * size * sizeof(int));
    int *tmp = (int*) malloc(half * sizeof(int));

    for (int j = d - 1; j >= 0; --j) {
        // decide which half to send/recv
        // lower rank sends upper half, higher rank sends lower half

        // send/recv halves
        int partner = rank ^ (1 << j);
        int *send_ptr = (rank < partner) ? sendbuf + half : sendbuf;
        MPI_Sendrecv(
            send_ptr, half, sendtype, partner, 0,
            tmp, half, recvtype, partner, 0,
            comm, MPI_STATUS_IGNORE
        );

        // interleave halves by block
        int *first = (rank < partner) ? sendbuf : tmp;
        int *second = (rank < partner) ? tmp : sendbuf + half;
        for (int i = 0; i < size / 2; ++i) {
            /// copy block from first list into even position
            memcpy(buf + (2 * i) * sendcount, first + i * sendcount, sendcount * sizeof(int));
            // copy block from second list into odd position
            memcpy(buf + (2 * i + 1) * sendcount, second + i * sendcount, sendcount * sizeof(int));
        }

        // copy back into sendbuf
        memcpy(sendbuf, buf, sendcount * size * sizeof(int));
    }

    // copy all into
    memcpy(recvbuf, buf, sendcount * size * sizeof(int));
    free(buf);
    free(tmp);

    ////////////////////////////////////////
}

void Custom_Alltoall_Arbitrary(int* sendbuf, int sendcount, MPI_Datatype sendtype,
                               int* recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm) {
    // Write your code below
    ////////////////////////////////////////


    // init rank and size
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    // copy local data into correct position of recvbuf using rank
    memcpy(recvbuf + rank * recvcount, sendbuf + rank * sendcount, sendcount * sizeof(int));

    for (int j = 1; j < size; ++j) {
        // use a cyclic shift to determine partners, then send/recv correct blocks
        int send_to = (rank + j) % size;
        int recv_from = (rank - j + size) % size;
        MPI_Sendrecv(
            sendbuf + send_to * sendcount, sendcount, sendtype, send_to, 0,
            recvbuf + recv_from * recvcount, recvcount, recvtype, recv_from, 0,
            comm, MPI_STATUS_IGNORE
        );
    }

    ////////////////////////////////////////
}
