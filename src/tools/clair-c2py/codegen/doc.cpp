#include "doc.hpp"
#include "clu/doc_string.hpp"
#include "../wdata.hpp"
#include "clu/fullqualifiedname.hpp"
#include <clang/AST/DeclCXX.h>
#include "clang/Lex/Lexer.h"
#include <fmt/format.h>
#include <string>
#include <itertools/itertools.hpp>
#include "utility/streams.hpp"
#include "utility/string_tools.hpp"
#include <algorithm>
#include <set>

using namespace fmt::literals;

static const struct {
  util::logger warn = util::logger{"-- ", "\033[38;5;212mDoc warning: \033[0m", 0};
} logs;

// ------------------------------

std::tuple<str_t, std::vector<std::vector<str_t>>, std::vector<str_t>> pydoc(std::vector<fnt_info_t> const &f_list) {
  // extract and format relevant doc strings (function, parameter and return descriptions)
  std::vector<std::tuple<str_t, str_t, std::vector<long>>> param_docs; // parameter name -> doc string -> overload indices with the same doc string
  std::vector<std::tuple<str_t, str_t, std::vector<long>>> ret_docs;   // return type -> doc string -> overload indices with the same doc string
  std::vector<std::pair<str_t, std::vector<long>>> func_docs;          // doc string -> overload indices with the same doc string
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
      auto it = std::ranges::find_if(param_docs, [&pname](auto const &p) { return std::get<0>(p) == pname; });
      if (it != param_docs.end()) {
        // if so, check if the doc string is the same --> if not, warn the user
        if (std::get<1>(*it) != pdoc) {
          logs.warn(fmt::format("Multiple parameter descriptions given for parameter {} in overloaded function {}", pname,
                                f.ptr->getQualifiedNameAsString()));
        } else {
          // if the doc string is the same, just add the overload index to the existing entry
          std::get<2>(*it).push_back(n);
        }
      } else {
        // otherwise, create a new entry with the parameter name, doc string and the overload index
        param_docs.emplace_back(pname, pdoc, std::vector<long>{n});
      }
    }

    // get return doc strings
    if (not doc.return_str.empty()) {
      // get return type
      auto const type_str = clu::get_fully_qualified_name(f.ptr->getReturnType(), f.ptr->getASTContext());

      // check if an overload already has the same return doc string + type
      auto it = std::ranges::find_if(
         ret_docs, [&doc, &type_str](auto const &tup) { return std::get<0>(tup) == type_str && std::get<1>(tup) == doc.return_str; });
      if (it != ret_docs.end()) {
        // if so, add the overload index to the existing entry
        std::get<2>(*it).push_back(n + 1);
      } else {
        // otherwise, create a new entry with the doc string and the overload index
        ret_docs.emplace_back(type_str, doc.return_str, std::vector<long>{n + 1});
      }
    }
  }

  // output streams
  std::stringstream fs;
  auto out = triqs::indented_ostream{fs, 3}; // Indent all lines with 3 spaces

  // write function doc strings
  for (auto const &[fdoc, vec] : func_docs) {
    fs << (func_docs.size() == 1 ? fmt::format("\n{}", fdoc) : fmt::format("\n[{}] {}", util::join(vec, ", "), fdoc)) << "\n";
    if (func_docs.size() > 1) fs << "\n------\n";
  }

  // write parameter doc strings and get parameter types
  std::vector<std::vector<str_t>> param_types;
  if (not param_docs.empty()) {
    fs << "\nParameters\n----------\n";
    for (int i = 0; auto const &[pname, pdoc, vec] : param_docs) {
      param_types.emplace_back();
      for (auto n : vec) {
        auto *f      = f_list[n].ptr;
        auto fparams = f->parameters();
        auto it      = std::ranges::find_if(fparams, [&pname](auto const &param) { return param->getNameAsString() == pname; });
        if (it == fparams.end()) {
          logs.warn(fmt::format("Function {} contains a doc string for parameter {} which is not an argument", f->getQualifiedNameAsString(), pname));
        } else {
          auto const type_str = clu::get_fully_qualified_name((*it)->getType(), f->getASTContext());
          if (std::ranges::find(param_types.back(), type_str) == param_types.back().end()) param_types.back().emplace_back(type_str);
        }
      }
      fs << fmt::format("{} : {{par_{}}}\n", pname, i++);
      out << pdoc << '\n';
    }
  }

  // write return doc strings and get return types
  std::vector<str_t> return_types;
  if (not ret_docs.empty()) {
    fs << "\nReturns\n-------";
    for (int i = 0; auto const &[rtype, rdoc, vec] : ret_docs) {
      return_types.emplace_back(rtype);
      fs << (ret_docs.size() == 1 ? fmt::format("\n{{ret_{}}}\n", i++) : fmt::format("\n[{}] : {{ret_{}}}\n", util::join(vec, ", "), i++));
      out << rdoc << '\n';
    }
  }

  // Add here treatment of custom \commands if any...
  // Cf doc_string.cpp. Register them as block command first.
  return {fs.str(), param_types, return_types};
}

//---------------------------------------------------------

str_t pydoc(cls_info_t const &cls) {
  // only extract the brief and details sections (the constructors are done in the codegen routines)
  std::stringstream fs;
  auto doc = clu::doc_string_t{cls.ptr};
  fs << doc.brief_str;
  fs << (doc.details_str.empty() ? "" : (doc.brief_str.empty() ? doc.details_str : fmt::format("\n\n{}", doc.details_str)));
  return fs.str();
}

// ------------------------------

// vector of [name, c++ type, initializer, doc]
std::vector<std::vector<std::string>> get_fields_info(cls_info_t const &cls_info) {
  std::vector<std::vector<std::string>> res;
  //res.push_back({"Field name", "C++ type", "Initializer", "Documentation"});
  clang::CXXRecordDecl const *cls = cls_info.ptr;
  clang::ASTContext *ctx          = &cls->getASTContext();

  for (auto *f : cls_info.fields) {
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

    auto fdoc = clu::doc_string_t{f};
    m[3]      = fdoc.brief_str;
    m[3] += fdoc.details_str.empty() ? "" : (m[3].empty() ? fdoc.details_str : fmt::format("\n\n{}", fdoc.details_str));
    res.push_back(std::move(m));
  }
  // regroup the field without initializer first
  std::stable_sort(begin(res), end(res), [](auto &&x, auto &&y) { return int(x[2].empty()) > int(y[2].empty()); });
  return res;
}

//---------------------------------------------------------

std::tuple<str_t, std::vector<str_t>> pydoc_of_synthetized_constructor(cls_info_t const &cls_info) {
  // output streams
  std::stringstream fs;
  auto out = triqs::indented_ostream{fs, 3}; // Indent all lines with 3 spaces

  fs << "Synthesized constructor with the following keyword arguments:\n";

  // write field doc strings and store field types
  auto info_vec = get_fields_info(cls_info);
  std::vector<str_t> field_types;
  if (not info_vec.empty()) {
    fs << "\nParameters\n----------\n";
    for (int i = 0; auto const &info : info_vec) {
      field_types.push_back({info[1]});
      fs << fmt::format("{} : {{par_{}}}{}\n\n", info[0], i++, info[2].empty() ? "" : fmt::format(", default={}", info[2]));
      // out << (info[3].empty() ? fmt::format("Initial value for attribute `{}`", info[0]) : info[3]) << '\n';
    }
  }

  return {fs.str(), field_types};
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
