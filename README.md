# MTAP (Minimal Templated Argument Parser)
A bare-minimum C++20 argument parser, inspired by getopt. Instead of trying to be featureful, this solely focuses on being an argument parser, not a help generator or something else. Because some of the runtime classes involved are not `constexpr`, options are specified as template arguments.

MTAP enforces certain conventions for options names:
- Options beginning with 1 dash are short options.
  - Short options can only be 1 character in length.
  - The character must be alphanumeric.
- Options beginning with 2 dashes are long options.
  - After the 2 dashes, you may use any sequence of alphanumeric characters and dashes.
  - The option's name may not end with a dash.

# Example usage
```c++
#include <iostream>
#include <type_traits>
#include <mtap.hpp>

int main(int argc, const char* argv[]) {
  using mtap::parser, mtap::option;
  
  parser<
    option<0, "--help">,
    option<0, "-a">,
    option<0, "-b">,
    multi_option<1, "-c">
  > opts;
  
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
```
# Licensing
This library, like any others that I intend specifically to open-source, is licensed under the Mozilla Public License. I do this specifically to ensure that my code remains open-source, while allowing you, the user, to put it in any project you need it for, whether proprietary or open-source.

# Future plans
- Proper documentation.