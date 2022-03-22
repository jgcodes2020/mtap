#include <cstdlib>
#include <iostream>
#include <string_view>
#include <mtap/mtap.hpp>

using mtap::option, mtap::pos_arg;

template <class T>
void print_csv(const T& x) {
  const char* sep = "";
  for (const auto& i: x) {
    std::cout << sep << i;
    sep = ", ";
  }
  std::cout << '\n';
}

int main(int argc, const char* argv[]) {
  mtap::parser(
    option<"--test", 1>([](std::string_view val) {
      std::cout << "--test: " << val << "\n";
    })
  ).parse(argc, argv);
}
