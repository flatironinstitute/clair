#include "doc.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
#include <algorithm>
#include <string>
#include <itertools/itertools.hpp>
#include "utility/streams.hpp"
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"
#include <algorithm>

using namespace fmt::literals;

static const struct {
  util::logger warn = util::logger{"-- ", "\033[38;5;212mDoc warning: \033[0m", 0};
} logs;

// ------------------------------

std::tuple<str_t, std::vector<std::vector<str_t>>, std::vector<str_t>> pydoc(std::vector<fnt_info_t> const &f_list) {
  std::vector<std::tuple<str_t, str_t, std::vector<long>>> param_docs;
  std::vector<std::tuple<str_t, str_t, std::vector<long>>> ret_docs;
  std::vector<std::pair<str_t, std::vector<long>>> func_docs;

  for (auto const &[n, f] : itertools::enumerate(f_list)) {
    if (not f.ptr) continue;
    auto &ir = *f.ptr;

    // Function-level doc
    auto fdoc = ir.doc_brief;
    fdoc += ir.doc_details.empty() ? "" : (fdoc.empty() ? ir.doc_details : fmt::format("\n\n{}", ir.doc_details));
    if (not fdoc.empty()) {
      if (auto it = std::ranges::find_if(func_docs, [&fdoc](auto const &p) { return p.first == fdoc; }); it != func_docs.end())
        it->second.push_back(n + 1);
      else
        func_docs.emplace_back(fdoc, std::vector<long>{n + 1});
    }

    // Per-parameter doc
    for (auto const &[pname, pdoc] : ir.params_doc) {
      auto it = std::ranges::find_if(param_docs, [&pname](auto const &p) { return std::get<0>(p) == pname; });
      if (it != param_docs.end()) {
        if (std::get<1>(*it) != pdoc)
          logs.warn(fmt::format("Multiple parameter descriptions given for parameter {} in overloaded function {}", pname, ir.qualified_name));
        else
          std::get<2>(*it).push_back(n);
      } else {
        param_docs.emplace_back(pname, pdoc, std::vector<long>{n});
      }
    }

    // Return doc
    if (not ir.return_doc.empty()) {
      auto const &type_str = ir.return_type.name;
      auto it = std::ranges::find_if(ret_docs, [&ir, &type_str](auto const &tup) {
        return std::get<0>(tup) == type_str && std::get<1>(tup) == ir.return_doc;
      });
      if (it != ret_docs.end())
        std::get<2>(*it).push_back(n + 1);
      else
        ret_docs.emplace_back(type_str, ir.return_doc, std::vector<long>{n + 1});
    }
  }

  std::stringstream fs;
  auto out = triqs::indented_ostream{fs, 3};

  for (auto const &[fdoc, vec] : func_docs) {
    fs << (func_docs.size() == 1 ? fmt::format("\n{}", fdoc) : fmt::format("\n[{}] {}", util::join(vec, ", "), fdoc)) << "\n";
    if (func_docs.size() > 1) fs << "\n------\n";
  }

  std::vector<std::vector<str_t>> param_types;
  if (not param_docs.empty()) {
    fs << "\nParameters\n----------\n";
    for (int i = 0; auto const &[pname, pdoc, vec] : param_docs) {
      param_types.emplace_back();
      for (auto n : vec) {
        if (n >= (long)f_list.size() or not f_list[n].ptr) continue;
        auto &ir_n = *f_list[n].ptr;
        auto it    = std::ranges::find_if(ir_n.params, [&pname](auto const &p) { return p.name == pname; });
        if (it == ir_n.params.end())
          logs.warn(fmt::format("Function {} contains a doc string for parameter {} which is not an argument", ir_n.qualified_name, pname));
        else if (std::ranges::find(param_types.back(), it->type.name) == param_types.back().end())
          param_types.back().emplace_back(it->type.name);
      }
      fs << fmt::format("{} : {{par_{}}}\n", pname, i++);
      out << pdoc << '\n';
    }
  }

  std::vector<str_t> return_types;
  if (not ret_docs.empty()) {
    fs << "\nReturns\n-------";
    for (int i = 0; auto const &[rtype, rdoc, vec] : ret_docs) {
      return_types.emplace_back(rtype);
      fs << (ret_docs.size() == 1 ? fmt::format("\n{{ret_{}}}\n", i++) : fmt::format("\n[{}] : {{ret_{}}}\n", util::join(vec, ", "), i++));
      out << rdoc << '\n';
    }
  }

  return {fs.str(), param_types, return_types};
}

//---------------------------------------------------------

str_t pydoc(cls_info_t const &cls) {
  std::stringstream fs;
  if (cls.ptr) {
    fs << cls.ptr->doc_brief;
    fs << (cls.ptr->doc_details.empty() ? "" : (cls.ptr->doc_brief.empty() ? cls.ptr->doc_details : fmt::format("\n\n{}", cls.ptr->doc_details)));
  }
  return fs.str();
}

// ------------------------------

// Returns: [{field_name, field_type_fqn, initializer_str, doc_string}]
std::vector<std::vector<std::string>> get_fields_info(cls_info_t const &cls_info) {
  std::vector<std::vector<std::string>> res;
  for (auto const &f : cls_info.fields) {
    std::vector<std::string> m(4);
    m[0] = f->name;
    m[1] = f->type.name;
    m[2] = f->initializer_str;
    m[3] = f->doc_brief;
    m[3] += f->doc_details.empty() ? "" : (m[3].empty() ? f->doc_details : fmt::format("\n\n{}", f->doc_details));
    res.push_back(std::move(m));
  }
  std::ranges::stable_sort(res, [](auto &&x, auto &&y) { return int(x[2].empty()) > int(y[2].empty()); });
  return res;
}

//---------------------------------------------------------

std::tuple<str_t, std::vector<str_t>> pydoc_of_synthetized_constructor(cls_info_t const &cls_info) {
  std::stringstream fs;
  auto out = triqs::indented_ostream{fs, 3};

  fs << "Synthesized constructor with the following keyword arguments:\n";

  auto info_vec = get_fields_info(cls_info);
  std::vector<str_t> field_types;
  if (not info_vec.empty()) {
    fs << "\nParameters\n----------\n";
    for (int i = 0; auto const &info : info_vec) {
      field_types.push_back({info[1]});
      fs << fmt::format("{} : {{par_{}}}{}\n\n", info[0], i++, info[2].empty() ? "" : fmt::format(", default={}", info[2]));
    }
  }

  return {fs.str(), field_types};
}
