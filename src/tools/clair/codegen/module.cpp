#include "./module.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
#include <set>
#include <sstream>
using namespace fmt::literals;
#include <itertools/itertools.hpp>

#include "utility/logger.hpp"
#include "./fnt.hpp"
#include "./classes.hpp"
#include "../c2py/c2py_version.hpp"

using util::join;

static const struct {
  util::logger mod = util::logger{"-- ", "\033[1;32mModule: \033[0m", 1};
  util::logger enu = util::logger{"-- ", "\033[1;32mEnum: \033[0m",   1};
} logs;

// =========== module code template ==============
// #embed is C, it will be C++23, meanwhile we silence the warning that we use a C extension
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
static constexpr char module_code_tpl[] = { //NOLINT
#embed "templates/module.txt"
   , '\0'};
#pragma clang diagnostic pop

// =========== forward declaration generation ==============

// Split "ns1::ns2::Name" → {"ns1::ns2", "Name"}.  No namespace → first element is empty.
static std::pair<str_t, str_t> split_ns(str_t const &fqn) {
  auto pos = fqn.rfind("::");
  if (pos == str_t::npos) return {"", fqn};
  return {fqn.substr(0, pos), fqn.substr(pos + 2)};
}

// Collect forward declarations for all classes and free functions and emit them
// grouped by namespace:
//   namespace mymod {
//     struct point_t;
//     void greet(const char *);
//     float add(float, float);
//   }
static str_t gen_forward_decls(module_info_t const &m) {
  std::map<str_t, std::vector<str_t>> ns_decls;

  // Struct forward declarations
  for (auto const &[pyname, cls_info] : m.classes) {
    auto [ns, name] = split_ns(cls_info.ptr->fully_qualified_name);
    ns_decls[ns].push_back("struct " + name + ";");
  }

  // Free function forward declarations (one per unique C++ signature)
  std::set<str_t> seen;
  for (auto const &[pyname, overloads] : m.functions) {
    for (auto const &fi : overloads) {
      if (!fi.ptr) continue;
      auto const &fd  = *fi.ptr;
      auto sig        = fd.qualified_name + "(" + fd.param_types_str() + ")";
      if (!seen.insert(sig).second) continue; // skip duplicate
      auto [ns, name] = split_ns(fd.qualified_name);
      ns_decls[ns].push_back(fd.return_type.name + " " + name + "(" + fd.param_types_str() + ");");
    }
  }

  // Emit grouped by namespace
  std::stringstream out;
  for (auto const &[ns, decls] : ns_decls) {
    if (ns.empty()) {
      for (auto const &d : decls) out << d << "\n";
    } else {
      out << "namespace " << ns << " {\n";
      for (auto const &d : decls) out << "  " << d << "\n";
      out << "}\n";
    }
  }
  return out.str();
}

// =========== module code generation ==============

