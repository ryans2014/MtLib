#include <stdlib.h>
#include <iostream>
#include <thread>
#include <vector>
#include <omp.h>

void range_work(int in1, int in2, int *out) {
	for (int idx = in1; idx < in2; idx++) {
		int in = idx;
		for (int i = 0; i < 10000; i++) {
			in = (in * 10000) % (in % 27 + 1);
		}
		*(out + idx) = in;
	}
}

void test_10_thread(int N) {
	using namespace std;
	int *result = new int[N];
	// parallel run
	vector<thread> thread_vector;
	double t1 = omp_get_wtime();
	for (int i = 0; i < 10; i++) {
		int i1 = N / 10 * i;
		int i2 = N / 10 * (i + 1);
		thread_vector.push_back(thread(range_work, i1, i2, result));
	}
	for (int i = 0; i < 10; i++) {
		thread_vector[i].join();
	}
	double t2 = omp_get_wtime();
	// plot result
	int sum = 0;
	for (int i = 0; i < N; i++) {
		sum += result[i];
	}
	printf("Pool thread:: Result is %d. Time cost is %f seconds \n", sum, t2 - t1);
}

void unit_work(int in, int *out) {
	for (int i = 0; i < 10000; i++) {
		in = (in * 10000) % (in % 27 + 1);
	}
	*out = in;
}

void test_basic_thread(int N) {
	using namespace std;
	int *result = new int[N];
	// parallel run
	vector<thread> thread_vector;
	double t1 = omp_get_wtime();
	for (int i = 0; i < N; i++) {
		thread_vector.push_back(thread(unit_work, i, result + i));
	}
	for (int i = 0; i < N; i++) {
		thread_vector[i].join();
	}
	double t2 = omp_get_wtime();
	// plot result
	int sum = 0;
	for (int i = 0; i < N; i++) {
		sum += result[i];
	}
	printf("Basic thread:: Result is %d. Time cost is %f seconds \n", sum, t2 - t1);
}

void test_serial(int N) {
	using namespace std;
	int *result = new int[N];
	// parallel run
	double t1 = omp_get_wtime();
	for (int i = 0; i < N; i++) {
		unit_work(i, result + i);
	}
	double t2 = omp_get_wtime();
	// plot result
	int sum = 0;
	for (int i = 0; i < N; i++) {
		sum += result[i];
	}
	printf("Serial:: Result is %d. Time cost is %f seconds \n", sum, t2 - t1);
}

void test_thread() {
	int N = 5000;
	test_basic_thread(N);
	test_serial(N);
	test_10_thread(N);
}