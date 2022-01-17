#ifndef _MTAP_OPTION_HPP_
#define _MTAP_OPTION_HPP_
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <mtap/fixed_string.hpp>
namespace mtap {

  enum class opt_type : bool { short_opt, long_opt };

  namespace details {
    template <class T, size_t I>
    using index_type_sink = T;

    template <class ISeq>
    struct callback_sig_helper;

    template <size_t... Is>
    struct callback_sig_helper<std::index_sequence<Is...>> {
      using type = void(index_type_sink<std::string_view, Is>...);
    };

    template <size_t I>
    using make_callback_sig = typename callback_sig_helper<std::make_index_sequence<I>>::type;

    template <class T, class F>
    struct callable_concept_helper;

    // clang-format off
    template <class T, class R, class... Args>
      struct callable_concept_helper<T, R(Args...)> :
        std::bool_constant <std::is_invocable_v<T, Args...> && std::is_convertible_v<std::invoke_result_t<T, Args...>, R>> {};
    // clang-format on

    template <class T, class F>
    concept callable = callable_concept_helper<T, F>::value;

    constexpr bool isalnum(char c) {
      return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
        ('0' <= c && c <= '9');
    }
    
    // Simultaneously classifies and validates an option.
    static constexpr std::optional<opt_type> classify_opt(
      std::string_view str) {
      auto cmp = str.size() <=> 2;
      if (cmp < 0 || str[0] != '-')
        return std::nullopt;
      else if (cmp == 0) {
        if (isalnum(str[1]))
          return opt_type::short_opt;
        else
          return std::nullopt;
      }
      else {
        auto test_fn = [](char c) {
          return c == '-' || isalnum(c);
        };
        for (auto it = str.begin() + 2; it < str.end(); it++) {
          if (!(isalnum(*it) || *it == '-'))
            return std::nullopt;
        }
        return opt_type::long_opt;
      }
    }
    
    template <fixed_string Switch, size_t NArgs, callable<make_callback_sig<NArgs>> F>
    struct opt_impl {
      static_assert(classify_opt(Switch).has_value(), "Invalid option switch");
      
      static constexpr auto name = Switch;
      static constexpr auto type = classify_opt(Switch);
      static constexpr auto nargs = NArgs;
      
      using function_t = F;
      using callback_t = make_callback_sig<NArgs>;
      
      F fn;
      opt_impl(F&& f) : fn(f) {}
    };
    
    
  }  // namespace details
  template <fixed_string Switch, size_t NArgs, details::callable<details::make_callback_sig<NArgs>> F>
  auto option(F&& fn) {
    return details::opt_impl<Switch, NArgs, F>(std::forward<F>(fn));
  }
  
  template <class... Opts>
  class parser;
  
  template <fixed_string... Ns, size_t... Ss, class... Fs>
  class parser<details::opt_impl<Ns, Ss, Fs>...> {
  private:
    std::unique_ptr<std::tuple<Fs...>> callbacks;
    
  };
}  // namespace mtap
#endif