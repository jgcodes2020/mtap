# MTAP (Minimal Templated Argument Parser)
---
A minimal, but heavily templated argument parser for C++20. This library aims to be an argument parser and nothing more. I originally conceived this library as part of cxx-coreutils.

Each option is templated on its value, the number of arguments it receives and a callback. MTAP then runs this callback each time it sees an option. The arguments are passed to the callbacks as function parameters of type std::string_view.

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