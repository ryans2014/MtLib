#include <queue>
#include "FunctionBinding.h"

void ffun1(int i, int j) {
	printf("%d, %d -> %d \n", i, j, i + j);
}

double ffun2(double i, double j, double *ret) {
	printf("%f, %f -> %f \n", i, j, i + j);
	*ret = i + j;
	return i + j;
}

class CA {
	int base = 10;
public:
	void prt(int j) {
		printf("----------- %d \n", base + j);
	}
};

void test_queue() {
	using TaskQueue = std::queue<std::function<void()>>;
	TaskQueue tq;
	tq.push(std::move(MtLib::package_function_no_return(ffun1, 1, 2)));
	double r1 = 0.0, r2 = 0.0;
	tq.push(std::move(MtLib::package_function_with_return(ffun2, &r1, 1.1, 2.2, &r2)));

	auto f1 = std::move(tq.front());
	tq.pop();
	auto f2 = std::move(tq.front());
	tq.pop();
	f1();
	f2();
	printf("Final %f %f \n", r1, r2);
}
