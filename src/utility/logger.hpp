#pragma once
#include <iostream>
#include <ostream>
#include <sstream>
#include <utility>
#include <fmt/core.h>
#include <fmt/format.h>

#include "string_tools.hpp"

namespace util {
  class logger {

    std::ostream *out = nullptr;
    bool activated    = false;
    std::string intro;
    std::string intro_spaces;
    std::string head_line;
    std::string head_line_spaces = std::string(head_line.size(), ' ');

    public:
    // ~logger() { *out << std::endl; }

    logger() = default;

    logger(std::ostream *out_, str_t headline, std::string introduction = {})
       : out(out_), activated(true), intro{std::move(introduction)}, intro_spaces(intro.size(), ' '), head_line(std::move(headline)) {}

    logger(logger l, str_t const &additional_head) : logger(std::move(l)) { head_line += additional_head; }

    void operator()(std::string const &mess) const { this->operator()(mess.c_str()); }

    void operator()(const char *mess) const {
      if (not activated) return;
      auto s = mess;
      *out << head_line << intro;
      lazy_split(
         s,
         [self = this, c = 0](auto &&s) mutable {
           if (c++ > 0) (*self->out) << '\n' << self->head_line_spaces << self->intro_spaces;
           (*self->out) << s;
         },
         '\n');
      *out << '\n';
    }

    template <typename... T> void operator()(fmt::format_string<T...> const &s, T &&...args) const { operator()(fmt::format(s, args...)); }

    void activate() { activated = true; }
    void deactivate() { activated = false; }

    static logger error() { return logger{&std::cerr, "-- ", "\033[1;31merror: \033[0m"}; }
    static logger warning() { return logger{&std::cerr, "-- ", "\033[1;35mwarning: \033[0m"}; }
    static logger debug() { return logger{&std::cerr, "-- ", "\033[1;31mDEBUG: \033[0m"}; }
  };

} // namespace util