str_t codegen_module(module_info_t const &m, bool add_forward_decls) {

  auto full_module_name = m.package_name.empty() ? m.module_name : m.package_name + '.' + m.module_name;

  logs.mod(full_module_name);

  std::stringstream EnumDecls;
  for (auto const &enu : m.enums) {
    auto &qname = enu->qualified_name;
    logs.enu(qname);
    EnumDecls << fmt::format(
       R"RAW(
       template <> const std::map<{0}, str_t> c2py::enum_to_string<{0}> = {{ {1} }};)RAW",
       qname,
       join(enu->enumerators, [&qname](auto const &val) { return fmt::format(R"RAW( {{ {0}::{1}, "{1}" }} )RAW", qname, val); }, ','));
  }

  std::stringstream ForwardDecls;
  if (add_forward_decls) ForwardDecls << gen_forward_decls(m);

  std::stringstream FunctionDecls, FunctionTable, FunctionDocs;
  std::stringstream ClassesDecls, PyTypeReadyDecls, AddTypeObjectDecls;
  std::stringstream Hdf5C2pyIncluder, Hdf5RegistrationInit, Hdf5Registration;

  // classes wrapped
  for (auto const &[cls_py_name, cls_info] : m.classes) {
    auto cls_alias = codegen_cls(ClassesDecls, cls_py_name, cls_info, full_module_name);

    if (cls_info.base != nullptr)
      PyTypeReadyDecls << fmt::format(R"RAW(   c2py::wrap_pytype<{0}>.tp_base = &c2py::wrap_pytype<{1}>; )RAW", cls_alias,
                                      cls_info.base->fully_qualified_name);
    PyTypeReadyDecls << fmt::format(R"RAW( if (PyType_Ready(&c2py::wrap_pytype<{}>) < 0) return NULL;)RAW", cls_alias);

    AddTypeObjectDecls << fmt::format(R"RAW(_add_type({0}, "{1}"); )RAW", cls_alias, cls_py_name);

    if (cls_info.has_hdf5) Hdf5Registration << fmt::format(R"RAW( register_h5_type<{0}>(register_class); )RAW", cls_alias);
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

  for (auto const &[fpyname, overloads] : m.functions)
    codegen::write_dispatch(FunctionDecls, FunctionTable, FunctionDocs, fpyname, overloads, nullptr, false);

  std::string ModuleInitFunction =
     m.module_init_fqn.empty() ? "" : fmt::format(" // Initialization of the module \n {}();", m.module_init_fqn);

  std::string r;
  try {
    r = fmt::format(module_code_tpl,                             //
                    "c2py_version_major"_a = c2py_version_major, //
                    "c2py_version_minor"_a = c2py_version_minor, //
                    "sourcefile"_a         = m.sourcefile,       //
                    "modulename"_a         = m.module_name,      //
                    "moduledoc"_a          = m.documentation,    //
                    //"package_name"_a            = (m.package_name.empty() ? m.package_name : m.package_name + '.' ), //
                    "ForwardDecls"_a         = ForwardDecls.str(),          //
                    "EnumDecls"_a            = EnumDecls.str(),            //
                    "ClassesDecls"_a         = ClassesDecls.str(),         //
                    "FunctionDecls"_a        = FunctionDecls.str(),        //
                    "FunctionDocs"_a         = FunctionDocs.str(),         //
                    "FunctionTable"_a        = FunctionTable.str(),        //
                    "PyTypeReadyDecls"_a     = PyTypeReadyDecls.str(),     //
                    "AddTypeObjectDecls"_a   = AddTypeObjectDecls.str(),   //
                    "Hdf5C2pyIncluder"_a     = Hdf5C2pyIncluder.str(),     //
                    "Hdf5RegistrationInit"_a = Hdf5RegistrationInit.str(), //
                    "Hdf5Registration"_a     = Hdf5Registration.str(),     //
                    "ModuleInitFunction"_a   = ModuleInitFunction);        //
  } catch (const fmt::format_error &ex) { std::cout << "ERROR" << ex.what() << std::endl; }

  return r;
}

// =========== wrp info generation ==============

str_t codegen_wrap_info(module_info_t const &m) {
  std::stringstream wrap_info;
  auto full_module_name = m.package_name.empty() ? m.module_name : m.package_name + '.' + m.module_name;
  for (auto const &[cls_py_name, cls_info] : m.classes) {
    auto &cls_name = cls_info.ptr->fully_qualified_name;
    wrap_info << fmt::format(R"RAW( template <> constexpr bool c2py::is_wrapped<{0}> = true;)RAW", cls_name);
    wrap_info << fmt::format(R"RAW(template <> inline constexpr auto c2py::tp_name<{0}> = "{1}.{2}";)RAW", cls_name, full_module_name, cls_py_name);
  }
  return wrap_info.str();
}

// =========== hxx generation ==============

str_t codegen_hxx(module_info_t const &m) {
  auto wrap_info = codegen_wrap_info(m);
  if (wrap_info.empty()) return {};

  std::stringstream hxx;
  hxx << "#include <c2py/c2py.hpp>\n\n";
  hxx << fmt::format(R"RAW(
    #ifndef C2PY_HXX_DECLARATION_{0}_GUARDS
    #define C2PY_HXX_DECLARATION_{0}_GUARDS
    )RAW",
                     m.module_name);
  hxx << wrap_info;
  hxx << "\n#endif";

  return hxx.str();
}
