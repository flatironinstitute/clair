#include "./check_convertibility.hpp"
#include <clang/AST/RecursiveASTVisitor.h>

#include <itertools/itertools.hpp>
#include "clu/misc.hpp"
#include "clu/concept.hpp"

// Strip reference, pointer, and cv-qualifiers to obtain the base type name for table lookup.
static std::string base_type_name(clang::QualType ty) {
  ty = ty.getNonReferenceType();
  if (ty->isPointerType()) ty = ty->getPointeeType();
  return ty.getUnqualifiedType().getAsString();
}

// Emit a note suggesting an #include if the type appears in the wrapped_type_to_header table.
static void suggest_header(clang::Decl const *d, clang::QualType ty, wdata_t const &wd) {
  if (auto it = wd.wrapped_type_to_header.find(base_type_name(ty)); it != wd.wrapped_type_to_header.end())
    clu::emit_note(d, "consider adding: #include \"" + it->second + "\"");
}

// ------------------------------
// Validates that every return statement in a reference-returning method
// returns a member of `this` (at any depth).
//
// The visitor traverses the full body and emits an error for any
// return statement whose expression is not of that form.  This covers all
// control-flow paths (if/else branches, early returns, etc.).
//
// In Clang's AST, MemberExpr represents a field access `obj.field`.
// It has two parts: the member (the field) and the base (the expression
// to the left of the dot, via getBase()). For a chain `this->a.b`, the
// outermost MemberExpr (.b) has a base that is another MemberExpr (.a),
// whose base is CXXThisExpr.
//
// Valid AST shapes (direct member, member-of-member, smart-ptr member, or method delegation):
//   ReturnStmt
//     ImplicitCastExpr*                    (zero or more, e.g. lvalue-to-rvalue)
//       MemberExpr (.field)
//         [ MemberExpr (.submember) ]*     (zero or more intermediate members)
//           [ ImplicitCastExpr* ]          (e.g. derived-to-base cast)
//             CXXThisExpr
//   ReturnStmt
//     MemberExpr (.field)                  (e.g. this->ptr->field)
//       CXXOperatorCallExpr (operator->)
//         MemberExpr (.ptr)
//           CXXThisExpr
//   ReturnStmt
//     CXXMemberCallExpr (this->method())
//       MemberExpr (.method)
//         [ MemberExpr* ]
//           CXXThisExpr
//
// The loop walks getBase()/getImplicitObjectArgument()/getArg(0) through
// ImplicitCastExprs, MemberExprs, and CXXOperatorCallExprs (overloaded operator->)
// until it reaches the root. If that root is CXXThisExpr the return is safe.
// Anything else (local variable, global) is rejected.
//
class check_return_visitor : public clang::RecursiveASTVisitor<check_return_visitor> {
  clang::FunctionDecl const *f;

  public:
  explicit check_return_visitor(clang::FunctionDecl const *f) : f{f} {}

  // Do not descend into nested scopes (lambdas, local classes) whose
  // return statements belong to the inner function, not to the method.
  bool TraverseLambdaExpr(clang::LambdaExpr *) { return true; }
  bool TraverseCXXRecordDecl(clang::CXXRecordDecl *) { return true; }

