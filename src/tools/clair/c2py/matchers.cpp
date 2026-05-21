#include "./matchers.hpp"
#include "clang/Basic/SourceManager.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include <clang/Sema/Sema.h>
#include <clang/Sema/Template.h>

#include <algorithm>
#include "clu/misc.hpp"
#include "clu/concept.hpp"
#include "clu/fullqualifiedname.hpp"
#include "clu/inject_bool_vartempl_specialization.hpp"
#include "utility/logger.hpp"
#include "utility/macros.hpp"
#include "./decl_utils.hpp"
#include "./check_convertibility.hpp"
#include "./analyze_operator.hpp"

static const struct {
  util::logger rejected = util::logger{"-- ", "\033[1;33mRejecting: \033[0m", 1};
  util::logger error    = util::logger{"-- ", "\033[1;31mError:  \033[0m",    0};
} logs;

static clang::QualType cls_qual_type(clang::CXXRecordDecl const *cls) {
#if LLVM_VERSION_MAJOR >= 22
  return cls->getASTContext().getCanonicalTagType(cls);
#else
  return cls->getASTContext().getTagDeclType(cls);
#endif
}

// ------------------------------

static void add_enum(clang::EnumDecl const *enu, wdata_t *wdata) {
  if (!enu) return;
  if (should_reject(enu, wdata->reject_names, &logs.rejected)) return;
  auto fqn = enu->getQualifiedNameAsString();
  if (std::ranges::any_of(wdata->module_info.enums, [&fqn](auto const &e) { return e->qualified_name == fqn; })) return;
  wdata->module_info.enums.push_back(wdata->module_info.intern(ir::EnumDecl{*enu}));
}

// ------------------------------

template <> void matcher<mtch::Concept>::run(const MatchResult &Result) {

  // warning : we must use the if here, as result may not be a concept
  // since there is no explicit ConceptDecl Matcher in AST yet.
  if (const auto *cpt = Result.Nodes.getNodeAs<clang::ConceptDecl>("conceptDecl")) {
    auto cname = cpt->getName().str();

    if (cname == "IsConvertiblePy2C")
      wdata->concepts.IsConvertiblePy2C = {cpt, wdata->ci};
    else if (cname == "IsConvertibleC2Py")
      wdata->concepts.IsConvertibleC2Py = {cpt, wdata->ci};
    else if (cname == "HasSerializeLikeBoost")
      wdata->concepts.HasSerializeLikeBoost = {cpt, wdata->ci};
    else if (cname == "HasNonDeletedDefaultConstructor")
      wdata->concepts.HasNonDeletedDefaultConstructor = {cpt, wdata->ci};
    else if (cname == "Storable")
      wdata->concepts.HasHdf5 = {cpt, wdata->ci};
    // else ignore the others concepts
  }
}

// ------------------------------

// Match the using PythonName = my_class<...> in c2py_module namespace
// in the case of a class template specialization only
template <> void matcher<mtch::ModuleClsWrap>::run(const MatchResult &Result) {
  auto *d = Result.Nodes.getNodeAs<clang::TypeAliasDecl>("decl");
  assert(d);
  if (auto *cls = d->getUnderlyingType()->getAsCXXRecordDecl()) {
    if (auto *ctsd = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cls)) {
      if (not cls->hasDefinition() and not ctsd->isCompleteDefinition() and ctsd->getSpecializationKind() != clang::TSK_ExplicitSpecialization) {
        // The alias (e.g. A<int>) was not instantiated in the code.
        // Clang is lazy with aliases, so we use Sema to instantiate it ourselves.
        clang::CXXRecordDecl *Pattern = ctsd->getSpecializedTemplate()->getTemplatedDecl();
        auto &SemaRef                 = wdata->ci->getSema();

        SemaRef.InstantiateClass(ctsd->getLocation(),                        // PointOfInstantiation
                                 ctsd,                                       // Instantiation
                                 Pattern,                                    // Pattern
                                 SemaRef.getTemplateInstantiationArgs(ctsd), // TemplateArgs
                                 clang::TSK_ExplicitInstantiationDefinition,
                                 /*Complain=*/true);
        if (ctsd->isInvalidDecl()) clu::emit_error(d, "c2py: Error in instantiating the class on the right hand side of the alias");
      }
    }
    wdata->add_class_to_module(d->getName(), cls);
    wdata->clang_cls_by_fqn.emplace(clu::get_fully_qualified_name(cls->getCanonicalDecl()), cls->getCanonicalDecl());
    clu::inject_bool_vartempl_specialization(wdata->ci->getSema(), wdata->is_wrapped_vtd,
                               cls_qual_type(cls), true);
  }
}

// ------------------------------

