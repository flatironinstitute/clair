#include "./fnt.hpp"
#include "./utils.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;

#include <itertools/itertools.hpp>
#include "clu/fullqualifiedname.hpp"
#include "clu/doc_string.hpp"
#include "utility/logger.hpp"
#include "./doc.hpp"

using util::join;

static const struct {
  util::logger cls         = util::logger{&std::cout, "-- ", "\033[1;32mClass: \033[0m"};
  util::logger cls_details = util::logger{&std::cout, "-- ", "\033[1;32m         -- \033[0m"};
  util::logger prop        = util::logger{&std::cout, "-- ", "\033[1;32m  Property: \033[0m"};
} logs;

// ===================================================================

// FIXME : move it up
// Find the initializer of a FieldDecl
// NB the case of a template class is specific, as the getInClassInitializer
// would not work there.
clang::Expr const *get_field_initializer(clang::FieldDecl const *f) {
  if (clang::Expr const *init = f->getInClassInitializer()) return init; // non-template structs

  // If f is from a template instantiation, find the original field in the primary template
  if (auto const *cls = dyn_cast<clang::CXXRecordDecl>(f->getParent()))
    if (auto const *tip = cls->getTemplateInstantiationPattern())
      for (clang::FieldDecl const *f_tpl : tip->fields())
        if (f_tpl->getName() == f->getName()) return f_tpl->getInClassInitializer(); // Retrieve from primary template
  return nullptr;
}

// ===================================================================

void codegen_synth_constructor(std::ostream &code, cls_info_t const &cls_info) {
  auto const *cls = cls_info.ptr;
  auto cls_name   = clu::get_fully_qualified_name(cls); //cls->getQualifiedNameAsString();
  EXPECTS(!cls_info.fields.empty());

  logs.cls_details(fmt::format("Synthesize constructor from pydict", cls_name));
  static long counter = 0;

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
        PyErr_SetString(PyExc_RuntimeError, ("Error in constructing {0}.\nNo positional arguments allowed. Use keywords arguments"));
        return -1;
       }}
      c2py::pydict_extractor de{{kwargs}};
      try {{ ((c2py::wrap<{0}> *)self)->_c = new {0}{{ {2}  }}; }}
      catch (std::exception const &e) {{
        PyErr_SetString(PyExc_RuntimeError, ("Error in constructing {0} from a Python dict.\n   "s + e.what()).c_str());
        return -1;
      }}
      auto & self_c = *(((c2py::wrap<{0}> *)self)->_c);
 )RAW",
                      clu::get_fully_qualified_name(cls), counter, join(non_default_const_params, ','));

  for (auto *f : simple_fields)
    code << fmt::format(R"RAW( de("{0}", self_c.{0}, {1}); )RAW", f->getNameAsString(), (get_field_initializer(f) != nullptr));

  code << fmt::format(R"RAW(
       return de.check();
     }}

     template <> constexpr initproc c2py::tp_init<{}> = synth_constructor_{};
   )RAW",
                      clu::get_fully_qualified_name(cls), counter);

  // doc string for synthesized constructor
  auto [doc, field_types] = pydoc_of_synthetized_constructor(cls_info);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_ctor_doc<{0}> = c2py::replace_tags()RAW", cls_name)
       << fmt::format(R"RAW(R"DOC({0})DOC", "par", {{{1}}});)RAW", doc, codegen::cpp_to_py_types(field_types));

  ++counter;
}

// ===================================================================

void codegen_synth___dict_attribute(std::ostream &code, std::ostream &table, cls_info_t const &cls_info) {

  static long counter = 0;
  auto const *cls     = cls_info.ptr;
  if (cls_info.fields.empty()) return; // Nothing to return

  code << '\n'
       << fmt::format(R"RAW( static PyObject *prop_get_dict_{0}(PyObject *self, void *) {{
                              auto & self_c = *(((c2py::wrap<{1}> *)self)->_c);
                              c2py::pydict dic; )RAW",
                      counter, clu::get_fully_qualified_name(cls));

  for (auto *f : cls_info.fields) {
    //if (f->getAccess() != clang::AS_public) continue; // SHOULD BE USELESS
    code << fmt::format(R"RAW( dic["{0}"] = self_c.{0}; )RAW", f->getNameAsString());
  }

  code << "return dic.new_ref();} \n";

  table << fmt::format(R"RAW( {{"__dict__", (getter)prop_get_dict_{0}, nullptr, "", nullptr}},)RAW", //
                       counter);

  ++counter;
}

// ===================================================================

