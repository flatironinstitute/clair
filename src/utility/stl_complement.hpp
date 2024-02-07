// Stuff that SHOULD be in C++ standard library !
#pragma once
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>

namespace util {

  // --------- Check T is an instantiation of a template -------------

  template <template <typename...> class TMPLT, typename T> constexpr bool is_instantiation_of_v                        = false;
  template <template <typename...> class TMPLT, typename... U> constexpr bool is_instantiation_of_v<TMPLT, TMPLT<U...>> = true;

  // --------- Overloading several lambda into one object -----------------

  template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
  };
  template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  // --------- Read an entire text file into a string -------------------

  inline std::string read_txt_file(std::filesystem::path const &f) {
    if (not std::filesystem::exists(f)) throw std::runtime_error("File " + std::string{f} + " does not exist");
    std::string res;
    std::ifstream in(f);
    std::getline(in, res, std::string::traits_type::to_char_type(std::string::traits_type::eof()));
    return res;
  }

} // namespace util
