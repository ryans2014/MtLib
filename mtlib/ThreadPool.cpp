#include "ThreadPool.h"
#include <thread>
#include <functional>

namespace MtLib {

   ThreadPool *ThreadPool::single_instance = nullptr;

   void ThreadPool::init(int max_num_threads) {
      if (ThreadPool::single_instance == nullptr) 
         ThreadPool::single_instance = new ThreadPool(max_num_threads);
   }

   ThreadPool *ThreadPool::fetch() {
      if (ThreadPool::single_instance == nullptr)
         ThreadPool::single_instance = new ThreadPool();
      return ThreadPool::single_instance;
   }

   ThreadPool::ThreadPool(int max_num_threads) {
      // use hardware thread if not specified
      if (max_num_threads <= 0)
         max_num_threads = std::thread::hardware_concurrency();
      // for safety purpose
      if (max_num_threads < 1) max_num_threads = 1;
      // create threads, std::thread is movable
      for (int i = 0; i < max_num_threads; i++) {
         auto thread_loop = std::bind(&ThreadPool::thread_running_loop, this);
         thread_pool.push_back(std::thread(thread_loop));
      }
   }

   ThreadPool::~ThreadPool() {
      // notify all running thread_running_loop to return 
      exit_flag = true;
      tp_cv.notify_all();
      // wait for all threads to join
      for (auto &t : thread_pool)
         t.join();
   }

   /*void ThreadPool::run(Task &&tsk) {
      // insert job to the queue within the lock scope
      {
         std::lock_guard<std::mutex> lck(tp_mutex);
         task_queue.push(std::move(tsk));
      }
      // notify one waiting thread outside the lock scope
      tp_cv.notify_one();
   }
	
	template<class F, class... Args>
	auto ThreadPool::run(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>
	{
		using return_type = typename std::result_of<F(Args...)>::type;

		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		std::future<return_type> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(queue_mutex);

			// don't allow enqueueing after stopping the pool
			if (stop)
				throw std::runtime_error("enqueue on stopped ThreadPool");

			tasks.emplace([task]() { (*task)(); });
		}
		condition.notify_one();
		return res;
	}
	*/


   void ThreadPool::thread_running_loop() {
     /* Task task;
      while (true) {
         // get the next job in queue
         {
            // lock the mutex that protects the queue
            std::unique_lock<std::mutex> lck(tp_mutex);
            // if empty job queue, release the lock and wait for signal
            // then, other thread may also come in and wait here for signal
            while (task_queue.empty())
               tp_cv.wait(lck);
            // to exit, set exit_flag and notify_all
            if (exit_flag) return;
            // get the job
            task = std::move(task_queue.front());
            task_queue.pop();
         }
         // run the job outside the lock scope
         task.run();
      }*/
   }
}

