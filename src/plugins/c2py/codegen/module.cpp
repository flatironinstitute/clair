#include "./module.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
#include <sstream>
using namespace fmt::literals;
#include <itertools/itertools.hpp>

#include "clu/fullqualifiedname.hpp"
#include "utility/logger.hpp"
#include "./fnt.hpp"
#include "./classes.hpp"
#include "../c2py_version.hpp"

using util::join;
static const struct {
  util::logger mod = util::logger{&std::cout, "-- ", "\033[1;32mModule: \033[0m"};
} logs;

// =========== module code template ==============

static constexpr auto module_code_tpl = R"RAW(
// C.f. https://numpy.org/doc/1.21/reference/c-api/array.html#importing-the-api
#define PY_ARRAY_UNIQUE_SYMBOL _cpp2py_ARRAY_API
#ifdef __clang__
// #pragma clang diagnostic ignored "-W#warnings"
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wcpp"
#endif

#define C2PY_VERSION_MAJOR {c2py_version_major}
#define C2PY_VERSION_MINOR {c2py_version_minor}

#include "c2py/c2py.hpp"
#include "{sourcefile}"
{Hdf5C2pyIncluder}

using c2py::operator"" _a;

// ==================== Wrapped classes =====================

{Modulehxxfile}

// ==================== enums =====================

{EnumDecls}

// ==================== module classes =====================

{ClassesDecls}

// ==================== module functions ====================

{FunctionDecls}

{FunctionDocs}
//--------------------- module function table  -----------------------------

static PyMethodDef module_methods[] = {{
{FunctionTable} {{nullptr, nullptr, 0, nullptr}}  // Sentinel
}};

//--------------------- module struct & init error definition ------------

//// module doc directly in the code or "" if not present...
/// Or mandatory ?
static struct PyModuleDef module_def = {{
   PyModuleDef_HEAD_INIT, "{modulename}", /* name of module */
   R"RAWDOC({moduledoc})RAWDOC",                        /* module documentation, may be NULL */
   -1, /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
   module_methods, NULL, NULL, NULL, NULL}};

//--------------------- module init function -----------------------------

extern "C" __attribute__((visibility("default"))) PyObject *PyInit_{modulename}() {{

  if (not c2py::check_python_version()) return NULL;

  // import numpy iff 'numpy/arrayobject.h' included
#ifdef Py_ARRAYOBJECT_H
  import_array();
#endif

  PyObject *m;

  if (PyType_Ready(&c2py::wrap_pytype<c2py::py_range>) < 0) return NULL;
  {PyTypeReadyDecls}

  m = PyModule_Create(&module_def);
  if (m == NULL) return NULL;

  auto &conv_table = *c2py::conv_table_sptr.get();

  conv_table[std::type_index(typeid(c2py::py_range)).name()] = &c2py::wrap_pytype<c2py::py_range>;
  {AddTypeObjectDecls}

  {ModuleInitFunction}
  
  {Hdf5RegistrationInit}
  {Hdf5Registration}

  return m;
}}
)RAW";

// =========== module code generation ==============

