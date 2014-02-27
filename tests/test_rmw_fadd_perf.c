/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <armci.h>

#ifdef USE_ARMCI_LONG
#  define INC_TYPE long
#  define ARMCI_OP ARMCI_FETCH_AND_ADD_LONG
#else
#  define INC_TYPE int
#  define ARMCI_OP ARMCI_FETCH_AND_ADD
#endif

int main(int argc, char* argv[])
{
    int provided;
#ifdef __bgp__
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    assert(provided==MPI_THREAD_MULTIPLE);
#else
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
#endif

    ARMCI_Init();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int count = ( argc > 1 ? atoi(argv[1]) : 1000000 );

    int * complete = (int *) malloc(sizeof(int) * count);
    for(i=0; i<count; i++) complete[i] = 0;

    void ** base_ptrs = malloc(sizeof(void*)*nproc);
    ARMCI_Malloc(base_ptrs, sizeof(INC_TYPE));

    ARMCI_Access_begin(base_ptrs[rank]);
    *(int*) base_ptrs[rank] = 0;
    ARMCI_Access_end(base_ptrs[rank]);

    ARMCI_Barrier();

    complete = (int *) malloc(sizeof(int) * count);

    if (rank == 0) {
        printf("ARMCI_Rmw Test - in usec \n");
        fflush(stdout);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank>0)
    {
        int target = 0;
        int nrecv = 0;
        INC_TYPE val = -1;
        t0 = MPI_Wtime();
        while(val < count) {
            ARMCI_Rmw(ARMCI_OP, &val, base_ptrs[target], 1, target);
            if (val < count) {
                complete[val] = rank;
                nrecv++;
            }
        }
        t1 = MPI_Wtime();
        tt = (t1-t0);
    }
    MPI_Allreduce(MPI_IN_PLACE, complete, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    dt = (double)tt/nrecv;
    printf("process %d received %d counters in %f seconds (%f per call)\n",
           rank, nrecv, tt, dt);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);

    if (0==rank) {
        printf("Checking for fairness...\n", rank);
        fflush(stdout);
        for(i=0; i<count; i++) {
            printf("counter value %d %s received (rank %d) \n", i,
                   0==complete[i] ? "was NOT" : "was",
                   0==complete[i] ? -911 : complete[i] );
        }
        fflush(stdout);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    ARMCI_Free(base_ptrs[rank]);
    free(base_ptrs);

    free(complete);

    ARMCI_Finalize();
    MPI_Finalize();

    return 0;
}