# MtLib: a C++ Thread Pool Library
MtLib is a C++ library that aims to offer thread reuse as well as a convenient API multithreading programming. The key feature of this library includes:
* **Thread Reusing:** Reusing of thread avoids thread creation/deletion time. It can reduce the multithreading overhead. This is especially helpful if you are trying to multithread a short/fast function where the thread creation time can be comparable to the function execution time.
* **Convenient Thread API:** The typical way of using std::thread include create thread and bind it with function, create future/promise pair and pass to the function, store the thread object for joining at a later time, check the function return from the future object. Now, with MtLib API, you just call the Thread::RunAndReturn method, give the function, the argument list, and where to write the return. When you are ready to see the result, just call Thread::Wait method to wait for the completion of all submitted tasks. 
```C++
ThreadPool::Fetch()->RunAndReturn(function_to_run, return_pointer, function_argument, ...);
ThreadPool::Fetch()->Wait();
```
* **Garbage Collection by Slave Thread:** Sometimes, calling the destructor on the master thread is just a wait of time. No further operation depends on the outcome of the destructor. Why not letting the slave threads to do that? This MtLib provides a solution. By calling TheadPool::Delete method, you can pass ownership of an object (move semantics) to a lambda function that takes care of the object destruction. The lambda function is then added to the task queue waiting to be processed. 
```C++
ThreadPool::Fetch()->Delete(object_to_be_deleted);
```

## Requirement
This library depends on C++14 standard features. No external libraries are used.


## Usage
There are two groups of features provided by this library:
* Run a function with the thead pool
```C++
#include <iostream>
#include <chrono>
#include "ThreadPool.h"

int SimpleFunction(int milliseconds_to_wait) {
   std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds_to_wait));
   return milliseconds_to_wait;
}

void TestThreadPoolRun() {
   constexpr int N = 50;
   constexpr int NT = 4;
   
   // thead pool initialization
   MtLib::ThreadPool *tp = MtLib::ThreadPool::Init(NT);
   
   // create data to store result
   int result[N];
   
   // start submitting tasks
   auto start_t = std::chrono::high_resolution_clock::now();
   for (int i = 0; i < N; i++)
      tp->RunAndReturn(SimpleFunction, result + i, i);
      
   // wait for all tasks to finish
   tp->Wait();
   auto end_t = std::chrono::high_resolution_clock::now();
   
   // process result
   int sum_t = 0;
   for (int res : result)
      sum_t += res;
   printf("Thread x %d: sum time = %d, real time = %f \n", NT, sum_t4,
      std::chrono::duration<double, std::milli>(end_t - start_t).count());
}
```

* Delete an object via slave threads
```C++
#include <iostream>
#include "ThreadPool.h"
MtLib::ThreadPool *tp = MtLib::ThreadPool::Fetch();

// case 1 - delete const object 
const MovableType const_obj(1);
tp->Delete(const_obj); // compiler error, Delete does not take const & type
tp->Delete(const_cast<MovableType&>(const_obj)); // corret

// case 2 - delete regular object
MovableType regular_obj(2);
tp->Delete(regular_obj); // corret, regular_obj will be moved out

// test 3 - delete r-value
tp->Delete(MovableType(3)); // corret, but useless, obj will be moved out

// test 4 - delete dynamically created object
const MovableType * const p1 = new MovableType(4);
tp->Delete(p1); // p1 pointer will become a dangling pointer if you dont reset it to null
                // do not call delete p1 again.
 
// test 5 - delete an object owned by a unique_ptr
// do not use Delete function on shared_ptr and weak_ptr, it does not make sense
std::unique_ptr<MovableType> smt_pt(new MovableType(5));
tp->Delete(smt_pt.release()); // do not pass a smart poiter to Delete method 

// do not do any of the following

int arr[3]{ 1, 2, 3 };
tp->Delete(arr); // undefined behavior

MovableType new_obj(5);
tp->Delete(&new_obj); // error, new_obj will be deleted twice

NonMovableType non_obj(6);
tp->Delete(non_obj); // compiler error, object should be movable
```

## How does it work
There is a thread-safe task queue maintained by the ThreadPool instance. When Run/RunAndReturn/RunRref methods are called (either from master thread or from slave threads), the task will be wrapped to a lambda function and pushed to the task queue. Then, an idled slave thread will be notify to start processing the task (via conditional variable).

At the initialization of the thread pool, all the slave thread are created. Every thread is binded to an infinite loop. Inside the loop, the thread processes tasks from the task queue. If the task queue is empty, it will simply release the thread queue lock and wait for signals from a conditional variable.

There is also a thread-safe counter that counts the number of pending tasks. It is decremented after a task is finished. If the Wait method is called on the master thread, the master thread will wait for signals from a conditional variable. This conditional variable gets notified when the last task gets processed.

