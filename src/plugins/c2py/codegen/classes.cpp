#include "./fnt.hpp"
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
} logs;

// ===================================================================

void codegen_synth_constructor(std::ostream &code, clang::CXXRecordDecl const *cls) {

  logs.cls_details(fmt::format("Synthesize constructor from pydict", clu::get_fully_qualified_name(cls))); //->getQualifiedNameAsString()));
  static long counter = 0;

  std::vector<std::string> non_default_const_params;
  std::vector<clang::FieldDecl *> simple_fields;
  simple_fields.reserve(100); //NOLINT

  for (clang::FieldDecl *f : cls->fields()) {
    if (f->getAccess() != clang::AS_public) { // FIXME error is quite late... in codegen ...
      clu::emit_error(f, "c2py: Synthetizing constructor for pydict. Private fields not supported");
      continue;
    }
    // if f is a type, which has no default constructor and no defaut initializer is the class
    // we build it at the construction of the object, using designated initializer (as we skip other fields)
    if (auto *clsf = f->getType()->getAsCXXRecordDecl(); clsf and not clsf->hasDefaultConstructor() and (f->getInClassInitializer() == nullptr)) {
      non_default_const_params.push_back(fmt::format(R"RAW(.{1} = de.get<{0}>("{1}"))RAW", clsf->getQualifiedNameAsString(), f->getNameAsString()));
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
                      cls->getQualifiedNameAsString(), counter, join(non_default_const_params, ','));

  for (auto *f : simple_fields)
    code << fmt::format(R"RAW( de("{0}", self_c.{0}, {1}); )RAW", f->getNameAsString(), (f->getInClassInitializer() != nullptr));

  code << fmt::format(R"RAW(
       return de.check();
     }}

     template <> constexpr initproc c2py::tp_init<{}> = synth_constructor_{};
   )RAW",
                      cls->getQualifiedNameAsString(), counter);

  ++counter;
}

// ===================================================================

void codegen_synth___dict_attribute(std::ostream &code, std::ostream &table, clang::CXXRecordDecl const *cls) {

  static long counter = 0;

  code << '\n'
       << fmt::format(R"RAW( static PyObject *prop_get_dict_{0}(PyObject *self, void *) {{
                              auto & self_c = *(((c2py::wrap<{1}> *)self)->_c);
                              c2py::pydict dic; )RAW",
                      counter, cls->getQualifiedNameAsString());

  for (clang::FieldDecl *f : cls->fields()) {
    if (f->getAccess() != clang::AS_public) continue;
    code << fmt::format(R"RAW( dic["{0}"] = self_c.{0}; )RAW", f->getNameAsString());
  }

  code << "return dic.new_ref();} \n";

  table << fmt::format(R"RAW( {{"__dict__", (getter)prop_get_dict_{0}, nullptr, "", nullptr}},)RAW", //
                       counter);

  ++counter;
}

// ===================================================================

void codegen_getter_setter(std::ostream &code, std::ostream &table, std::ostream &doc, str_t const &prop_name, cls_info_t::property const &prop) {

  static long counter = 0;

  bool has_setter = !prop.setters.empty();

  // FIXME : implement teh LIST of setters.
  if (has_setter)
    code << fmt::format(R"RAW(

      static int prop_set_{counter}(PyObject *self, PyObject *value, void *closure) {{
         if (value == NULL) return (PyErr_SetString(PyExc_AttributeError, "Cannot delete the attribute {prop_name}"), -1);
         static c2py::dispatcher_f_kw_t d = {{c2py::cfun(&{setter}, "i")}};
         d(self, c2py::pyref(PyTuple_Pack(1, value)), nullptr);
        return 0;
      }}

)RAW",
                        "counter"_a = counter, "prop_name"_a = prop_name, "setter"_a = prop.setters[0].ptr->getQualifiedNameAsString());

  doc << fmt::format(R"RAW( static constexpr auto prop_doc_{0} = R"DOC({1})DOC"; )RAW", counter, clu::get_raw_comment(prop.getter.ptr));

  // Put in the table
  auto m = prop.getter.as_method();
  EXPECTS(m);
  auto cast_op = fmt::format("cast{}<>", (m ? (m->isStatic() ? "" : (m->isConst() ? "mc" : "m")) : ""));
  // static method -> cast, const method -> castmc, non const, non static method -> castm

  auto setter = (has_setter ? fmt::format("(setter)prop_set_{0}", counter) : str_t{"nullptr"});
  table << fmt::format(R"RAW( {{"{1}", c2py::getter_from_method<c2py::{3}(&{2})>, {4}, prop_doc_{0}, nullptr}},)RAW", //
                       counter, prop_name, prop.getter.ptr->getQualifiedNameAsString(), cast_op, setter);

  counter++;
}

