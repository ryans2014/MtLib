#pragma once

#include <functional>
#include <utility>

namespace MtLib {

	/*	Wrapper of std::bind function, add return value to the binded list */

	/* Condition #1: Function f has no return
			F is task function, Args is argument list
			Does not support L-reference as argument of function f
			Returns a lambda function of type void() */
	template<typename F, typename... Args>
	inline auto package_function_no_return(F&& f, Args&&... args) {
		auto binded_task = std::bind(f, std::forward<Args>(args)...);
		return [&, binded_task]() mutable { binded_task(); };
	}

	/* Condition #2: Function f has return
			F is task function, Args is argument list
			R is a pointer of returned type, used to store returned value
			Does not support L-reference as argument of function f
			Returns a lambda function of type void() */
	template<typename F, typename R, typename... Args>
	inline auto package_function_with_return(F&& f, R&& r, Args&&... args) {
		auto binded_task = std::bind(f, std::forward<Args>(args)...);
		return [&, binded_task, r]() mutable { *r = binded_task(); };
	}

}