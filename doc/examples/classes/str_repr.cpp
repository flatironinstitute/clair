// String representation
#include <c2py/c2py.hpp>
#include <string>
#include <sstream>

namespace mylib {

  class person {
    std::string name_;
    int age_;

    public:
    person(std::string name, int age) : name_(std::move(name)), age_(age) {}

    std::string name() const { return name_; }
    int age() const { return age_; }
  };

} // namespace mylib

// Define Python string representations
namespace c2py {
  template <> inline std::string to_string(mylib::person const &p) { return p.name() + " (" + std::to_string(p.age()) + " years old)"; }

  template <> inline std::string repr(mylib::person const &p) { return "Person('" + p.name() + "', " + std::to_string(p.age()) + ")"; }
} // namespace c2py
