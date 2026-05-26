#pragma once
#include "utility/string_tools.hpp"
#include <vector>

// Forward declarations
namespace clang {
  class ASTContext;
  class CXXRecordDecl;
  class EnumDecl;
  class FieldDecl;
  class FunctionDecl;
  class QualType;
} // namespace clang

namespace Fortran::semantics {
  class Symbol;
} // namespace Fortran::semantics

namespace ir {

  // Represents a C++ qualified type with its fully-qualified name.
  struct QualType {
    str_t name;       // fully qualified type name (e.g. "ns::MyClass const &")
    bool is_const = false;

    QualType() = default;
    QualType(clang::QualType t, clang::ASTContext &ctx);

    bool isConstQualified() const { return is_const; }
  };

  // Represents a function parameter with name, type, and optional default value.
  struct ParamVarDecl {
    str_t name;        // parameter name (synthetic "_p_N" if unnamed)
    QualType type;     // fully qualified parameter type
    bool has_default = false;
    str_t default_val; // textual default value (e.g. "2", "A{}", "ns::val")
    // Fortran-specific: set when the dummy argument carries the VALUE attribute.
    // Affects the extern "C" signature: VALUE → pass by value, otherwise → pass by pointer.
    bool is_fortran_value = false;

    ParamVarDecl() = default;
    // Construct from a Fortran dummy-argument symbol.
    ParamVarDecl(Fortran::semantics::Symbol const &, std::string const &module_name);
  };

  // Language-agnostic representation of a function or method declaration.
  // Captures all data needed by codegen at AST traversal time.
  struct FunctionDecl {
    str_t qualified_name;     // fully qualified call name (e.g. "ns::Cls::foo")
    str_t simple_name;        // bare name (e.g. "foo")
    QualType return_type;
    std::vector<ParamVarDecl> params;
    std::vector<str_t> targs; // template argument FQN strings (empty if non-template)

    bool is_method        = false; // is a member function (CXXMethodDecl)
    bool is_static        = false;
    bool is_const_method  = false; // has const qualifier
    bool is_constructor   = false;
    bool is_inline_friend = false; // defined inside a class body, only callable via ADL
    bool is_template_instantiation = false;
    bool rewrite = true; // use lambda wrapper (vs. direct pointer cast)

    str_t parent_class_fqn; // FQN of class where this method is DEFINED (empty for free fns)

    // When non-empty, the actual external symbol name (extern "C" linkage).
    // Set by the Fortran traversal to the Flang-mangled name, e.g. "_QMmymodPfoo".
    // Empty for C++ functions — they use standard C++ name mangling via qualified_name.
    str_t linkage_name;

    // Documentation extracted from C++ source comments at traversal time
    str_t doc_brief;
    str_t doc_details;
    std::vector<std::pair<str_t, str_t>> params_doc; // {param_name, doc_string}
    str_t return_doc;

    str_t deprecated_params_annotation; // raw "old:new,..." annotation string, or empty

    FunctionDecl() = default;
    explicit FunctionDecl(clang::FunctionDecl const &f);
    // Construct from a Fortran module-level procedure (subroutine or function).
    FunctionDecl(Fortran::semantics::Symbol const &, std::string const &module_name);
    // Construct from a Fortran type-bound procedure (method).
    // binding_sym carries the binding name and NOPASS attribute;
    // actual_sym  carries the SubprogramDetails (return type, dummy args).
    FunctionDecl(Fortran::semantics::Symbol const &binding_sym,
                 Fortran::semantics::Symbol const &actual_sym,
                 std::string const &module_name,
                 std::string const parent_fqn,
                 bool is_nopass);

    // Helpers for codegen — backend-independent, implemented in types.cpp.
    str_t param_names_str() const;          // "a,b"
    str_t param_types_str() const;          // "A,B"
    str_t params_with_types_str() const;    // "A a,B b"
    str_t params_with_defaults_str() const; // ' "a"_a = 2, "b"'
    str_t targs_str() const;                // "int,double"
  };

  // Language-agnostic representation of a C++ class/struct.
  struct RecordDecl {
    str_t fully_qualified_name;
    bool is_aggregate = false; // aggregate type (no user-provided ctors, etc.)
    bool has_bases    = false; // has at least one base class

    str_t doc_brief;
    str_t doc_details;

    RecordDecl() = default;
    explicit RecordDecl(clang::CXXRecordDecl const &r);
    // Construct from a Fortran derived-type symbol.
    explicit RecordDecl(Fortran::semantics::Symbol const &, std::string const &module_name);

    str_t const &getFQN() const { return fully_qualified_name; }
    bool synthetize_init_from_pydict() const { return is_aggregate && !has_bases; }
  };

  // Language-agnostic representation of a data member.
  struct FieldDecl {
    str_t name;
    QualType type;
    bool is_const = false;
    bool has_in_class_initializer = false;
    str_t initializer_str;                  // source text of in-class initializer, or empty
    bool field_type_has_no_default_ctor = false; // type is a class without a default ctor

    str_t doc_brief;
    str_t doc_details;

    FieldDecl() = default;
    explicit FieldDecl(clang::FieldDecl const &d);
    // Construct from a Fortran derived-type component symbol.
    explicit FieldDecl(Fortran::semantics::Symbol const &, std::string const &module_name);
  };

  // Language-agnostic representation of an enum declaration.
  struct EnumDecl {
    str_t qualified_name;
    std::vector<str_t> enumerators; // name of each enumerator

    EnumDecl() = default;
    explicit EnumDecl(clang::EnumDecl const &e);
  };

} // namespace ir
