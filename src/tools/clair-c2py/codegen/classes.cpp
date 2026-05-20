#include "./fnt.hpp"
#include "./utils.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;

#include "utility/logger.hpp"
#include "utility/macros.hpp"
#include "./doc.hpp"

using util::join;

static const struct {
  util::logger cls         = util::logger{"-- ", "\033[1;32mClass: \033[0m",      1};
  util::logger cls_details = util::logger{"-- ", "\033[1;32m         -- \033[0m", 1};
  util::logger prop        = util::logger{"-- ", "\033[1;32m  Property: \033[0m", 1};
} logs;

// ===================================================================

void codegen_synth_constructor(std::ostream &code, cls_info_t const &cls_info, str_t const &cls_alias, str_t const &cls_full_name) {
  EXPECTS(not cls_info.fields.empty());

  logs.cls_details("Synthesize constructor from pydict");
  static long counter = 0;

  // Fields whose class type has no default constructor and no in-class initializer
  // must be designated-initialized; all other fields are "simple".
  std::vector<std::string> non_default_const_params;
  std::vector<ir::FieldDecl const *> simple_fields;
  simple_fields.reserve(100); //NOLINT

  for (auto const &f : cls_info.fields) {
    if (f->field_type_has_no_default_ctor and not f->has_in_class_initializer)
      non_default_const_params.push_back(fmt::format(R"RAW(.{1} = de.get<{0}>("{1}"))RAW", f->type.name, f->name));
    else
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
      // NOTE: `new` here is emitted C++ source code, not an allocation in this program.
      try {{ ((c2py::wrap<{0}> *)self)->_c = new {0}{{ {3}  }}; }}
      catch (std::exception const &e) {{
        PyErr_SetString(PyExc_RuntimeError, ("Error in constructing {2} from a Python dict.\n   "s + e.what()).c_str());
        return -1;
      }}
      auto & self_c = *(((c2py::wrap<{0}> *)self)->_c);
 )RAW",
                      cls_alias, counter, cls_full_name, join(non_default_const_params, ','));

  for (auto const *f : simple_fields)
    code << fmt::format(R"RAW( de("{0}", self_c.{0}, {1}); )RAW", f->name, int(f->has_in_class_initializer));

  code << fmt::format(R"RAW(
       return de.check();
     }}

     template <> constexpr initproc c2py::tp_init<{}> = synth_constructor_{};
   )RAW",
                      cls_alias, counter);

  auto [doc, field_types] = pydoc_of_synthetized_constructor(cls_info);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_ctor_doc<{0}> = c2py::replace_tags()RAW", cls_alias)
       << fmt::format(R"RAW(R"DOC({0})DOC", "par", {{{1}}});)RAW", doc, codegen::cpp_to_py_types(field_types));

  ++counter;
}

// ===================================================================

void codegen_synth___dict_attribute(std::ostream &code, std::ostream &table, cls_info_t const &cls_info, str_t const &cls_alias) {

  static long counter = 0;
  if (cls_info.fields.empty()) return;

  code << '\n'
       << fmt::format(R"RAW( static PyObject *prop_get_dict_{0}(PyObject *self, void *) {{
                              auto & self_c = *(((c2py::wrap<{1}> *)self)->_c);
                              c2py::pydict dic; )RAW",
                      counter, cls_alias);

  for (auto const &f : cls_info.fields)
    code << fmt::format(R"RAW( dic["{0}"] = self_c.{0}; )RAW", f->name);

  code << "return dic.new_ref();} \n";

  table << fmt::format(R"RAW( {{"__dict__", (getter)prop_get_dict_{0}, nullptr, "", nullptr}},)RAW", counter);

  ++counter;
}

// ===================================================================

