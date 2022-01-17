/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef _MTAP_FIXED_STRING_HPP_
#define _MTAP_FIXED_STRING_HPP_

#include <algorithm>
#include <compare>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mtap {
  template <size_t S>
  struct fixed_string {
    char _m_data[S + 1];

  private:
    constexpr fixed_string() = default;

  public:
    static constexpr size_t npos = std::numeric_limits<size_t>::max();

    constexpr fixed_string(const char (&str)[S + 1]) {
      if (str[S] != '\0')
        throw std::invalid_argument("Argument is not null-terminated");
      std::copy(std::begin(str), std::end(str), std::begin(_m_data));
    }

    constexpr size_t size() const { return S; }

    constexpr char& operator[](size_t i) { return _m_data[i]; }
    constexpr const char& operator[](size_t i) const { return _m_data[i]; }

    constexpr char& at(size_t i) {
      if (i >= S)
        throw std::out_of_range("Index is out of range");
      return _m_data[i];
    }
    constexpr const char& at(size_t i) const {
      if (i >= S)
        throw std::out_of_range("Index is out of range");
      return _m_data[i];
    }

    constexpr operator std::string_view() const {
      return std::string_view(cbegin(), cend());
    }

    constexpr char* begin() { return _m_data; }
    constexpr char* end() { return _m_data + S; }
    constexpr const char* begin() const { return _m_data; }
    constexpr const char* end() const { return _m_data + S; }
    constexpr const char* cbegin() const { return _m_data; }
    constexpr const char* cend() const { return _m_data + S; }

    template <size_t B, size_t L = npos>
    constexpr auto substr() {
      if constexpr (L == npos) {
        fixed_string<S - B> res;
        std::copy(this->begin() + B, this->end(), res.begin());
        return res;
      }
      else {
        static_assert(B + L <= S, "Substring indices out of bounds");
        fixed_string<L> res;
        std::copy_n(this->begin() + B, L, res.begin());
        return res;
      }
    }
  };

  template <size_t Sa, size_t Sb>
  constexpr bool operator==(fixed_string<Sa> a, fixed_string<Sb> b) {
    if constexpr (Sa != Sb)
      return false;
    else
      return std::equal(a.begin(), a.end(), b.begin());
  }
  template <size_t Sa, size_t Sb>
  constexpr std::strong_ordering operator<=>(
    fixed_string<Sa> a, fixed_string<Sb> b) {
    return std::lexicographical_compare_three_way(
      a.begin(), a.end(), b.begin(), b.end());
  }

  template <size_t S>
  fixed_string(const char (&str)[S]) -> fixed_string<S - 1>;

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

  namespace details {
    template <size_t I, fixed_string S>
    struct string_index_leaf {
      static constexpr fixed_string value = S;
    };

    template <class ISeq, fixed_string... Ss>
    struct string_index_root;

    template <size_t... Is, fixed_string... Ss>
    struct string_index_root<std::index_sequence<Is...>, Ss...> :
      string_index_leaf<Is, Ss>... {};

    template <size_t I, fixed_string S>
    string_index_leaf<I, S> deduce_string_index(string_index_leaf<I, S>);

  }  // namespace details

  template <size_t I, class SSeq>
  struct string_sequence_element;

  template <size_t I, fixed_string... Ss>
  struct string_sequence_element<I, string_sequence<Ss...>> {
    static constexpr fixed_string value = decltype(details::deduce_string_index<I>(
      details::string_index_root<
        std::make_index_sequence<sizeof...(Ss)>, Ss...> {}))::value;
  };

  template<size_t I, class SSeq>
  inline constexpr fixed_string string_sequence_element_v = string_sequence_element<I, SSeq>::value;
}  // namespace mtap
#endif