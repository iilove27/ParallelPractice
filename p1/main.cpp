#include "mpi.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>

using namespace std;

void serialOddEvenSort(int arr[], int);

int main(int argc, char *argv[])
{
    int myid, numproc;
    int globalsize = atoi(argv[1]); /* Input this argument in Terminal */
    int globaldat[globalsize]; /* This is globaldat, just an illustration. */
    srand(time(NULL)); /* Set random seed */
    for (int i = 0; i < globalsize; i++) globaldat[i] = rand() % 100+ 1; /* Random initialization */
    int result[globalsize];
    int localsize = 0;
    int addinsize = 0; /* Define how many element(s) will be added into globaldata */
    
    /* This is trying to find the biggest number in globaldata, because we don't know whether we are dealing with
     non-negative integers or all integers */
    vector<int> v(globaldat, globaldat + globalsize);
    vector<int>:: iterator big;
    big = max_element(v.begin(), v.end());
    int bignumber = *big;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numproc);
    
    /* Whatever globalsize is, I will add element(s) to globaldata (padding) so that
     localsize is EVEN, because in that case it is much easier to communicate
     between processes. */
    /* Trying to determine localsize after adding in supplementary vector (with size "addinsize") */
    if (globalsize % numproc == 0) {
        /* If numproc == globalsize, remainder is 0 but localsize is odd (1), so we need to add something. */
        if (globalsize / numproc == 1) {
            localsize = 2;
            addinsize = numproc;
        }
        else {
            localsize = globalsize / numproc + 1;
            addinsize = numproc;
        }
    } /* If remainder is 0, then addinsize = 0 and localsize is the quotient. */
    else {
        int shang, remainder;
        shang = globalsize / numproc;
        remainder = globalsize % numproc;
        if (shang % 2 == 0) addinsize = 2 * numproc - remainder; /* This is to make sure localsize is even. */
        else addinsize = numproc - remainder;
        localsize = (globalsize + addinsize) / numproc;
    }

    /* Get the appendix vector */
    vector<int> addinvec(addinsize);
    for (int i = 0; i < addinsize; i++) addinvec[i] = bignumber;
    
    /* Combine two vectors */
    v.insert(v.end(), addinvec.begin(), addinvec.end());
    int* globaldata = &v[0];
    int localdata[localsize];

    /* If there is only 1 process, just do serial odd-even sort */
    if (numproc == 1) {
        double t1, t2;
        t1 = MPI_Wtime();
        serialOddEvenSort(globaldata, globalsize);
        for (int i = 0; i < globalsize; i++) cout << globaldata[i] << endl;
        cout << "DONE with 1 process (Serial case)" << endl;
        t2 = MPI_Wtime();
        printf( "Elapsed time is %f\n", t2 - t1);
        MPI_Finalize();
    }
    
    
    else { /* If there is more than one processes */
        double t1, t2;
        t1 = MPI_Wtime();
        /* Chop and scatter (give out) the globaldata to processes */
        MPI_Scatter(globaldata, localsize, MPI_INT, localdata, localsize, MPI_INT, 0, MPI_COMM_WORLD);
        for (int i = 0; i < globalsize/2; i++) {
            /* This is odd phase: number with index 0 and number with index 1 compare,
                                  number with index 2 and number with index 3 compare, etc.
             Odd phase is within process, so no MPI communication is needed. */
            for (int j = 0; j < localsize; j = j+2){
                if (localdata[j] > localdata[j+1]) swap(localdata[j], localdata[j+1]);
            }
            
            /* This is even phase: number with index 1 and number with index 2 compare,
                                   number with index 3 and number with index 4 compare, etc.
             Even phase does require communication between processes. */
            if (myid < numproc - 1) {
                int temp = localdata[localsize-1]; /* Create a buffer to load the data being transmitted. */
                MPI_Send(&temp, 1, MPI_INT, (myid+1), 0, MPI_COMM_WORLD);
            }
            
            /* Every process (including the last process but excluding the first process) needs to receive data
             being delivered from the former processes. */
            if (myid > 0) {
                int recTemp; /* Create a buffer to store received data. */
                MPI_Recv(&recTemp, 1, MPI_INT, (myid-1), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                /* If the data received is larger than the first element in local data, then we swap them. */
                if (recTemp > localdata[0]) {
                    int sendTemp = localdata[0];
                    localdata[0] = recTemp;
                    MPI_Send(&sendTemp, 1, MPI_INT, (myid-1), 0, MPI_COMM_WORLD);
                }
                /* If not, then send it back. */
                else {
                    int sendTemp = recTemp;
                    MPI_Send(&sendTemp, 1, MPI_INT, (myid-1), 0, MPI_COMM_WORLD);
                }
                /* Either way there will be some data being sent back. */
            }
            
            /* Receive the data being sent back. */
            if (myid < numproc - 1) {
                int recTemp;
                MPI_Recv(&recTemp, 1, MPI_INT, (myid+1), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                localdata[localsize-1] = recTemp;
            }
            
            for (int j = 1; j < localsize-1; j = j+2) {
                if (localdata[j] > localdata[j+1])
                    swap(localdata[j], localdata[j+1]);
            }
        }
        if (myid == 0) {
            printf("Name: Yiwei Zhang\n");
            printf("ID: 115010287\n");
            printf("Sequence ");
            for (int i = 0; i < globalsize; i++) {
                printf("%i ", globaldat[i]);
            }
            printf("\n");
            cout << "Rank " << myid << " outputs ";
            for (int i = 0; i < localsize; i++) cout << localdata[i] << " ";
            cout << endl;
            MPI_Barrier(MPI_COMM_WORLD);

        }
        else {
            for (int i = 1; i < numproc; i++) {
                if (myid == i) {
                    cout << "Rank " << myid << " outputs ";
                    for (int j = 0; j < localsize; j++) cout << localdata[j] << " ";
                    cout << endl;
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);

        }
        
        int rec[globalsize]; /* Create a buffer to get the result. */
        /* Use MPI_Gather to gather all data to root process. */
        MPI_Gather(&localdata, localsize, MPI_INT, rec, localsize, MPI_INT, 0, MPI_COMM_WORLD);
        for (int i = 0; i < globalsize; i++) result[i] = rec[i];
        
        /* Let root process print the result. */
        if (myid == 0) {
            cout << "\nNote that the last few processors may contain additional numbers added before, which will be discarded and will not be in the final result.\n" << endl;
            cout << "Final result ";
            for (int i = 0; i < globalsize; i++) cout << result[i] << " ";
            cout << endl;
            cout << "Done with " << numproc << " processes" << endl;
            t2 = MPI_Wtime();
            printf( "Elapsed time is %f seconds\n", t2 - t1);
        }
        MPI_Finalize();
    }
    return 0;
}

/* Serial odd-even sort. */
void serialOddEvenSort(int arr[], int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int i = 1; i <= n-2; i = i+2) {
            if (arr[i] > arr[i + 1]) swap(arr[i], arr[i+1]);
        }
        for (int i=0; i<=n-2; i=i+2) {
            if (arr[i] > arr[i+1]) swap(arr[i], arr[i+1]);
        }
    }
    return;
}
