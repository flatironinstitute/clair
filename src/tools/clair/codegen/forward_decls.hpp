#pragma once

#include <string>
#include "../module_info.hpp"

namespace codegen {

  /// Generate the forward-declaration block for a Fortran module.
  ///
  /// Emits an  extern "C" { }  block with the Flang-mangled symbols, followed by
  /// namespace-grouped inline wrappers and struct forward declarations so that the
  /// rest of the generated wrapper can call  mymod::foo(a, b)  as usual:
  ///
  ///   extern "C" { float _QMmymodPadd(float *, float *); }
  ///   namespace mymod {
  ///     struct point_t;
  ///     inline float add(float _p0, float _p1) { return _QMmymodPadd(&_p0, &_p1); }
  ///   }
  ///
  /// NOTE: CHARACTER arguments use a complex ABI (hidden length argument); for
  /// correct interop add BIND(C) + VALUE to the Fortran declarations.
  std::string gen_forward_decls(module_info_t const &m);

} // namespace codegen
