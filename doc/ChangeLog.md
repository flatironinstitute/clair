(changelog)=

# Changelog

## Version 0.9.0

clair/c2py 0.9.0 is a major release that matures clair into a standalone Clang tool for
automatic C++ → Python bindings generation, the result of a long development effort since
the initial 0.1 series. The headline changes are:

* **Standalone Clang tool**: `clair-c2py` is now a standalone Clang LibTooling tool rather
  than a compiler plugin, which simplifies deployment and CMake integration.
* **TOML configuration**: each module is configured through an optional `<module>.toml`
  file (`package_name`, `documentation`, `namespaces`, `match_names`, `reject_names`,
  `match_files`, `wrap_no_arg_methods_as_properties`, `exclude_system_headers`); use
  `--gen-default-config` / `--update-config` to create and refresh it.
* **Automatic documentation**: C++ Doxygen / `///` comments are transcribed into
  NumPy-style Python docstrings, including dispatch documentation for overloaded functions.
* **Wider C++ coverage**: class and method renaming, properties, reference return values, a
  broad set of operators, enums (including nested and cross-module), and function / class
  templates with parameter packs.
* **Cross-module bindings**: namespace support, generated `<module>.wrap.hxx` companion
  headers, and `c2py_module` declarations spread across header files.
* **Diagnostics & logging**: `CLAIR_VERBOSE` logging with a per-module log file,
  `CLAIR_SKIP_CLANG_FORMAT`, and converter-header suggestions in convertibility errors.
* The minimum required LLVM/Clang version is now **20** (tested through LLVM 22).
* Fixes several issues.

We thank all contributors: Thomas Hahn, Olivier Parcollet, Nils Wentzell

Find below an itemized list of changes in this release.

### General
* `clair-c2py` is now a standalone Clang tool instead of a compiler plugin.
* Add a per-module TOML configuration with validation, plus the `--gen-default-config` and `--update-config` options to create and refresh it.
* Add the `exclude_system_headers` configuration option.
* Add `--generate-depfile` to emit a CMake-style depfile of the headers the generated bindings depend on.
* Add `CLAIR_VERBOSE` logging with a per-module log file, and `CLAIR_SKIP_CLANG_FORMAT` to skip the clang-format pass.
* Suggest the relevant converter headers in convertibility error messages.
* Support class and method renaming (#11).
* Support properties through `C2PY_PROPERTY_GET` / `C2PY_PROPERTY_SET` and wrapping no-argument methods as properties.
* Support returning references, including references to members (of members) at any depth (#14).
* Wrap enums, including nested enums under namespace filtering (#17) and cross-module enum use.
* Wrap function and class templates through explicit instantiations in the `c2py_module` namespace, including template parameter packs (#19) and template members with default arguments (#22).
* Allow `c2py_module` declarations in included files.
* Allow friend functions to be wrapped (#21).
* Allow `PyObject*` as a function return type.
* Improve handling of default arguments, including unqualified (#15) and unresolved types.
* Improve the `--version` output and document all options in the usage text.
* Bump the minimum required LLVM/Clang version to 20 and add compatibility through LLVM 22.
* Make clair fetchable from GitHub for dependent repositories (#25).

### operators
* Wrap operators: function call, subscript (`operator[]`), `operator<<`, unary minus, left shift, and in-place operators (#20).
* Add a return-type check for `operator<<` in `analyze_operator`.

### codegen
* Add `C2PY_DEPRECATED_PARAMETER_NAME` annotation support.

### c2py
* Skip generating the `.wrap.hxx` file when there is no wrap information to emit.

### clu
* Cache the result of concept evaluation.

### doc
* Transcribe C++ comments into NumPy-style Python docstrings for functions and classes, preserving parameter order and handling multiple overloads.
* Restructure and expand the documentation (getting started, CMake integration, code annotations, TOML options, function/class template examples).

### cmake
* Overhaul the CMake files and project structure.
* Use `GNUInstallDirs` more consistently (#3).
* On macOS, link against the libc++ provided by the LLVM installation.
* Add a `Findsanitizer` module.
