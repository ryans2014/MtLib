#include <stdlib.h>
#include <iostream>
#include <type_traits>
#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include "ThreadPool.h"

std::mutex print_mutex;

class MovableType {

public:
   int *val = 0;

   void Prt(char *str) const {
      std::unique_lock<std::mutex> lck(print_mutex);
      if (val == 0)
         printf("%s: value Empty \n", str);
      else
         printf("%s: value %d \n", str, *val);
   }

   MovableType() noexcept {
      std::unique_lock<std::mutex> lck(print_mutex);
      printf("Construct Default \n");
   }

   MovableType(int val) noexcept {
      std::unique_lock<std::mutex> lck(print_mutex);
      this->val = new int;
      *(this->val) = val;
      std::cout << "Construct " << val << " by thread " << std::this_thread::get_id() << std::endl;
   }

   ~MovableType() noexcept {
      std::unique_lock<std::mutex> lck(print_mutex);
      if (val == 0)
         ; // std::cout << "Destroy Empty by thread " << std::this_thread::get_id() << std::endl;
      else
         std::cout << "Destroy " << *val << " by thread " << std::this_thread::get_id() << std::endl;
      if (val != 0) delete val;
   }

   MovableType(const MovableType &that) noexcept {
      std::unique_lock<std::mutex> lck(print_mutex);
      val = new int;
      *val = *(that.val);
      std::cout << "Copy Construct " << *val << " by thread " << std::this_thread::get_id() << std::endl;
   }

   MovableType& operator=(const MovableType &that) noexcept {
      std::unique_lock<std::mutex> lck(print_mutex);
      if (val != 0) delete val;
      val = new int;
      *val = *(that.val);
      std::cout << "Copy Assign " << *val << " by thread " << std::this_thread::get_id() << std::endl;
      return *this;
   }

   MovableType(MovableType &&that) noexcept {
      std::unique_lock<std::mutex> lck(print_mutex);
      val = that.val;
      that.val = 0;
      std::cout << "Move Construct " << *val << " by thread " << std::this_thread::get_id() << std::endl;
   }

   MovableType& operator=(MovableType &&that) noexcept {
      std::unique_lock<std::mutex> lck(print_mutex);
      if (val != 0) delete val;
      val = that.val;
      that.val = 0;
      std::cout << "Move Assign " << *val << " by thread " << std::this_thread::get_id() << std::endl;
      return *this;
   }
};

void TestDeleteFunction() {
   // get global thread pool instance
   MtLib::ThreadPool *tp = MtLib::ThreadPool::Fetch();

   printf("============== TestDeleteFunction ===============\n\n");
   {
      // test 1 - delete const object 
      const MovableType const_obj(1);
      // tp->Delete(const_obj); // compiler error
      tp->Delete(const_cast<MovableType&>(const_obj)); // corret
   
      //// test 2 - delete regular object
      MovableType regular_obj(2);
      tp->Delete(regular_obj);

      // test 3 - delete r-value
      tp->Delete(MovableType(3));

      // test 4 - delete dynamically created object
      const MovableType * const p1 = new MovableType(4);
      tp->Delete(p1);

      // test 5 - delete an object owned by a unique_ptr
      // do not use Delete function on shared_ptr and weak_ptr, it does not make sense
      std::unique_ptr<MovableType> smt_pt(new MovableType(5));
      tp->Delete(smt_pt.release());

      // see the result
      tp->Wait();
      const_obj.Prt("const_obj");
      regular_obj.Prt("regular_obj");
      // p1->Prt("p1"); // error, p1 is already deleted
   }
   printf("=================== Done ====================\n\n\n");

   // do not do any of the following
   // int arr[3]{ 1, 2, 3 };
   // tp->Delete(arr); // undefined behavior
   // MovableType new_obj(5);
   // tp->Delete(&new_obj); // error, new_obj will be deleted twice
}

int SimpleFunction(int milliseconds_to_wait) {
   std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds_to_wait));
   return milliseconds_to_wait;
}

void TestThreadPoolRun() {
   printf("============== TestThreadPoolRun ===============\n\n");
   constexpr int N = 50;

   // single thread
   int sum_t1 = 0;
   auto start_t1 = std::chrono::high_resolution_clock::now();
   for (int i = 0; i < N; i++)
      sum_t1 += SimpleFunction(i);   
   auto end_t1 = std::chrono::high_resolution_clock::now();
   printf("Thread x 1: sum = %d, time = %f \n\n", sum_t1, 
      std::chrono::duration<double, std::milli>(end_t1 - start_t1).count());
   
   // 4 threads
   MtLib::ThreadPool *tp = MtLib::ThreadPool::Fetch();
   int result[N];
   auto start_t4 = std::chrono::high_resolution_clock::now();
   for (int i = 0; i < N; i++)
      tp->RunAndReturn(SimpleFunction, result + i, i);
   tp->Wait();
   auto end_t4 = std::chrono::high_resolution_clock::now();
   int sum_t4 = 0;
   for (int res : result)
      sum_t4 += res;
   printf("Thread x 4: sum = %d, time = %f \n\n", sum_t4,
      std::chrono::duration<double, std::milli>(end_t4 - start_t4).count());

   printf("===================== Done ======================\n\n\n");
}


void RunAllTests() {
   // 4 max thread
   MtLib::ThreadPool::Init(4);
   TestDeleteFunction();
   TestThreadPoolRun();
   printf("Finished..\n");
   while (1) {}
}