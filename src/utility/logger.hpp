#pragma once
#include <fstream>
#include <iostream>
#include <ostream>
#include <utility>
#include <fmt/core.h>
#include <fmt/format.h>
#include <llvm/ADT/StringRef.h>

#include "string_tools.hpp"

template <> struct fmt::formatter<llvm::StringRef> : fmt::formatter<std::string_view> {
  auto format(llvm::StringRef s, fmt::format_context &ctx) const {
    return fmt::formatter<std::string_view>::format({s.data(), s.size()}, ctx);
  }
};

namespace util {
  class logger {

    inline static int s_verbose          = 0;  // threshold from CLAIR_VERBOSE env var
    inline static std::string s_log;           // log file path, set once per run
    inline static std::ofstream s_log_stream;  // always-on file output

    bool active_    = false;
    int  verbosity_ = 1;
    std::string intro_;
    std::string intro_spaces_;
    std::string head_line_;
    std::string head_line_spaces_;

    void emit_(std::ostream &os, const char *mess) const {
      auto spl = split(std::string{mess}, '\n');
      os << head_line_ << intro_;
      int c = 0;
      for (auto const &x : spl) {
        if (c++ > 0) os << '\n' << head_line_spaces_ << intro_spaces_;
        os << x;
      }
      os << '\n';
    }

    public:

    logger() = default;

    logger(str_t headline, std::string introduction = {}, int verbosity = 1)
       : active_{true}, verbosity_{verbosity}, intro_{std::move(introduction)},
         intro_spaces_(intro_.size(), ' '), head_line_{str_t{headline}},
         head_line_spaces_(head_line_.size(), ' ') {}

    logger(logger l, str_t const &additional_head) : logger(std::move(l)) { head_line_ += additional_head; }

    void operator()(std::string const &mess) const { this->operator()(mess.c_str()); }

    void operator()(const char *mess) const {
      if (!active_) return;
      if (verbosity_ <= s_verbose) emit_(std::cerr, mess);
      if (s_log_stream.is_open()) emit_(s_log_stream, mess);
    }

    template <typename... T> void operator()(fmt::format_string<T...> const &s, T &&...args) const { operator()(fmt::format(s, args...)); }

    static void set_verbose(int v) { s_verbose = v; }
    static void set_log(std::string path) { s_log = std::move(path); s_log_stream.open(s_log); }

    static logger error()   { return {"-- ", "\033[1;31merror: \033[0m",   0}; }
    static logger warning() { return {"-- ", "\033[1;35mwarning: \033[0m", 0}; }
    static logger debug()   { return {"-- ", "\033[1;31mDEBUG: \033[0m",   1}; }
  };

} // namespace util
