#include "./matchers.hpp"
#include "clang/Basic/SourceManager.h"
#include "clu/misc.hpp"
#include "utility/logger.hpp"
#include "utility/macros.hpp"
#include "clang/ASTMatchers/ASTMatchers.h"
static const struct {
  util::logger rejected = util::logger{&std::cout, "-- ", "\033[1;33mRejecting: \033[0m"};
  util::logger note     = util::logger{&std::cout, "-- ", "\033[1;32mNote:  \033[0m"};
} logs;
namespace matchers {

  // ******************************************************
  //  extract_literal functions.
  //  Take a string/int/bool, etc from a VarDecl with literal value
  // ******************************************************

  // Gets the initializer of the VarDecl, removing the ImplicitCastExpr
  const clang::Expr *get_vardecl_initlalizer(clang::VarDecl const *decl) {
    auto *value = decl->getAnyInitializer();
    auto *iexpr = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(value);
    while (iexpr) {
      value = iexpr->getSubExpr();
      iexpr = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(value);
    }
    return value;
  }

  // Literal string
  void extract_literal(clang::VarDecl const *d, str_t &x) {
    auto value = get_vardecl_initlalizer(d);
    if (auto *s = llvm::dyn_cast_or_null<clang::StringLiteral>(value))
      x = s->getString().str();
    else { clu::emit_error(d, "c2py: Variable of incorrect type. Expected a literal string."); }
  }

  // Regex : a string into a regex with control
  void extract_literal(clang::VarDecl const *d, std::optional<std::regex> &x) {
    str_t s;
    extract_literal(d, s);
    try {
      if (not s.empty()) x = std::regex{s};
    } catch (std::regex_error const &e) { clu::emit_error(d, "c2py: invalid C++ regular expression. Cf std::regex documentation for information."); }
  }

  // Bool
  void extract_literal(clang::VarDecl const *d, bool &x) {
    auto value = get_vardecl_initlalizer(d);
    if (auto *b = llvm::dyn_cast_or_null<clang::CXXBoolLiteralExpr>(value))
      x = b->getValue();
    else
      clu::emit_error(d, "c2py: Variable of incorrect type. Expected a literal bool.");
  }

  // long
  void extract_literal(clang::VarDecl const *d, long &x) {
    auto value      = get_vardecl_initlalizer(d);
    auto const &ctx = d->getASTContext();
    if (auto *i = llvm::dyn_cast_or_null<clang::IntegerLiteral>(value))
      x = long{i->EvaluateKnownConstInt(ctx).getExtValue()};
    else
      clu::emit_error(d, "c2py: Variable of incorrect type. Expected a literal integer.");
  }

  // Check the module_init function exists
  void check_module_init(clang::VarDecl const *d, bool &x) {
    auto value = get_vardecl_initlalizer(d);
    if (auto *l = llvm::dyn_cast_or_null<clang::LambdaExpr>(value)) {
      if (l->getCallOperator()->getNumParams() != 0)
        clu::emit_error(d, "The init function must take 0 parameters.");
      else
        x = true;
    } else
      clu::emit_error(d, "c2py: Variable of incorrect type. Expected a lambda.");
  }

  // -----------------------------------------------------

