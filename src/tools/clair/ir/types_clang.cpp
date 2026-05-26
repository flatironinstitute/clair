#include "types.hpp"

#include "clu/doc_string.hpp"
#include "clu/fullqualifiedname.hpp"
#include "clu/misc.hpp"
#include "utility/string_tools.hpp"

#include <clang/AST/DeclTemplate.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <fmt/format.h>

namespace ir {

  // ---- QualType ----

  QualType::QualType(clang::QualType t, clang::ASTContext &ctx) : is_const(t.isConstQualified()) {
    name = clu::get_fully_qualified_name(t, ctx);
  }

  // ---- FunctionDecl ----

  FunctionDecl::FunctionDecl(clang::FunctionDecl const &f) {
    simple_name    = f.getNameAsString();
    qualified_name = f.getQualifiedNameAsString();
    auto &ctx      = f.getASTContext();
    return_type    = QualType(f.getReturnType(), ctx);

    // Method flags
    if (auto const *m = llvm::dyn_cast<clang::CXXMethodDecl>(&f)) {
      is_method       = true;
      is_static       = m->isStatic();
      is_const_method = m->isConst();
      is_constructor  = llvm::isa<clang::CXXConstructorDecl>(m);
      parent_class_fqn = clu::get_fully_qualified_name(m->getParent());
    }

    is_inline_friend = not is_method and f.getFriendObjectKind() != clang::Decl::FOK_None;

    // Template arguments
    auto *targs_info = f.getTemplateSpecializationArgs();
    is_template_instantiation = (targs_info != nullptr);
    if (targs_info) {
      for (auto &&ta : targs_info->asArray()) {
        if (ta.getKind() == clang::TemplateArgument::Pack) {
          for (auto const &elem : ta.pack_elements())
            targs.push_back(clu::get_name_of_TemplateArgument(elem, &ctx));
          break;
        }
        targs.push_back(clu::get_name_of_TemplateArgument(ta, &ctx));
      }
    }

    // Find the declaration that carries default arguments and parameter names.
    // For template specializations, the primary template holds the defaults.
    // For member functions of template class instantiations, the pattern holds the defaults.
    clang::FunctionDecl const *f_for_types    = &f;
    clang::FunctionDecl const *f_for_defaults = &f;
    if (auto *info = f.getTemplateSpecializationInfo(); info and info->isExplicitInstantiationOrSpecialization())
      f_for_defaults = info->getTemplate()->getTemplatedDecl();
    else if (auto *pattern = f.getInstantiatedFromMemberFunction())
      f_for_defaults = pattern;

    // Build unique parameter names (handle pack expansion duplicates)
    int n = f_for_types->getNumParams();
    llvm::SmallVector<str_t, 16> names(n);
    for (int i = 0; i < n; ++i) {
      auto nm = f_for_types->getParamDecl(i)->getNameAsString();
      names[i] = nm.empty() ? fmt::format("_p_{}", i) : nm;
    }
    if (targs_info) {
      bool has_pack = llvm::any_of(targs_info->asArray(), [](auto &a) { return a.getKind() == clang::TemplateArgument::Pack; });
      if (has_pack) {
        llvm::StringMap<std::pair<int, int>> seen;
        for (int i = 0; i < n; ++i) {
          auto [it, inserted] = seen.try_emplace(names[i], i, 0);
          if (!inserted) {
            auto &[first_idx, next] = it->second;
            if (next == 0) { names[first_idx] += '0'; next = 1; }
            names[i] += fmt::format("{}", next++);
          }
        }
      }
    }

    // Extract default argument text from a parameter declaration.
    // p_type is the resolved type from the instantiated function.
    auto extract_default = [&](clang::ParmVarDecl const *p, clang::QualType p_type) -> str_t {
      clang::Expr const *defarg = p->getDefaultArg()->IgnoreParenImpCasts();
      if (auto *ice = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(defarg)) defarg = ice->getSubExpr();
      if (auto *dre = llvm::dyn_cast_or_null<clang::DeclRefExpr>(defarg))
        return dre->getFoundDecl()->getQualifiedNameAsString();
      auto s = clu::get_source_range_as_string(p->getDefaultArgRange(), &ctx);
      size_t i = 0;
      while (i < s.size() && (isspace((unsigned char)s[i]) || (s[i] == '='))) ++i;
      s = s.substr(i);
      // If the default uses a braced init, qualify the type to avoid name-lookup issues.
      if (auto pos = s.find('{'); pos != str_t::npos)
        s = clu::get_fully_qualified_name(p_type.getNonReferenceType().getUnqualifiedType(), ctx) + s.substr(pos);
      return s;
    };

    // Build parameter list
    for (int i = 0; i < n; ++i) {
      ParamVarDecl pv;
      pv.name = names[i];
      pv.type = QualType(f_for_types->getParamDecl(i)->getType(), ctx);
      // Use the template declaration for names/defaults when available, but fall back
      // to the instantiated function for expanded pack parameters beyond the template's count.
      clang::ParmVarDecl const *p_decl = (i < (int)f_for_defaults->getNumParams())
                                           ? f_for_defaults->getParamDecl(i)
                                           : f_for_types->getParamDecl(i);
      if (p_decl->hasDefaultArg()) {
        pv.has_default = true;
        pv.default_val = extract_default(p_decl, f_for_types->getParamDecl(i)->getType());
      }
      params.push_back(std::move(pv));
    }

    // Documentation strings
    auto doc  = clu::doc_string_t{&f};
    doc_brief  = doc.brief_str;
    doc_details = doc.details_str;
    params_doc  = doc.params_vec;
    return_doc  = doc.return_str;

    // Deprecated parameter annotation
    if (auto annot = clu::get_annotation_value(&f, "c2py_deprecated_params"))
      deprecated_params_annotation = *annot;
  }

