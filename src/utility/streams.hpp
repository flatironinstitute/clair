#pragma once
#include <ostream>
#include <streambuf>
#include <string>

namespace triqs {

  /// FIXME : MERGE : replace triqs::report_steam, -> merge + alias
  /**
   * @brief A stream wrapper that conditionally forwards output to an underlying std::ostream
   *        based on a verbosity level.
   *
   * The stream_with_verbosity class allows selective printing to an output stream
   * depending on a user-defined verbosity threshold.
   *
   * Usage:
   * @code
   * stream_with_verbosity out(std::cout, verbosity);
   * out << "This message is printed if verbosity > 0." << std::endl;
   * out(3) << "This will print only if verbosity >= 3." << std::endl;
   * @endcode
   */
  class ostream_with_verbosity {

    std::ostream *out_;
    int verbosity_;

    public:
    /**
     * @brief Construct a stream_with_verbosity.
     * @param out Reference to the output stream (e.g., std::cout).
     * @param verbosity Initial verbosity level (default is 1).
     */
    ostream_with_verbosity(std::ostream &out, int verbosity = 1) : out_(&out), verbosity_(verbosity) {}

    /**
     * @brief Create a derived stream with adjusted verbosity.
     * @param n The minimum verbosity required for output.
     * @return A new stream_with_verbosity with reduced verbosity.
     */
    ostream_with_verbosity operator()(int n) const { return {*out_, verbosity_ - n + 1}; }

    /**
     * @brief Stream insertion operator for generic types.
     * @tparam T Any type supporting operator<< with std::ostream.
     * @param value The value to write to the stream.
     * @return Reference to this stream_with_verbosity instance.
     */
    template <typename T> ostream_with_verbosity &operator<<(T const &value) {
      if (verbosity_ > 0) *out_ << value;
      return *this;
    }

    /**
     * @brief Stream insertion for manipulators like std::endl.
     * @param manip The manipulator function (e.g., std::endl).
     * @return Reference to this stream_with_verbosity instance.
     */
    ostream_with_verbosity &operator<<(std::ostream &(*manip)(std::ostream &)) {
      if (verbosity_ > 0) manip(*out_);
      return *this;
    }
  };

  // =============================================================================

  /**
   * @class indented_ostream
   * @brief A custom output stream that automatically indents each new line by a specified number of spaces.
   * 
   * This class is useful for formatting output with consistent indentation. It wraps around an existing 
   * `std::ostream` and ensures that every new line starts with a specified number of spaces.
   * 
   * Example usage:
   * @code
   *   auto out = triqs::indented_ostream{std::cout, 3}; // Indent all lines with 3 spaces
   *   out << "Hello, world!" << std::endl;
   *   out << "Indented text." << std::endl;
   * @endcode
   * 
   * @note This class inherits from `std::ostream` and uses a custom stream buffer to handle indentation.
   */
  class indented_ostream : public std::ostream {

    class indented_streambuf : public std::streambuf {
      std::streambuf *dest;
      std::string head;
      bool at_line_start = true;

      public:
      indented_streambuf(std::streambuf *dest, int indent) : dest(dest), head(indent, ' ') {}

      protected:
      int_type overflow(int_type c) override {
        if (c == EOF) return !EOF;
        if (at_line_start && c != '\n') dest->sputn(head.c_str(), long(head.size()));
        at_line_start = (c == '\n');
        return dest->sputc(c); //NOLINT
      }
      int sync() override { return dest->pubsync(); }
    };
    indented_streambuf buffer;

    public:
    /**
     * @brief Constructor
     * 
     * @param os The underlying std::ostream to write into.
     * @param indent The number of spaces to use for indentation.
     */
    indented_ostream(std::ostream &os, int indent) : std::ostream(&buffer), buffer(os.rdbuf(), indent) {}
  };

} // namespace triqs