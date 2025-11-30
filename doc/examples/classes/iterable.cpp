// Iterable objects
#include <c2py/c2py.hpp>
#include <vector>

namespace mylib {

  class range {
    std::vector<int> values_;

    public:
    range(int n) {
      for (int i = 0; i < n; ++i) values_.push_back(i);
    }

    // Provide begin() and end() for iteration
    auto begin() const { return values_.begin(); }
    auto end() const { return values_.end(); }
  };

} // namespace mylib
