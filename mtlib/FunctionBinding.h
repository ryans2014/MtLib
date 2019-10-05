#pragma once
#include <functional>
#include <utility>

namespace MtLib {

   // Case #1 - bind function parameters and function return into a Function<void()> object
   //    F is task function
   //    Args is argument list, L-reference argument is not supoprted, use pointer instead
   //    R is a pointer to which returned value is written to 
   template<typename F, typename R, typename... Args>
   inline auto MtBindWithReturn(F&& f, R&& r, Args&&... args) {
      auto binded_task = std::bind(f, std::forward<Args>(args)...);
      return [&, binded_task, r]() mutable { *r = binded_task(); };
   }

   // Case #2 - simpler version of case #1
   //    Should be used if f has no return or if return value is intended to be neglected
   template<typename F, typename... Args>
   inline auto MtBindNoReturn(F&& f, Args&&... args) {
      auto binded_task = std::bind(f, std::forward<Args>(args)...);
      return [&, binded_task]() mutable { binded_task(); };
   }

}