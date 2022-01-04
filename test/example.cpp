#include <ios>
#include <iostream>
#include <type_traits>
#include <mtap/mtap.hpp>

int main(int argc, const char* argv[]) {
  using mtap::option, mtap::multi_option;
  // clang-format off
  using parse_t = mtap::parser<
    option<0, "--help">,
    option<0, "-a">,
    option<0, "-b">,
    multi_option<1, "-c">
  >;
  parse_t opts;
  // clang-format on
  try {
    opts.parse_args(argc, argv);
  }
  catch (const mtap::argument_error& err) {
    std::cout << argv[0] << ": " << err.what() << "\n";
    return 1;
  }
  
  std::cout << "-a present: " << opts.get<"-a">().is_present() << '\n';
  std::cout << "-b present: " << opts.get<"-b">().is_present() << '\n';
  auto res = opts.get<"-c">();
  if (res.is_present()) {
    std::cout << "-c value: ";
    for (size_t i = 0; i < res.count(); i++) {
      std::cout << res[i][0] << " ";
    }
  }
}