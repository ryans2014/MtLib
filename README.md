# MtLib: a C++ Thread Pool Library
MtLib is a C++ library that aims to offer thread reuse as well as a convenient API multithreading programming. The key feature of this library includes:
* Thread reusing. Reusing of thread avoids thread creation/deletion time. It can reduce the multithreading overhead. This is especially helpful if you are trying to multithread a short/fast function where the thread creation time can be comparable to the function execution time.
* Convenient thread API. The typical way of using std::thread include create thread and bind it with function, create future/promise pair and pass to the function, store the thread object for joining at a later time, check the function return from the future object. Now, with MtLib API, you just call the Thread::RunAndReturn method, give the function, the argument list, and where to write the return. When you are ready to see the result, just call Thread::Wait method to wait to wait for the completion of all sutmitted tasks. 
* Garbage collection via pooled thread. Sometimes, calling the destructor on the master thread is just a wait of time. No further operation depend on the outcome of the destructor. Why not letting the slave threads to do that? This MtLib provides a solution. By calling TheadPool::Delete method, you can pass ownership of a object (move semantics) to a lambda function that takes care of the object destruction. The lambda function is then added to the task queue waiting to be processed. 

## Requirement
This library depend on C++14 standard features. No external libraries are used.


## Usage
There are two groups of features provided by this library:
* Run a function via slave threads 
* Delete a object via slave threads

## How does it work
There is a thread-safe task queue maintained by the ThreadPool instance. When Run/RunAndReturn/RunRef method are called (either from master thread or from slave threads), the task will be wrapped to a lambda function can pushed to the task queue. Then, an idle slave thread will be notify to start processing the task (via conditional variable).

At the initialization of the thread pool, all the slave thread are created. Every thread is binded to a infinite loop. In side the loop, the thread processes tasks from the task queue. If the task queue is empty, it will simply release the thread queue lock and wait for signals from a conditional variable.

There is also a thread-safe counter that counts the number of pending tasks. It is decremented after a task is finished. If the Wait method is called on the master thread, the master thread will wait for signals from a conditional variable. This conditional variable get notified when the last task get processed.

The TheadPool::Delete method is based on the TheadPool::RunRef method. When a object reference is passed to the Delete method, the ownership of the object will be passed to a lambda function via a capture clause. The function object is then pushed to the task queue by TheadPool::RunRef method. 

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

* Note: Both function f and Argument list are passed either by value (or pointer value) or by r-reference. Passing a l-value/l-reference to the argument list will lead to a copy of the objet. Therefore, changes made to the object is no longer visible to the caller. The rule of thumb is: always use a pointer argument if you intend to pass by l-reference. See more details in the limitation section.


```C++
template<typename F, typename R, typename... Args>
void ThreadPool::RunAndReturn(F&& f, R&& r, Args&&... args);
```
* This function is mostly the same as the ThreadPool::Run function. The only difference is that this function write the returned value to the designated location.

* "r" should be a pointer where returned value will be written to.
  
  
```C++
void ThreadPool::RunRref(std::function<void()>&& f);
```
* This function is a simplified version of ThreadPool::Run. It envokes a no-argument, no-return function on a pooled thread. One special feature of this method is that function object f is passed with moving semantics. This feature is especially useful when you want to pass ownership of a object to a lambda function and let the destructor of the lambda function object take care of the object destruction from a different thread (see ThreadPool::Delete implementation for more details).


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
* Destroy a data structure by pooled thread. This template function takes a non-const reference to the object that needs to be deleted. Both R-reference and L-reference are OK, but the object need to be movable. Under the hood, the object will be moved to a lambda function object and the function object destructor will be added to the task queue, waiting to be processed.

* This function can save master thread time by delegating the destructor task to slave threads. This is only the case is move constructor/assignment is properly defined. 

* Do not pass a smart pointer object to this function. Use the next function instead.


```C++
template<typename T>
void ThreadPool::Delete(T* t);
```
* Destroy a data structure by pooled thread. This template takes a pointer, call delete on the pooled thread. To delete a object owned by a std::unique_ptr<T>, there are two ways:
  
  * call ThreadPool::Delete from T's destructor and let the unique pointer go out of scope as normal
  
  * release the ownership and call ThreadPool::Delete(T* t):
  
  ```C++
  TheadPool::Fetch()->Delete(smart_ptr->release());
  ```

## Limitations

