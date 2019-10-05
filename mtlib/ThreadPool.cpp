#include <thread>
#include <functional>
#include <type_traits>
#include "ThreadPool.h"

namespace MtLib {

   ThreadPool *ThreadPool::single_instance_ = nullptr;

   void ThreadPool::Init(int max_num_threads) {
      if (ThreadPool::single_instance_ == nullptr) 
         ThreadPool::single_instance_ = new ThreadPool(max_num_threads);
   }

   ThreadPool *ThreadPool::Fetch() {
      if (ThreadPool::single_instance_ == nullptr)
         ThreadPool::single_instance_ = new ThreadPool();
      return ThreadPool::single_instance_;
   }

   ThreadPool::ThreadPool(int max_num_threads) {
      // use hardware thread if not specified
      if (max_num_threads <= 0)
         max_num_threads = std::thread::hardware_concurrency();
      // for safety purpose
      if (max_num_threads < 1) max_num_threads = 1;
      // create threads, std::thread is movable
      for (int i = 0; i < max_num_threads; i++) {
         auto thread_loop = std::bind(&ThreadPool::ThreadRunningLoop, this);
         thread_pool_.push_back(std::thread(thread_loop));
      }
   }

   ThreadPool::~ThreadPool() {
      // notify all running ThreadRunningLoop to return 
      exit_flag_ = true;
      new_task_notifier_.notify_all();
      // wait for all threads to join
      for (auto &t : thread_pool_)
         t.join();
   }

   void ThreadPool::ThreadRunningLoop() {
      while (true) {
         // get the next job in queue
         std::function<void()> task;
         {
            // lock the mutex that protects the queue
            std::unique_lock<std::mutex> lck(task_queue_mutex_);
            // if empty job queue, release the lock and wait for new_task signal
            // other thread may also come in and wait here for signal
            while (task_queue_.empty())
               new_task_notifier_.wait(lck);
            // to exit, set exit_flag_ and notify_all
            if (exit_flag_) return;
            // get the job
            task = std::move(task_queue_.front());
            task_queue_.pop();
         }
         // run the job outside the lock scope
         task();
         // decrement num_pending_tasks_
         {
            // lock the mutext that protect num_pending_tasks_
            std::unique_lock<std::mutex> lck(pending_count_mutex_);
            // decrement the counter
            --num_pending_tasks_;
            // if zero pending tasks, notify the master thread in case it is waiting for finish
            if (num_pending_tasks_ == 0)
               completion_notifier_.notify_all();
         }
      }
   }

   void ThreadPool::Wait() {
      while (true) {
         {
            // lock the mutext that protect num_pending_tasks_
            std::unique_lock<std::mutex> lck(pending_count_mutex_);
            // if zero pending tasks, meaning all tasks are finished
            // this also includes the tasks submitted by slave threads
            if (num_pending_tasks_ == 0)
               return;
            // if there is still running jobs, release the lock and wait for notification
            completion_notifier_.wait(lck);
         }
      }
   }
   
   void ThreadPool::RunRref(std::function<void()>&& f) {
      // insert job to the queue within the lock scope
      {
         std::lock_guard<std::mutex> lck(task_queue_mutex_);
         task_queue_.push(std::move(f));
      }
      // increment num_pending_tasks_
      {
         std::unique_lock<std::mutex> lck(pending_count_mutex_);
         ++num_pending_tasks_;
      }
      // notify one waiting thread outside the lock scope
      new_task_notifier_.notify_one();
   }
}

