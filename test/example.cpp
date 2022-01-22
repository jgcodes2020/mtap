#include <cstdlib>
#include <iostream>
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
    option<"--help", 0>([]() {
      std::cout << "Some usage options\n";
      std::exit(0);
    }),
    option<"-a", 0>([]() {
      std::cout << "Option A set\n";
    }),
    option<"-b", 0>([]() {
      std::cout << "Option B set\n";
    }),
    option<"-c", 1>([](std::string_view value) {
      std::cout << "Option C set, value = " << value << '\n';
    }),
    
  ).parse(argc, argv);
}
