#include "custom_compilation_database.hpp"

namespace clu {

  std::vector<clang::tooling::CompileCommand> custom_compilation_database::getCompileCommands(llvm::StringRef FilePath) const {
    std::vector<clang::tooling::CompileCommand> result;
    for (const auto &C : Commands)
      if (C.Filename == FilePath) result.push_back(C);
    return result;
  }

  std::vector<std::string> custom_compilation_database::getAllFiles() const {
    std::vector<std::string> files;
    for (const auto &C : Commands) files.push_back(C.Filename);
    return files;
  }

} // namespace clu
