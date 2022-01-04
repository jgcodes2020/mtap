#ifndef _MTAP_HPP_
#define _MTAP_HPP_

#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <memory>

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

  template <size_t N, fixed_string... Ws>
  struct option {
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
  };
  
  template <fixed_string... Ws>
  using flag_option = option<0, Ws...>;
  
  template <fixed_string... Ws>
  using arg_option = option<1, Ws...>;

  template <size_t N>
  class parse_result;

  namespace details {

    // BASIC CLASSES
    // ======================

    template <class T>
    struct is_option : std::false_type {};

    template <size_t N, fixed_string... Ws>
    struct is_option<option<N, Ws...>> : std::true_type {};

    template <class T>
    concept is_option_c = is_option<T>::value;

    template <fixed_string S>
    struct string_constant {
      using type = string_constant<S>;

      static constexpr fixed_string value = S;

      constexpr operator fixed_string<S.size()>() { return S; }

      constexpr fixed_string<S.size()> operator()() { return S; }
    };

    template <fixed_string... Ss>
    struct string_sequence {
      static constexpr size_t size = sizeof...(Ss);
    };

    template <char... Cs>
    using char_sequence = std::integer_sequence<char, Cs...>;

    // FORWARD DECLS
    // ==================
    template <size_t RS, size_t I0, size_t... Is>
    void update_parse_result_impl(
      parse_result<RS>& res, int argc, const char* argv[], size_t ibegin,
      size_t islice, std::index_sequence<I0, Is...>);

    inline void update_parse_result_impl(
      parse_result<0>& res, int argc, const char** argv, size_t ibegin,
      size_t islice, std::index_sequence<>);
  }  // namespace details

  template <size_t N>
  class parse_result {
    template <size_t RS, size_t I0, size_t... Is>
    friend void details::update_parse_result_impl(
      parse_result<RS>& res, int argc, const char* argv[], size_t ibegin,
      size_t islice, std::index_sequence<I0, Is...>);

  private:
    std::array<const char*, N> m_data {};
    
    void update(const char* const* p_begin, size_t i_slice) {
      m_data[0] = p_begin[0] + i_slice;
      for (size_t i = 1; i < N; i++) {
        m_data[i] = p_begin[i];
      }
    }
  public:
    parse_result() = default;
  
    static constexpr size_t size = N;
    bool is_present() const { return m_data[0] != nullptr; }
    const char* operator[](size_t i) const {
      if (m_data[0] == nullptr)
        throw std::runtime_error("Argument was not found");
      return m_data[i];
    }
    const char* at(size_t i) const {
      if (i >= N)
        throw std::out_of_range("Index out of range");
      return (*this)[i];
    }

    auto begin() const { return m_data.cbegin(); }
    auto end() const { return m_data.cend(); }
  };

  template <>
  class parse_result<0> {
    friend inline void details::update_parse_result_impl(
      parse_result<0>& res, int argc, const char** argv, size_t ibegin,
      size_t islice, std::index_sequence<>);

  private:
    bool present = false;

    void update() { present = true; }
  public:
    parse_result() = default;
    
    static constexpr size_t size = 0;
    bool is_present() const { return present; }
  };

  namespace details {
    // VERIFICATION CLASSES
    // ====================

    template <fixed_string... Ss>
    struct option_string_set : string_constant<Ss>... {
      template <fixed_string S>
      constexpr auto operator+(string_constant<S>) {
        if constexpr (std::is_base_of_v<string_constant<S>, option_string_set>)
          return option_string_set {};
        else
          return option_string_set<Ss..., S> {};
      }

      template <size_t N, fixed_string... Ws>
      constexpr auto operator<<(std::type_identity<option<N, Ws...>>) {
        return (*this + ... + string_constant<Ws> {});
      }

      constexpr size_t size() const { return sizeof...(Ss); }
    };

    template <is_option_c... Ts>
    inline constexpr bool all_option_switches_unique =
      (option_string_set<> {} << ... << std::type_identity<Ts> {}).size() ==
      (Ts::nopts + ...);

    // METAPROGRAMMING MADNESS
    // =======================

    template <size_t I, class T>
    struct indexed_type {
      using type = T;
    };

    template <class ISeq, class... Ts>
    struct type_indexer;

    template <size_t... Is, class... Ts>
    struct type_indexer<std::index_sequence<Is...>, Ts...> :
      indexed_type<Is, Ts>... {};

    template <size_t I, class T>
    indexed_type<I, T> deduce_indexed_type(indexed_type<I, T>);

    template <size_t I, class... Ts>
    using variadic_at = typename decltype(deduce_indexed_type<I>(
      type_indexer<std::index_sequence_for<Ts...>, Ts...> {}))::type;

    template <fixed_string Q, size_t I = 0, bool F = false>
    struct option_indexer {
      template <size_t N, fixed_string... Ws>
      constexpr auto operator<<(std::type_identity<option<N, Ws...>>) {
        if constexpr (F)
          return option_indexer {};
        else if constexpr (((Q == Ws) || ...))
          return option_indexer<Q, I, true> {};
        else
          return option_indexer<Q, I + 1, false> {};
      }

      constexpr size_t index() const {
        static_assert(F, "Did not match option");
        return I;
      }
    };

    template <fixed_string Q, is_option_c... Ts>
    inline constexpr size_t option_switch_index =
      (option_indexer<Q> {} << ... << std::type_identity<Ts> {}).index();

    template <class SSeq, class LSeq>
    struct option_filter;

    template <fixed_string... Ss, fixed_string... Ls>
    struct option_filter<string_sequence<Ss...>, string_sequence<Ls...>> {
      using short_opts = string_sequence<Ss...>;
      using long_opts  = string_sequence<Ls...>;

      template <fixed_string W>
      constexpr auto operator+(string_constant<W>) {
        if constexpr (W.size() > 2 && long_option_valid(W))
          return option_filter<
            string_sequence<Ss...>, string_sequence<Ls..., W>> {};
        else if constexpr (W.size() == 2 && short_option_valid(W))
          return option_filter<
            string_sequence<Ss..., W>, string_sequence<Ls...>> {};
        else
          static_assert(
            W.size() != W.size(), "Next string is not a valid option");
      }

      template <size_t N, fixed_string... Ws>
      constexpr auto operator<<(std::type_identity<option<N, Ws...>>) {
        return ((*this) + ... + string_constant<Ws> {});
      }
    };

    option_filter()->option_filter<string_sequence<>, string_sequence<>>;

    // CODEGEN AND VTABLES
    // ===================

    template <is_option_c... Ts>
    using make_result_tuple = std::tuple<parse_result<Ts::nargs>...>;

    template <class T>
    struct is_parse_result : std::false_type {};

    template <size_t S>
    struct is_parse_result<parse_result<S>> : std::true_type {};

    template <class T>
    concept is_parse_result_c = is_parse_result<T>::value;

    template <class T>
    struct is_result_tuple : std::false_type {};

    template <is_parse_result_c... Ts>
    struct is_result_tuple<std::tuple<Ts...>> : std::true_type {};

    template <class T>
    concept is_result_tuple_c = is_result_tuple<T>::value;

    template <size_t RS, size_t I0, size_t... Is>
    void update_parse_result_impl(
      parse_result<RS>& res, int argc, const char* argv[], size_t ibegin,
      size_t islice, std::index_sequence<I0, Is...>) {
      static_assert(RS == (sizeof...(Is) + 1), "Invalid arguments");
      res.update(argv + ibegin, islice);
    }

    void update_parse_result_impl(
      parse_result<0>& res, int argc, const char* argv[], size_t ibegin,
      size_t islice, std::index_sequence<>) {
      res.update();
    }

    template <size_t I, is_result_tuple_c T>
    void update_result_tuple(
      T& res, int argc, const char* argv[], size_t ibegin, size_t islice = 0) {
      update_parse_result_impl(
        std::get<I>(res), argc, argv, ibegin, islice,
        std::make_index_sequence<std::tuple_element_t<I, T>::size> {});
    }

    template <is_result_tuple_c T>
    using update_dispatcher = void (*)(T&, int, const char*[], size_t, size_t);

    // Runtime structs.
    namespace rt {
      template <is_result_tuple_c T>
      struct optinfo {
        size_t nargs;
        update_dispatcher<T> updater;
      };
      template <is_result_tuple_c T>
      struct opt_table {
        const std::unordered_map<char, optinfo<T>> short_opts;
        const std::unordered_map<std::string_view, optinfo<T>> long_opts;
      };
    }  // namespace rt

    template <
      is_result_tuple_c R, is_option_c... Ts, fixed_string... Ss,
      fixed_string... Ls>
    rt::opt_table<R> make_option_table_impl(
      string_sequence<Ss...>, string_sequence<Ls...>) {
      // clang-format off
      return {
        .short_opts =
          {{Ss[1], {
            variadic_at<option_switch_index<Ss, Ts...>, Ts...>::nargs,
            &update_result_tuple<option_switch_index<Ss, Ts...>, R>,
          }}...},
        .long_opts =
          {{Ls, {
            variadic_at<option_switch_index<Ls, Ts...>, Ts...>::nargs,
            &update_result_tuple<option_switch_index<Ls, Ts...>, R>,
          }}...}
      };
      // clang-format on
    };

    template <is_result_tuple_c R, is_option_c... Ts>
    const rt::opt_table<R> make_option_table() {
      using filter_t =
        decltype((option_filter {} << ... << std::type_identity<Ts> {}));
      return make_option_table_impl<R, Ts...>(
        typename filter_t::short_opts {}, typename filter_t::long_opts {});
    }
  }  // namespace details

  class argument_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  template <details::is_option_c... Ts>
  class parser {
    static_assert(
      details::all_option_switches_unique<Ts...>,
      "All option switch names must be unique");
  public:
    struct config {
      
    };
  private:
    using res_tuple_t = details::make_result_tuple<Ts...>;
    using otable_t    = details::rt::opt_table<res_tuple_t>;
    
    otable_t otable;
    bool parsed;
    std::unique_ptr<res_tuple_t> options;
    std::vector<const char*> positionals;

  public:
    parser() :
      otable(details::make_option_table<res_tuple_t, Ts...>()),
      parsed(false),
      options {new res_tuple_t()},
      positionals() {}

    parser(const parser&) = delete;

    parser(parser&&) = default;

    void parse_args(unsigned int argc, const char* argv[]) {
      using namespace std::string_literals;
      using namespace std::string_view_literals;

      if (parsed)
        return;
      parsed = true;
      if (argc < 2)
        return;

      bool done_parsing_opts = false;
      for (size_t i = 1; i < argc; i++) {
        if (done_parsing_opts) {
          positionals.push_back(argv[i]);
          continue;
        }
        auto arg = std::string_view(argv[i]);
        // argument is probably an option
        if (arg.size() >= 2 && arg[0] == '-') {
          if (arg[1] == '-') {
            if (arg.size() == 2) {
              // stop parsing options, string is --
              done_parsing_opts = true;
              continue;
            }
            // long option
            auto& info    = otable.long_opts.at(arg);
            size_t skip_n = info.nargs;
            if (i + skip_n >= argc)
              throw argument_error("Not enough arguments for option");

            info.updater(*options, argc, argv, i + 1, 0);
            i += skip_n;
            continue;
          }
          // parse some short options
          for (size_t j = 1; j < arg.size(); j++) {
            const auto& info = otable.short_opts.at(arg[j]);
            if (j + 1 < arg.size()) [[likely]] {
              auto cmp = info.nargs <=> 1;
              if (cmp == std::strong_ordering::greater) {
                throw argument_error(
                  "Multi-argument short option cannot be spliced");
              }
              else if (cmp == std::strong_ordering::equal) {
                // Argument splicing thing. Example:
                // cmake -DCMAKE_BUILD_TYPE=Release treats
                // CMAKE_BUILD_TYPE=Release as an argument to -D
                info.updater(*options, argc, argv, i, j + 1);
                break;
              }
              else {
                info.updater(*options, argc, argv, i, 0);
              }
            }
            // End of the list
            else [[unlikely]] {
              if (info.nargs > 0) {
                // Short option with arguments
                size_t skip_n = info->nargs;
                if (i + skip_n >= argc)
                  throw argument_error("Not enough arguments for option");

                info.updater(*options, argc, argv, i + 1, 0);
                i += skip_n;
                continue;
              }
              // Short option without arguments
              info.updater(*options, argc, argv, i, 0);
            }
          }
        }
        else {
          positionals.push_back(argv[i]);
        }
      }
    }
    template <fixed_string W>
    const auto& get() const {
      if (!parsed)
        throw std::runtime_error("Parser has not parsed arguments yet.");
      return std::get<details::option_switch_index<W, Ts...>>(*options);
    }
  };
}  // namespace mtap
#endif