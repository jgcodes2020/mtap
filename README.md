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
#include <cstdlib>
#include <iostream>
#include <mtap/mtap.hpp>

using mtap::option;

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
    })
  ).parse(argc, argv);
}
```
# Licensing
This library, like any others that I intend specifically to open-source, is licensed under the [Mozilla Public License](LICENSE.md). I do this specifically to ensure that my code remains open-source, while allowing you, the user, to put it in any project you need it for, whether proprietary or open-source.

The tests are licensed under [0-clause BSD](test/LICENSE.md).

# Future plans
- Proper documentation.