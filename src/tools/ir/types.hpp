#pragma once
#include "utility/string_tools.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <vector>

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

    // Documentation extracted from C++ source comments at traversal time
    str_t doc_brief;
    str_t doc_details;
    std::vector<std::pair<str_t, str_t>> params_doc; // {param_name, doc_string}
    str_t return_doc;

    str_t deprecated_params_annotation; // raw "old:new,..." annotation string, or empty

    FunctionDecl() = default;
    explicit FunctionDecl(clang::FunctionDecl const &f);

    // Helpers for codegen — pre-computed from params/targs
    str_t param_names_str() const;          // "a,b"
    str_t param_types_str() const;          // "A,B"
    str_t params_with_types_str() const;    // "A a,B b"
    str_t params_with_defaults_str() const; // ' "a"_a = 2, "b"'
    str_t targs_str() const;               // "int,double"
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
  };

  // Language-agnostic representation of an enum declaration.
  struct EnumDecl {
    str_t qualified_name;
    std::vector<str_t> enumerators; // name of each enumerator

    EnumDecl() = default;
    explicit EnumDecl(clang::EnumDecl const &e);
  };

} // namespace ir
