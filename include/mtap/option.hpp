#ifndef _MTAP_OPTION_HPP_
#define _MTAP_OPTION_HPP_
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mtap/fixed_string.hpp>
#include <mtap/meta_helpers.hpp>

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
    using make_callback_sig =
      typename callback_sig_helper<std::make_index_sequence<I>>::type;

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
        // a long option may consist of alnum characters and dashes
        for (auto it = str.begin() + 2; it < str.end(); it++) {
          if (!(isalnum(*it) || *it == '-'))
            return std::nullopt;
        }
        // a long option's name must begin and end with an alnum character
        if (!(isalnum(str[2]) && isalnum(str.back())))
          return std::nullopt;
        return opt_type::long_opt;
      }
    }

    template <
      fixed_string Switch, size_t NArgs, callable<make_callback_sig<NArgs>> F>
    struct opt_impl {
      static_assert(classify_opt(Switch).has_value(), "Invalid option switch");

      static constexpr auto name  = Switch;
      static constexpr auto type  = classify_opt(Switch);
      static constexpr auto nargs = NArgs;

      using function_t = F;
      using callback_t = make_callback_sig<NArgs>;

      F fn;
      opt_impl(F&& f) : fn(f) {}
    };

    template <class T>
    struct is_some_opt : std::false_type {};
    template <
      fixed_string N, size_t S,
      details::callable<details::make_callback_sig<S>> F>
    struct is_some_opt<opt_impl<N, S, F>> : std::true_type {};

    template <class T>
    concept some_option = is_some_opt<T>::value;
  }  // namespace details
  template <
    fixed_string Switch, size_t NArgs,
    details::callable<details::make_callback_sig<NArgs>> F>
  auto option(F&& fn) {
    return details::opt_impl<Switch, NArgs, F>(std::forward<F>(fn));
  }

  namespace details {
    template <
      class ShortSeq = type_sequence<>, class LongSeq = type_sequence<>,
      int Index = 0>
    struct option_filter;

    template <
      fixed_string... SNs, size_t... SIs, fixed_string... LNs, size_t... LIs,
      int I>
    struct option_filter<
      type_sequence<string_index_leaf<SIs, SNs>...>,
      type_sequence<string_index_leaf<LIs, LNs>...>, I> {
      using short_opts = type_sequence<string_index_leaf<SIs, SNs>...>;
      using long_opts  = type_sequence<string_index_leaf<LIs, LNs>...>;

      template <fixed_string N, size_t S, class F>
      constexpr auto operator+(std::type_identity<opt_impl<N, S, F>>) {
        if constexpr (opt_impl<N, S, F>::type == opt_type::short_opt) {
          return option_filter<
            type_sequence<
              string_index_leaf<SIs, SNs>..., string_index_leaf<I, N>>,
            type_sequence<string_index_leaf<LIs, LNs>...>, I + 1> {};
        }
        else {
          return option_filter<
            type_sequence<string_index_leaf<SIs, SNs>...>,
            type_sequence<
              string_index_leaf<LIs, LNs>..., string_index_leaf<I, N>>,
            I + 1> {};
        }
      }
    };
  }  // namespace details

  template <class... Opts>
  class parser;

  template <fixed_string... Ns, size_t... Ss, class... Fs>
  class parser<details::opt_impl<Ns, Ss, Fs>...> {
    static_assert(
      string_sequence_unique_v<string_sequence<Ns...>>,
      "All option switches must be unique");

  private:
    using vtable_short_t = std::unordered_map<
      char, size_t (*)(std::tuple<Fs...>&, int, const char*[], int, int)>;
    using vtable_long_t = std::unordered_map<
      std::string_view,
      size_t (*)(std::tuple<Fs...>&, int, const char*[], int)>;

    std::unique_ptr<std::tuple<Fs...>> ctable;
    vtable_short_t short_vtable;
    vtable_long_t long_vtable;

    // Dispatches short options.
    // iarg = first arg containing argument values
    // clip = beginning of argument data for spliced options.
    // Returns the number of arguments for the dispatched option.
    template <size_t I>
    static size_t dispatch_short(
      std::tuple<Fs...>& tup, int argc, const char* argv[], int iarg,
      int clip) {
      constexpr size_t arg_size =
        integer_sequence_element_v<I, std::index_sequence<Ss...>>;
      if constexpr (arg_size == 0) {
        std::get<I>(tup)();
      }
      else if constexpr (arg_size == 1) {
        if (iarg + (clip ? 0 : 1) >= argc)
          throw std::runtime_error("Not enough arguments remaining");
        std::get<I>(tup)(argv[iarg + (clip ? 0 : 1)] + clip);
      }
      else {
        if (clip)
          throw std::runtime_error(
            "Multi-arg short option cannot be specified in the same argument");
        if (iarg + arg_size >= argc)
          throw std::runtime_error("Not enough arguments remaining");
        // Inline pack expansion to split the next arg_size arguments into
        // parameters.
        [&]<size_t... Is>(std::index_sequence<Is...>) {
          std::get<I>(tup)(argv[iarg + 1 + Is]...);
        }
        (std::make_index_sequence<arg_size> {});
      }
      return arg_size;
    }

    template <size_t I>
    static size_t dispatch_long(
      std::tuple<Fs...>& tup, int argc, const char* argv[], int iarg) {
      constexpr size_t arg_size =
        integer_sequence_element_v<I, std::index_sequence<Ss...>>;
      if (iarg + arg_size >= argc)
        throw std::runtime_error("Not enough arguments remaining");
      if constexpr (arg_size == 0) {
        std::get<I>(tup)();
      }
      else {
        [&]<size_t... Is>(std::index_sequence<Is...>) {
          std::get<I>(tup)(argv[iarg + Is]...);
        }
        (std::make_index_sequence<arg_size> {});
      }
      return arg_size;
    }

    static vtable_short_t make_short_vtable() {
      using opt_pairs = typename decltype((
        details::option_filter<> {} + ... +
        std::type_identity<details::opt_impl<Ns, Ss, Fs>> {}))::short_opts;
      
      return []<fixed_string... VNs, size_t... VIs>(
        type_sequence<details::string_index_leaf<VIs, VNs>...>) {
        return vtable_short_t {{VNs[1], dispatch_short<VIs>}...};
      }
      (opt_pairs {});
    }

    static vtable_long_t make_long_vtable() {
      using opt_pairs = typename decltype((
        details::option_filter<> {} + ... +
        std::type_identity<details::opt_impl<Ns, Ss, Fs>> {}))::long_opts;

      return []<fixed_string... VNs, size_t... VIs>(
        type_sequence<details::string_index_leaf<VIs, VNs>...>) {
        return vtable_long_t {{VNs.template substr<2>(), dispatch_long<VIs>}...};
      }
      (opt_pairs {});
    }

  public:
    parser(details::opt_impl<Ns, Ss, Fs>&&... opts) :
      ctable(new std::tuple<Fs...>(std::move(opts.fn)...)),
      short_vtable(make_short_vtable()),
      long_vtable(make_long_vtable()) {}

    // Special methods

    parser(const parser&) = delete;
    parser& operator=(const parser&) = delete;

    parser(parser&&) = default;
    parser& operator=(parser&&) = default;

    ~parser() = default;

  private:
    void main_parser(int argc, const char* argv[]) {
      int i               = 1;
      while (i < argc) {
        const char* arg = argv[i];
        if (arg[0] == '-') {
          if (arg[1] == '-') {
            if (arg[2] == '\0') {
              // argument is '--', stop
              break;
            }
            else if (details::isalnum(arg[2])) {
              // argument is long option
              size_t last_narg =
                long_vtable.at(argv[i] + 2)(*ctable, argc, argv, i);
              i += (last_narg + 1);
            }
            else
              throw std::runtime_error("bad option");
          }
          else if (details::isalnum(arg[1])) {
            // parse short options
            size_t last_narg;
            for (size_t j = 1; arg[j] != '\0'; j++) {
              // clip == 0 signals "don't splice"
              last_narg = short_vtable.at(arg[j])(
                *ctable, argc, argv, i, (arg[j + 1] == '\0') ? 0 : j + 1);
              if (last_narg == 0) {
                if (arg[j + 1] == '\0')
                  ++i;
              }
              else if (last_narg == 1) {
                // next arg is the argument if this is the last option
                if (arg[j + 1] == '\0')
                  i += 2;
                // otherwise next arg is a new set of options
                else
                  i += 1;
                break;
              }
              else {
                i += (last_narg + 1);
                break;
              }
            }
          }
        }
      }
    }

  public:
    // Parse
    void parse(int argc, const char* argv[]) { main_parser(argc, argv); }
  };

  template <fixed_string... Ns, size_t... Ss, class... Fs>
  parser(details::opt_impl<Ns, Ss, Fs>...)
    -> parser<details::opt_impl<Ns, Ss, Fs>...>;
}  // namespace mtap
#endif