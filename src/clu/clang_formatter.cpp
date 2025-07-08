#include "./clang_formatter.hpp"
#include <filesystem>
#include <fstream>
#include "utility/string_tools.hpp"
#include "clang/Tooling/Tooling.h"
#include "clang/Format/Format.h"

namespace clf = clang::format;
namespace clt = clang::tooling;

class pushd_guard_t { // NOLINT
  std::filesystem::path parent_dir;

  public:
  pushd_guard_t(std::filesystem::path p, bool create = false) {
    parent_dir = std::filesystem::current_path();
    if (!p.empty()) {
      if (create) std::filesystem::create_directories(p);
      std::filesystem::current_path(p);
    }
  }
  ~pushd_guard_t() { std::filesystem::current_path(parent_dir); }
};

// -----------------------------------------

inline std::string read_txt_file(std::filesystem::path const &f) {
  if (not std::filesystem::exists(f)) throw std::runtime_error("File " + std::string{f} + " does not exist");
  std::string res;
  std::ifstream in(f);
  std::getline(in, res, std::string::traits_type::to_char_type(std::string::traits_type::eof()));
  return res;
}

// ---------------------
std::filesystem::path find_clang_format() {
  auto __guard_cwd = pushd_guard_t{"."};
  while (1) {
    if (std::filesystem::exists(".clang-format")) return str_t{std::filesystem::current_path()} + '/' + ".clang-format";
    if (std::filesystem::current_path() == "/") return {};
    std::filesystem::current_path("..");
  }
  return {}; // for warnings
}

// ---------------------

class clang_formatter_t {
  clf::FormatStyle style;

  // ---------------------

  public:
  clang_formatter_t(int n_column_max = 120, bool is_verbose = true) {
    style                           = clf::getLLVMStyle(); // CXX is default clang::Language::CXX);
    auto clang_format_file_location = find_clang_format();

    if (not clang_format_file_location.empty()) {
      if (is_verbose) std::cerr << "-- Found clang format at " << clang_format_file_location << std::endl;
      auto clform = read_txt_file(clang_format_file_location);
      if (clform.empty()) return;
      std::error_code err = clf::parseConfiguration(clform, &style);
      if (err) { std::cerr << "ERROR reading clang format"; } // continue with LLVM Format
    } else if (is_verbose)
      std::cerr << "-- Clang format not found. Using LLVM default format" << std::endl;

    style.ColumnLimit = n_column_max;
  }

  // --------------------

  std::string operator()(std::string const &code, int col_limit = 0) const {

    std::vector<clt::Range> the_ranges{{0, static_cast<unsigned int>(code.size())}};

    clt::Replacements repl                       = clf::reformat(style, code, the_ranges);
    llvm::Expected<std::string> code_reformatted = clt::applyAllReplacements(code, repl);

    if (code_reformatted)
      return code_reformatted.get();
    else
      return {};
  }
};

// -------------------- clang-format function

str_t clu::clang_format(str_t const &code) {
  static clang_formatter_t cl;
  return cl(code);
}
