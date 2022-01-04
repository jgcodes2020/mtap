#ifndef _MTAP_OPTION_HPP_
#define _MTAP_OPTION_HPP_

#include <algorithm>
#include <cstddef>
#include <mtap/fixed_string.hpp>

namespace mtap {
  namespace details {
    namespace ctype {
      constexpr bool is_alnum(char c) {
        return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
          ('0' <= c && c <= '9');
      }

      constexpr bool is_alnum_or_dash(char c) {
        return is_alnum(c) || c == '-';
      }
    }  // namespace ctype

    template <size_t S>
    consteval bool short_option_valid(fixed_string<S> str) {
      return (str[0] == '-' && ctype::is_alnum(str[1]));
    }

    template <size_t S>
    consteval bool long_option_valid(fixed_string<S> str) {
      return (
        (str[0] == '-' && str[1] == '-') &&
        std::find_if_not(str.begin() + 2, str.end(), ctype::is_alnum_or_dash) ==
          str.end() &&
        str[S - 1] != '-');
    }
  }  // namespace details
  
  // Type parameters:
  // N - number of arguments
  // M - if true, the argument can be specified multiple times
  // Ws... - the option switches for this option
  template <size_t N, bool M, fixed_string... Ws>
  struct basic_option {
    static_assert(sizeof...(Ws) > 0, "Option must have option names");
    static_assert(
      ((Ws.size() >= 2) && ...), "Options cannot be a single character");
    static_assert(
      ((Ws.size() > 2 || details::short_option_valid(Ws)) && ...),
      "Short options must be a single dash followed by a single alphanumeric character");
    static_assert(
      ((Ws.size() == 2 || details::long_option_valid(Ws)) && ...),
      "Long options must be two dashes followed by a sequence of alphanumeric characters and/or dashes, but cannot end with a dash");

    static constexpr size_t nargs = N;
    static constexpr size_t nopts = sizeof...(Ws);
    static constexpr bool is_multi = M;
  };
  
  template <fixed_string... Ws>
  using flag_option = basic_option<0, false, Ws...>;
  
  template <fixed_string... Ws>
  using val_option = basic_option<1, false, Ws...>;
  
  template <size_t N, fixed_string... Ws>
  using option = basic_option<N, false, Ws...>;
  
  template <size_t N, fixed_string... Ws>
  using multi_option = basic_option<N, true, Ws...>;
}
#endif