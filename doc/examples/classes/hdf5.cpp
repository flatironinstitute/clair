// HDF5 serialization
#include <c2py/c2py.hpp>
#include <h5/h5.hpp>
#include <vector>

namespace mylib {

  class matrix {
    std::vector<std::vector<double>> data_;

    public:
    matrix(size_t rows, size_t cols) : data_(rows, std::vector<double>(cols, 0.0)) {}

    // HDF5 write
    friend void h5_write(h5::group g, std::string const &name, matrix const &m) { h5::write(g, name, m.data_); }

    // HDF5 read
    friend void h5_read(h5::group g, std::string const &name, matrix &m) { h5::read(g, name, m.data_); }
  };

} // namespace mylib
