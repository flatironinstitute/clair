#include "./doc.hpp"
#include "clu/doc_string.hpp"
#include "data.hpp"
#include "clu/fullqualifiedname.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
using namespace fmt::literals;
#include <numeric>

std::string pydoc(fnt_info_t const &f) {
  auto doc = clu::doc_string_t{f.ptr};
  std::stringstream fs;
  if (not doc.brief.empty()) fs << doc.brief << "\n\n";
  if (not doc.content.empty()) fs << doc.content << "\n\n";
  if (doc.params.size()) {
    fs << "Parameters\n----------\n\n";
    for (auto const &[n, d] : doc.params) fs << n << ":\n" << util::indent_string(d, "   ") << '\n';
  }
  if (doc.misc.contains("return")) {
    fs << "\nReturns\n-------\n\n";
    fs << util::indent_string(doc.misc["return"], "   ") << '\n';
  }
  // Add here other \commands ... Cf doc_string.cpp. Register them as block command first.
  return util::indent_string(util::trim(fs.str()), "   ");
}
//---------------------------------------------------------
std::string doc_of_synthetized_constructor(cls_ptr_t cls);

std::string pydoc(cls_info_t const &cls) {
  std::stringstream fs;
  auto doc = clu::doc_string_t{cls.ptr};
  if (not doc.brief.empty()) fs << doc.brief << "\n\n";
  if (not doc.content.empty()) fs << doc.content << "\n\n";
  if (cls.synthetize_init_from_pydict()) fs << doc_of_synthetized_constructor(cls.ptr);
  return util::indent_string(util::trim(fs.str()), "   ");
}

// ----------------------------------------------

// vector of [name, c++ type, initializer, doc]
std::vector<std::vector<std::string>> get_fields_info(const clang::CXXRecordDecl *cls) {
  std::vector<std::vector<std::string>> res;
  //res.push_back({"Field name", "C++ type", "Initializer", "Documentation"});
  clang::ASTContext *ctx = &cls->getASTContext();

  for (clang::FieldDecl *f : cls->fields()) {
    if (f->getAccess() != clang::AS_public) continue;
    std::vector<std::string> m(4);
    m[0] = f->getNameAsString();
    m[1] = clu::get_fully_qualified_name(f->getType(), *ctx);

    if (clang::Expr *init = f->getInClassInitializer()) {
      llvm::StringRef s =
         clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(init->getSourceRange()), ctx->getSourceManager(), ctx->getLangOpts());
      // remove some strange leading = in some cases ?!
      if (auto pos = s.find('='); pos != llvm::StringRef::npos) {
        m[2] = s.substr(pos + 1).ltrim().str();
      } else
        m[2] = s.ltrim().str();
    }
    if (auto *rc = ctx->getRawCommentForDeclNoCache(f)) {
      //m[3] = rc->getBriefText(*ctx);
      // Choice here. First line of doc ? or full doc and wrap the lines
      // use llvm::stringRef::trim function ?
      m[3] = rc->getRawText(ctx->getSourceManager());
    }
    res.push_back(std::move(m));
  }
  // regroup the field without initializer first
  std::stable_sort(begin(res), end(res), [](auto &&x, auto &&y) { return int(x[2].empty()) > int(y[2].empty()); });
  return res;
}

//---------------------------------------------------------

std::string doc_of_synthetized_constructor(cls_ptr_t cls) {
  std::stringstream doc;
  // doc << '\n';
  static std::regex start1(R"RAW(^\s*\/*\s*)RAW");
  static std::regex start2(R"RAW(\n\s*\/*\s*)RAW");
  // FIXME : clean when upgrading
  // gcc 11 does not have the multline implemented ...
  //static std::regex start(R"RAW(^\s*\/*\s*)RAW", std::regex_constants::multiline);
  for (auto const &f : get_fields_info(cls)) {
    doc << "* " << f[0] << ": " << f[1];
    if (not f[2].empty()) doc << " = " << f[2];
    doc << "\n" << std::regex_replace(std::regex_replace(f[3], start1, "   "), start2, "\n   ");
    //doc << "\n" << std::regex_replace(f[3], start, "   ");
    doc << "\n\n";
  }
  llvm::errs() << doc.str();
  return doc.str();
}

// //----------------------------------

// Old table version.
// std::string doc_of_synthetized_constructor1(cls_ptr_t cls) {
//   auto field_vec = get_fields_info(cls);
//   // make a table out of vector<vector of 4 strings>
//   std::vector<long> lmax(4);
//   for (auto const &l : field_vec)
//     for (int i = 0; i < 4; ++i) lmax[i] = std::max(lmax[i], long(l[i].size()));
//   for (int i = 0; i < 4; ++i) lmax[i] += 2;

//   std::stringstream doc;
//   doc << '\n';
//   auto sep = fmt::format("|{0:─<{1}}|\n", "", 3 + lmax.size() * 2 + std::accumulate(begin(lmax), end(lmax), 0)); // just an example copied from fmt
//   for (auto const &l : field_vec)
//     doc << sep << fmt::format("| {0:^{4}} | {1:^{5}} | {2:^{6}} | {3:^{7}} |\n", l[0], l[1], l[2], l[3], lmax[0], lmax[1], lmax[2], lmax[3]);

//   doc << sep << '\n';
//   llvm::errs() << doc.str();
//   return doc.str();
// }