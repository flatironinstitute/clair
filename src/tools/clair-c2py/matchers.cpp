#include "./matchers.hpp"
#include "clang/Basic/SourceManager.h"
#include "clu/misc.hpp"
#include "clu/concept.hpp"
#include "utility/logger.hpp"
#include "utility/macros.hpp"
#include "clang/ASTMatchers/ASTMatchers.h"
#include <clang/Sema/Sema.h>
#include <clang/Sema/Template.h>

static const struct {
  util::logger rejected = util::logger{&std::cout, "-- ", "\033[1;33mRejecting: \033[0m"};
  util::logger note     = util::logger{&std::cout, "-- ", "\033[1;32mNote:  \033[0m"};
  util::logger error    = util::logger{&std::cout, "-- ", "\033[1;33mError:  \033[0m"};
} logs;

// -----------------------------------------------------

template <> void matcher<mtch::Concept>::run(const MatchResult &Result) {

  // warning : we must use the if here, as result may not be a concept
  // since there is no explicit ConceptDecl Matcher in AST yet.
  if (const auto *cpt = Result.Nodes.getNodeAs<clang::ConceptDecl>("conceptDecl")) {
    auto cname = cpt->getName().str();

    if (cname == "IsConvertiblePy2C")
      worker->IsConvertiblePy2C = cpt;
    else if (cname == "IsConvertibleC2Py")
      worker->IsConvertibleC2Py = cpt;
    else if (cname == "force_instantiation_add_methods")
      worker->force_instantiation_add_methods = cpt;
    else if (cname == "HasSerializeLikeBoost")
      worker->HasSerializeLikeBoost = cpt;
    else if (cname == "HasNonDeletedDefaultConstructor")
      worker->HasNonDeletedDefaultConstructor = cpt;
    else if (cname == "Storable")
      worker->HasHdf5 = cpt;
    // else ignore the others concepts
  }
}

// --------------------------------------------------------------------------------

template <> void matcher<mtch::ModuleClsWrap>::run(const MatchResult &Result) {
  auto *d = Result.Nodes.getNodeAs<clang::TypeAliasDecl>("decl");
  assert(d);
  if (auto *cls = d->getUnderlyingType()->getAsCXXRecordDecl()) {
    if (llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cls)) {
      if (not cls->hasDefinition()) {
        // We have an alias e.g. A<int>, but it was not instantiated in the code
        // clang is lazy with aliases, it does not instantiate them
        // We use the Sema to instantiate the class
        if (auto *ctsd = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cls); not ctsd->isCompleteDefinition()) {
          clang::CXXRecordDecl *Pattern = ctsd->getSpecializedTemplate()->getTemplatedDecl();
          auto &SemaRef                 = worker->ci->getSema();

          SemaRef.InstantiateClass(ctsd->getLocation(),                        // PointOfInstantiation
                                   ctsd,                                       // Instantiation
                                   Pattern,                                    // Pattern
                                   SemaRef.getTemplateInstantiationArgs(ctsd), // TemplateArgs
                                   clang::TSK_ExplicitInstantiationDefinition,
                                   /*Complain=*/true);
          if (ctsd->isInvalidDecl()) clu::emit_error(d, "c2py: Error in instantiating the class");
        }
        // Default previous behaviour: request that the user explicitely instantiate the class.
        //clu::emit_error(d, "c2py: Please instantiate the class explicitely");
      }
    }
    worker->module_info.add_class(d->getName().str(), cls);
  }
}

// -------------------------------------------------

template <> void matcher<mtch::Cls>::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  const auto *cls = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");

  if (!cls) return; // just in case
  if (cls->getASTContext().getDiagnostics().hasErrorOccurred()) return;

  // Filter some automatic instantiation from the compiler, and alike
  if (!cls->getSourceRange().isValid()) return;
  if (!cls->isCompleteDefinition()) return; // skip forward declaration.
  if (cls->isLambda()) return;              // no lambda
  if (auto a = cls->getAccess(); a == clang::AccessSpecifier::AS_protected or a == clang::AccessSpecifier::AS_private)
    return; // remove protected/private classes

#if LLVM_VERSION_MAJOR < 18
  if (not((cls->getTagKind() == clang::TTK_Struct) or (cls->getTagKind() == clang::TTK_Class))) return; // just struct and class
#else
  if (not((cls->getTagKind() == clang::TagTypeKind::Struct) or (cls->getTagKind() == clang::TagTypeKind::Class))) return; // just struct and class
