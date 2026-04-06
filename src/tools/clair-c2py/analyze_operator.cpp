#include "./analyze_operator.hpp"
#include "./check_convertibility.hpp"
#include "clu/misc.hpp"

// ------------------------------

// Map operator name + arity to OpKind. Returns std::nullopt for unsupported operators.
// Arity is the number of operands (1 = unary, 2 = binary).
static std::optional<OpKind> operator_name_to_kind(std::string_view name, int arity) {
  if (name == "operator+" and arity == 2) return OpKind::Add;
  if (name == "operator-" and arity == 2) return OpKind::Sub;
  if (name == "operator+" and arity == 1) return OpKind::Pos;
  if (name == "operator-" and arity == 1) return OpKind::Neg;
  if (name == "operator*") return OpKind::Mul;
  if (name == "operator/") return OpKind::Div;
  if (name == "operator==") return OpKind::Eq;
  if (name == "operator!=") return OpKind::Ne;
  if (name == "operator<") return OpKind::Lt;
  if (name == "operator>") return OpKind::Gt;
  if (name == "operator<=") return OpKind::Le;
  if (name == "operator>=") return OpKind::Ge;
  if (name == "operator<<") return OpKind::LShift;
  if (name == "operator+=") return OpKind::IAdd;
  if (name == "operator-=") return OpKind::ISub;
  if (name == "operator*=") return OpKind::IMul;
  if (name == "operator/=") return OpKind::IDiv;
  return std::nullopt;
}

// ------------------------------

void analyze_operator(clang::FunctionDecl const *f, wdata_t &wd) {

  auto name    = f->getNameAsString();
  auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(f);

  // Compute the operand arity (including implicit this for methods)
  int arity = int(f->getNumParams()) + (method ? 1 : 0);
  if (arity < 1 or arity > 2) return;

  auto op = operator_name_to_kind(name, arity);
  if (not op) return;

  // In-place operators and LShift return a reference (T&) which the c2py runtime discards
  // (in-place returns self; LShift copies the result via auto return type deduction in arith_op::invoke).
  // Skip the return type check for them.
  bool skip_return_check = (*op == OpKind::IAdd or *op == OpKind::ISub or *op == OpKind::IMul or *op == OpKind::IDiv or *op == OpKind::LShift);
  if (not check_convertibility(f, wd, /*test_return_type=*/!skip_return_check)) return;

  // For LShift, verify return type is T& where T is the class (canonical for operator<<).
  // The Python wrapping assumes this: it calls the operator and discards the returned reference.
  if (*op == OpKind::LShift) {
    auto ret = f->getReturnType();
    clang::CXXRecordDecl const *class_decl;
    if (method)
      class_decl = method->getParent();
    else
      class_decl = f->getParamDecl(0)->getType().getNonReferenceType()->getAsCXXRecordDecl();
    auto *ret_record = ret->isLValueReferenceType() ? ret.getNonReferenceType()->getAsCXXRecordDecl() : nullptr;
    if (not ret_record or ret_record != class_decl) {
      clu::emit_error(f, "c2py: operator<< must return T& where T is the class");
      return;
    }
  }

  // Build the full argument type list
  std::vector<clang::QualType> args;
  if (method) args.push_back(method->getThisType()->getPointeeType().getUnqualifiedType());
  for (unsigned i = 0; i < f->getNumParams(); ++i) args.push_back(f->getParamDecl(i)->getType().getNonReferenceType().getUnqualifiedType());

  // Associate with the class of the first argument; fall back to second if first is not wrapped
  auto *cli = wd.module_info.get_wrapped_cls_info(args[0]);
  if (not cli and args.size() > 1) cli = wd.module_info.get_wrapped_cls_info(args[1]);
  if (cli) cli->operators[*op].push_back(std::move(args));
}