void codegen_getter_setter(std::ostream &table, std::ostream &doc, str_t const &prop_name, cls_info_t::property const &prop,
                           cls_info_t const &cls_info) {

  static long counter = 0;
  auto &getter     = *prop.getter.ptr;
  bool has_setter     = not prop.setters.empty();

  auto doc_str = getter.doc_brief;
  doc_str += getter.doc_details.empty() ? "" : (doc_str.empty() ? getter.doc_details : fmt::format("\n\n{}", getter.doc_details));
  doc << fmt::format(R"RAW( static constexpr auto prop_doc_{0} = R"DOC({1})DOC"; )RAW", counter, doc_str);

  auto &cls_fqn = cls_info.ptr->fully_qualified_name;

  // ---- getter ----
  str_t getter_entry;
  if (getter.is_method) {
    bool is_inherited = (getter.parent_class_fqn != cls_fqn);
    auto cast_op      = getter.is_static ? "cast<>" : (getter.is_const_method ? "castmc<>" : "castm<>");
    if (is_inherited) {
      getter_entry = fmt::format("c2py::getter_from_method_B<{0}, c2py::{1}(&{2}::{3})>",
                                 cls_fqn, cast_op, getter.parent_class_fqn, getter.simple_name);
    } else {
      getter_entry = fmt::format("c2py::getter_from_method<c2py::{}(&{})>", cast_op, getter.qualified_name);
    }
  } else {
    getter_entry = fmt::format("c2py::getter_from_fun<&{}>", getter.qualified_name);
  }

  // ---- setter ----
  str_t setter_entry  = "nullptr";
  str_t closure_entry = "nullptr";
  if (has_setter) {
    auto &setter      = *prop.setters[0].ptr;
    if (setter.is_method) {
      bool setter_inherited = (setter.parent_class_fqn != cls_fqn);
      if (setter_inherited) {
        setter_entry = fmt::format("(setter)c2py::setter_from_method_B<{0}, &{1}::{2}>",
                                   cls_fqn, setter.parent_class_fqn, setter.simple_name);
      } else {
        setter_entry = fmt::format("(setter)c2py::setter_from_method<&{}>", setter.qualified_name);
      }
    } else {
      setter_entry = fmt::format("(setter)c2py::setter_from_fun<&{}>", setter.qualified_name);
    }
    closure_entry = fmt::format(R"RAW((void*)"Cannot delete the attribute {}")RAW", prop_name);
  }

  table << fmt::format(R"RAW( {{"{}", {}, {}, prop_doc_{}, {}}},)RAW", prop_name, getter_entry, setter_entry, counter, closure_entry);

  counter++;
}

// ===================================================================

void codegen_getsetitem(std::ostream &code, cls_info_t const &cls_info, str_t const &cls_alias) {

  static long counter = 0;

  if ((not cls_info.has_size_method) and cls_info.getitems.empty()) return;

  auto get_ovs = [&cls_alias](auto &f_info) {
    return fmt::format(R"RAW( c2py::cfun2(c2py::getitem<{0}, {1}>))RAW", cls_alias, fnt_paramtypes(*f_info.ptr));
  };

  auto set_ovs = [&cls_alias](auto &f_info) {
    return fmt::format(R"RAW( c2py::cfun2(c2py::setitem<{0}, {1}>))RAW", cls_alias, fnt_paramtypes(*f_info.ptr));
  };

  str_t size_code    = (cls_info.has_size_method ? fmt::format("c2py::tpxx_size<{}>", cls_alias) : "nullptr");
  str_t getitem_code = "nullptr";
  str_t setitem_code = "nullptr";

  if (not cls_info.getitems.empty()) {
    getitem_code = fmt::format("getitem_{}", counter);
    code << fmt::format(R"RAW(

      static PyObject *getitem_{0}(PyObject *self, PyObject *key) {{
       static c2py::dispatcher_t<c2py::pycfun23> disp = {{ {1} }};
       return disp(self, key);
      }}

   )RAW",
                        counter, join(cls_info.getitems, get_ovs, ','));

    if (not cls_info.setitems.empty()) {
      setitem_code = fmt::format("setitem_{}", counter);
      code << fmt::format(R"RAW(

        static int setitem_{0}(PyObject *self, PyObject *key, PyObject *val) {{
         static c2py::dispatcher_t<c2py::pycfun23> disp{{ {1} }};
         c2py::pyref r = disp(self, key, val);
         return (r== nullptr ? -1 : 0);
        }}

      )RAW",
                          counter, join(cls_info.setitems, set_ovs, ','));
    }
  }

  code << fmt::format(R"RAW(
           template <> PyMappingMethods c2py::tp_as_mapping<{0}> = {{ {1}, {2}, {3} }};
          )RAW",
                      cls_alias, size_code, getitem_code, setitem_code);

  counter++;
}

// ===================================================================

void codegen_operators(std::ostream &code, cls_info_t const &cls_info, str_t const &cls_alias) {

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

  int count = 0;
  for (auto const &[kind, sigs] : cls_info.operators) {
    auto *op_name = to_arith_name(kind);
    if (not op_name or sigs.empty()) continue;
    ++count;

    if (kind == OpKind::Neg) {
      std::vector<str_t> types;
      for (auto const &sig : sigs) {
        EXPECTS(sig.size() == 1);
        types.push_back(sig[0].name);
      }
      code << fmt::format("\ntemplate <> struct c2py::arithmetic<{0}, c2py::OpName::{1}> : std::tuple<{2}> {{}};\n",
                          cls_alias, op_name, join(types, ", "));
    } else {
      std::vector<str_t> pairs;
      for (auto const &sig : sigs) {
        EXPECTS(sig.size() == 2);
        pairs.push_back(fmt::format("std::pair<{}, {}>", sig[0].name, sig[1].name));
      }
      code << fmt::format("\ntemplate <> struct c2py::arithmetic<{0}, c2py::OpName::{1}> : std::tuple<{2}> {{}};\n",
                          cls_alias, op_name, join(pairs, ", "));
    }
  }

  if (count > 0)
    code << fmt::format("\ntemplate <> constexpr PyNumberMethods *c2py::tp_as_number<{0}> = &c2py::tp_as_number_impl<{0}>;\n", cls_alias);
}