The TheadPool::Delete method is based on the TheadPool::RunRref method. When an object reference is passed to the Delete method, the ownership of the object will be passed to a lambda function via a capture clause. The function object is then pushed to the task queue by TheadPool::RunRref method. 

The TheadPool::Delete method can also take a pointer argument and push the delete pointer function to the task queue.

## API Reference
```CPP
class MtLib::ThreadPool
```
* This class manages the thread pool, the task queue, and provide various method to run client functions via pooled threads. Each process should have at most one TheadPool instances.


```CPP
static void ThreadPool::Init(int max_num_threads = -1);
```
* Create the global TheadPool instance if it is not created yet. If max_num_threads is zero or negative, the hardware thead will be used as the maximum number of theads. If the global TheadPool is already created (by Init or Fetch method), the current TheadPool instance will be returned. 


```C++
static ThreadPool *ThreadPool::Fetch();
```
* Create the global TheadPool instance if it is not created yet. The hardware thead will be used as the maximum number of theads. If the global TheadPool is already created (by Init or Fetch method), the current TheadPool instance will be returned. 


```C++
template<typename F, typename... Args>
void ThreadPool::Run(F&& f, Args&&... args);
```
* Run a client side function by the pooled thread. This function is non-blocking and does not wait for the completion of the task. 

* "f" is the client side function/member function.
  
* "args" is argument list. If f is a non-static member function, the first argument should be the class instance pointer.
  
* Function return and exception are neglected.

* Note: Both function f and Argument list are passed by value (or pointer value). Passing a reference to the argument list will lead to a copy of the object. Therefore, changes made to the object is no longer visible to the caller. The rule of thumb is: always use a pointer argument if you intend to pass by reference. See more details in the limitation section.


```C++
template<typename F, typename R, typename... Args>
void ThreadPool::RunAndReturn(F&& f, R&& r, Args&&... args);
```
* This function is mostly the same as the ThreadPool::Run function. The only difference is that this function write the returned value to the designated location.

* "r" should be a pointer where returned value will be written to.
  
  
```C++
void ThreadPool::RunRref(std::function<void()>&& f);
```
* This function is a simplified version of ThreadPool::Run. It envokes a no-argument, no-return function on a pooled thread. One special feature of this method is that function object f is passed with moving semantics. This feature is especially useful when you want to pass ownership of an object to a lambda function and let the destructor of the lambda function object take care of the object destruction from a different thread (see ThreadPool::Delete implementation for more details).


```C++
void ThreadPool::Wait();
```
* Block until all pending tasks are cleared. This includes both tasks submitted by master thread, and by tasks submitted by slave thread themselves.
* Calling the Wait method from a slave thread is undefined behavior.


```C++
template<typename T>
void ThreadPool::Delete(T&& t) {
   static_assert(!std::is_const<std::remove_reference_t<T>>::value, ...);
   ...
}
```
* Destroy a data structure by pooled thread. This template function takes a non-const reference to the object that needs to be deleted. Both R-reference and L-reference are OK, but the object needs to be movable. Under the hood, the object will be moved to a lambda function object and the function object destructor will be added to the task queue, waiting to be processed.

* This function can save master thread time by delegating the destructor task to slave threads. This is only the case if move constructor/assignment is properly defined. 

* Do not pass a smart pointer object to this function. Use the next function instead.


```C++
template<typename T>
void ThreadPool::Delete(T* t);
```
* Destroy a data structure by pooled thread. This template takes a pointer, call delete on the pooled thread. To delete an object owned by a std::unique_ptr<T>, there are two ways:
  
  * call ThreadPool::Delete from T's destructor and let the unique pointer go out of scope as normal
  
  * release the ownership and call ThreadPool::Delete(T* t):
  
  ```C++
  TheadPool::Fetch()->Delete(smart_ptr->release());
  ```

## Limitations

* TheadPool::RunAndReturn and TheadPool::Run method passes the function object and argument list object by copy. If a function takes references as input and you rely on the function to modify the referenced data, then this function should not be invoked by either TheadPool::RunAndReturn or TheadPool::Run. To fix that, you need to modify your function to passing by pointer value. This limitation is introduced because the lambda function capture clause is used in combination with a generic template parameter (see file FunctionBinding.h). There are articles that describes how to overcome this issue, but I did not have the time to understand and implement it yet. Here is one article https://vittorioromeo.info/index/blog/capturing_perfectly_forwarded_objects_in_lambdas.html.
* TheadPool::Delete method does not work on primitive arrays. Deleting arrays can be tricky because arrays do not have a well defined destructor. Improper deletion of an array may lead to memory leak or even a crash. If you want to delete an array via thread pool, write a lambda function and invoke it by TheadPool::RunRref.
* Exceptions raised by the client function is currently not catched/handled. This part is not well tested yet.
