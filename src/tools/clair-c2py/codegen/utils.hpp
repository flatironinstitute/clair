#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace codegen {

  // Given a vector of C++ types, generate code that produces a vector of corresponding Python types.
  std::string cpp_to_py_types(std::vector<std::string> const &cpp_types);

  // Stable 8-hex-char identifier suffix derived from `input` (32-bit FNV-1a).
  // Idempotent: same input always returns the same id within one process run.
  // On collision two distinct inputs map to the same base hash, the later one
  // gets a numeric suffix (_1, _2, …) to stay unique.
  std::string id_hash(std::string_view input);

} // namespace codegen