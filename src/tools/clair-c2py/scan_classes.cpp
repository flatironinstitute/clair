#include <iostream>
#include "llvm/ADT/DenseSet.h"

#include "clu/misc.hpp"
#include "clu/concept.hpp"
#include "utility/logger.hpp"

#include "./check_convertibility.hpp"
#include "./analyze_operator.hpp"
#include "./decl_utils.hpp"
#include "./scan_classes.hpp"

static const struct {
  util::logger rejected = util::logger{&std::cout, "-- ", "\033[1;33mRejecting: \033[0m"};
} logs;

// ------------------------------
// Takes a list of methods, and return a list without const/non const method duplication.
// When both a const and non-const method share the same parameter types, keep only the non-const version.
// Preserves original order.
static std::vector<fnt_info_t> rm_const_overloads(std::vector<fnt_info_t> const &mlist) {

  auto get_param_types = [](fnt_info_t const &fi) {
    llvm::SmallVector<clang::QualType> params;
    params.reserve(fi.ptr->getNumParams());
    for (auto const *p : fi.ptr->parameters()) params.push_back(p->getType());
    return params;
  };

  // For each param signature, record the index of the preferred overload (non-const wins).
  std::map<llvm::SmallVector<clang::QualType>, size_t> best;
  for (size_t i = 0; i < mlist.size(); ++i) {
    auto params         = get_param_types(mlist[i]);
    auto *method        = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(mlist[i].ptr);
    bool is_const       = method && method->isConst();
    auto [it, inserted] = best.try_emplace(std::move(params), i);
    if (!inserted && !is_const) it->second = i; // non-const wins
  }

  // Collect winners in original order.
  llvm::DenseSet<size_t> winner_indices;
  for (auto const &[_, idx] : best) winner_indices.insert(idx);

  std::vector<fnt_info_t> result;
  result.reserve(winner_indices.size());
  for (size_t i = 0; i < mlist.size(); ++i)
    if (winner_indices.contains(i)) result.push_back(mlist[i]);
  return result;
}

// ------------------------------

static void analyze_one_method(clang::FunctionDecl const *f, cls_info_t &cls_info, cls_ptr_t cls, wdata_t &wd) {

  if (f->isDeleted()) return;
  auto *m = llvm::dyn_cast<clang::CXXMethodDecl>(f);
  if (!m) return;
  if (llvm::isa<clang::CXXDestructorDecl>(m)) return;                   // no destructors
  if (m->isMoveAssignmentOperator()) return;                            // no move assign
  if (auto *ctr = llvm::dyn_cast_or_null<clang::CXXConstructorDecl>(m); //
      ctr and ctr->isCopyOrMoveConstructor())
    return; // no move or copy constructor

  auto name = m->getNameAsString();

  // ---- constructors
  if (llvm::isa<clang::CXXConstructorDecl>(m)) {
    const bool is_base_class = (cls != cls_info.ptr);
    if (not is_base_class and check_convertibility(m, wd)) cls_info.constructors.push_back({m});
    return;
  }
  // ---- operators : keep only [] and ()
  if (name.starts_with("operator")) {
    if (name == "operator[]") {
      // Do not check the return type, only the parameters for the setitem, it is coded differently
      // than other functions
      if (check_convertibility(m, wd, m->isConst())) (m->isConst() ? cls_info.getitems : cls_info.setitems).push_back({m});
    } else if (name == "operator()") {
      if (check_convertibility(m, wd)) cls_info.methods["__call__"].push_back({m});
    } else {
      analyze_operator(f, wd);
    }
    return;
  }

  // ---- special cases first
  if (name == "size" and (f->getNumParams() == 0)) {
    cls_info.has_size_method = true;
    return;
  }

  // ---- iterator methods : begin, end and co
  if (name == "begin" or name == "end" or name == "cend" or name == "cbegin") {
    cls_info.has_iterator = true;
    return; // do not wrap these methods
  }

  // ---- explicit property annotations
  if (auto prop_name = clu::get_annotation_value(m, "c2py_property_get")) {
    if (m->getNumParams() != 0)
      clu::emit_error(m, "c2py: C2PY_PROPERTY_GET method must take no parameters");
    else if (m->getReturnType()->isVoidType())
      clu::emit_error(m, "c2py: C2PY_PROPERTY_GET method must not return void");
    else if (check_convertibility(m, wd))
      cls_info.properties[*prop_name].getter = {.ptr = m};
    return;
  }
  if (auto prop_name = clu::get_annotation_value(m, "c2py_property_set")) {
    if (check_convertibility(m, wd)) cls_info.properties[*prop_name].setters.push_back({.ptr = m});
    return;
  }

  // wrap_no_arg_methods_as_properties: treat no-arg non-void methods as read-only properties.
  if (wd.config.wrap_no_arg_methods_as_properties and m->getNumParams() == 0 and not m->getReturnType()->isVoidType()) {
    if (check_convertibility(m, wd)) cls_info.properties[get_python_name(m)].getter = {.ptr = m};
    return;
  }

  // generic case
  if (check_convertibility(m, wd)) cls_info.methods[get_python_name(m)].push_back({m});
}