str_t codegen_module(module_info_t const &m) {

  auto full_module_name = m.package_name.empty() ? m.module_name : m.package_name + '.' + m.module_name;

  logs.mod(full_module_name);

  std::stringstream EnumDecls;
  for (auto const &enu : m.enums) {
    auto qname = enu->getQualifiedNameAsString();
    EnumDecls << fmt::format(
       R"RAW( 
       template <> std::map<{0}, str_t> c2py::enum_to_string<{0}> = {{ {1} }};)RAW",
       qname,
       join(
          enu->enumerators(), [&qname](auto &&val) { return fmt::format(R"RAW( {{ {0}::{1}, "{1}" }} )RAW", qname, val->getNameAsString()); }, ','));
  }

  std::stringstream FunctionDecls, FunctionTable, FunctionDocs;
  std::stringstream ClassesDecls, ClassesDocs, PyTypeReadyDecls, AddTypeObjectDecls;
  std::stringstream Hdf5C2pyIncluder, Hdf5RegistrationInit, Hdf5Registration;

  for (auto const &[cls_py_name, cls_info] : m.classes) {
    codegen_cls(ClassesDecls, ClassesDocs, cls_py_name, cls_info, full_module_name);

    auto cls_name = clu::get_fully_qualified_name(cls_info.ptr);
    if (cls_info.base != nullptr)
      PyTypeReadyDecls << fmt::format(R"RAW(   c2py::wrap_pytype<{0}>.tp_base = &c2py::wrap_pytype<{1}>; )RAW", cls_name,
                                      clu::get_fully_qualified_name(cls_info.base));
    PyTypeReadyDecls << fmt::format(R"RAW( if (PyType_Ready(&c2py::wrap_pytype<{}>) < 0) return NULL;)RAW", cls_name);

    AddTypeObjectDecls << fmt::format(R"RAW(  CLAIR_C2PY_ADD_TYPE_OBJECT({0}, "{1}"); )RAW", cls_name, cls_py_name);

    if (cls_info.has_hdf5) Hdf5Registration << fmt::format(R"RAW( register_h5_type<{0}>(register_class); )RAW", cls_name);
  }

  //
  if (not Hdf5Registration.str().empty()) {
    Hdf5C2pyIncluder << "#include <c2py/serialization/h5.hpp> \n";
    Hdf5RegistrationInit << R"RAW( 
        c2py::pyref module = c2py::pyref::module("h5.formats");
        if (not module) return nullptr;
        c2py::pyref register_class = module.attr("register_class");
)RAW";
  }

  for (auto const &[fpyname, overloads] : m.functions) //
    codegen::write_dispatch(FunctionDecls, FunctionTable, FunctionDocs, fpyname, overloads, nullptr, false);

  std::string ModuleInitFunction = (m.has_module_init ? " // Initialization of the module \n c2py_module::module_init();" : "");

  std::string r;
  try {
    r = fmt::format(module_code_tpl,                             //
                    "c2py_version_major"_a = c2py_version_major, //
                    "c2py_version_minor"_a = c2py_version_minor, //
                    "sourcefile"_a         = m.sourcefile,       //
                    "modulename"_a         = m.module_name,      //
                    "moduledoc"_a          = m.documentation,    //
                    //"package_name"_a            = (m.package_name.empty() ? m.package_name : m.package_name + '.' ), //
                    "EnumDecls"_a            = EnumDecls.str(),            //
                    "ClassesDecls"_a         = ClassesDecls.str(),         //
                    "FunctionDecls"_a        = FunctionDecls.str(),        //
                    "FunctionDocs"_a         = FunctionDocs.str(),         //
                    "FunctionTable"_a        = FunctionTable.str(),        //
                    "Modulehxxfile"_a        = codegen_hxx(m),             //
                    "PyTypeReadyDecls"_a     = PyTypeReadyDecls.str(),     //
                    "AddTypeObjectDecls"_a   = AddTypeObjectDecls.str(),   //
                    "Hdf5C2pyIncluder"_a     = Hdf5C2pyIncluder.str(),     //
                    "Hdf5RegistrationInit"_a = Hdf5RegistrationInit.str(), //
                    "Hdf5Registration"_a     = Hdf5Registration.str(),     //
                    "ModuleInitFunction"_a   = ModuleInitFunction);          //
  } catch (const fmt::format_error &ex) { std::cout << "ERROR" << ex.what() << std::endl; }

  return r;
}
// =========== hxx generation ==============

str_t codegen_hxx(module_info_t const &m) {
  std::stringstream hxx;
  hxx << fmt::format(R"RAW(
    #ifndef C2PY_HXX_DECLARATION_{0}_GUARDS
    #define C2PY_HXX_DECLARATION_{0}_GUARDS
    )RAW",
                     m.module_name);

  for (auto const &[_, cls_info] : m.classes) {
    hxx << fmt::format(R"RAW( template <> constexpr bool c2py::is_wrapped<{0}>   = true;)RAW", //
                       clu::get_fully_qualified_name(cls_info.ptr));
  }
  hxx << "\n#endif";

  return hxx.str();
}