#pragma once
#include <string>
#include <regex>
#include <sstream>
#include <locale> // std::locale, std::toupper

// ------- Simple general alias : more readable

using str_t = std::string;

namespace util {

  // ------- trim a string: trim (both sides), rtrim (right only), ltrim

  const str_t whitespace_and_star = " \n\r\t\f\v";

  inline str_t trim(const str_t &s, str_t const &delim = whitespace_and_star) {
    size_t start = s.find_first_not_of(delim);
    size_t end   = s.find_last_not_of(delim);
    return (end == str_t::npos) ? "" : s.substr(start, end + 1 - start);
  }

  inline str_t ltrim(const str_t &s, str_t const &delim = whitespace_and_star) {
    size_t start = s.find_first_not_of(delim);
    return (start == str_t::npos) ? "" : s.substr(start);
  }

  inline str_t rtrim(const str_t &s, str_t const &delim = whitespace_and_star) {
    size_t end = s.find_last_not_of(delim);
    return (end == str_t::npos) ? "" : s.substr(0, end + 1);
  }

  // -------  split

  // FIXME C++20 replace by ranges
  template <typename F, typename StringChar> void lazy_split(str_t const &str, F f, StringChar const &delim = ' ', int nlim = -1) {
    std::size_t current = str.find(delim), previous = 0;
    for (int i = 0; (current != str_t::npos) and (i != nlim); ++i) {
      f(str.substr(previous, current - previous));
      if constexpr (std::is_same_v<std::decay_t<std::remove_pointer_t<std::remove_extent_t<StringChar>>>, char>) {
        previous = current + 1;
      } else {
        previous = current + delim.size();
      }
      current = str.find(delim, previous);
    }
    f(trim(str.substr(previous, current - previous)));
  }

  inline std::vector<str_t> split(str_t const &str, char delim = ' ') {
    std::size_t current = str.find(delim), previous = 0;
    std::vector<str_t> res;
    while (current != str_t::npos) {
      res.push_back(str.substr(previous, current - previous));
      previous = current + 1;
      current  = str.find(delim, previous);
    }
    res.push_back(trim(str.substr(previous, current - previous)));
    return res;
  }

  inline std::vector<str_t> split(str_t const &str, str_t delim, int nlim = -1) {
    std::size_t current = str.find(delim), previous = 0;
    std::vector<str_t> res;
    for (int i = 0; (current != str_t::npos) and (i != nlim); ++i) {
      res.push_back(str.substr(previous, current - previous));
      previous = current + delim.size();
      current  = str.find(delim, previous);
    }
    res.push_back(trim(str.substr(previous, current - previous)));
    return res;
  }
  // -------  join ---------

  // join the mapping of F onto the range R, with separator sep.
  // f must return a string
  template <typename It, typename F, typename Sep> str_t join(It p, It e, F f, Sep sep, bool add_last = false) {
    if (p == e) return {}; // empty R
    std::stringstream result;
    result << f(*p);
    for (++p; p != e; ++p) result << sep << f(*p);
    if (add_last) result << sep;
    return result.str();
  }

  // join the mapping of F onto the range R, with separator sep.
  // f must return a string
  template <typename R, typename F, typename Sep> str_t join(R const &r, F f, Sep sep, bool add_last = false) {
    return join(r.begin(), r.end(), f, sep, add_last);
  }

  // join with f as identity by default
  template <typename R, typename Sep> str_t join(R const &r, Sep sep, bool add_last = false) {
    return join(
       r, [](auto &&x) { return x; }, sep, add_last);
  }

  // -------  indent_string ---------

  inline str_t indent_string(str_t const &s, str_t const &indent) { return indent + join(split(trim(s), "\n"), "\n" + indent); }

  // -------  indent_string ---------

  inline bool starts_with(str_t const &s, str_t const &start) {
    return (s.rfind(start, 0) == 0);
    //NB : C++20 has it in std::string.
    // Here is a simple use of rfind,  Cf https://stackoverflow.com/questions/1878001/how-do-i-check-if-a-c-stdstring-starts-with-a-certain-string-and-convert-a
  }

  // -------  CamelCase hash
  // change a class name into CamelCase, for default Python naming

  inline str_t camel_case(str_t const &c_type) {

    auto v = split(c_type, '_');
    std::locale loc;
    return join(
       v,
       [&loc](auto s) {
         s[0] = std::toupper(s[0], loc);
         return s;
       },
       "");
  }

  // -------  hash

  // A simple *reproducible* hash for a string. Same function as Java std lib
  // std::hash is not reproducible !!
  inline uint64_t hash_string_to_hex(std::string const &s) {
    uint64_t r = 0;
    long size  = long(s.size());
    for (long i = 0; i < size; i++) r = r * 31 + s[i]; //NOLINT
    return r;
  }

  // put an in into a string in hex format
  inline str_t to_string_hex(uint64_t x) {
    std::stringstream fs;
    fs << std::hex << x;
    return fs.str();
  }

  // reverse from to_string_hex : string in hex format -> int
  inline uint64_t from_string_hex(str_t const &x) {
    uint64_t r = 0;
    std::istringstream fs(x);
    fs >> std::hex >> r;
    return r;
  }

} // namespace util
