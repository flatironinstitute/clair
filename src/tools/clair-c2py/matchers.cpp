#include "./matchers.hpp"
#include "clang/Basic/SourceManager.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include <clang/Sema/Sema.h>
#include <clang/Sema/Template.h>

#include "clu/misc.hpp"
#include "clu/concept.hpp"
#include "utility/logger.hpp"
#include "utility/macros.hpp"

static const struct {
  util::logger rejected = util::logger{&std::cout, "-- ", "\033[1;33mRejecting: \033[0m"};
  util::logger error    = util::logger{&std::cout, "-- ", "\033[1;31mError:  \033[0m"};
} logs;

// -----------------------------------------------------

template <> void matcher<mtch::Concept>::run(const MatchResult &Result) {

  // warning : we must use the if here, as result may not be a concept
  // since there is no explicit ConceptDecl Matcher in AST yet.
  if (const auto *cpt = Result.Nodes.getNodeAs<clang::ConceptDecl>("conceptDecl")) {
    auto cname = cpt->getName().str();

    if (cname == "IsConvertiblePy2C")
      worker->concepts.IsConvertiblePy2C = cpt;
    else if (cname == "IsConvertibleC2Py")
      worker->concepts.IsConvertibleC2Py = cpt;
    else if (cname == "HasSerializeLikeBoost")
      worker->concepts.HasSerializeLikeBoost = cpt;
    else if (cname == "HasNonDeletedDefaultConstructor")
      worker->concepts.HasNonDeletedDefaultConstructor = cpt;
    else if (cname == "Storable")
      worker->concepts.HasHdf5 = cpt;
    // else ignore the others concepts
  }
}

// --------------------------------------------------------------------------------

// Match the using PythonClass = my_class<...> in c2py_module namespace
// in the case of a class template specialization only
template <> void matcher<mtch::ModuleClsWrap>::run(const MatchResult &Result) {
  auto *d = Result.Nodes.getNodeAs<clang::TypeAliasDecl>("decl");
  assert(d);
  if (auto *cls = d->getUnderlyingType()->getAsCXXRecordDecl()) {
    if (auto *ctsd = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cls)) {
      if (not cls->hasDefinition() and not ctsd->isCompleteDefinition()) {
        // The alias (e.g. A<int>) was not instantiated in the code.
        // Clang is lazy with aliases, so we use Sema to instantiate it ourselves.
        clang::CXXRecordDecl *Pattern = ctsd->getSpecializedTemplate()->getTemplatedDecl();
        auto &SemaRef                 = worker->ci->getSema();

        SemaRef.InstantiateClass(ctsd->getLocation(),                        // PointOfInstantiation
                                 ctsd,                                       // Instantiation
                                 Pattern,                                    // Pattern
                                 SemaRef.getTemplateInstantiationArgs(ctsd), // TemplateArgs
                                 clang::TSK_ExplicitInstantiationDefinition,
                                 /*Complain=*/true);
        if (ctsd->isInvalidDecl()) clu::emit_error(d, "c2py: Error in instantiating the class on the right hand side of the alias");
      }
    }
    worker->module_info.add_class(d->getName().str(), cls);
  }
}

// -------------------------------------------------

// the analysis part of the class matcher
// pulled out because it recursively calls itself on nested classes
void analyze_class(clang::CXXRecordDecl const *cls, worker_t *worker) {

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
  if (worker->is_rejected(cls, &logs.rejected)) return;

  // reject a class which already HAS a converter Py2C
  // otherwise the wrapping would take precedence
  if (clu::satisfy_concept(cls, worker->concepts.IsConvertiblePy2C, worker->ci)) {
    logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", cls->getQualifiedNameAsString(), "Already has a converter"));
    return;
  }

  // Insert in the module class list
  worker->module_info.add_class(worker->get_python_name(cls), cls);

  // Finally analyze recursively the nested classes, as they can be pruned by the namespaces directive
  for (auto const *d : cls->decls()) {
    if (auto const *inner = llvm::dyn_cast<clang::CXXRecordDecl>(d); inner) analyze_class(inner, worker);
  }
}

// -------------------------------------------------