// ------------------------------

// Given cls, stores its methods and friend functions
static void scan_class_elements(cls_info_t &cls_info, cls_ptr_t cls, wdata_t &wd) {

  for (clang::Decl *decl : cls->decls()) { // all declarations in the class
    if (decl->getAccess() != clang::AS_public) continue;
    if (should_reject(decl, wd.reject_names, &logs.rejected)) continue;

    // --------  method
    if (auto *m = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
      analyze_one_method(m, cls_info, cls, wd);
    }
    // -------- templated method
    else if (auto *m_tpl = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl)) {
      for (auto *spec : m_tpl->specializations())
        if (auto *info = spec->getTemplateSpecializationInfo(); info and info->isExplicitInstantiationOrSpecialization())
          analyze_one_method(spec, cls_info, cls, wd);
    }
    // -------- fields
    else if (auto *f = llvm::dyn_cast<clang::FieldDecl>(decl)) {
      auto ty = f->getType();
      if (not wd.module_info.is_wrapped(ty)) {
        if (not wd.concepts.IsConvertiblePy2C.is_satisfied_by(ty)) clu::emit_error(f, "c2py: Can not be converted from python to C++");
        if (not wd.concepts.IsConvertibleC2Py.is_satisfied_by(ty)) clu::emit_error(f, "c2py: Can not be converted from C++ to python");
      }
      cls_info.fields.push_back(f);
    }
  }
}

// ------------------------------

/// Scan a single class: detect serialization and HDF5 support, collect methods/fields from
/// base classes, deduplicate overloads, and verify default-constructibility.
static void scan_class(wdata_t &wd, cls_info_t &cls_info) {

  // h5
  cls_info.has_hdf5 = wd.concepts.HasHdf5.is_satisfied_by(cls_info.ptr);

  // Serialization
  if (wd.concepts.HasSerializeLikeBoost.is_satisfied_by(cls_info.ptr))
    cls_info.serialization = Serialization::Tuple;
  else if (cls_info.has_hdf5)
    cls_info.serialization = Serialization::H5;

  // Get the methods and fields of the class
  scan_class_elements(cls_info, cls_info.ptr, wd);

  // We loop on base classes which are not wrapped
  // and authorize 1 base class to be wrapped (Python C API limitation)
  for (auto b : cls_info.ptr->bases()) {
    if (b.getAccessSpecifier() != clang::AccessSpecifier::AS_public) continue; // only public bases
    auto *c = b.getType()->getAsCXXRecordDecl();
    if (not wd.module_info.is_wrapped(b.getType())) {
      // We merge the element of the base into the class in progress.
      scan_class_elements(cls_info, c, wd);
    } else {
      if (cls_info.base != nullptr) clu::emit_error(cls_info.ptr, "This class has more than one bases to wrap");
      cls_info.base = c;
    }
  }
  // finally we remove the const/non const duplicate in methods
  for (auto &[n, v] : cls_info.methods) {
    v = make_unique(v);
    v = rm_const_overloads(v);
  }

  // Check that the class is default constructible if it has no wrapped constructors
  if (cls_info.constructors.empty() and not cls_info.synthetize_dict_attribute()
      and not wd.concepts.HasNonDeletedDefaultConstructor.is_satisfied_by(cls_info.ptr))
    clu::emit_error(cls_info.ptr, "This class has no wrapped constructor and is not default constructible.");
}

// ------------------------------

void scan_classes(wdata_t &wd) {
  for (auto &[_, cls_info] : wd.module_info.classes) scan_class(wd, cls_info);
}