  bool VisitReturnStmt(clang::ReturnStmt *ret) {
    // Peel implicit casts on the returned expression (e.g. lvalue-to-rvalue).
    clang::Expr const *ret_value = ret->getRetValue();
    while (auto *ice = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(ret_value)) ret_value = ice->getSubExpr();

    // The expression must be a member access or a method call on this.
    clang::Expr const *base = nullptr;
    if (auto *ex = llvm::dyn_cast_or_null<clang::MemberExpr>(ret_value))
      base = ex->getBase();
    else if (auto *call = llvm::dyn_cast_or_null<clang::CXXMemberCallExpr>(ret_value))
      base = call->getImplicitObjectArgument();
    // Walk the access chain: peel ImplicitCastExprs, MemberExprs, and CXXOperatorCallExprs
    // until we reach the root base. This accepts `this->member`, `this->member.submember`,
    // and `this->smart_ptr->member` (overloaded operator->) at any depth.
    while (base) {
      if (auto *ice = llvm::dyn_cast<clang::ImplicitCastExpr>(base))
        base = ice->getSubExpr();
      else if (auto *me = llvm::dyn_cast<clang::MemberExpr>(base))
        base = me->getBase();
      else if (auto *op = llvm::dyn_cast<clang::CXXOperatorCallExpr>(base)) {
        if (op->getOperator() == clang::OO_Arrow)
          base = op->getArg(0); // first arg of overloaded operator is the object (e.g. smart_ptr)
        else
          break; // other operators (e.g. operator*) are not allowed in the access chain
      } else
        break;
    }

    // Accept `this->member`, `this->member.submember`, or `this->method()`.
    // Reject: nullptr, DeclRefExpr (local/global variable), etc.
    if (not llvm::dyn_cast_or_null<clang::CXXThisExpr>(base)) {
      clu::emit_error(f->getReturnTypeSourceRange().getBegin(), f->getASTContext(),
                      "c2py: Can not be converted from C++ to Python. I can not check that this method returns a member of `this`.");
      clu::emit_error(ret->getBeginLoc(), f->getASTContext(), "c2py: ... due to this return statement.");
    }
    return true;
  }
};

// ------------------------------

// Check if function parameter and return type are convertible.
// Emits clang diagnostics for each failing type.
// Returns true if the function should be kept (all checks pass), false otherwise.
bool check_convertibility(clang::FunctionDecl const *f, wdata_t const &wd, bool test_return_type) {

  // Types that should cause the function to be silently ignored (no error).
  // E.g. operator<<(std::ostream&, ...) is for stream output, not a real operator to wrap.
  // FIXME: to be generalized.
  auto involves_ignored_type = [](clang::QualType const &ty) { return ty.getAsString().find("ostream") != std::string::npos; };

  // We do this check first, to avoid emitting errors about non-convertible types in functions that we will skip.
  if (involves_ignored_type(f->getReturnType())) return false;
  for (auto i : itertools::range(f->getNumParams()))
    if (involves_ignored_type(f->getParamDecl(i)->getType())) return false;

  bool ok = true;
  for (auto i : itertools::range(f->getNumParams())) {
    auto *p = f->getParamDecl(i);
    auto ty = p->getType();
    if ((not ty->isVoidType()) and (not wd.concepts.IsConvertiblePy2C.is_satisfied_by(ty)) and (not wd.module_info.is_wrapped(ty))) {
      clu::emit_error(p, "c2py: Can not convert this argument from python to C++");
      suggest_header(p, ty, wd);
      ok = false;
    }
  }
  if (test_return_type) {
    auto ty = f->getReturnType();
    if (ty->isPointerType() and (ty->getPointeeType().getAsString() != "PyObject")) {
      clu::emit_error(f, "c2py: Can not convert a raw C++ pointer to python");
      ok = false;
    } else if ((not ty->isVoidType()) and (not wd.concepts.IsConvertibleC2Py.is_satisfied_by(ty)) and (not wd.module_info.is_wrapped(ty))) {
      clu::emit_error(f, "c2py: Can not be converted from C++ to python");
      suggest_header(f, ty, wd);
      ok = false;
    } else {
      if (ty->isReferenceType()) { // further checks if we return a reference
                                   // it must be a method, and we only return this->a_member;
                                   // everything else is rejected
        if (auto m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f); !m) {
          clu::emit_error(f, "c2py: Can not be converted from C++ to Python. Only methods can return a reference.");
        } else {
          auto visitor = check_return_visitor{f};
          visitor.TraverseStmt(m->getBody());
        }
      }
    }
  }
  return ok;
}
