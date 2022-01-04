#include <ios>
#include <iostream>
#include <type_traits>
#include <mtap/mtap.hpp>

int main(int argc, const char* argv[]) {
  using mtap::parser;
  // clang-format off
  using parse_t = parser<
    mtap::option<0, "--help">,
    mtap::option<0, "-a">,
    mtap::option<0, "-b">,
    mtap::option<1, "-c">
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
  if (res.is_present()) std::cout << "-c value: " << res[0] << '\n';
}