// the registration part of the class matcher
// pulled out because it recursively calls itself on nested classes
static void register_class(clang::CXXRecordDecl const *cls, wdata_t *wdata) {

  if (!cls) return; // just in case
  if (cls->getASTContext().getDiagnostics().hasErrorOccurred()) return;

  // Filter some automatic instantiation from the compiler, and alike
  if (!cls->getSourceRange().isValid()) return;
  if (!cls->isCompleteDefinition()) return; // skip forward declaration.
  if (cls->isLambda()) return;              // no lambda
  if (auto a = cls->getAccess(); a == clang::AccessSpecifier::AS_protected or a == clang::AccessSpecifier::AS_private)
    return; // remove protected/private classes

  // only struct and class
  if (not((cls->getTagKind() == clang::TagTypeKind::Struct) or (cls->getTagKind() == clang::TagTypeKind::Class))) return;

  // Reject the declaration of a class template (not an instantiation)
  if (cls->getDescribedClassTemplate()) return;

  // Reject template specialization (even explicit ones — they are handled
  // via the ModuleClsWrap matcher through using-declarations instead).
  if (llvm::isa<clang::ClassTemplateSpecializationDecl>(cls)) return;

  // ---- Apply the filters

  // apply c2py_ignore and reject_names
  if (should_reject(cls, wdata->reject_names, &logs.rejected)) return;

  // Reject a class that already has an explicit converter, either because:
  // (a) is_wrapped<cls>=true was injected (wrapped in another module), or
  // (b) an explicit py_converter<cls> specialization exists in source (e.g. py_converter_as_any).
  // We check the AST directly to avoid evaluating IsConvertiblePy2C, which would cache
  // is_wrapped<cls>=false and poison subsequent concept checks after we inject true below.
  {
    auto &Ctx    = wdata->ci->getASTContext();
    auto clstype = Ctx.getCanonicalType(cls_qual_type(cls));
    clang::TemplateArgument Arg{clstype};
    void *IP = nullptr;
    if (auto *ex = wdata->is_wrapped_vtd->findSpecialization({Arg}, IP);
        ex and ex->getTemplateSpecializationKind() == clang::TSK_ExplicitSpecialization) {
      logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", cls->getQualifiedNameAsString(), "Already has a converter (is_wrapped = true)"));
      return;
    }
    IP = nullptr;
    if (auto *ex = wdata->py_converter_ctd->findSpecialization({Arg}, IP);
        ex and ex->getTemplateSpecializationKind() == clang::TSK_ExplicitSpecialization) {
      logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", cls->getQualifiedNameAsString(),
                                "Already has a converter (Explicit py_converter specialization in source)"));
      return;
    }
  }

  // Insert in the module class list; also register clang ptr for concept checking in scan_classes.
  wdata->add_class_to_module(get_python_name(cls), cls);
  wdata->clang_cls_by_fqn.emplace(clu::get_fully_qualified_name(cls->getCanonicalDecl()), cls->getCanonicalDecl());
  // mark as wrapped in the AST
  // so that check_convertibility sees is_wrapped<cls> = true for subsequent checks.
  clu::inject_bool_vartempl_specialization(wdata->ci->getSema(), wdata->is_wrapped_vtd,
                             cls_qual_type(cls), true);

  // Finally register recursively the nested classes and enums, as they can be pruned by the namespaces directive
  for (auto const *d : cls->decls()) {
    if (auto const *inner = llvm::dyn_cast<clang::CXXRecordDecl>(d); inner) register_class(inner, wdata);
    if (auto const *enu = llvm::dyn_cast<clang::EnumDecl>(d)) add_enum(enu, wdata);
  }
}

// ------------------------------

template <> void matcher<mtch::Cls>::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  const auto *cls = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");
  register_class(cls, this->wdata);
}

// ------------------------------

/// Return the underlying CXXRecordDecl if the canonical type is a C++ class/struct, or nullptr otherwise.
static clang::CXXRecordDecl *as_CXXRecordDecl(clang::QualType qtype) {
  qtype = qtype.getNonReferenceType().getCanonicalType();
  if (auto *rtype = qtype->getAs<clang::RecordType>())
    if (auto *cxxrec = llvm::dyn_cast<clang::CXXRecordDecl>(rtype->getDecl())) return cxxrec->getCanonicalDecl();
  return nullptr; // Not a class/struct type
}

// ------------------------------

// Find the cls_info_t for the first parameter of f, which must be a wrapped class.
// Returns a pointer to it, or nullptr after emitting an error if f has no parameters
// or its first parameter is not a wrapped class.
static cls_info_t *find_wrapped_cls_for_first_arg(clang::FunctionDecl const *f, module_info_t &M) {
  if (f->param_size() == 0) {
    clu::emit_error(f, "c2py: This annotated function must take at least 1 argument (self)");
    return nullptr;
  }
  auto *first_arg_type = as_CXXRecordDecl(f->getParamDecl(0)->getType());
  if (first_arg_type) {
    auto fqn = clu::get_fully_qualified_name(first_arg_type);
    if (auto it = M.classes_fqn_to_info.find(fqn); it != M.classes_fqn_to_info.end()) return &M.classes[it->second].second;
  }
  clu::emit_error(f->getParamDecl(0), "c2py: First argument is not a class being wrapped");
  return nullptr;
}