  // ---- RecordDecl ----

  RecordDecl::RecordDecl(clang::CXXRecordDecl const &r) {
    fully_qualified_name = clu::get_fully_qualified_name(&r);
    if (r.isCompleteDefinition()) {
      is_aggregate = r.isAggregate();
      has_bases    = (r.getNumBases() > 0);
    }
    auto doc     = clu::doc_string_t{&r};
    doc_brief    = doc.brief_str;
    doc_details  = doc.details_str;
  }

  // ---- FieldDecl ----

  FieldDecl::FieldDecl(clang::FieldDecl const &d) {
    name     = d.getNameAsString();
    is_const = d.getType().isConstQualified();
    auto &ctx = d.getASTContext();
    type      = QualType(d.getType(), ctx);

    // Obtain the in-class initializer; for template instantiations, fall back to the primary template.
    clang::Expr const *init = d.getInClassInitializer();
    if (!init) {
      if (auto const *cls = llvm::dyn_cast<clang::CXXRecordDecl>(d.getParent()))
        if (auto const *tip = cls->getTemplateInstantiationPattern())
          for (clang::FieldDecl const *f_tpl : tip->fields())
            if (f_tpl->getName() == d.getName()) { init = f_tpl->getInClassInitializer(); break; }
    }

    has_in_class_initializer = (init != nullptr);
    if (init) {
      llvm::StringRef s = clang::Lexer::getSourceText(
        clang::CharSourceRange::getTokenRange(init->getSourceRange()),
        ctx.getSourceManager(), ctx.getLangOpts());
      if (auto pos = s.find('='); pos != llvm::StringRef::npos)
        initializer_str = s.substr(pos + 1).ltrim().str();
      else
        initializer_str = s.ltrim().str();
    }

    // Check whether the field type is a class without a default constructor.
    if (auto const *clsf = d.getType()->getAsCXXRecordDecl())
      field_type_has_no_default_ctor = not clsf->hasDefaultConstructor();

    auto doc    = clu::doc_string_t{&d};
    doc_brief   = doc.brief_str;
    doc_details = doc.details_str;
  }

  // ---- EnumDecl ----

  EnumDecl::EnumDecl(clang::EnumDecl const &e) {
    qualified_name = e.getQualifiedNameAsString();
    for (auto const *val : e.enumerators())
      enumerators.push_back(val->getNameAsString());
  }

} // namespace ir
