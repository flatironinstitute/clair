// Comparison operators
#include <c2py/c2py.hpp>
#include <tuple>

namespace mylib {

  class version {
    int major_, minor_, patch_;

    public:
    version(int major, int minor, int patch) : major_(major), minor_(minor), patch_(patch) {}

    // C++20 spaceship operator
    auto operator<=>(version const &other) const { return std::tie(major_, minor_, patch_) <=> std::tie(other.major_, other.minor_, other.patch_); }

    bool operator==(version const &other) const { return major_ == other.major_ && minor_ == other.minor_ && patch_ == other.patch_; }
  };

} // namespace mylib
