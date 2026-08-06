#include "./cls_tools.hpp"
#include "./fnt.hpp"
#include "./utils.hpp"
#include <fmt/format.h>
using namespace fmt::literals;

#include <itertools/itertools.hpp>
#include "clu/fullqualifiedname.hpp"
#include "clu/doc_string.hpp"
#include "utility/logger.hpp"
#include "./doc.hpp"

using util::join;

static const struct {
  util::logger cls         = util::logger{"-- ", "\033[1;32mClass: \033[0m",      1};
  util::logger cls_details = util::logger{"-- ", "\033[1;32m         -- \033[0m", 1};
  util::logger prop        = util::logger{"-- ", "\033[1;32m  Property: \033[0m", 1};
} logs;

// ===================================================================

void codegen_synth_constructor(std::ostream &code, cls_info_t const &cls_info, str_t const &cls_alias, str_t const &cls_full_name) {
  EXPECTS(!cls_info.fields.empty());

  logs.cls_details("Synthesize constructor from pydict");
  // One synth_constructor per class — hash the class alias (itself derived from the class's C++ FQN).
  auto synth_id = codegen::id_hash(cls_alias);

  std::vector<std::string> non_default_const_params;
  std::vector<clang::FieldDecl *> simple_fields;
  simple_fields.reserve(100); //NOLINT

  for (auto *f : cls_info.fields) {

    // if f is a type, which has no default constructor and no defaut initializer is the class
    // we build it at the construction of the object, using designated initializer (as we skip other fields)
    if (auto *clsf = f->getType()->getAsCXXRecordDecl(); clsf and not clsf->hasDefaultConstructor() and (get_field_initializer(f) == nullptr)) {
      non_default_const_params.push_back(
         fmt::format(R"RAW(.{1} = de.get<{0}>("{1}"))RAW", clu::get_fully_qualified_name(clsf), f->getNameAsString()));
    } else
      simple_fields.push_back(f);
  }

  code << '\n'
       << fmt::format(R"RAW(
      static int synth_constructor_{1}(PyObject *self, PyObject *args, PyObject *kwargs) {{
       if (args and PyTuple_Check(args) and (PyTuple_Size(args)>0)) {{
        PyErr_SetString(PyExc_RuntimeError, ("Error in constructing {2}.\nNo positional arguments allowed. Use keywords arguments"));
        return -1;
       }}
      c2py::pydict_extractor de{{kwargs}};
      try {{ ((c2py::wrap<{0}> *)self)->_c = new {0}{{ {3}  }}; }}
      catch (std::exception const &e) {{
        PyErr_SetString(PyExc_RuntimeError, ("Error in constructing {2} from a Python dict.\n   "s + e.what()).c_str());
        return -1;
      }}
      auto & self_c = *(((c2py::wrap<{0}> *)self)->_c);
 )RAW",
                      cls_alias, synth_id, cls_full_name, join(non_default_const_params, ','));

  for (auto *f : simple_fields)
    code << fmt::format(R"RAW( de("{0}", self_c.{0}, {1}); )RAW", f->getNameAsString(), (get_field_initializer(f) != nullptr));

  code << fmt::format(R"RAW(
       return de.check();
     }}

     template <> constexpr initproc c2py::tp_init<{}> = synth_constructor_{};
   )RAW",
                      cls_alias, synth_id);

  // doc string for synthesized constructor
  auto [doc, field_types] = pydoc_of_synthetized_constructor(cls_info);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_ctor_doc<{0}> = c2py::replace_tags()RAW", cls_alias)
       << fmt::format(R"RAW(R"DOC({0})DOC", "par", {{{1}}});)RAW", doc, codegen::cpp_to_py_types(field_types));
}

// ===================================================================

