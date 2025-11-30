// Const methods
#include <c2py/c2py.hpp>
#include <string>

namespace mylib {

  class book {
    std::string title_;
    std::string author_;

    public:
    book(std::string title, std::string author) : title_(std::move(title)), author_(std::move(author)) {}

    // Const accessors
    std::string const &title() const { return title_; }
    std::string const &author() const { return author_; }

    // Non-const mutator
    void set_title(std::string title) { title_ = std::move(title); }
  };

} // namespace mylib
