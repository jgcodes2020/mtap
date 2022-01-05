#ifndef _MTAP_FIXED_STRING_HPP_
#define _MTAP_FIXED_STRING_HPP_

#include <algorithm>
#include <compare>
#include <cstddef>
#include <string_view>
#include <stdexcept>

namespace mtap {
  template <size_t S>
  struct fixed_string {
    char _m_data[S + 1];

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
  };

  template <size_t Sa, size_t Sb>
  constexpr bool operator==(fixed_string<Sa> a, fixed_string<Sb> b) {
    if constexpr (Sa != Sb)
      return false;
    else
      return std::equal(a.begin(), a.end(), b.begin());
  }
  
  template <size_t Sa, size_t Sb>
  constexpr std::strong_ordering operator<=>(fixed_string<Sa> a, fixed_string<Sb> b) {
    return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end());
  }

  template <size_t S>
  fixed_string(const char (&str)[S]) -> fixed_string<S - 1>;
}
#endif