#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <sstream>
#include "custom_collectives.h"

std::string format_chunk(const std::vector<int> &buf, int max_show = 8)
{
    std::ostringstream oss;
    oss << "[";

    int show = std::min(static_cast<int>(buf.size()), max_show);
    for (int i = 0; i < show; ++i)
    {
        if (i > 0)
        {
            oss << ", ";
        }
        oss << buf[i];
    }

    if (static_cast<int>(buf.size()) > max_show)
    {
        oss << ", ...";
    }

    oss << "] (count=" << buf.size() << ")";
    return oss.str();
}

void print_rank_ordered(const std::string &label, const std::vector<int> &buf, int size, MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    for (int r = 0; r < size; ++r)
    {
        MPI_Barrier(comm);
        if (rank == r)
        {
            std::cout << label << " rank " << rank << " -> " << format_chunk(buf) << std::endl;
        }
    }
    MPI_Barrier(comm);
}

void test_scatter(int n, int size, int root, MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    std::vector<int> sendbuf(n);
    std::vector<int> recvbuf(n / size);
    std::vector<int> expected(n / size);

    if (rank == root)
    {
        for (int i = 0; i < n; ++i)
        {
            sendbuf[i] = i;
        }

        std::cout << "Problem Size: " << n << std::endl;
        std::cout << "Number of Processors: " << size << std::endl << std::endl;
    }

    int repeats = 100;
    double total_custom_time = 0.0;
    double total_mpi_time = 0.0;
    for (int i = 0; i < repeats; ++i) {
        double start, end;
        double start2, end2;
        if (rank == root)
        {
            start = MPI_Wtime();
        }
        MPI_Scatter(sendbuf.data(), n / size, MPI_INT, expected.data(), n / size, MPI_INT, root, comm);
        if (rank == root)
        {
            end = MPI_Wtime();
            total_mpi_time += end - start;
            std::cout << "MPI_Scatter time: " << end - start << std::endl;
        }

        print_rank_ordered("MPI_Scatter result", expected, size, comm);
        if (rank == size - 1) 
        {
            std::cout << std::endl;
        }

        if (rank == root)
        {
            start2 = MPI_Wtime();
        }

        Custom_Scatter(sendbuf.data(), n / size, MPI_INT, recvbuf.data(), n / size, MPI_INT, root, comm);
        if (rank == root)
        {
            end2 = MPI_Wtime();
            total_custom_time += end2 - start2;
            std::cout << "Custom_Scatter time: " << end2 - start2 << std::endl;
        }

        print_rank_ordered("Custom_Scatter result", recvbuf, size, comm);
        if (rank == size - 1) 
        {
            std::cout << std::endl;
        }

        int correct = std::equal(recvbuf.begin(), recvbuf.end(), expected.begin());
        int result;
        MPI_Reduce(&correct, &result, 1, MPI_INT, MPI_SUM, root, comm);

        if (rank == root)
        {
            if (result == size)
            {
                std::cout << "Implementation is correct!" << std::endl;
            }
            else
            {
                std::cout << "Implementation failed" << std::endl;
            }
            std::cout << "Slow down: " << (end2 - start2) / (end - start) << "x" << std::endl; 
        }
    }

    if (rank == root) {
        std::cout << std::endl << std::endl << "Average MPI_Scatter time: " << total_mpi_time / repeats << std::endl;
        std::cout << "Average Custom_Scatter time: " << total_custom_time / repeats << std::endl;
        std::cout << "Average slow down: " << (total_custom_time / repeats) / (total_mpi_time / repeats) << "x" << std::endl << std::endl;
    }
}

void test_allgather(int n, int size, MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    std::vector<int> sendbuf(n / size);
    std::vector<int> recvbuf(n);
    std::vector<int> expected(n);

    for (int i = 0; i < n / size; ++i)
    {
        sendbuf[i] = rank * n / size + i;
    }

    if (rank == 0)
    {
        std::cout << "Problem Size: " << n << std::endl;
        std::cout << "Number of Processors: " << size << std::endl;
    }

    double start, end;
    if (rank == 0)
    {
        start = MPI_Wtime();
    }
    // print data on each rank before gather
    print_rank_ordered("Before MPI_Allgather, sendbuf", sendbuf, size, comm);

    MPI_Allgather(sendbuf.data(), n / size, MPI_INT, expected.data(), n / size, MPI_INT, comm);
    if (rank == 0)
    {
        end = MPI_Wtime();
        std::cout << "MPI_Allgather time: " << end - start << std::endl;
    }

    if (rank == 0)
    {
        start = MPI_Wtime();
    }
    Custom_Allgather(sendbuf.data(), n / size, MPI_INT, recvbuf.data(), n / size, MPI_INT, comm);
    if (rank == 0)
    {
        end = MPI_Wtime();
        std::cout << "Custom_Allgather time: " << end - start << std::endl;
    }
    print_rank_ordered("Custom_Allgather result", recvbuf, size, comm);

    int correct = std::equal(recvbuf.begin(), recvbuf.end(), expected.begin());
    int result;
    MPI_Reduce(&correct, &result, 1, MPI_INT, MPI_SUM, 0, comm);

    if (rank == 0)
    {
        if (result == size)
        {
            std::cout << "Implementation is correct!" << std::endl;
        }
        else
        {
            std::cout << "Implementation failed" << std::endl;
            printf("Expected: %d and Received: %d\n", size, result);
        }
    }
}

