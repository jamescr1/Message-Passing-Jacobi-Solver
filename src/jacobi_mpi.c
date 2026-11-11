#include <stdio.h>
#include <mpi.h>
#include <math.h>

#define TAG_HALO 0
#define LOCAL_N 6
#define GLOBAL_N 10
#define MAX_ITERATIONS 1000
#define TOLERANCE 0.00001
#define COMPARISON_TOLERANCE 0.00001

int jacobi_serial(double solution[GLOBAL_N])
{
    double values_a[GLOBAL_N] = {100.0};
    double values_b[GLOBAL_N] = {100.0};

    values_a[GLOBAL_N - 1] = 0.0;
    values_b[GLOBAL_N - 1] = 0.0;

    double *old_values = values_a;
    double *new_values = values_b;

    double max_change;

    int iterations = 0;

    for (int iteration = 0; iteration < MAX_ITERATIONS; iteration++)
    {
        max_change = 0;
        for (int i = 1; i < (GLOBAL_N - 1); i++)
        {
            new_values[i] = (old_values[i - 1] + old_values[i + 1]) / 2.0;
            double difference = fabs(new_values[i] - old_values[i]);
            if (difference > max_change)
            {
                max_change = difference;
            }
        }

        double *temp = old_values;
        old_values = new_values;
        new_values = temp;

        iterations++;

        if (max_change < TOLERANCE)
        {
            break;
        }
    }

    // populating output array
    for (int i = 0; i < GLOBAL_N; i++)
    {
        solution[i] = old_values[i];
    }
    return iterations;
}

int main(int argc, char **argv)
{
    int rank;
    int size;
    double global_max_change;
    // declaring variables related to timing
    double mpi_start;
    double mpi_end;
    double mpi_elapsed;
    double mpi_max_elapsed;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2)
    {
        MPI_Finalize();
        return 1;
    }

    // initialising both arrays to consist entirely of 0.0
    double values_a[LOCAL_N] = {0.0};
    double values_b[LOCAL_N] = {0.0};

    double *old_values = values_a;
    double *new_values = values_b;

    // setting correct boundary value for rank 0's arrays
    if (rank == 0)
    {
        values_a[0] = 100.0;
        values_b[0] = 100.0;
    }

    int iterations = 0;
    // starting timer for mpi loop
    MPI_Barrier(MPI_COMM_WORLD);
    mpi_start = MPI_Wtime();
    for (int iteration = 0; iteration < MAX_ITERATIONS; iteration++)
    {
        iterations++;

        if (rank == 0)
        {
            MPI_Sendrecv(
                &old_values[4], 1, MPI_DOUBLE, !rank, TAG_HALO,
                &old_values[5], 1, MPI_DOUBLE, !rank, TAG_HALO,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else if (rank == 1)
        {
            MPI_Sendrecv(
                &old_values[1], 1, MPI_DOUBLE, !rank, TAG_HALO,
                &old_values[0], 1, MPI_DOUBLE, !rank, TAG_HALO,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        double local_max_change = 0.0;
        for (int i = 1; i < 5; i++)
        {
            new_values[i] = (old_values[i - 1] + old_values[i + 1]) / 2.0;
            double difference = fabs(new_values[i] - old_values[i]);

            if (difference > local_max_change)
            {
                local_max_change = difference;
            }
        }

        MPI_Allreduce(
            &local_max_change,
            &global_max_change,
            1,
            MPI_DOUBLE,
            MPI_MAX,
            MPI_COMM_WORLD);

        double *temp = old_values;
        old_values = new_values;
        new_values = temp;

        if (global_max_change < TOLERANCE)
        {
            break;
        }
    }
    // ending timer and storing total time in mpi_elapsed
    MPI_Barrier(MPI_COMM_WORLD);
    mpi_end = MPI_Wtime();
    mpi_elapsed = mpi_end - mpi_start;

    // working out which process took the most time and storing the result
    MPI_Reduce(
        &mpi_elapsed,
        &mpi_max_elapsed,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD);

    // creating an array that consists only of each processes' interior points
    double local_solution[4];
    for (int i = 1, j = 0; i < 5; i++, j++)
    {
        local_solution[j] = old_values[i];
    }

    // gathering interior points from all processes into one array
    double gathered_solution[8];

    MPI_Gather(
        local_solution,
        4,
        MPI_DOUBLE,
        gathered_solution,
        4,
        MPI_DOUBLE,
        0,
        MPI_COMM_WORLD);

    double mpi_solution[10];

    if (rank == 0)
    {
        mpi_solution[0] = 100.0;
        mpi_solution[9] = 0.0;

        for (int i = 1, j = 0; i < 9; i++, j++)
        {
            mpi_solution[i] = gathered_solution[j];
        }

        double serial_solution[GLOBAL_N];

        // starting timer
        double serial_start = MPI_Wtime();
        int serial_iterations = jacobi_serial(serial_solution);

        // comparing the serial and mpi solution
        int comparison_passed = 1;

        for (int i = 0; i < GLOBAL_N; i++)
        {
            double difference = fabs(serial_solution[i] - mpi_solution[i]);

            // if difference is too great there is a mismatch, print the mismatch
            if (difference > COMPARISON_TOLERANCE)
            {
                comparison_passed = 0;
                printf("Mismatch at index %d: Serial = %f, MPI = %f\n",
                       i, serial_solution[i], mpi_solution[i]);
            }
        }
        // ending timer and storing total time taken
        double serial_end = MPI_Wtime();
        double serial_elapsed = serial_end - serial_start;

        printf("Serial iterations: %d\n", serial_iterations);
        printf("MPI iterations:    %d\n", iterations);

        // printing verification message to terminal
        if (comparison_passed && serial_iterations == iterations)
        {
            printf("Correctness verification PASSED\n");
        }
        else
        {
            printf("Correctness verification FAILED\n");
        }

        printf("Serial Time: %f\n", serial_elapsed);
        printf("MPI Time: %f\n", mpi_max_elapsed);

        for (int i = 0; i < 10; i++)
        {
            printf("%f | ", mpi_solution[i]);
        }
        printf("\n");
    }

    for (int i = 1; i < 5; i++)
    {
        printf("Rank %d, local index %d: %f\n",
               rank, i, old_values[i]);
    }
    printf("Rank %d completed %d iterations\n", rank, iterations);

    MPI_Finalize();

    return 0;
}