void codegen_synth___dict_attribute(std::ostream &code, std::ostream &table, cls_info_t const &cls_info, str_t const &cls_alias) {

  if (cls_info.fields.empty()) return; // Nothing to return

  // One synth dict getter per class.
  auto dict_id = codegen::id_hash(cls_alias);

  code << '\n'
       << fmt::format(R"RAW( static PyObject *prop_get_dict_{0}(PyObject *self, void *) {{
                              auto & self_c = *(((c2py::wrap<{1}> *)self)->_c);
                              c2py::pydict dic; )RAW",
                      dict_id, cls_alias);

  for (auto *f : cls_info.fields) {
    //if (f->getAccess() != clang::AS_public) continue; // SHOULD BE USELESS
    code << fmt::format(R"RAW( dic["{0}"] = self_c.{0}; )RAW", f->getNameAsString());
  }

  code << "return dic.new_ref();} \n";

  table << fmt::format(R"RAW( {{"__dict__", (getter)prop_get_dict_{0}, nullptr, "", nullptr}},)RAW", //
                       dict_id);
}

// ===================================================================

void codegen_getter_setter(std::ostream &table, std::ostream &doc, str_t const &prop_name, cls_info_t::property const &prop,
                           cls_info_t const &cls_info) {

  // One prop_doc per (class, property).
  auto prop_id = codegen::id_hash(clu::get_fully_qualified_name(cls_info.ptr) + "::" + prop_name);

  bool has_setter     = !prop.setters.empty();
  auto *getter_method = prop.getter.as_method(); // null when getter is a free function

  auto gsdoc   = clu::doc_string_t{prop.getter.ptr};
  auto doc_str = gsdoc.brief_str;
  doc_str += gsdoc.details_str.empty() ? "" : (doc_str.empty() ? gsdoc.details_str : fmt::format("\n\n{}", gsdoc.details_str));
  doc << fmt::format(R"RAW( static constexpr auto prop_doc_{0} = R"DOC({1})DOC"; )RAW", prop_id, doc_str);

  // Returns {parent_fqn, is_inherited} for a method, where is_inherited is true iff
  // the method belongs to a strict base class of cls_info.ptr. Throws if unrelated.
  auto method_origin = [&](clang::CXXMethodDecl const *m) -> std::pair<str_t, bool> {
    auto *base        = m->getParent()->getCanonicalDecl();
    auto *derived     = cls_info.ptr->getCanonicalDecl();
    bool is_inherited = derived->isDerivedFrom(base);
    if (!is_inherited and base != derived)
      throw std::runtime_error(fmt::format("Error in property '{}': the method '{}' is not declared in the class '{}' nor inherited from it.",
                                           prop_name, m->getQualifiedNameAsString(), clu::get_fully_qualified_name(cls_info.ptr)));
    return {clu::get_fully_qualified_name(m->getParent()), is_inherited};
  };

  // ---- getter ----
  str_t getter_entry;
  if (getter_method) {
    auto [method_parent, is_inherited] = method_origin(getter_method);
    auto cast_op                       = fmt::format("cast{}<>", getter_method->isStatic() ? "" : (getter_method->isConst() ? "mc" : "m"));
    if (is_inherited) {
      getter_entry = fmt::format("c2py::getter_from_method_B<{0}, c2py::{1}(&{2}::{3})>", clu::get_fully_qualified_name(cls_info.ptr), cast_op,
                                 method_parent, getter_method->getNameAsString());
    } else {
      getter_entry = fmt::format("c2py::getter_from_method<c2py::{}(&{})>", cast_op, prop.getter.ptr->getQualifiedNameAsString());
    }
  } else {
    getter_entry = fmt::format("c2py::getter_from_fun<&{}>", prop.getter.ptr->getQualifiedNameAsString());
  }

  // ---- setter ----
  // FIXME: implement the LIST of setters.
  str_t setter_entry  = "nullptr";
  str_t closure_entry = "nullptr";
  if (has_setter) {
    auto *setter_method = prop.setters[0].as_method();
    if (setter_method) {
      auto [setter_parent, is_inherited] = method_origin(setter_method);
      if (is_inherited) {
        setter_entry = fmt::format("(setter)c2py::setter_from_method_B<{0}, &{1}::{2}>", clu::get_fully_qualified_name(cls_info.ptr), setter_parent,
                                   setter_method->getNameAsString());
      } else {
        setter_entry = fmt::format("(setter)c2py::setter_from_method<&{}>", prop.setters[0].ptr->getQualifiedNameAsString());
      }
    } else {
      setter_entry = fmt::format("(setter)c2py::setter_from_fun<&{}>", prop.setters[0].ptr->getQualifiedNameAsString());
    }
    closure_entry = fmt::format(R"RAW((void*)"Cannot delete the attribute {}")RAW", prop_name);
  }

  table << fmt::format(R"RAW( {{"{}", {}, {}, prop_doc_{}, {}}},)RAW", prop_name, getter_entry, setter_entry, prop_id, closure_entry);
}

