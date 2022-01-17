#ifndef _MTAP_FUNCTION_VIEW_HPP_
#define _MTAP_FUNCTION_VIEW_HPP_

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace mtap {
  template <class T>
  struct function_view;

  template <class R, class... Args>
  struct function_view<R(Args...)> {
  private:
    template <std::invocable<Args...> F>
    requires std::is_same_v<std::invoke_result_t<F, Args...>, R>
    static R call_fn(const void* ptr, Args&&... args) {
      return std::invoke(*reinterpret_cast<const F*>(ptr), std::forward<Args>(args)...);
    }

  public:
    const void* fn;
    R (*caller)(const void*, Args&&...);
    
    function_view() = delete;
    
    template <std::invocable<Args...> F>
    requires std::is_same_v<std::invoke_result_t<F, Args...>, R>
    function_view(const F& value) : fn(reinterpret_cast<const void*>(std::addressof(value))), caller(call_fn<F>) {
    }
    
    R operator()(Args&&... args) {
      return caller(fn, std::forward<Args>(args)...);
    }
  };
}  // namespace mtap
#endif