// ===================================================================

void codegen_getsetitem(std::ostream &code, cls_info_t const &cls_info) {

  static long counter = 0;

  if ((not cls_info.has_size_method) and cls_info.getitems.empty()) return;

  auto const *cls = cls_info.ptr;

  auto get_ovs = [cls](auto &f_info) {
    auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f_info.ptr);
    return fmt::format(R"RAW( c2py::cfun2(c2py::getitem<{0}, {1}>))RAW", cls->getQualifiedNameAsString(), fnt_paramtypes(m));
  };

  auto set_ovs = [cls](auto &f_info) {
    auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f_info.ptr);
    return fmt::format(R"RAW( c2py::cfun2(c2py::setitem<{0}, {1}>))RAW", cls->getQualifiedNameAsString(), fnt_paramtypes(m));
  };

  str_t size_code    = (cls_info.has_size_method ? fmt::format("c2py::tpxx_size<{}>", cls->getQualifiedNameAsString()) : "nullptr");
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
                      cls->getQualifiedNameAsString(), size_code, getitem_code, setitem_code);

  counter++;
}

// ===================================================================

void codegen_cls(std::ostream &code, str_t const &cls_py_name, cls_info_t const &cls_info, str_t const &full_module_name) {

  logs.cls(fmt::format("{1} [Python: {0}]", cls_py_name, cls_info.ptr->getQualifiedNameAsString()));

  auto *cls     = cls_info.ptr;
  auto cls_name = clu::get_fully_qualified_name(cls); //cls->getQualifiedNameAsString();

  // -- first declaration
  code << fmt::format(R"RAW(

       template <> inline const std::string c2py::cpp_name<{0}> = "{0}";
       template <> inline constexpr auto c2py::tp_name<{0}>      = "{3}.{1}";
       template <> inline constexpr const char *c2py::tp_doc<{0}> = {2};

     )RAW",
                      cls_name, cls_py_name, fmt::format(R"RAW( R"DOC({})DOC" )RAW", pydoc(cls_info)), full_module_name);

  // ---------- Methods ------------
  {
    std::stringstream MethodDecls, MethodTable, MethodDocs;

    // ---- constructor
    if (cls_info.synthetize_init_from_pydict()) {
      // FIXME : check
      codegen_synth_constructor(MethodDecls, cls_info.ptr);
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
  for (auto f : cls_info.fields) {

    auto name     = str_t{f->getName()};
    auto type     = clu::get_fully_qualified_name(f->getType(), f->getASTContext());
    auto is_const = f->getType().isConstQualified();
    if (is_const) type = "const " + type;

    // FIXME Clean this, not great.
    auto doc = clu::get_raw_comment(f);
    // regex : replace ^  ///  by a space
    //auto doc1 = clu::doc_string_t(f);
    //auto doc = doc1.brief + " " + doc1.content;

    MembersDoc << fmt::format(R"RAW( constexpr auto doc_member_{0} = R"DOC({1})DOC"; )RAW", member_counter, doc);

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

  std::stringstream PropertiesDecls, Properties, PropertiesDocs;
  for (auto const &[pyname, prop] : cls_info.properties) codegen_getter_setter(PropertiesDecls, Properties, PropertiesDocs, pyname, prop);

  if (cls_info.synthetize_dict_attribute()) codegen_synth___dict_attribute(code, Properties, cls_info.ptr);

  code << PropertiesDecls.str();
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

  // ----------- import other modules
}