// ===================================================================

void codegen_getsetitem(std::ostream &code, cls_info_t const &cls_info, str_t const &cls_alias) {

  if ((not cls_info.has_size_method) and cls_info.getitems.empty()) return;

  // One getitem/setitem pair per class.
  auto subscript_id = codegen::id_hash(cls_alias);

  auto get_ovs = [&cls_alias](auto &f_info) {
    auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f_info.ptr);
    return fmt::format(R"RAW( c2py::cfun2(c2py::getitem<{0}, {1}>))RAW", cls_alias, fnt_paramtypes(m));
  };

  auto set_ovs = [&cls_alias](auto &f_info) {
    auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f_info.ptr);
    return fmt::format(R"RAW( c2py::cfun2(c2py::setitem<{0}, {1}>))RAW", cls_alias, fnt_paramtypes(m));
  };

  str_t size_code    = (cls_info.has_size_method ? fmt::format("c2py::tpxx_size<{}>", cls_alias) : "nullptr");
  str_t getitem_code = "nullptr";
  str_t setitem_code = "nullptr";

  if (not cls_info.getitems.empty()) {
    getitem_code = fmt::format("getitem_{}", subscript_id);
    code << fmt::format(R"RAW(

      static PyObject *getitem_{0}(PyObject *self, PyObject *key) {{
       static c2py::dispatcher_t<c2py::pycfun23> disp = {{ {1} }};
       return disp(self, key);
      }}

   )RAW",
                        subscript_id, join(cls_info.getitems, get_ovs, ','));

    // yes, nested : no setitem if no getitems
    if (not cls_info.setitems.empty()) {
      setitem_code = fmt::format("setitem_{}", subscript_id);

      code << fmt::format(R"RAW(

        static int setitem_{0}(PyObject *self, PyObject *key, PyObject *val) {{
         static c2py::dispatcher_t<c2py::pycfun23> disp{{ {1} }};
         c2py::pyref r = disp(self, key, val);
         return (r== nullptr ? -1 : 0);
        }}

      )RAW",
                          subscript_id, join(cls_info.setitems, set_ovs, ','));
    }
  }

  code << fmt::format(R"RAW(
           template <> PyMappingMethods c2py::tp_as_mapping<{0}> = {{ {1}, {2}, {3} }};
          )RAW",
                      cls_alias, size_code, getitem_code, setitem_code);
}

// ===================================================================

