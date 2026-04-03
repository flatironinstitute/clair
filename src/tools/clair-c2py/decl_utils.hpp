#pragma once
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "utility/logger.hpp"
#include "utility/string_tools.hpp"
#include <optional>
#include "llvm/Support/Regex.h"

/// Check whether decl should be skipped due to a c2py_ignore annotation
/// or a qualified name matching reject_names. Logs the reason if log is provided.
bool should_reject(clang::Decl const *decl, std::optional<llvm::Regex> const &reject_names, util::logger const *log = nullptr);

/// Return the Python name for a class, honoring c2py_rename or falling back to camelCase.
str_t get_python_name(clang::CXXRecordDecl const *cls);

/// Return the Python name for a function, honoring c2py_rename or keeping the C++ name.
str_t get_python_name(clang::FunctionDecl const *f);
