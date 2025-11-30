// Synthesized constructor from parameter struct
#include <c2py/c2py.hpp>
#include <string>

namespace mylib {

  /// Configuration parameters
  struct config_params {
    int width         = 800;
    int height        = 600;
    bool fullscreen   = false;
    std::string title = "Application";
  };

  class config {
    config_params params_;

    public:
    // Constructor synthesized from params
    config(config_params params) : params_(params) {}

    int width() const { return params_.width; }
    int height() const { return params_.height; }
  };

} // namespace mylib
