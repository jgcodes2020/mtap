#ifndef _MTAP_META_HELPERS_HPP_
#define _MTAP_META_HELPERS_HPP_

#include <cstddef>
#include <utility>

#include <mtap/fixed_string.hpp>

namespace mtap {
  
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
  
  template <class... Ts>
  struct type_sequence {
    static constexpr size_t size = sizeof...(Ts);
  };

  namespace details {
    template <size_t I, fixed_string S>
    struct string_index_leaf {
      static constexpr size_t index = I;
      static constexpr fixed_string value = S;
    };

    template <class ISeq, fixed_string... Ss>
    struct string_index_root;

    template <size_t... Is, fixed_string... Ss>
    struct string_index_root<std::index_sequence<Is...>, Ss...> :
      string_index_leaf<Is, Ss>... {};

    template <size_t I, fixed_string S>
    string_index_leaf<I, S> deduce_string_index(string_index_leaf<I, S>);
    
    template <size_t I>
    void deduce_string_index(...) = delete;
    
    template <fixed_string S, size_t I>
    string_index_leaf<I, S> deduce_string_query(string_index_leaf<I, S>);
    
    template <fixed_string S>
    void deduce_string_query(...) = delete;

  }  // namespace details

  template <size_t I, class SSeq>
  struct string_sequence_element;

  template <size_t I, fixed_string... Ss>
  struct string_sequence_element<I, string_sequence<Ss...>> {
    static constexpr fixed_string value =
      decltype(details::deduce_string_index<I>(
        details::string_index_root<
          std::make_index_sequence<sizeof...(Ss)>, Ss...> {}))::value;
  };

  template <size_t I, class SSeq>
  inline constexpr fixed_string string_sequence_element_v =
    string_sequence_element<I, SSeq>::value;
  
  template <fixed_string Q, class SSeq>
  struct string_sequence_lookup;
  
  template <fixed_string Q, fixed_string... Ss>
  struct string_sequence_lookup<Q, string_sequence<Ss...>> {
    static constexpr size_t value =
      decltype(details::deduce_string_query<Q>(
        details::string_index_root<
          std::make_index_sequence<sizeof...(Ss)>, Ss...> {}))::index;
  };
  
  template <fixed_string Q, class SSeq>
  inline constexpr size_t string_sequence_lookup_v = string_sequence_lookup<Q, SSeq>::value;

  namespace details {
    template <fixed_string... Ss>
    struct strseq_unique_helper : string_constant<Ss>... {
      using sequence_type = string_sequence<Ss...>;

      template <fixed_string S>
      constexpr auto operator+(string_constant<S>) {
        if constexpr (std::is_base_of_v<
                        string_constant<S>, strseq_unique_helper>)
          return strseq_unique_helper {};
        else
          return strseq_unique_helper<Ss..., S> {};
      }

      constexpr size_t size() const { return sizeof...(Ss); }
    };
  }  // namespace details

  template <class SSeq>
  struct string_sequence_unique;

  template <fixed_string... Ss>
  struct string_sequence_unique<string_sequence<Ss...>> :
    std::bool_constant<
      (details::strseq_unique_helper<> {} + ... + string_constant<Ss> {})
        .size() == sizeof...(Ss)> {};

  template <class SSeq>
  inline constexpr bool string_sequence_unique_v =
    string_sequence_unique<SSeq>::value;
  
  namespace details {
    template <size_t I, class T, T V>
    struct int_index_leaf {
      static constexpr T value = V;
    };

    template <class ISeq, class T, T... Vs>
    struct int_index_root;

    template <size_t... Is, class T, T... Vs>
    struct int_index_root<std::index_sequence<Is...>, T, Vs...> :
      int_index_leaf<Is, T, Vs>... {};

    template <size_t I, class T, T V>
    int_index_leaf<I, T, V> deduce_int_index(int_index_leaf<I, T, V>);
  }  // namespace details

  template <size_t I, class VSeq>
  struct integer_sequence_element;

  template <size_t I, class T, T... Vs>
  struct integer_sequence_element<I, std::integer_sequence<T, Vs...>> {
    static constexpr T value = decltype(details::deduce_int_index<I, T>(
      details::int_index_root<
        std::make_index_sequence<sizeof...(Vs)>, T, Vs...> {}))::value;
  };
  
  template <size_t I, class VSeq>
  inline constexpr typename VSeq::value_type integer_sequence_element_v = integer_sequence_element<I, VSeq>::value;
}  // namespace mtap
#endif