#endif

  if (auto *s = llvm::dyn_cast_or_null<clang::ClassTemplateSpecializationDecl>(cls)) {
    if (!s->isExplicitInstantiationOrSpecialization()) return;
  }

  // Reject the declaration of the template itself.
  if (cls->getDescribedClassTemplate()) return;

  // FIXME : check ? why ?
  // Reject template specialization
  if (llvm::dyn_cast_or_null<clang::ClassTemplateSpecializationDecl>(cls)) return;

  // Apply the filters
  auto qname = cls->getQualifiedNameAsString();
  auto &M    = worker->module_info;

  // apply c2py_ignore and reject_names
  if (is_rejected(cls, worker->reject_names, &logs.rejected)) return;

  // Reject classes defined in c2py_module
  if (qname.starts_with("c2py_module::")) return;

  // reject a class which already HAS a converter Py2C
  // NB : if the class has already a C2py converter, it is overuled
  // by the wrapping. It is necessary since all classes with iterator
  // can have a default c2py converter as a generator
  // which is superseded by the wrapping it is exists
  if (clu::satisfy_concept(cls, worker->IsConvertiblePy2C, worker->ci)) {
    logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", qname, "Already has a converter"));
    return;
  }

  // Insert in the module class list
  str_t py_name = util::camel_case(cls->getNameAsString());
  M.add_class(py_name, cls); // classes.emplace_back(py_name, cls_info_t{cls}); //

  //if (not inserted) clu::emit_error(cls, "Class rejected. Should have another class with the same Python name ??");
}

// -------------------------------------------------

clang::CXXRecordDecl *get_as_CXXRecordDecl(clang::QualType qtype) {
  qtype = qtype.getNonReferenceType().getCanonicalType();
  if (auto *rtype = qtype->getAs<clang::RecordType>())
    if (auto *cxxrec = llvm::dyn_cast<clang::CXXRecordDecl>(rtype->getDecl())) return cxxrec;
  return nullptr; // Not a class/struct type
}
// -------------------------------------------------
template <> void matcher<mtch::Fnt>::run(const MatchResult &Result) {

  auto *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("func");
  if (!f) return;
  // ............. Discard some automatic instantiation from the compiler, and alike

  // f in e.g. operator new, internal function, not defined in the sources
  // function defined in std headers are already filtered by the AST Matching
  if (!f->getBeginLoc().isValid()) return;

  // Instantiation: accept only EXPLICIT instantiation
  if (const auto *info = f->getTemplateSpecializationInfo(); info and not info->isExplicitInstantiationOrSpecialization()) return;
  // We could also accept all instantiation on the main file ...
  //auto &SM = worker->ci->getSourceManager();
  //(SM.isInMainFile(f->getPointOfInstantiation()))
  //fmt::println("Point of instantitation : {}", f->getPointOfInstantiation().printToString(SM));
  //if (f->isFunctionTemplateSpecialization()) return;

  // Skip deleted function
  if (f->isDeleted()) return;

  // Skip some internal functions generated by coroutines
  if (f->getNameAsString().starts_with("__builtin_coro_")) return;

  // skip the deduction guides (CTAD)
  if (llvm::dyn_cast_or_null<clang::CXXDeductionGuideDecl>(f)) return;

  // Reject the declaration of the template itself.
  if (f->getDescribedFunctionTemplate()) return;

  // reject method
  // FIXME : in matcher ?
  if (llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f)) return;

  // Discard some special function
  if (f->getNameAsString().starts_with("operator")) return;

  // apply c2py_ignore and the reject_name regex
  auto &M = worker->module_info;
  if (is_rejected(f, worker->reject_names, &logs.rejected)) return;

  // Reject functions defined in c2py_module
  auto fqname = f->getQualifiedNameAsString();
  if (fqname.starts_with("c2py_module::")) return;

  // Insert in the module function list. Unicity will be taken care of later by worker.
  str_t py_name = f->getNameAsString();
  if (auto rename = clu::get_annotation_value(f, "c2py_rename")) py_name = *rename;

  // One function can be tagged as module_init
  if (clu::has_annotation(f, "c2py_module_init")) {
    if (M.module_init) {
      clu::emit_error(f, "Only one function can be tagged c2py_module_init.");
      clu::emit_error(M.module_init, "The previous one was here.");
    }
    if (f->param_size() != 0) clu::emit_error(f, "A function tagged c2py_module_init must take no arguments");
    if (not f->getReturnType()->isVoidType()) clu::emit_error(f, "A function tagged c2py_module_init must return void");
    M.module_init = f;
  }

  if (not clu::has_annotation(f, "c2py_wrap_as_method"))
    M.functions[py_name].push_back(fnt_info_t{f});
  else {
    if (f->param_size() == 0) {
      clu::emit_error(f, "A function tagged c2py_wrap_as_method must take at least 1 argument (self)");
      return;
    }
    auto first_arg_type = get_as_CXXRecordDecl(f->getParamDecl(0)->getType());
    if (auto it = M.classes_ptr_to_info.find(first_arg_type); it != M.classes_ptr_to_info.end())
      M.classes[it->second].second.methods[py_name].push_back(fnt_info_t{.ptr = f, .rewrite = false});
    else
      clu::emit_error(f->getParamDecl(0), "You request to wrap this function as a method, but the first argument is not a class being wrapped.");
  }
}

// -------------------------------------------------

template <> void matcher<mtch::Enum>::run(const MatchResult &Result) {
  auto *enu = Result.Nodes.getNodeAs<clang::EnumDecl>("en");
  if (!enu) return;

  auto &M    = worker->module_info;
  auto qname = enu->getQualifiedNameAsString();

  if (worker->reject_names and std::regex_match(qname, worker->reject_names.value())) {
    logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", qname, "reject_names"));
    return;
  }

  M.enums.push_back(enu);
}