void test_allreduce(int n, int size, MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    std::vector<int> sendbuf(n);
    std::vector<int> recvbuf(n);
    std::vector<int> expected(n);

    for (int i = 0; i < n; ++i)
    {
        sendbuf[i] = rank * n + i;
    }

    if (rank == 0)
    {
        std::cout << "Problem Size: " << n << std::endl;
        std::cout << "Number of Processors: " << size << std::endl;
    }
    print_rank_ordered("Before MPI_Allreduce, sendbuf", sendbuf, size, comm);
    if (rank == 0) printf("\n");

    double start, end;
    if (rank == 0)
    {
        start = MPI_Wtime();
    }
    MPI_Allreduce(sendbuf.data(), expected.data(), n, MPI_INT, MPI_SUM, comm);
    if (rank == 0)
    {
        end = MPI_Wtime();
        std::cout << "MPI_Allreduce time: " << end - start << std::endl;
    }
    print_rank_ordered("After MPI_Allreduce, expected", expected, size, comm);
    if (rank == 0) printf("\n");

    if (rank == 0)
    {
        start = MPI_Wtime();
    }
    Custom_Allreduce(sendbuf.data(), recvbuf.data(), n, MPI_INT, MPI_SUM, comm);
    if (rank == 0)
    {
        end = MPI_Wtime();
        std::cout << "Custom_Allreduce time: " << end - start << std::endl;
    }
    print_rank_ordered("After Custom_Allreduce, recvbuf", recvbuf, size, comm);
    if (rank == 0) printf("\n");

    int correct = std::equal(recvbuf.begin(), recvbuf.end(), expected.begin());
    int result;
    MPI_Reduce(&correct, &result, 1, MPI_INT, MPI_SUM, 0, comm);

    if (rank == 0)
    {
        if (result == size)
        {
            std::cout << "Implementation is correct!" << std::endl;
        }
        else
        {
            std::cout << "Implementation failed" << std::endl;
        }
    }
}

void test_alltoall_arbitrary(int n, int size, MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    std::vector<int> sendbuf(n);
    std::vector<int> recvbuf_arbitrary(n);
    std::vector<int> expected(n);

    for (int i = 0; i < n; ++i)
    {
        sendbuf[i] = rank * n + i;
    }

    if (rank == 0)
    {
        std::cout << "Problem Size: " << n << std::endl;
        std::cout << "Number of Processors: " << size << std::endl;
    }
    print_rank_ordered("Before MPI_Alltoall, sendbuf", sendbuf, size, comm);

    double start, end;
    if (rank == 0)
    {
        start = MPI_Wtime();
    }
    MPI_Alltoall(sendbuf.data(), n / size, MPI_INT, expected.data(), n / size, MPI_INT, comm);
    if (rank == 0)
    {
        end = MPI_Wtime();
        std::cout << "MPI_Alltoall time: " << end - start << std::endl;
    }
    print_rank_ordered("MPI_Alltoall result", expected, size, comm);

    if (rank == 0)
    {
        start = MPI_Wtime();
    }
    Custom_Alltoall_Arbitrary(sendbuf.data(), n / size, MPI_INT, recvbuf_arbitrary.data(), n / size, MPI_INT, comm);
    if (rank == 0)
    {
        end = MPI_Wtime();
        std::cout << "Custom_Alltoall_Arbitrary time: " << end - start << std::endl;
    }
    print_rank_ordered("Custom_Alltoall_Arbitrary result", recvbuf_arbitrary, size, comm);

    int correct_arbitrary = std::equal(recvbuf_arbitrary.begin(), recvbuf_arbitrary.end(), expected.begin());
    int result_arbitrary;
    MPI_Reduce(&correct_arbitrary, &result_arbitrary, 1, MPI_INT, MPI_SUM, 0, comm);

    if (rank == 0)
    {
        if (result_arbitrary == size)
        {
            std::cout << "Implementation is correct!" << std::endl;
        }
        else
        {
            std::cout << "Implementation failed" << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int root = std::stoi(argv[3]);
    int prob_size = std::stoi(argv[2]);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(time(NULL) + rank);
    
    // test_scatter(prob_size, size, root, MPI_COMM_WORLD);
    // test_allgather(prob_size, size, MPI_COMM_WORLD);
    // test_allreduce(prob_size, size, MPI_COMM_WORLD);
    test_alltoall_arbitrary(prob_size, size, MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}