void codegen_operators(std::ostream &code, cls_info_t const &cls_info, str_t const &cls_alias) {

  // Map OpKind to c2py::OpName string; returns nullptr for ops not exposed via PyNumberMethods
  auto to_arith_name = [](OpKind k) -> const char * {
    switch (k) {
      case OpKind::Add: return "Add";
      case OpKind::Sub: return "Sub";
      case OpKind::Mul: return "Mul";
      case OpKind::Div: return "Div";
      case OpKind::LShift: return "LShift";
      case OpKind::Neg: return "Neg";
      case OpKind::IAdd: return "IAdd";
      case OpKind::ISub: return "ISub";
      case OpKind::IMul: return "IMul";
      case OpKind::IDiv: return "IDiv";
      default: return nullptr;
    }
  };

  auto const *cls = cls_info.ptr;
  auto &ctx       = cls->getASTContext();

  int count = 0;
  for (auto const &[kind, sigs] : cls_info.operators) {
    auto *op_name = to_arith_name(kind);
    if (not op_name or sigs.empty()) continue;
    ++count;

    if (kind == OpKind::Neg) {
      // Unary: arithmetic<Cls, OpName::Neg> : std::tuple<T, ...>
      std::vector<str_t> types;
      for (auto const &sig : sigs) {
        EXPECTS(sig.size() == 1);
        types.push_back(clu::get_fully_qualified_name(sig[0], ctx));
      }
      code << fmt::format("\ntemplate <> struct c2py::arithmetic<{0}, c2py::OpName::{1}> : std::tuple<{2}> {{}};\n", cls_alias, op_name,
                          join(types, ", "));
    } else {
      // Binary: arithmetic<Cls, OpName::X> : std::tuple<std::pair<T1,T2>, ...>
      std::vector<str_t> pairs;
      for (auto const &sig : sigs) {
        EXPECTS(sig.size() == 2);
        pairs.push_back(fmt::format("std::pair<{}, {}>", clu::get_fully_qualified_name(sig[0], ctx), clu::get_fully_qualified_name(sig[1], ctx)));
      }
      code << fmt::format("\ntemplate <> struct c2py::arithmetic<{0}, c2py::OpName::{1}> : std::tuple<{2}> {{}};\n", cls_alias, op_name,
                          join(pairs, ", "));
    }
  }

  if (count > 0)
    code << fmt::format("\ntemplate <> constexpr PyNumberMethods *c2py::tp_as_number<{0}> = &c2py::tp_as_number_impl<{0}>;\n", cls_alias);
}

// ===================================================================

