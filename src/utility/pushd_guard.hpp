#pragma once
#include <filesystem>

// Usage:
// RAII pushd like.
// pushd_guard_t(some path) changes directory
// returns to the previous directory when this object is destroyed (end of scope)
//
namespace util {

  class pushd_guard_t {

    std::filesystem::path parent_dir;

    public:
    pushd_guard_t(std::filesystem::path p, bool create = false) {
      parent_dir = std::filesystem::current_path();
      if (!p.empty()) {
        if (create) std::filesystem::create_directories(p);
        std::filesystem::current_path(p);
      }
    }
    pushd_guard_t(pushd_guard_t const &)            = delete;
    pushd_guard_t(pushd_guard_t &&)                 = default;
    pushd_guard_t &operator=(pushd_guard_t const &) = delete;
    pushd_guard_t &operator=(pushd_guard_t &&)      = delete;

    ~pushd_guard_t() { std::filesystem::current_path(parent_dir); }
  };

} // namespace util