// ===================================================================

str_t codegen_cls(std::ostream &code, str_t const &cls_py_name, cls_info_t const &cls_info, str_t const &full_module_name) {

  auto &cls_full_name = cls_info.ptr->fully_qualified_name;

  logs.cls(fmt::format("{1} [Python: {0}]", cls_py_name, cls_full_name));

  static int cls_counter = 0;
  auto cls_alias         = fmt::format("_c2py_cls_{}", cls_counter++);
  code << '\n' << fmt::format("// --------- class {} -----------", cls_alias);
  code << '\n' << fmt::format("using {} = {};", cls_alias, cls_full_name);
  code << '\n' << fmt::format("template <> constexpr bool c2py::is_wrapped<{}> = true;", cls_alias);
  code << '\n' << fmt::format(R"RAW(template <> inline constexpr auto c2py::tp_name<{0}> = "{1}.{2}";)RAW", cls_alias, full_module_name, cls_py_name);

  // ---------- Methods ------------
  {
    std::stringstream MethodDecls, MethodTable, MethodDocs;

    if (cls_info.synthetize_init_from_pydict() and not cls_info.fields.empty()) {
      codegen_synth_constructor(MethodDecls, cls_info, cls_alias, cls_full_name);
    } else
      codegen::write_dispatch_constructors(MethodDecls, cls_alias, cls_full_name, cls_info.constructors);

    for (auto const &[fpyname, overloads] : cls_info.methods)
      codegen::write_dispatch(MethodDecls, MethodTable, MethodDocs, fpyname, overloads, cls_info.ptr, true, cls_alias);

    if (cls_info.has_hdf5) MethodTable << fmt::format(R"RAW( {{"__write_hdf5__", c2py::tpxx_write_h5<{0}>, METH_VARARGS, "  "}}, )RAW", cls_alias);

    if (cls_info.serialization != Serialization::None) {
      static auto ser_opt_vec = std::vector<str_t>{"", "tuple", "h5", "repr"};
      auto set_opt            = ser_opt_vec[int(cls_info.serialization)];
      MethodTable << fmt::format(R"RAW({{"__getstate__", c2py::getstate_{0}<{1}>, METH_NOARGS, ""}},)RAW", set_opt, cls_alias);
      MethodTable << fmt::format(R"RAW({{"__setstate__", c2py::setstate_{0}<{1}>, METH_O, ""}},)RAW", set_opt, cls_alias);
    }

    code << MethodDecls.str();
    code << MethodDocs.str();

    code << fmt::format(R"RAW(

      // ----- Method table ----
      template <> PyMethodDef c2py::tp_methods<{0}>[] = {{
           {1}
           {{nullptr, nullptr, 0, nullptr}} // Sentinel
      }};

     )RAW",
                        cls_alias, MethodTable.str());
  }

  // ---------- Members ------------

  static long member_counter = 0;

  std::stringstream MembersDoc, Members;
  for (auto const &f : cls_info.fields) {
    auto &name    = f->name;
    auto &type    = f->type.name;
    auto is_const = f->is_const;

    auto fdoc_str = f->doc_brief;
    fdoc_str += f->doc_details.empty() ? "" : (fdoc_str.empty() ? f->doc_details : fmt::format("\n\n{}", f->doc_details));

    MembersDoc << fmt::format(R"RAW( constexpr auto _c2py_doc_member_{0} = R"DOC({1})DOC"; )RAW", member_counter, fdoc_str);

    if (is_const)
      Members << fmt::format(R"RAW(
                         {{"{0}", c2py::get_member<&{1}::{0}, {1}>, nullptr, _c2py_doc_member_{3}, nullptr}},
                          )RAW",
                             name, cls_alias, type, member_counter);
    else
      Members << fmt::format(R"RAW( c2py::getsetdef_from_member<&{1}::{0}, {1}>("{0}", _c2py_doc_member_{3}),)RAW",
                             name, cls_alias, type, member_counter);
    ++member_counter;
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

  codegen_getsetitem(code, cls_info, cls_alias);
  codegen_operators(code, cls_info, cls_alias);

  // -- tp_doc
  // ORDERING INVARIANT: tp_doc<T> must be emitted after tp_ctor_doc<T> (written by
  // write_dispatch_constructors into MethodDecls).
  auto cls_doc = pydoc(cls_info);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_doc<{0}> = R"DOC({1})DOC" + )RAW", cls_alias, cls_doc)
       << (cls_doc.empty() ? "" : R"RAW( std::string{"\n\n----------\n\n"}  + )RAW")
       << fmt::format(R"RAW(c2py::tp_ctor_doc<{0}>;)RAW", cls_alias);

  return cls_alias;
}