template <> void matcher<mtch::Cls>::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  const auto *cls = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");
  analyze_class(cls, this->worker);
}

// -------------------------------------------------

static clang::CXXRecordDecl *as_CXXRecordDecl(clang::QualType qtype) {
  qtype = qtype.getNonReferenceType().getCanonicalType();
  if (auto *rtype = qtype->getAs<clang::RecordType>())
    if (auto *cxxrec = llvm::dyn_cast<clang::CXXRecordDecl>(rtype->getDecl())) return cxxrec;
  return nullptr; // Not a class/struct type
}

// Validate that f has at least one parameter whose type is a wrapped class.
// Returns the corresponding cls_info_t, or nullptr after emitting an error.
static cls_info_t *find_wrapped_cls_for_first_arg(clang::FunctionDecl const *f, module_info_t &M) {
  if (f->param_size() == 0) {
    clu::emit_error(f, "c2py: This annotated function must take at least 1 argument (self)");
    return nullptr;
  }
  auto *first_arg_type = as_CXXRecordDecl(f->getParamDecl(0)->getType());
  if (auto it = M.classes_ptr_to_info.find(first_arg_type); it != M.classes_ptr_to_info.end())
    return &M.classes[it->second].second;
  clu::emit_error(f->getParamDecl(0), "c2py: First argument is not a class being wrapped");
  return nullptr;
}
// -------------------------------------------------
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

  // method should not be here
  EXPECTS(not llvm::isa<clang::CXXMethodDecl>(f));

  if (f->getQualifiedNameAsString().starts_with("c2py::")) {
    logs.error("FATAL ERROR: incorrect configuration or includes. It requests wrapping c2py functions which makes no sense.");
    std::abort();
  }

  // apply c2py_ignore and the reject_name regex
  auto &M = worker->module_info;
  if (worker->is_rejected(f, &logs.rejected)) return;

  // Special treatment for operator
  if (f->getNameAsString().starts_with("operator")) {
    worker->analyze_operator(f);
    return;
  }

  // ---- module_init tag
  // At most one function can be tagged as module_init
  // Its signature must be () -> void
  // it will be called by the Python module init function
  if (clu::has_annotation(f, "c2py_module_init")) {
    if (M.module_init) {
      clu::emit_error(f, "Only one function can be tagged c2py_module_init.");
      clu::emit_error(M.module_init, "The previous one was here.");
    }
    if (f->param_size() != 0) clu::emit_error(f, "A function tagged c2py_module_init must take no arguments");
    if (not f->getReturnType()->isVoidType()) clu::emit_error(f, "A function tagged c2py_module_init must return void");
    M.module_init = f;
  }

  // ----- Check convertibility
  if (not worker->check_convertibility(f)) return;

  // ---- property annotations on free functions
  if (auto prop_name = clu::get_annotation_value(f, "c2py_property_get")) {
    if (auto *cli = find_wrapped_cls_for_first_arg(f, M))
      cli->properties[*prop_name].getter = fnt_info_t{.ptr = f, .rewrite = false};
    return;
  }
  if (auto prop_name = clu::get_annotation_value(f, "c2py_property_set")) {
    if (auto *cli = find_wrapped_cls_for_first_arg(f, M))
      cli->properties[*prop_name].setters.push_back(fnt_info_t{.ptr = f, .rewrite = false});
    return;
  }

  // ---- wrap as method of the class of the first argument
  if (clu::has_annotation(f, "c2py_wrap_as_method")) {
    if (auto *cli = find_wrapped_cls_for_first_arg(f, M))
      cli->methods[worker->get_python_name(f)].push_back(fnt_info_t{.ptr = f, .rewrite = false});
    return;
  }

  // ---- generic free function
  M.functions[worker->get_python_name(f)].push_back(fnt_info_t{f});
}

// -------------------------------------------------

template <> void matcher<mtch::Enum>::run(const MatchResult &Result) {
  auto *enu = Result.Nodes.getNodeAs<clang::EnumDecl>("en");
  if (!enu) return;
  if (worker->is_rejected(enu, &logs.rejected)) return;
  worker->module_info.enums.push_back(enu);
}
