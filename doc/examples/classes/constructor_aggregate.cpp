// Synthesized constructor from parameter struct
#include <c2py/c2py.hpp>
#include <string>

struct params {
  // The width
  int width;

  // The height
  int height;

  // Some boolean flag
  bool fullscreen = false;

  // A title
  std::string title = "Application";
};

#include "constructor_aggregate.wrap.cxx"
