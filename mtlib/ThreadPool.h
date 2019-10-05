#pragma once
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include "FunctionBinding.h"

namespace MtLib {

   class ThreadPool {

   public:
      // create the global thread pool
      static void Init(int max_num_threads = -1);

      // get unique thread pool instance
      static ThreadPool *Fetch();
     
      // run a function by pooled thread, non-blocking
      //    f is function or class member function
      //    args is argument list
      //    if f is non-static member function, first args should be class instance pointer
      template<typename F, typename... Args>
      void Run(F&& f, Args&&... args);

      // run a function by pooled thread, non-blocking, with write back of returned value
      //    f is function or class member function
      //    r is a pointer where returned value will be written to
      //    args is argument list
      //    if f is non-static member function, first args should be class instance pointer
      template<typename F, typename R, typename... Args>
      void RunAndReturn(F&& f, R&& r, Args&&... args);

      // run a function by pooled thread, non-blocking, with function object passed by r-reference
      //    this allows move semantics of objects inside the function object
      //    this function is especially useful when you want to pass ownership of a object to a 
      //       lambda function and let the destructor of the function object to take care of the 
      //       object destruction at a different thread (see ThreadPool::Delete for more detail)
      void RunRref(std::function<void()>&& f);

      // block until all current tasks are finished
      // this function also wait for tasks submitted by slave threads
      void Wait();
      
      // destroy a data structure by pooled thread
      //    this template takes a non-const reference to the object that needs to be deleted
      //    both r-reference and l-reference are ok, but the object will be moved to a lambda function object
      //    after calling this function, master thread is good to go with an "empty" object
      //    the deletion of object in lambda function will be done by slave threads
      template<typename T>
      void Delete(T&& t);

      // destroy a data structure by pooled thread
      //    this template takes a pointer, call delete on the pooled thread
      template<typename T>
      void Delete(T* t);
      
      // wait for all thread to join before destroy
      ~ThreadPool();

   private:
      // can only have 1 ThreadPool instance
      static ThreadPool *single_instance_;

      // construct certain number of threads, if <= 0, use the hardware to determine
      ThreadPool(int max_num_threads = -1);
      
      // condition_variable that notifies the master thread the completion of all jobs
      std::condition_variable joinable_cv_;
      std::mutex joinable_mutex_;

      // fixed length vector of threads
      std::vector<std::thread> thread_pool_;

      // task queue
      using TaskQueue = std::queue<std::function<void()>>;
      TaskQueue task_queue_;

      // mutex to protect the task queue
      std::mutex task_queue_mutex_;

      // condition_variable that notify the idled thread to continue working
      std::condition_variable new_task_notifier_;

      // number of pending tasks (tasks in the queue + tasks not yet finished)
      int num_pending_tasks_ = 0;
      
      // mutex to protect num_pending_tasks
      std::mutex pending_count_mutex_;

      // condition_variable that notify the master thread that all jobs are done
      std::condition_variable completion_notifier_;

      // running loop for slave threads, running endlessly until exit_flag_ is set to true
      void ThreadRunningLoop();
      bool exit_flag_ = false;

      // not copyable, not movable
      ThreadPool(ThreadPool &in) = delete;
      ThreadPool &operator=(ThreadPool &in) = delete;
      ThreadPool &operator=(ThreadPool &&in) = delete;
   };

   template<typename F, typename... Args>
   void ThreadPool::Run(F&& f, Args&&... args) {
      // insert job to the queue within the lock scope
      {
         std::lock_guard<std::mutex> lck(task_queue_mutex_);
         task_queue_.push(std::move(MtBindNoReturn(std::move(f), args...)));
      }
      // increment num_pending_tasks_
      {
         std::unique_lock<std::mutex> lck(pending_count_mutex_);
         ++num_pending_tasks_;
      }
      // notify one waiting thread outside the lock scope
      new_task_notifier_.notify_one();
   }

   template<typename F, typename R, typename... Args>
   void ThreadPool::RunAndReturn(F&& f, R&& r, Args&&... args) {
      // insert job to the queue within the lock scope
      {
         std::lock_guard<std::mutex> lck(task_queue_mutex_);
         task_queue_.push(std::move(MtBindWithReturn(std::move(f), r, args...)));
      }
      // increment num_pending_tasks_
      {
         std::unique_lock<std::mutex> lck(pending_count_mutex_);
         ++num_pending_tasks_;
      }
      // notify one waiting thread outside the lock scope
      new_task_notifier_.notify_one();
   }

   template<typename T>
   void ThreadPool::Delete(T&& t) {
      // only take reference to non-constant type object
      static_assert(!std::is_const<std::remove_reference_t<T>>::value,
         "ThreadPool::Delete(T&& t) does not take \"const T&\" type!, use const_cast if it approriate.");
      // pass ownership of t to lambda function object
      auto del_function = [temp_t = std::move(t)]() {};
      // need to move the function into the thread object to avoid copy of temp_t object
      RunRref(std::move(del_function));
   }

   template<typename T>
   void ThreadPool::Delete(T* t) {
      // pointer case is easy, just call delete in lambda, no need to do std::move of object
      auto del_function = [t]() { delete t; };
      RunRref(std::move(del_function));
   }

}
