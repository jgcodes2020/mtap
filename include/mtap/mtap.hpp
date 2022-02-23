/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef _MTAP_OPTION_HPP_
#define _MTAP_OPTION_HPP_
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mtap/fixed_string.hpp>
#include <mtap/meta_helpers.hpp>

namespace mtap {
  class argument_error : public std::runtime_error {
  public:
    argument_error(const char* what) : runtime_error(what) {}
    argument_error(const std::string& what) : runtime_error(what) {}

    argument_error(const argument_error& rhs) noexcept = default;
  };

  enum class opt_type : uint16_t { short_opt, long_opt, pos_arg };

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

    template <class T, class R, class... Args>
    constexpr bool callable_helper(
      std::type_identity<T>, std::type_identity<R(Args...)>) {
      if constexpr (std::is_invocable_v<T, Args...>)
        return std::is_convertible_v<std::invoke_result_t<T, Args...>, R>;
      else
        return false;
    }

    template <class T, class F>
    concept callable =
      callable_helper(std::type_identity<T> {}, std::type_identity<F> {});

    constexpr bool isalnum(char c) {
      return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
        ('0' <= c && c <= '9');
    }

    // Simultaneously classifies and validates an option.
    static constexpr std::optional<opt_type> classify_opt(
      std::string_view str, size_t nargs) {
      using namespace std::string_view_literals;
      if (str == "\1"sv) {
        if (nargs == 1)
          return opt_type::pos_arg;
        else
          return std::nullopt;
      }
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
      static_assert(
        classify_opt(Switch, NArgs).has_value(), "Invalid option switch");

      static constexpr auto name  = Switch;
      static constexpr auto type  = classify_opt(Switch, NArgs).value();
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

  template <details::callable<details::make_callback_sig<1>> F>
  auto pos_arg(F&& fn) {
    return details::opt_impl<"\1", 1, F>(std::forward<F>(fn));
  }

  namespace details {
    template <
      class ShortSeq = type_sequence<>, class LongSeq = type_sequence<>,
      class PAIndex = void, size_t Index = 0>
    struct option_filter;

    template <class PA>
    struct eval_posarg_index;

    template <>
    struct eval_posarg_index<void> {
      static constexpr std::optional<size_t> value = std::nullopt;
    };

    template <size_t I>
    struct eval_posarg_index<std::integral_constant<size_t, I>> {
      static constexpr std::optional<size_t> value = I;
    };

    template <class PA>
    inline constexpr std::optional<size_t> eval_posarg_index_v =
      eval_posarg_index<PA>::value;

    template <
      fixed_string... SNs, size_t... SIs, fixed_string... LNs, size_t... LIs,
      class PA, size_t I>
    struct option_filter<
      type_sequence<string_index_leaf<SIs, SNs>...>,
      type_sequence<string_index_leaf<LIs, LNs>...>, PA, I> {
      using short_opts = type_sequence<string_index_leaf<SIs, SNs>...>;
      using long_opts  = type_sequence<string_index_leaf<LIs, LNs>...>;
      static constexpr std::optional<size_t> pos_arg_index =
        eval_posarg_index_v<PA>;

      template <fixed_string N, size_t S, class F>
      constexpr auto operator+(std::type_identity<opt_impl<N, S, F>>) {
        if constexpr (opt_impl<N, S, F>::type == opt_type::short_opt) {
          return option_filter<
            type_sequence<
              string_index_leaf<SIs, SNs>..., string_index_leaf<I, N>>,
            type_sequence<string_index_leaf<LIs, LNs>...>, PA, I + 1> {};
        }
        else if constexpr (opt_impl<N, S, F>::type == opt_type::long_opt) {
          return option_filter<
            type_sequence<string_index_leaf<SIs, SNs>...>,
            type_sequence<
              string_index_leaf<LIs, LNs>...,
              string_index_leaf<I, N.template substr<2>()>>,
            PA, I + 1> {};
        }
        else if constexpr (opt_impl<N, S, F>::type == opt_type::pos_arg) {
          static_assert(
            std::is_void_v<PA>,
            "Only one positional argument handler is allowed");
          return option_filter<
            type_sequence<string_index_leaf<SIs, SNs>...>,
            type_sequence<string_index_leaf<LIs, LNs>...>,
            std::integral_constant<size_t, I>, I + 1> {};
        }
      }
    };

    template <fixed_string... Ss, size_t... Ns, class... Fs>
    constexpr size_t count_shorts(type_sequence<opt_impl<Ss, Ns, Fs>...>) {
      std::initializer_list<std::pair<std::string_view, size_t>> pairs = {
        {std::string_view(Ss), Ns}...};

      return std::count_if(
        pairs.begin(), pairs.end(),
        [](const std::pair<std::string_view, size_t>& pair) {
        return classify_opt(pair.first, pair.second) == opt_type::short_opt;
        });
    }
    template <fixed_string... Ss, size_t... Ns, class... Fs>
    constexpr size_t count_longs(type_sequence<opt_impl<Ss, Ns, Fs>...>) {
      std::initializer_list<std::pair<std::string_view, size_t>> pairs = {
        {std::string_view(Ss), Ns}...};

      return std::count_if(
        pairs.begin(), pairs.end(),
        [](const std::pair<std::string_view, size_t>& pair) {
        return classify_opt(pair.first, pair.second) == opt_type::long_opt;
        });
    }

    template <fixed_string... Ss, size_t... Ns, class... Fs>
    constexpr std::array<
      std::pair<char, size_t>,
      count_shorts(type_sequence<opt_impl<Ss, Ns, Fs>...> {})>
    filter_shorts(type_sequence<opt_impl<Ss, Ns, Fs>...>) {
      std::initializer_list<std::pair<std::string_view, size_t>> pairs = {
        {std::string_view(Ss), Ns}...};
      std::array<
        std::pair<char, size_t>,
        count_shorts(type_sequence<opt_impl<Ss, Ns, Fs>...> {})>
        arr;

      size_t i = 0;
      auto it  = arr.begin();
      for (const auto& [sw, nargs]: pairs) {
        if (classify_opt(sw, nargs) == opt_type::short_opt)
          *it++ = {sw[1], i};
        i++;
      }

      return arr;
    }
    template <fixed_string... Ss, size_t... Ns, class... Fs>
    constexpr std::array<
      std::pair<std::string_view, size_t>,
      count_shorts(type_sequence<opt_impl<Ss, Ns, Fs>...> {})>
    filter_longs(type_sequence<opt_impl<Ss, Ns, Fs>...>) {
      std::initializer_list<std::pair<std::string_view, size_t>> pairs = {
        {std::string_view(Ss), Ns}...};
      std::array<
        std::pair<std::string_view, size_t>,
        count_shorts(type_sequence<opt_impl<Ss, Ns, Fs>...> {})>
        arr;

      size_t i = 0;
      auto it  = arr.begin();
      for (const auto& [sw, nargs]: pairs) {
        if (classify_opt(sw, nargs) == opt_type::long_opt)
          *it++ = {sw.substr(2), i};
        i++;
      }

      return arr;
    }
  }  // namespace details

  template <class... Opts>
  class parser;

  template <fixed_string... Ns, size_t... Ss, class... Fs>
  class parser<details::opt_impl<Ns, Ss, Fs>...> {
    static_assert(
      string_pack_unique_v<Ns...>, "All option switches must be unique");

  private:
    using vtable_short_t = std::unordered_map<
      char, size_t (*)(std::tuple<Fs...>&, int, const char*[], int, int)>;
    using vtable_long_t = std::unordered_map<
      std::string_view,
      size_t (*)(std::tuple<Fs...>&, int, const char*[], int)>;

    using option_filter_t = decltype((
      details::option_filter<> {} + ... +
      std::type_identity<details::opt_impl<Ns, Ss, Fs>> {}));

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
          throw argument_error("Not enough arguments remaining");
        std::get<I>(tup)(argv[iarg + (clip ? 0 : 1)] + clip);
      }
      else {
        if (clip)
          throw argument_error(
            "Multi-arg short option cannot be specified in the same argument");
        if (iarg + arg_size >= argc)
          throw argument_error("Not enough arguments remaining");
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
        throw argument_error("Not enough arguments remaining");
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
      constexpr auto vals = details::filter_shorts(
        type_sequence<details::opt_impl<Ns, Ss, Fs>...> {});
      return [&]<size_t... Is>(std::index_sequence<Is...>) {
        return vtable_short_t {
          {vals[Is].first, dispatch_short<vals[Is].second>}...};
      }
      (std::make_index_sequence<vals.size()> {});
    }

    static vtable_long_t make_long_vtable() {
      constexpr auto vals = details::filter_longs(
        type_sequence<details::opt_impl<Ns, Ss, Fs>...> {});
      return [&]<size_t... Is>(std::index_sequence<Is...>) {
        return vtable_long_t {
          {vals[Is].first, dispatch_long<vals[Is].second>}...};
      }
      (std::make_index_sequence<vals.size()> {});
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
      bool parse_opts = true;
      for (int i = 1; i < argc;) {
        const char* arg = argv[i];
        if (arg[0] == '-' && parse_opts) {
          if (arg[1] == '-') {
            if (arg[2] == '\0') {
              // argument is '--', stop
              parse_opts = false;
            }
            else if (details::isalnum(arg[2])) {
              // argument is long option
              size_t last_narg;
              try {
                last_narg = long_vtable.at(arg + 2)(*ctable, argc, argv, i);
              }
              catch (const std::out_of_range& out) {
                std::throw_with_nested(argument_error("Cannot use option"));
              }
              i += (last_narg + 1);
            }
            else
              throw argument_error("Invalid long-option string");
          }
          else if (details::isalnum(arg[1])) {
            // parse short options
            size_t last_narg;
            for (size_t j = 1; arg[j] != '\0'; j++) {
              // clip == 0 signals "don't splice"
              try {
                last_narg = short_vtable.at(arg[j])(
                  *ctable, argc, argv, i, (arg[j + 1] == '\0') ? 0 : j + 1);
              }
              catch (const std::out_of_range& out) {
                std::throw_with_nested(argument_error("Cannot use option"));
              }
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
        else {
          static constexpr auto posarg = option_filter_t::pos_arg_index;
          if constexpr (posarg.has_value()) {
            std::get<posarg.value()> (*ctable)(argv[i++]);
          }
        }
      }
    }

  public:
    // Parse
    void parse(int argc, const char* argv[]) {
      try {
        main_parser(argc, argv);
      }
      catch (const argument_error& err) {
        std::cerr << argv[0] << ": " << err.what() << '\n';
        exit(0);
      }
    }
  };

  template <fixed_string... Ns, size_t... Ss, class... Fs>
  parser(details::opt_impl<Ns, Ss, Fs>...)
    -> parser<details::opt_impl<Ns, Ss, Fs>...>;
}  // namespace mtap
#endif