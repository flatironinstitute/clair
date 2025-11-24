#pragma once
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include <vector>
#include <string>

namespace clu {

  /**
   * @brief Custom implementation of CompilationDatabase.
   * 
   * This class provides a simple in-memory compilation database that stores
   * a vector of compile commands. It is useful when you need to:
   * - Modify compile commands from an existing database
   * - Create a compilation database programmatically
   * - Work with a fixed set of compile commands
   */
  class custom_compilation_database : public clang::tooling::CompilationDatabase {
    std::vector<clang::tooling::CompileCommand> Commands;

    public:
    /**
     * @brief Construct a custom compilation database from a vector of compile commands.
     * @param Cmds The compile commands to store in the database.
     */
    custom_compilation_database(std::vector<clang::tooling::CompileCommand> Cmds) : Commands(std::move(Cmds)) {}

    /**
     * @brief Get compile commands for a specific file.
     * @param FilePath The path to the file.
     * @return Vector of compile commands matching the given file path.
     */
    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const override;

    /**
     * @brief Get all files in the compilation database.
     * @return Vector of file paths.
     */
    std::vector<std::string> getAllFiles() const override;

    /**
     * @brief Get all compile commands.
     * @return Vector of all compile commands stored in the database.
     */
    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override { return Commands; }
  };

} // namespace clu
