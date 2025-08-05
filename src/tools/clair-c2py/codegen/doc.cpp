#include "doc.hpp"
#include "clu/doc_string.hpp"
#include "../data.hpp"
#include "clu/fullqualifiedname.hpp"
#include <clang/AST/DeclCXX.h>
#include "clang/Lex/Lexer.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <itertools/itertools.hpp>
#include "utility/streams.hpp"
#include "utility/string_tools.hpp"
#include <algorithm>
#include <regex>

using namespace fmt::literals;

static const struct {
  util::logger warn = util::logger{&std::cout, "-- ", "\033[1;31mDoc warning: \033[0m"};
} logs;

// -----------------------------------------------

std::string pydoc(std::vector<fnt_info_t> const &f_list) {
  // extract and format relevant doc strings (function, parameter and return descriptions)
  std::vector<std::pair<str_t, str_t>> param_docs;                      // parameter name -> doc string
  std::vector<std::pair<str_t, std::vector<long>>> func_docs, ret_docs; // doc string -> overload indices with the same doc string
  for (auto const &[n, f] : itertools::enumerate(f_list)) {
    auto doc = clu::doc_string_t{f.ptr};

    // get function doc string
    auto fdoc = doc.brief_str;
    fdoc += doc.details_str.empty() ? "" : (fdoc.empty() ? doc.details_str : fmt::format("\n\n{}", doc.details_str));
    if (not fdoc.empty()) {
      // check if an overload already has the same function doc string
      if (auto it = std::ranges::find_if(func_docs, [&fdoc](auto const &p) { return p.first == fdoc; }); it != func_docs.end()) {
        // if so, add the overload index to the existing entry
        it->second.push_back(n + 1);
      } else {
        // otherwise, create a new entry with the doc string and the overload index
        func_docs.emplace_back(fdoc, std::vector<long>{n + 1});
      }
    }

    // get parameter doc strings
    for (auto const &[pname, pdoc] : doc.params_vec) {
      // check if a parameter name has already been encountered
      auto it = std::ranges::find_if(param_docs, [&pname](auto const &p) { return p.first == pname; });
      if (it != param_docs.end()) {
        // if so, check if the doc string is the same --> if not, warn the user
        if (it->second != pdoc)
          logs.warn(fmt::format("Multiple parameter descriptions given for parameter {} in overloaded function {}", pname,
                                f.ptr->getQualifiedNameAsString()));
      } else {
        // otherwise, add the parameter name + doc string
        param_docs.emplace_back(pname, pdoc);
      }
    }

    // get return doc strings
    if (not doc.return_str.empty()) {
      // check if an overload already has the same return doc string
      auto it = std::ranges::find_if(ret_docs, [&doc](auto const &p) { return p.first == doc.return_str; });
      if (it != ret_docs.end()) {
        // if so, add the overload index to the existing entry
        it->second.push_back(n + 1);
      } else {
        // otherwise, create a new entry with the doc string and the overload index
        ret_docs.emplace_back(doc.return_str, std::vector<long>{n + 1});
      }
    }
  }

  // output streams
  std::stringstream fs;
  auto out  = triqs::indented_ostream{fs, 3};   // Indent all lines with 3 spaces

  // write function doc strings
  for (int i = 0; auto const &[fdoc, vec] : func_docs) {
    if (i++ > 0) fs << "\n------\n";
    fs << (func_docs.size() == 1 ? fmt::format("\n{}", fdoc) : fmt::format("\n[{}] {}", util::join(vec, ", "), fdoc)) << "\n";
  }

  // write parameter doc strings
  if (not param_docs.empty()) {
    fs << "\nParameters\n----------\n";
    for (auto const &[pname, pdoc] : param_docs) {
      fs << pname << '\n';
      out << pdoc << '\n';
    }
  }

  // write return doc strings
  if (not ret_docs.empty()) {
    fs << "\nReturns\n-------";
    for (auto const &[rdoc, vec] : ret_docs) {
      out << (ret_docs.size() == 1 ? fmt::format("\n{}", rdoc) : fmt::format("\n[{}] {}", util::join(vec, ", "), rdoc)) << '\n';
    }
  }

  // Add here treatment of custom \commands if any...
  // Cf doc_string.cpp. Register them as block command first.
  return fs.str();
}

//---------------------------------------------------------
std::string doc_of_synthetized_constructor(cls_info_t const &cls_info);

//---------------------------------------------------------
std::string pydoc(cls_info_t const &cls) {
  std::stringstream fs;
  auto doc = clu::doc_string_t{cls.ptr};
  if (not doc.brief_str.empty()) fs << doc.brief_str << "\n\n";
  if (not doc.details_str.empty()) fs << doc.details_str << "\n\n";
  if (cls.synthetize_init_from_pydict()) fs << doc_of_synthetized_constructor(cls);
  return util::indent_string(util::trim(fs.str()), "   ");
}

// ----------------------------------------------

// vector of [name, c++ type, initializer, doc]
std::vector<std::vector<std::string>> get_fields_info(cls_info_t const &cls_info) {
  std::vector<std::vector<std::string>> res;
  //res.push_back({"Field name", "C++ type", "Initializer", "Documentation"});
  clang::CXXRecordDecl const *cls = cls_info.ptr;
  clang::ASTContext *ctx          = &cls->getASTContext();

  for (auto *f : cls_info.fields) {
    std::vector<std::string> m(4);
    m[0] = f->getNameAsString();
    m[1] = clu::get_fully_qualified_name(f->getType(), *ctx, /*canonical*/ false);

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

std::string doc_of_synthetized_constructor(cls_info_t const &cls_info) {
  std::stringstream doc;
  // doc << '\n';
  static std::regex start1(R"RAW(^\s*\/*\s*)RAW");
  static std::regex start2(R"RAW(\n\s*\/*\s*)RAW");
  // FIXME : clean when upgrading
  // gcc 11 does not have the multline implemented ...
  //static std::regex start(R"RAW(^\s*\/*\s*)RAW", std::regex_constants::multiline);
  for (auto const &f : get_fields_info(cls_info)) {
    doc << "* " << f[0] << ": " << f[1];
    if (not f[2].empty()) doc << " = " << f[2];
    doc << "\n" << std::regex_replace(std::regex_replace(f[3], start1, "   "), start2, "\n   ");
    //doc << "\n" << std::regex_replace(f[3], start, "   ");
    doc << "\n\n";
  }
  //llvm::errs() << doc.str();
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