str_t codegen_cls(std::ostream &code, str_t const &cls_py_name, cls_info_t const &cls_info, str_t const &full_module_name) {

  auto *cls          = cls_info.ptr;
  auto cls_full_name = clu::get_fully_qualified_name(cls);

  logs.cls(fmt::format("{1} [Python: {0}]", cls_py_name, cls_full_name));

  // -- emit using alias for this class.
  // Stable id derived from the class's fully-qualified C++ name.
  auto cls_alias = fmt::format("_c2py_cls_{}", codegen::id_hash(cls_full_name));
  code << '\n' << fmt::format("// --------- class {} -----------", cls_alias);
  code << '\n' << fmt::format("using {} = {};", cls_alias, cls_full_name);
  code << '\n' << fmt::format("template <> constexpr bool c2py::is_wrapped<{}> = true;", cls_alias);

  // -- tp_name
  code << '\n' << fmt::format(R"RAW(template <> inline constexpr auto c2py::tp_name<{0}> = "{1}.{2}";)RAW", cls_alias, full_module_name, cls_py_name);

  // ---------- Methods ------------
  {
    std::stringstream MethodDecls, MethodTable, MethodDocs;

    // ---- constructor
    if (cls_info.synthetize_init_from_pydict() and not(cls_info.fields.empty())) {
      codegen_synth_constructor(MethodDecls, cls_info, cls_alias, cls_full_name);
    } else
      codegen::write_dispatch_constructors(MethodDecls, cls_alias, cls_full_name, cls_info.constructors);

    // ---- methods
    for (auto const &[fpyname, overloads] : cls_info.methods)
      codegen::write_dispatch(MethodDecls, MethodTable, MethodDocs, fpyname, overloads, cls, true, cls_alias);

    // ----- hdf5 : __write_hdf5__
    // NB. one row per line, indentation written here: cf. the DIFF STABILITY note in write_dispatch.
    if (cls_info.has_hdf5)
      MethodTable << fmt::format(R"RAW(   {{"__write_hdf5__", c2py::tpxx_write_h5<{0}>, METH_VARARGS, "  "}},)RAW", cls_alias) << '\n';

    // ----- Serialization

    if (cls_info.serialization != Serialization::None) {
      static auto ser_opt_vec = std::vector<str_t>{"", "tuple", "h5", "repr"};
      auto set_opt            = ser_opt_vec[int(cls_info.serialization)];
      MethodTable << fmt::format(R"RAW(   {{"__getstate__", c2py::getstate_{0}<{1}>, METH_NOARGS, ""}},)RAW", set_opt, cls_alias) << '\n';
      MethodTable << fmt::format(R"RAW(   {{"__setstate__", c2py::setstate_{0}<{1}>, METH_O, ""}},)RAW", set_opt, cls_alias) << '\n';
    }
    // ----- assemble the code

    code << MethodDecls.str();
    code << MethodDocs.str();

    // The rows are pre-indented, one per line; clang-format off keeps them verbatim (cf. write_dispatch).
    code << fmt::format(R"RAW(

      // ----- Method table ----
// clang-format off
template <> PyMethodDef c2py::tp_methods<{0}>[] = {{
{1}   {{nullptr, nullptr, 0, nullptr}} // Sentinel
}};
// clang-format on

     )RAW",
                        cls_alias, MethodTable.str());
  }

  // ---------- Members ------------

  std::stringstream MembersDoc, Members;
  for (auto *f : cls_info.fields) {

    auto name     = str_t{f->getName()};
    auto type     = clu::get_fully_qualified_name(f->getType(), f->getASTContext());
    auto is_const = f->getType().isConstQualified();
    if (is_const) type = "const " + type;

    auto fdoc     = clu::doc_string_t{f};
    auto fdoc_str = fdoc.brief_str;
    fdoc_str += fdoc.details_str.empty() ? "" : (fdoc_str.empty() ? fdoc.details_str : fmt::format("\n\n{}", fdoc.details_str));

    // One member doc per (class, field).
    auto member_id = codegen::id_hash(cls_full_name + "::" + name);

    MembersDoc << fmt::format(R"RAW( constexpr auto _c2py_doc_member_{0} = R"DOC({1})DOC"; )RAW", member_id, fdoc_str);

    if (is_const)
      Members << fmt::format(R"RAW(
                         {{"{0}", c2py::get_member<&{1}::{0}, {1}>, nullptr, _c2py_doc_member_{3}, nullptr}},
                          )RAW",
                             name, cls_alias, type, member_id);
    else
      // {{"{0}", c2py::get_member<&{1}::{0}>, c2py::set_member<&{1}::{0}>, _c2py_doc_member_{3}, nullptr}},
      Members << fmt::format(R"RAW( c2py::getsetdef_from_member<&{1}::{0}, {1}>("{0}", _c2py_doc_member_{3}),)RAW", name, cls_alias, type, member_id);
  }

  code << MembersDoc.str();

  // ---------- Properties ------------

  std::stringstream Properties, PropertiesDocs;
  for (auto const &[pyname, prop] : cls_info.properties) {
    logs.prop(fmt::format("{}", pyname));
    codegen_getter_setter(Properties, PropertiesDocs, pyname, prop, cls_info);
  }

  if (cls_info.synthetize_init_from_pydict()) codegen_synth___dict_attribute(code, Properties, cls_info, cls_alias);

  code << PropertiesDocs.str();

  // ---------- member & prop table
  if (not cls_info.fields.empty() or not cls_info.properties.empty())
    code << fmt::format(R"RAW(

      // ----- Member and property table ----

      template <> constinit PyGetSetDef c2py::tp_getset<{0}>[] = {{
         {1}
         {2}
         {{nullptr,nullptr,nullptr,nullptr,nullptr }}
      }};

      )RAW",
                        cls_alias, Members.str(), Properties.str());

  // ---------- operator [] & size as len

  codegen_getsetitem(code, cls_info, cls_alias);

  // ---------- arithmetic operators

  codegen_operators(code, cls_info, cls_alias);

  // ----------- import other modules

  // -- tp_doc
  // ORDERING INVARIANT: tp_doc<T> must be emitted after tp_ctor_doc<T> (written by
  // write_dispatch_constructors into MethodDecls).
  auto cls_doc = pydoc(cls_info);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_doc<{0}> = R"DOC({1})DOC" + )RAW", cls_alias, cls_doc)
       << (cls_doc.empty() ? "" : R"RAW( std::string{"\n\n----------\n\n"}  + )RAW") << fmt::format(R"RAW(c2py::tp_ctor_doc<{0}>;)RAW", cls_alias);

  return cls_alias;
}
