/*
    -- MAGMA (version 1.0) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       November 2010

    @generated s

*/

// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cublas.h>

// includes, project
#include "magma.h"
#include "magma_lapack.h"
#include "testings.h"

/* ////////////////////////////////////////////////////////////////////////////
   -- Testing ssyev
*/
int main( int argc, char** argv) 
{
    TESTING_CUDA_INIT();

    float *h_A, *h_R, *h_work;
    float *w1, *w2;
    magma_int_t *iwork;
    float gpu_time, cpu_time;

    TimeStruct start, end;

    /* Matrix size */
    magma_int_t N=0, n2;
    magma_int_t size[8] = {1024,2048,3072,4032,5184,6016,7040,8064};

    magma_int_t i, info;
    magma_int_t ione     = 1;
    magma_int_t ISEED[4] = {0,0,0,1};

    char *uplo = (char*)MagmaLowerStr;
    char *jobz = (char*)MagmaVectorsStr;


    if (argc != 1){
        for(i = 1; i<argc; i++){
            if (strcmp("-N", argv[i])==0)
                N = atoi(argv[++i]);
        }
        if (N>0)
            printf("  testing_ssyev -N %d\n\n", N);
        else
            {
                printf("\nUsage: \n");
                printf("  testing_ssyev -N %d\n\n", N);
                exit(1);
            }
    }
    else {
        printf("\nUsage: \n");
        printf("  testing_ssyev -N %d\n\n", 1024);
        N = size[7];
    }

    n2  = N * N;

    /* Allocate host memory for the matrix */
    TESTING_MALLOC(   h_A, float, n2);
    TESTING_MALLOC(    w1, float         ,  N);
    TESTING_MALLOC(    w2, float         ,  N);
    TESTING_HOSTALLOC(h_R, float, n2);

    magma_int_t nb = 128;//magma_get_ssyev_nb(N);
    magma_int_t lwork = 1 + 6*N*nb + 2*N*N;
    magma_int_t liwork = 3 + 5*N;

    TESTING_HOSTALLOC(h_work, float,  lwork);
    TESTING_MALLOC(    iwork,     magma_int_t, liwork);
    
    printf("\n\n");
    printf("  N     CPU Time(s)    GPU Time(s)     ||R||_F / ||A||_F\n");
    printf("==========================================================\n");
    for(i=0; i<8; i++){
        if (argc==1){
            N = size[i];
            n2 = N*N;
        }

        /* Initialize the matrix */
        lapackf77_slarnv( &ione, ISEED, &n2, h_A );
        lapackf77_slacpy( MagmaUpperLowerStr, &N, &N, h_A, &N, h_R, &N );

	magma_ssyev(jobz[0], uplo[0],
		     N, h_R, N, w1,
		     h_work, lwork, 
		     iwork, liwork, 
		     &info);
	
	lapackf77_slacpy( MagmaUpperLowerStr, &N, &N, h_A, &N, h_R, &N );

        /* ====================================================================
           Performs operation using MAGMA
           =================================================================== */
        start = get_current_time();
	magma_ssyev(jobz[0], uplo[0],
                     N, h_R, N, w1,
                     h_work, lwork,
                     iwork, liwork,
                     &info);
        end = get_current_time();

        gpu_time = GetTimerValue(start,end)/1000.;

        /* =====================================================================
           Performs operation using LAPACK
           =================================================================== */
        start = get_current_time();
	lapackf77_ssyev(jobz, uplo,
			 &N, h_A, &N, w2,
			 h_work, &lwork,
			 iwork, &liwork,
			 &info);
        end = get_current_time();
        if (info < 0)
            printf("Argument %d of ssyev had an illegal value.\n", -info);

        cpu_time = GetTimerValue(start,end)/1000.;

        /* =====================================================================
           Check the result compared to LAPACK
           =================================================================== */
        float work[1], matnorm = 1., mone = MAGMA_S_NEG_ONE;
        magma_int_t one = 1;

        matnorm = lapackf77_slange("f", &N, &one, w1, &N, work);
        blasf77_saxpy(&N, &mone, w1, &one, w2, &one);

        printf("%5d     %6.2f         %6.2f         %e\n",
               N, cpu_time, gpu_time,
               lapackf77_slange("f", &N, &one, w2, &N, work) / matnorm);

        if (argc != 1)
            break;
    }
 
    /* Memory clean up */
    TESTING_FREE(       h_A);
    TESTING_FREE(        w1);
    TESTING_FREE(        w2);
    TESTING_FREE(     iwork);
    TESTING_HOSTFREE(h_work);
    TESTING_HOSTFREE(   h_R);

    /* Shutdown */
    TESTING_CUDA_FINALIZE();
}