// ------------------------------

template <> void matcher<mtch::Fnt>::run(const MatchResult &Result) {

  auto *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("func");
  if (!f) return;

  // ............. Discard some automatic instantiation from the compiler, and alike
  // f in e.g. operator new, internal function, not defined in the sources
  // function defined in std headers are already filtered by the AST Matching
  if (!f->getBeginLoc().isValid()) return;

  // Skip deleted function
  if (f->isDeleted()) return;

  // Skip some internal functions generated by coroutines
  if (f->getNameAsString().starts_with("__builtin_coro_")) return;

  // skip the deduction guides (CTAD)
  if (llvm::isa<clang::CXXDeductionGuideDecl>(f)) return;

  // Instantiation: accept only EXPLICIT instantiation
  if (const auto *info = f->getTemplateSpecializationInfo(); info and not info->isExplicitInstantiationOrSpecialization()) return;

  // Reject function template declaration
  if (f->getDescribedFunctionTemplate()) return;

  // Reject functions with dependent (unresolved) types.
  // This filters out the pattern FunctionDecl of friend functions defined inside 
  // class templates whose types still contain template parameters like `type-parameter-0-0`.
  if (f->getType()->isDependentType()) return;

  // method should not be here
  EXPECTS(not llvm::isa<clang::CXXMethodDecl>(f));

  // Deduplicate friend declarations vs out-of-class definitions.
  // If this is a friend declaration (not a definition) and the definition
  // exists in this TU, skip — the definition will be matched separately.
  // If no definition is visible (defined in another .cpp), keep the declaration.
  if (f->getFriendObjectKind() != clang::Decl::FOK_None and not f->isThisDeclarationADefinition())
    if (f->getDefinition()) return;

  // Wrapping c2py functions makes no sense, and is probably a mistake in the configuration or includes. Panic...
  if (f->getQualifiedNameAsString().starts_with("c2py::")) {
    logs.error("FATAL ERROR: incorrect configuration or includes. It requests wrapping c2py functions, which makes no sense.");
    std::abort();
  }

  // apply c2py_ignore and the reject_name regex
  auto &M = wdata->module_info;
  if (should_reject(f, wdata->reject_names, &logs.rejected)) return;

  // h5_write/h5_read/h5_read_construct  are HDF5 serialization helpers, never meant to be wrapped
  // if we use the h5 method.
  if (wdata->concepts.HasHdf5)
    if (auto name = f->getNameAsString(); name == "h5_write" || name == "h5_read" || name == "h5_read_construct") {
      logs.rejected(fmt::format(R"RAW({0} [treated directly in h5 support])RAW", name));
      return;
    }

  // Special treatment for operator
  if (f->getNameAsString().starts_with("operator")) {
    analyze_operator(f, *wdata);
    return;
  }

  // ---- module_init tag
  // At most one function can be tagged as module_init
  // Its signature must be () -> void
  // it will be called by the Python module init function
  if (clu::has_annotation(f, "c2py_module_init")) {
    if (not M.module_init_fqn.empty())
      clu::emit_error(f, "Only one function can be tagged c2py_module_init.");
    if (f->param_size() != 0) clu::emit_error(f, "A function tagged c2py_module_init must take no arguments");
    if (not f->getReturnType()->isVoidType()) clu::emit_error(f, "A function tagged c2py_module_init must return void");
    M.module_init_fqn = f->getQualifiedNameAsString();
  }

  // ----- Check convertibility
  if (not check_convertibility(f, *wdata)) return;

  // ---- property annotations on free functions
  if (auto prop_name = clu::get_annotation_value(f, "c2py_property_get")) {
    if (auto *cli = find_wrapped_cls_for_first_arg(f, M))
      cli->properties[*prop_name].getter = fnt_info_t{.ptr = wdata->module_info.intern(ir::FunctionDecl{*f}), .rewrite = false};
    return;
  }
  if (auto prop_name = clu::get_annotation_value(f, "c2py_property_set")) {
    if (auto *cli = find_wrapped_cls_for_first_arg(f, M))
      cli->properties[*prop_name].setters.push_back(fnt_info_t{.ptr = wdata->module_info.intern(ir::FunctionDecl{*f}), .rewrite = false});
    return;
  }

  // ---- wrap as method of the class of the first argument
  if (clu::has_annotation(f, "c2py_wrap_as_method")) {
    if (auto *cli = find_wrapped_cls_for_first_arg(f, M))
      cli->methods[get_python_name(f)].push_back(fnt_info_t{.ptr = wdata->module_info.intern(ir::FunctionDecl{*f}), .rewrite = false});
    return;
  }

  // ---- generic free function
  M.functions[get_python_name(f)].push_back(fnt_info_t{.ptr = wdata->module_info.intern(ir::FunctionDecl{*f})});
}

// ------------------------------

template <> void matcher<mtch::Enum>::run(const MatchResult &Result) {
  auto *enu = Result.Nodes.getNodeAs<clang::EnumDecl>("en");
  add_enum(enu, wdata);
}
