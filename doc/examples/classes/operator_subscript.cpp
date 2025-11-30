// operator[] wrapping
#include <c2py/c2py.hpp>
#include <vector>

namespace mylib {

  class container {
    std::vector<int> data_;

    public:
    container(size_t size) : data_(size, 0) {}

    // Subscript operator for reading
    int operator[](size_t index) const { return data_[index]; }

    // Subscript operator for writing
    int &operator[](size_t index) { return data_[index]; }

    size_t size() const { return data_.size(); }
  };

} // namespace mylib