void codegen_getter_setter(std::ostream &table, std::ostream &doc, str_t const &prop_name, cls_info_t::property const &prop) {

  static long counter = 0;

  bool has_setter     = !prop.setters.empty();
  auto *getter_method = prop.getter.as_method(); // null when getter is a free function

  auto gsdoc   = clu::doc_string_t{prop.getter.ptr};
  auto doc_str = gsdoc.brief_str;
  doc_str += gsdoc.details_str.empty() ? "" : (doc_str.empty() ? gsdoc.details_str : fmt::format("\n\n{}", gsdoc.details_str));
  doc << fmt::format(R"RAW( static constexpr auto prop_doc_{0} = R"DOC({1})DOC"; )RAW", counter, doc_str);

  // ---- getter ----
  str_t getter_entry;
  if (getter_method) {
    auto cast_op = fmt::format("cast{}<>", getter_method->isStatic() ? "" : (getter_method->isConst() ? "mc" : "m"));
    getter_entry = fmt::format("c2py::getter_from_method<c2py::{}(&{})>", cast_op, prop.getter.ptr->getQualifiedNameAsString());
  } else {
    getter_entry = fmt::format("c2py::getter_from_fun<&{}>", prop.getter.ptr->getQualifiedNameAsString());
  }

  // ---- setter ----
  // FIXME: implement the LIST of setters.
  str_t setter_entry  = "nullptr";
  str_t closure_entry = "nullptr";
  if (has_setter) {
    auto setter_name    = prop.setters[0].ptr->getQualifiedNameAsString();
    auto *setter_method = prop.setters[0].as_method();
    setter_entry  = fmt::format("(setter)c2py::{}<&{}>", setter_method ? "setter_from_method" : "setter_from_fun", setter_name);
    closure_entry = fmt::format(R"RAW((void*)"Cannot delete the attribute {}")RAW", prop_name);
  }

  table << fmt::format(R"RAW( {{"{}", {}, {}, prop_doc_{}, {}}},)RAW", prop_name, getter_entry, setter_entry, counter, closure_entry);

  counter++;
}

// ===================================================================

void codegen_getsetitem(std::ostream &code, cls_info_t const &cls_info) {

  static long counter = 0;

  if ((not cls_info.has_size_method) and cls_info.getitems.empty()) return;

  auto const *cls = cls_info.ptr;
  auto cls_name   = clu::get_fully_qualified_name(cls);

  auto get_ovs = [&cls_name](auto &f_info) {
    auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f_info.ptr);
    return fmt::format(R"RAW( c2py::cfun2(c2py::getitem<{0}, {1}>))RAW", cls_name, fnt_paramtypes(m));
  };

  auto set_ovs = [&cls_name](auto &f_info) {
    auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f_info.ptr);
    return fmt::format(R"RAW( c2py::cfun2(c2py::setitem<{0}, {1}>))RAW", cls_name, fnt_paramtypes(m));
  };

  str_t size_code    = (cls_info.has_size_method ? fmt::format("c2py::tpxx_size<{}>", cls_name) : "nullptr");
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

    // yes, nested : no setitem if no getitems
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
                      cls_name, size_code, getitem_code, setitem_code);

  counter++;
}

// ===================================================================

void codegen_operators(std::ostream &code, cls_info_t const &cls_info) {

  // Map binary arithmetic OpKinds to c2py::OpName strings; returns nullptr for non-arithmetic ops
  auto to_arith_name = [](OpKind k) -> const char * {
    switch (k) {
      case OpKind::Add: return "Add";
      case OpKind::Sub: return "Sub";
      case OpKind::Mul: return "Mul";
      case OpKind::Div: return "Div";
      default: return nullptr;
    }
  };

  auto const *cls = cls_info.ptr;
  auto cls_name   = clu::get_fully_qualified_name(cls);
  auto &ctx       = cls->getASTContext();

  int count = 0;
  for (auto const &[kind, sigs] : cls_info.operators) {
    auto *op_name = to_arith_name(kind);
    if (not op_name or sigs.empty()) continue;
    ++count;

    std::vector<str_t> pairs;
    for (auto const &sig : sigs) {
      EXPECTS(sig.size() == 2);
      pairs.push_back(fmt::format("std::pair<{}, {}>", clu::get_fully_qualified_name(sig[0], ctx), clu::get_fully_qualified_name(sig[1], ctx)));
    }

    code << fmt::format("\ntemplate <> struct c2py::arithmetic<{0}, c2py::OpName::{1}> : std::tuple<{2}> {{}};\n", cls_name, op_name, join(pairs, ", "));
  }

  if (count > 0) code << fmt::format("\ntemplate <> constexpr PyNumberMethods *c2py::tp_as_number<{0}> = &c2py::tp_as_number_impl<{0}>;\n", cls_name);
}

// ===================================================================

