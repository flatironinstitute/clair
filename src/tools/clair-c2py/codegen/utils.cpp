#include "./utils.hpp"
#include "../../../utility/string_tools.hpp"
#include "../../../utility/logger.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <fmt/format.h>

namespace codegen {

  std::string cpp_to_py_types(std::vector<std::string> const &cpp_types) {
    if (cpp_types.empty()) return std::string{};
    return fmt::format(R"RAW(c2py::python_typename<{}>())RAW", util::join(cpp_types, ">(), c2py::python_typename<"));
  };

  // ------------------------------

  std::string id_hash(std::string_view input) {
    static std::unordered_map<std::string, std::string> input_to_id; // idempotent: same input → same id
    static std::unordered_map<std::string, std::string> id_to_input; // collision registry: id → owner

    auto key = std::string{input};
    if (auto it = input_to_id.find(key); it != input_to_id.end()) return it->second;

    uint32_t h = 2166136261u; // FNV-1a, 32-bit
    for (unsigned char c : input) { h ^= c; h *= 16777619u; }
    auto base = fmt::format("{:08x}", h);

    auto id = base;
    for (int i = 1; !id_to_input.emplace(id, key).second; ++i) id = fmt::format("{}_{}", base, i);

    if (id != base)
      util::logger::warning()(
         fmt::format("clair codegen: id_hash collision on '{}' between '{}' and '{}'. Assigned '{}' to the latter.", base, id_to_input[base], key, id));

    input_to_id[key] = id;
    return id;
  }

} // namespace codegen