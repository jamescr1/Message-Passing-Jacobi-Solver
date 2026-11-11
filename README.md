# Distributed Jacobi Solver Using MPI

## Overview

This project is a distributed-memory parallel program and was created to build my experience in developing a message passing program. The program presented here is a simple implementation of a one-dimensional Jacobi solver in C, using OpenMPI. The workload is distributed across two different processes. A serial implementation was also developed in order to act as a comparison against the message passing program. Any intepretations made regarding the difference in execution times between the two programs should be treated with caution, since the size of the workload is not significant. The project enabled me to gain more experience with distributed-memory parallel programming principles and practices, such as blocking point-to-point communication, collective communication and halo-exchange.

## What is the Jacobi method?

It is an algorithm in which each interior grid point (a grid point that is not a boundary) is updated to become the average of the values of its neighbouring points from the previous iteration.

The algorithm is as follows:

$$
u_i^{(k+1)} = \frac{u_{i-1}^{(k)} + u_{i+1}^{(k)}}{2}
$$

Fixed boundary values were used in this project. The left boundary was fixed at 100, the right boundary was fixed at zero.

## Implementation

The grid in this project is made up of ten different elements, meaning there are eight 'interior elements' in total. These interior elements were divided equally between two different processes. Each process maintained a local array consisting of its own four interior elements, its boundary element, and a ghost cell/halo value (given to it by the other process).

The grid starts off thus;

```text
100 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0
```

Process 0 / Rank 0's local array:

```text
[100] [0] [0] [0] [0] [halo]
```

Process 1 / Rank 1's local array:

```text
[halo] [0] [0] [0] [0] [0]
```

## MPI Operations Used in This Project

- `MPI_Sendrecv`: Used to faciliate halo-exchange between process 0 and process 1.
- `MPI_Allreduce`: Used to calculate global maximum change of both processes
- `MPI_Gather`: Used to gather the interior elements from process 0 and process 1 into one combined array.
- `MPI_Reduce`: Used to determine the maximum execution time across the processes.
- `MPI_Barrier`: Used to synchronise the two processes in order to facilitate MPI timing measurement.

## Serial Implementation

A serial implementation of the Jacobi solver was also developed, in order to compare its final array and total number of iterations against the MPI Jacobi Solver. This helped to verify that the MPI Jacobi solver was operating correctly and returned the correct results.

## Performance

Due to the small size of the array, the difference in execution time between the serial Jacobi solver and MPI Jacobi solver provides little value for intepretation. The primary purpose of this project was to gain experience in message passing programming. Due to the overhead involved in MPI, the MPI Jacobi Solver actually has a greater execution time than the serial solver.

## How to run the program?

Please compile with 'mpicc', using the following command;

```bash
mpicc jacobi_mpi.c -o jacobi_mpi -lm
```

The program is specifically designed to be run with two processes, so please use the following command to execute the program;

```bash
mpirun -np 2 ./jacobi_mpi
```

## What I learnt from this project

- The project gave me experience in developing a message passing program and in using the OpenMPI library.
- I learnt about point-to-point communication using `MPI_Sendrecv`.
- I learnt about collective communication using `MPI_Allreduce`, `MPI_Gather` and `MPI_Reduce`.
- I learnt about halo-exchange
- I explored and used concepts such as MPI processes, ranks and communicators.
