
#include <functional>

/*
template<typename F, typename... Args>
void run(F&& f, Args&&... args) {
	// f is the task function
	// r is a pointer to store return value
	// args is the argument list

	f(std::forward<Args>(args)...);

	// bind the function parameters 
	// auto binded_task = std::bind(f, std::forward<Args>(args)...);

	// wrap the binded function by packaged_task to retrieve function return/exception
	// using FReturnType = typename std::result_of<F(Args...)>::type;
	// using TaskPackage = std::packaged_task<FReturnType()>;
	// using TaskPackagePtr = std::unique_ptr<TaskPackage>;
	// TaskPackagePtr task_package_ptr = std::make_unique<TaskPackage>(binded_task);
	// std::queue<TaskPackagePtr> task_queue;
	// 
	// (*task_package_ptr)();
}
*/

// Know limitation: cannot pass value by r-reference, pass by pointer instead

template<typename F, typename R, typename... Args>
auto package_function_with_return(F&& f, R&& r, Args&&... args) {
	// f is the task function
	// r is a pointer to store return value
	// args is the argument list

	auto binded_task = std::bind(f, std::forward<Args>(args)...);
	return [&, binded_task, r]() mutable { *r = binded_task(); };
}

template<typename F, typename... Args>
auto package_function_no_return(F&& f, Args&&... args) {
	// f is the task function
	// args is the argument list
	auto binded_task = std::bind(f, std::forward<Args>(args)...);
	return [&, binded_task]() mutable { binded_task(); };
}

void fun1(int i, int j) {
	printf("%d, %d -> %d \n", i, j, i + j);
}

double fun2(double i, double j, double *ret) {
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

void test_runner() {

	auto f1 = package_function_no_return(fun1, 1, 2);
	f1();

	double ret = 0.0, ret2 = 0.0;
	auto f2 = package_function_with_return(fun2, &ret, 1.2, 2.2, &ret2);
	f2();
	printf("Final %f %f \n", ret, ret2);

	CA c;
	auto f3 = package_function_no_return(&CA::prt, c, 3);
	f3();
}