void codegen_cls(std::ostream &code, str_t const &cls_py_name, cls_info_t const &cls_info, str_t const &full_module_name) {

  logs.cls(fmt::format("{1} [Python: {0}]", cls_py_name, cls_info.ptr->getQualifiedNameAsString()));

  auto *cls     = cls_info.ptr;
  auto cls_name = clu::get_fully_qualified_name(cls); //cls->getQualifiedNameAsString();

  // -- tp_name
  code << '\n' << fmt::format(R"RAW(template <> inline constexpr auto c2py::tp_name<{0}> = "{1}.{2}";)RAW", cls_name, full_module_name, cls_py_name);

  // ---------- Methods ------------
  {
    std::stringstream MethodDecls, MethodTable, MethodDocs;

    // ---- constructor
    if (cls_info.synthetize_init_from_pydict() and not(cls_info.fields.empty())) {
      codegen_synth_constructor(MethodDecls, cls_info);
    } else
      codegen::write_dispatch_constructors(MethodDecls, cls_name, cls_info.constructors);

    // ---- methods
    for (auto const &[fpyname, overloads] : cls_info.methods)
      codegen::write_dispatch(MethodDecls, MethodTable, MethodDocs, fpyname, overloads, cls, true);

    // ----- hdf5 : __write_hdf5__
    if (cls_info.has_hdf5) MethodTable << fmt::format(R"RAW( {{"__write_hdf5__", c2py::tpxx_write_h5<{0}>, METH_VARARGS, "  "}}, )RAW", cls_name);

    // ----- Serialization

    if (cls_info.serialization != Serialization::None) {
      static auto ser_opt_vec = std::vector<str_t>{"", "tuple", "h5", "repr"};
      auto set_opt            = ser_opt_vec[int(cls_info.serialization)];
      MethodTable << fmt::format(R"RAW({{"__getstate__", c2py::getstate_{0}<{1}>, METH_NOARGS, ""}},)RAW", set_opt, cls_name);
      MethodTable << fmt::format(R"RAW({{"__setstate__", c2py::setstate_{0}<{1}>, METH_O, ""}},)RAW", set_opt, cls_name);
    }
    // ----- assemble the code

    code << MethodDecls.str();
    code << MethodDocs.str();

    code << fmt::format(R"RAW(

      // ----- Method table ----
      template <> PyMethodDef c2py::tp_methods<{0}>[] = {{
           {1}
           {{nullptr, nullptr, 0, nullptr}} // Sentinel
      }};

     )RAW",
                        cls_name, MethodTable.str());
  }

  // ---------- Members ------------

  static long member_counter = 0;

  std::stringstream MembersDoc, Members;
  for (auto *f : cls_info.fields) {

    auto name     = str_t{f->getName()};
    auto type     = clu::get_fully_qualified_name(f->getType(), f->getASTContext());
    auto is_const = f->getType().isConstQualified();
    if (is_const) type = "const " + type;

    auto fdoc     = clu::doc_string_t{f};
    auto fdoc_str = fdoc.brief_str;
    fdoc_str += fdoc.details_str.empty() ? "" : (fdoc_str.empty() ? fdoc.details_str : fmt::format("\n\n{}", fdoc.details_str));

    MembersDoc << fmt::format(R"RAW( constexpr auto doc_member_{0} = R"DOC({1})DOC"; )RAW", member_counter, fdoc_str);

    if (is_const)
      Members << fmt::format(R"RAW(
                         {{"{0}", c2py::get_member<&{1}::{0}, {1}>, nullptr, doc_member_{3}, nullptr}},
                          )RAW",
                             name, cls_name, type, member_counter);
    else
      // {{"{0}", c2py::get_member<&{1}::{0}>, c2py::set_member<&{1}::{0}>, doc_member_{3}, nullptr}},
      Members << fmt::format(R"RAW( c2py::getsetdef_from_member<&{1}::{0}, {1}>("{0}", doc_member_{3}),)RAW", name, cls_name, type, member_counter);
    ++member_counter;
  }

  code << MembersDoc.str();

  // ---------- Properties ------------

  std::stringstream Properties, PropertiesDocs;
  for (auto const &[pyname, prop] : cls_info.properties) {
    logs.prop(fmt::format("{}", pyname));
    codegen_getter_setter(Properties, PropertiesDocs, pyname, prop);
  }

  if (cls_info.synthetize_dict_attribute()) codegen_synth___dict_attribute(code, Properties, cls_info);

  code << PropertiesDocs.str();

  // ---------- member & prop table

  code << fmt::format(R"RAW(

      // ----- Method table ----

      template <> constinit PyGetSetDef c2py::tp_getset<{0}>[] = {{
         {1}
         {2}
         {{nullptr,nullptr,nullptr,nullptr,nullptr }}
      }};

      )RAW",
                      cls_name, Members.str(), Properties.str());

  // ---------- operator [] & size as len

  codegen_getsetitem(code, cls_info);

  // ---------- arithmetic operators

  codegen_operators(code, cls_info);

  // ----------- import other modules

  // -- tp_doc
  auto cls_doc = pydoc(cls_info);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_doc<{0}> = R"DOC({1})DOC" + )RAW", cls_name, cls_doc)
       << (cls_doc.empty() ? "" : R"RAW( std::string{"\n\n----------\n\n"}  + )RAW") << fmt::format(R"RAW(c2py::tp_ctor_doc<{0}>;)RAW", cls_name);
}
