#include <numeric>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <mtap/function_view.hpp>

mtap::function_view<int64_t()> time_getter() {
  return []() -> int64_t {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  };
}
void override_stack_frame() {
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int main() {
  auto fn = time_getter();
  override_stack_frame();
  std::cout << (fn() / 1000) << "\n";
}