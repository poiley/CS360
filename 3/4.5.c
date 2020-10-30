/*
 * Cpts 360 - Lab 3
 * Benjamin Poile
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#define N 8

double A[N][N + 1];
int thread[N][N];
int user_threads;

pthread_barrier_t barrier;

/*Print matrix*/
int print() {
   	printf("\n");
   	for(int i = 0; i < N; i++, putchar("\n"))
	   	for(int j = 0; j < N + 1; j++)
			printf("%6.2f, ",  A[i][j]);
}

/*Gaussian Elimination function*/
void *gauss_elim(void *arg) { 
	for(int i = 0, id = (int) arg; i < N - 1; i++) {
		for(int j = 0, pivot = i, t = 0.0; j < N / user_threads + 1; j++) {
			if (i == thread[id][j]) {
				printf("Part Pivot, Thread %d\t| Row %d: ", id, i);
				for (int k = i; k <= N; k++)
					if (fabs(A[k][i]) > t) {
						t = fabs(A[k][i]);
						pivot = k;
					}

				printf("Pivot Row: %d\t| Pivot: %6.2f\n", pivot, A[pivot][i]);
				if (pivot != i)
					for (int k = i; k < N + 1; k++) {
						t = A[i][k];
						A[i][k] = A[pivot][k];
						A[pivot][k] = t;
					}
			}
		}

		// pThread Wait for: Partial Pivoting 
		pthread_barrier_wait(&barrier);
		for(int j = i + 1; j < N; j++) {
			for(int h = 0; h < N / user_threads + 1; h++) {
				if (j == thread[id][h]) {
					printf("Thread %d, Row %d\n", id, j);
					double factor = A[j][i]/A[i][i];
					for (int k = i + 1; k <= N; k++)
						A[j][k] -= A[i][k] * factor;
					A[j][i] = 0.0;
				}
			}
		}

		// pThread Wait for: Row Reductions
		pthread_barrier_wait(&barrier);
		for(int h = 0; h < N / user_threads + 1; h++)
			if (i == thread[id][h]) {
				printf("\n");
				for(int i = 0; i < N; i++, putchar("\n"))
					for(int j = 0; j < N + 1; j++)
						printf("%6.2f, ",  A[i][j]);
			}
  	}
}

/*Main function*/
int main(int argc, char *argv[]) {
	printf("Instantiating matrix A[N][N+1] as [A|B]...\n"); 
	for (int i = 0; i < N; i++)
		for (int j = 0; j<N; j++)
			A[i][j] = 1.0;

	for (int i = 0; i < N; i++)
		A[i][N - i - 1] = 1.0 * N;

	for (int i = 0; i < N; i++)
		A[i][N] = (N * (N + 1)) / 2 + (N - i) * (N - 1);

  	print_matrix();

	do {
		printf("Desired thread count: ");
		scanf("%d", &user_threads);
	} while(user_threads > N);

	// Generate array, store the threads that contain their respective rows
  	for(int i = 0; i < N; i++)
	 	for(int j = 0, z = 0; z != 8 && j < user_threads; j++){
			thread[j][i] = z++; 
			printf("Row: %d\t\t| Val: %d\n", j, thread[j][i]);
	 	}

	pthread_t threads[user_threads];					// Create pthread_t array
	pthread_barrier_init(&barrier, NULL, user_threads); // Initialize barrier thread

	printf("Initializing %d threads...\n", user_threads); //Initialize threads
	for (int i = 0; i < user_threads; i++)
		if(pthread_create(&threads[i], NULL, gauss_elim, (void *) i) != 0)
			printf("ERROR:\tCreation failed, Thread #%d.\n", i);

	printf("Holding until all %d threads join...\n", user_threads); //Hold threads
	for (int i = 0; i < user_threads; i++)
		if(pthread_join(threads[i], NULL) != 0)
			printf("ERROR:\tJoin failed, Thread #%d\n", i);
		else
			printf("Killing Thread #%d\n", i);

  	printf("Substituting...\n"); // Substitute
	double sum;
  	for (int i = N - 1, sum = 0.0; i >= 0; i--) {
		for (int j = i + 1; j < N; j++)
			sum += A[i][j] * A[j][N];
		A[i][N] = (A[i][N] - sum)/A[i][i];
  	}

  	printf("Result:\n"); // Output result
  	for(int i = 0; i < N; i++)
		printf("%6.2f  ", A[i][N]);

  	printf("\n");
}

