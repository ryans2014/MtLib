#pragma once
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include <type_traits>
#include <future>

namespace MtLib {
   // class Task;

   class ThreadPool {

   public:
      /* create the global thread pool */
      static void init(int max_num_threads = -1);

      /* get unique thread pool instance */
      static ThreadPool *fetch();
     
      /* run a task on the next available thread, non-blocking */
      //void run(Task &&tsk, int task_group = -1);

      // template<class F, class... Args>
		// auto run(F&& f, Args&&... args)
		//    ->std::future<typename std::result_of<F(Args...)>::type>;

      /* join a certain group of task, if task_group < 0 join all */
      // void join(int task_goup = -1);

      /* wait for all thread to join before destroy */
      ~ThreadPool();

   private:
      /* can only have 1 ThreadPool instance */
      static ThreadPool *single_instance;

      /* construct certain number of threads, if <= 0, use the hardware to determine */
      ThreadPool(int max_num_threads = -1);
      
      /* condition_variable that notify the idled thread to continue working */
      std::condition_variable tp_cv;

      /* fixed length vector of threads */
      std::vector<std::thread> thread_pool;

      /* task queue and the mutex that protect it */
      // std::queue<Task> task_queue;
      std::mutex tp_mutex;

      /* running loop for slave threads, running endlessly until exit_flag is set to true */
      void thread_running_loop();
      bool exit_flag = false;

      /* not copyable, not movable */
      ThreadPool(ThreadPool &in) = delete;
      ThreadPool &operator=(ThreadPool &in) = delete;
      ThreadPool &operator=(ThreadPool &&in) = delete;
   };

}
