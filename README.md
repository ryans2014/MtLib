# MtLib: a C++ Thread Pool Library
MtLib is a C++ library that aims to offer thread reuse as well as convenient API for using std::thread. The library is based on C++14 features. 

## Usage

## API Reference
```CPP
class MtLib::ThreadPool
```
This class manages the thread pool, the task queue, and provide various method to run client functions via pooled threads. Each process should have at most one TheadPool instances.
```CPP
static void ThreadPool::Init(int max_num_threads = -1);
```
Create the global TheadPool instance if it is not created yet. If max_num_threads is zero or negative, the hardware thead will be used as the maximum number of theads. If the global TheadPool is already created (by Init or Fetch method), the current TheadPool instance will be returned. 
```C++
static ThreadPool *ThreadPool::Fetch();
```
Create the global TheadPool instance if it is not created yet. The hardware thead will be used as the maximum number of theads. If the global TheadPool is already created (by Init or Fetch method), the current TheadPool instance will be returned. 
```C++
template<typename F, typename... Args>
void ThreadPool::Run(F&& f, Args&&... args);
```
Run a client side function by the pooled thread. This function is non-blocking and does not wait for the completion of the task. 
  "f" is the client side function/member function.
  "args" is argument list. If f is a non-static member function, the first argument should be the class instance pointer.
Function return and exception are neglected.
Note: Both function f and Argument list are passed either by value (or pointer value) or by r-reference. Passing a l-value/l-reference to the argument list will lead to a copy of the objet. Therefore, changes made to the object is no longer visible to the caller. The rule of thumb is: always use a pointer argument if you intend to pass by l-reference. See more details in the limitation section.
```C++
template<typename F, typename R, typename... Args>
void ThreadPool::RunAndReturn(F&& f, R&& r, Args&&... args);
```
This function is mostly the same as the ThreadPool::Run function. The only difference is that this function write the returned value to the designated location.
  "r" should be a pointer where returned value will be written to.
```C++
void ThreadPool::RunRref(std::function<void()>&& f);
```
This function is a simplified version of ThreadPool::Run. It envokes a no-argument, no-return function on a pooled thread. One special feature of this method is that function object f is passed with moving semantics. This feature is especially useful when you want to pass ownership of a object to a lambda function and let the destructor of the lambda function object take care of the object destruction from a different thread (see ThreadPool::Delete implementation for more details).
```C++
void ThreadPool::Wait();
```
Block until all pending tasks are cleared.
```C++
template<typename T>
void ThreadPool::Delete(T&& t) {
   static_assert(!std::is_const<std::remove_reference_t<T>>::value, ...);
   ...
}
```
Destroy a data structure by pooled thread. This template function takes a non-const reference to the object that needs to be deleted. Both R-reference and L-reference are OK, but the object need to be movable. Under the hood, the object will be moved to a lambda function object and the function object destructor will be added to the task queue, waiting to be processed.
This function can save master thread time by delegating the destructor task to slave threads. This is only the case is move constructor/assignment is properly defined. 
Do not pass a smart pointer object to this function. Use the next function instead.
```C++
template<typename T>
void ThreadPool::Delete(T* t);
```
Destroy a data structure by pooled thread. This template takes a pointer, call delete on the pooled thread.
To delete a object owned by a std::unique_ptr<T>, there are two ways:
  (1) call ThreadPool::Delete from T's destructor and let the unique pointer go out of scope as normal
  (2) release the ownership and call ThreadPool::Delete(T* t):
  ```C++
  TheadPool::Fetch()->Delete(smart_ptr->release());
  ```

## Limitations

## Technical Details