  void concept_t::run(const MatchResult &Result) {

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
      else if (cname == "Storable")
        worker->HasHdf5 = cpt;
      // else ignore the others concepts
    }
  }

  // -------------------------------------------------

  void module_vars_t::run(const MatchResult &Result) {
    auto const *decl = Result.Nodes.getNodeAs<clang::VarDecl>("decl");
    assert(decl);

    static auto vars = std::map<str_t, std::function<void(clang::VarDecl const *, module_info_t &)>>{
       {"module_name", [](auto *d, auto &M) { extract_literal(d, M.module_name); }},
       {"package_name", [](auto *d, auto &M) { extract_literal(d, M.package_name); }},
       {"documentation", [](auto *d, auto &M) { extract_literal(d, M.documentation); }},
       {"module_init", [](auto *d, auto &M) { check_module_init(d, M.has_module_init); }},
       {"get_set_as_properties", [](auto *d, auto &M) { extract_literal(d, M.get_set_as_properties); }},
       {"match_names", [](auto *d, auto &M) { extract_literal(d, M.match_names); }},
       {"reject_names", [](auto *d, auto &M) { extract_literal(d, M.reject_names); }},
       {"match_files", [](auto *d, auto &M) { extract_literal(d, M.match_files); }},
    };

    if (auto it = vars.find(decl->getName().str()); it != vars.end())
      it->second(decl, worker->module_info);
    else
      clu::emit_error(decl, "c2py: module variable declaration is not recognized.");
  }

  // -------------------------------------------------

  void module_fun_dispatch_t::run(const MatchResult &Result) {
    auto const *decl = Result.Nodes.getNodeAs<clang::VarDecl>("decl");
    assert(decl);
    analyse_dispatch(worker->module_info.functions, decl);
  }

  // --------------------------------------------------------------------------------

  void module_cls_wrap_t::run(const MatchResult &Result) {
    auto *d = Result.Nodes.getNodeAs<clang::TypeAliasDecl>("decl");
    assert(d);
    if (auto *cls = d->getUnderlyingType()->getAsCXXRecordDecl()) {
      if (auto *cls_i = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cls)) {
        // probably useless ?
        if (not cls->hasDefinition()) { clu::emit_error(d, "c2py: The class template should be explicitly instantiated"); }
      }
      worker->module_info.classes.emplace_back(d->getName().str(), cls_info_t{cls});
      //if (not inserted) clu::emit_error(d, "[c2py] Class declaration duplication");
    }
  }

  // -------------------------------------------------

  void module_cls_info_t::run(const MatchResult &Result) {
    auto const *decl = Result.Nodes.getNodeAs<clang::ClassTemplateDecl>("decl");
    assert(decl);

    auto varname = decl->getName().str();
    if (varname == "add_methods_to")
      worker->add_methods_to = decl;
    else
      clu::emit_error(decl, "c2py: unknown class declaration");
  }

  // -------------------------------------------------

  void matchers::cls_t::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
    const auto *cls = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");

    if (!cls) return; // just in case
    if (cls->getASTContext().getDiagnostics().hasErrorOccurred()) return;

    // Filter some automatic instantiation from the compiler, and alike
    if (!cls->getSourceRange().isValid()) return;
    if (!cls->isCompleteDefinition()) return;                                                             // skip forward declaration.
    if (cls->isLambda()) return;                                                                          // no lambda
    if (not((cls->getTagKind() == clang::TTK_Struct) or (cls->getTagKind() == clang::TTK_Class))) return; // just struct and class

    if (auto *s = llvm::dyn_cast_or_null<clang::ClassTemplateSpecializationDecl>(cls)) {
      if (!s->getTypeAsWritten()) return;
    }

    // Reject the declaration of the template itself.
    if (cls->getDescribedClassTemplate()) return;

    // FIXME : check ? why ?
    // Reject template specialization
    if (llvm::dyn_cast_or_null<clang::ClassTemplateSpecializationDecl>(cls)) return;

    //  Discard if C2PY_ignore annotation is present
    if (clu::has_annotation(cls, "c2py_ignore")) return;

    // Apply the filters
    auto qname = cls->getQualifiedNameAsString();
    auto &M    = worker->module_info;

    // Reject classes defined in c2py_module
    if (qname.starts_with("c2py_module::")) return;

    bool force_wrap = (clu::has_annotation(cls, "c2py_wrap"));

    if (!force_wrap and M.reject_names and std::regex_match(qname, M.reject_names.value())) {
      logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", qname, "reject_names"));
      return;
    }
    // Insert in the module class list
    str_t py_name = util::camel_case(cls->getNameAsString());
    M.classes.emplace_back(py_name, cls_info_t{cls}); //

    //if (not inserted) clu::emit_error(cls, "Class rejected. Should have another class with the same Python name ??");
  }

  // -------------------------------------------------
  template <> void matcher<Fnt>::run(const MatchResult &Result) {

    auto *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("func");
    if (!f) return;

    // ............. Discard some automatic instantiation from the compiler, and alike

    // f in e.g. operator new, internal function, not defined in the sources
    // function defined in std headers are already filtered by the AST Matching
    if (!f->getBeginLoc().isValid()) return;

    // Skip automatic template instantiation by the compiler
    if (f->isFunctionTemplateSpecialization()) return;

    // Skip deleted function
    if (f->isDeleted()) return;

    // Skip some internal functions generated by coroutines
    if (f->getNameAsString().starts_with("__builtin_coro_")) return;

    // skip the deduction guides (CTAD)
    if (llvm::dyn_cast_or_null<clang::CXXDeductionGuideDecl>(f)) return;

    // Reject the declaration of the template itself.
    if (f->getDescribedFunctionTemplate()) return;

    // reject method --> FIXME : in matcher ?
    if (llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f)) return;

    //  Discard if C2PY_ignore annotation is present
    if (clu::has_annotation(f, "c2py_ignore")) return;

    // Discard some special function
    if (f->getNameAsString().starts_with("operator")) return;

    // Apply the filters. regex_filter, regex_exclude, then the _fun version
    auto fqname = f->getQualifiedNameAsString();
    auto &M     = worker->module_info;

    // Reject functions defined in c2py_module
    if (fqname.starts_with("c2py_module::")) return;

    bool force_wrap = (clu::has_annotation(f, "c2py_wrap"));

    if (!force_wrap and M.reject_names and std::regex_match(fqname, M.reject_names.value())) {
      logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", fqname, "reject_names"));
      return;
    }

    // Insert in the module function list. Unicity will be taken care of later by worker.
    str_t py_name = f->getNameAsString();
    M.functions[py_name].push_back(fnt_info_t{f});
  }

  // -------------------------------------------------

  void enum_t::run(const MatchResult &Result) {
    auto *enu = Result.Nodes.getNodeAs<clang::EnumDecl>("en");
    if (!enu) return;

    auto &M    = worker->module_info;
    auto qname = enu->getQualifiedNameAsString();

    if (M.reject_names and std::regex_match(qname, M.reject_names.value())) {
      logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", qname, "reject_names"));
      return;
    }

    M.enums.push_back(enu);
  }

} // namespace matchers
