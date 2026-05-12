#include "doc_string.hpp"
#include "../utility/logger.hpp"
#include "../utility/streams.hpp"
#include <clang/AST/Comment.h>
#include <clang/AST/Attr.h>
#include <clang/Basic/SourceManager.h>
#include <fmt/format.h>
#include <itertools/itertools.hpp>
#include <algorithm>
#include <sstream>
#include <utility>

// Logger for warnings related to documentation processing.
static const struct {
  util::logger warn = util::logger{&std::cout, "-- ", "\033[38;5;212mDoc warning: \033[0m"};
} logs;

namespace {

  // Remove a single whitespace character from the beginning of a string.
  std::string trim_single_ws(const std::string &str) { return !str.empty() && str[0] == ' ' ? str.substr(1) : str; }

  // Check if a string is either empty or ends with a newline character.
  bool on_new_line(const std::string &str) { return str.empty() || str.back() == '\n'; }

  // Count the number of trailing newline characters in a string.
  long count_trailing_newlines(const std::string &str) {
    long count = 0;
    for (auto it = str.rbegin(); it != str.rend() && *it == '\n'; ++it, ++count);
    return count;
  }

  // Add a given number of new lines. The maximum is 2. No new lines are added to an empty string.
  void add_new_lines(long new_lines, str_t &str) {
    if (str.empty()) return;
    new_lines = std::min(2l - count_trailing_newlines(str), new_lines);
    for (long i = 0; i < new_lines; ++i) str += "\n";
  }

  // Indent each line of a given string with a given number of white spaces.
  std::string indent_lines(str_t const &str, int indent) {
    std::ostringstream oss;
    triqs::indented_ostream indented_os{oss, indent};
    indented_os << str;
    return oss.str();
  }

  // Find a string in a vector of pairs of strings and return a reference to the second element of the pair if found.
  // Otherwise, push back a new pair and return a reference to the second element of the new pair.
  std::string &get_str_ref(std::vector<std::pair<std::string, std::string>> &vec, const std::string &str) {
    auto it = std::ranges::find_if(vec, [&str](const auto &p) { return p.first == str; });
    if (it != vec.end()) return (*it).second;
    vec.emplace_back(str, "");
    return vec.back().second;
  }

} // namespace

namespace clu {

  str_t get_raw_comment(const clang::Decl *d) {
    auto &ctx = d->getASTContext();
    if (const clang::RawComment *rc = ctx.getRawCommentForDeclNoCache(d)) {
      return str_t{rc->getRawText(ctx.getSourceManager())};
    } else
      return {}; // no comment is not an error
  }

  // ------------------------------

  doc_string_t::doc_string_t(const clang::Decl *d) : d_(d), ctx_(&d->getASTContext()) {
    // get FullComment of a declaration
    auto *full_com = ctx_->getCommentForDecl(d, nullptr);

    // early return if there is nothing to process
    if (!full_com || full_com->child_begin() == full_com->child_end()) return;

    // initialize the iterator for looping over all BlockContentComments of the FullComment
    auto it     = full_com->child_begin();
    auto it_end = full_com->child_end();

    // keep track of the source location of the last processed comment
    src_loc_ = get_source_rg(*it).first;

    // ignore an empty first ParagraphComment
    if (auto *pc = llvm::dyn_cast_or_null<clang::comments::ParagraphComment>(*it); pc && pc->isWhitespace()) {
      ++it;
      src_loc_ = (it != it_end) ? get_source_rg(*it).first : src_loc_;
    }

    // loop over all children/blocks of the FullComment
    while (it != it_end) {
      // reset the source location to the current comment if requested
      if (reset_src_loc_) {
        src_loc_       = get_source_rg(*it).first;
        reset_src_loc_ = false;
      }

      if (auto *pcc = llvm::dyn_cast_or_null<clang::comments::ParamCommandComment>(*it)) {
        // @param
        block_command(it, get_str_ref(params_vec, pcc->getParamNameAsWritten().str()));
      } else if (auto *tpcc = llvm::dyn_cast_or_null<clang::comments::TParamCommandComment>(*it)) {
        // @tparam
        block_command(it, get_str_ref(tparams_vec, tpcc->getParamNameAsWritten().str()));
      } else if (auto *vbc = llvm::dyn_cast_or_null<clang::comments::VerbatimBlockComment>(*it)) {
        // \f$ ... \f$, \f[ ... \f], @code ... @endcode, etc.
        verbatim_block_comment(vbc, details_str);
      } else if (auto *bcc = llvm::dyn_cast_or_null<clang::comments::BlockCommandComment>(*it)) {
        auto const cmd_name = bcc->getCommandName(ctx_->getCommentCommandTraits());
        if (cmd_name == "brief") {
          // @brief
          block_command(it, brief_str);
        } else if (cmd_name == "details") {
          // @details
          block_command(it, details_str);
        } else if (cmd_name == "return") {
          // @return
          block_command(it, return_str);
        } else if (cmd_name == "note" || cmd_name == "warning") {
          // @note or @warning
          str_t tmp_str{};
          block_command(it, tmp_str);
          add_new_lines(2, details_str);
          details_str += fmt::format(".. {}::\n\n{}", cmd_name.str(), indent_lines(tmp_str, 3));
          details_str += "\n\n";
        } else if (cmd_name == "ingroup") {
          // @ingroup
          misc_vec.emplace_back(cmd_name.str(), "");
          block_command(it, misc_vec.back().second);
        } else {
          // other unsupported block commands
          logs.warn(
             fmt::format("Unsupported BlockCommandComment {} at {}", cmd_name.str(), bcc->getSourceRange().printToString(ctx_->getSourceManager())));
          misc_vec.emplace_back(cmd_name.str(), "");
          block_command(it, misc_vec.back().second);
        }
      } else if (auto *pc = llvm::dyn_cast_or_null<clang::comments::ParagraphComment>(*it)) {
        // standalone ParagraphComment
        paragraph_comment(pc, details_str);
      }
      ++it;
    }

    // trim any leading and trailing whitespace from the strings
    brief_str   = util::trim(brief_str);
    details_str = util::trim(details_str);
    return_str  = util::trim(return_str);
    for (auto &[pname, pdoc] : params_vec) pdoc = util::trim(pdoc);
    for (auto &[tpname, tpdoc] : tparams_vec) tpdoc = util::trim(tpdoc);
    for (auto &[mname, mdoc] : misc_vec) mdoc = util::trim(mdoc);
  }

  // Check if a given child_iterator (clang::comments::Comment *const*) points to an inline math equation.
  bool doc_string_t::is_inline_math(clang::comments::Comment::child_iterator it) const {
    if (auto *vbc = llvm::dyn_cast_or_null<clang::comments::VerbatimBlockComment>(*it)) {
      return vbc->getCommandName(ctx_->getCommentCommandTraits()).str() == "f$";
    }
    return false;
  }

  // Get the source range of a Comment as a pair of ints, i.e. (begin line #, end line #).
  std::pair<long, long> doc_string_t::get_source_rg(const clang::comments::Comment *cmt) const {
    auto &sm = d_->getASTContext().getSourceManager();
    auto beg = sm.getPresumedLoc(cmt->getBeginLoc());
    auto end = sm.getPresumedLoc(cmt->getEndLoc());
    return {static_cast<long>(beg.getLine()), static_cast<long>(end.getLine())};
  }

  // A ParagraphComment is processed as follows:
  // - TextComment: --> as is
  // - InlineCommandComment: --> ``arg1 arg2 ...``
  // - HTMLStartTagComment: --> .. raw::html\n\n   <tag attr1="value1" ...>
  // - HTMLEndTagComment: --> </tag>\n\n
  void doc_string_t::paragraph_comment(const clang::comments::ParagraphComment *pc, str_t &str) {
    for (auto it = pc->child_begin(); it != pc->child_end(); ++it) {
      // add new lines if the current source location is greater than the last one
      auto current_src_loc = get_source_rg(*it).first;
      add_new_lines(current_src_loc - src_loc_, str);

      // handle different types of comments
      if (auto *tc = llvm::dyn_cast_or_null<clang::comments::TextComment>(*it)) {
        // text comment
        str += on_new_line(str) ? trim_single_ws(tc->getText().str()) : tc->getText().str();
      } else if (auto *icc = llvm::dyn_cast_or_null<clang::comments::InlineCommandComment>(*it)) {
        // inline command comment
        if (icc->getNumArgs() == 0) continue;
        str += fmt::format("``{}", icc->getArgText(0).str());
        for (unsigned int i = 1; i < icc->getNumArgs(); ++i) str += " " + icc->getArgText(i).str();
        str += "``";
      } else if (auto *hstc = llvm::dyn_cast_or_null<clang::comments::HTMLStartTagComment>(*it)) {
        // HTML start tag comment
        add_new_lines(2, str);
        str += ".. raw:: html\n\n";
        str += fmt::format("{:<3}<{}", "", hstc->getTagName().str());
        for (int i = 0; i < hstc->getNumAttrs(); ++i) {
          auto const &attr = hstc->getAttr(i);
          str += fmt::format(" {}=\"{}\"", attr.Name.str(), attr.Value.str());
        }
        str += ">";
      } else if (auto *hetc = llvm::dyn_cast_or_null<clang::comments::HTMLEndTagComment>(*it)) {
        // HTML end tag comment (has to be on the same line as the corresponding HTMLStartTagComment)
        str += fmt::format("</{}>\n\n", hetc->getTagName().str());
      }

      // update last source location
      src_loc_ = get_source_rg(*it).second;
    }
  }

  // Process verbatim block comments in the following way:
  // - \f$ ... \f$ --> :math:`...`,
  // - \f[ ... \f] --> .. math:\n\n   ...\n\n,
  // - \f{env}{ ... \f} --> .. math:\n\n   \begin{env} ... \end{env}\n\n,
  // -  @code ... @endcode, etc. --> ::\n\n   ...\n\n
  void doc_string_t::verbatim_block_comment(const clang::comments::VerbatimBlockComment *vbc, str_t &str) {
    // ignore empty math environments
    if (vbc->getNumLines() == 0) {
      logs.warn(fmt::format("Empty math environment: {}", vbc->getSourceRange().printToString(d_->getASTContext().getSourceManager())));
      src_loc_ = get_source_rg(vbc).second;
      return;
    }

    // get the current source location
    auto current_src_loc = get_source_rg(vbc).first;

    // format the output string based on the command name
    auto const cmd_start = vbc->getCommandName(d_->getASTContext().getCommentCommandTraits()).str();
    if (cmd_start == "f$") {
      // inline math equation
      add_new_lines(current_src_loc - src_loc_, str);
      str += ":math:`";
      str += fmt::format("{}", vbc->getText(0).trim().str());
      for (int i = 1; i < vbc->getNumLines(); ++i) str += fmt::format("\n{}", vbc->getText(i).trim().str());
      str += "`";
    } else if (cmd_start == "f[") {
      // multiline math equation
      add_new_lines(2, str);
      str += ".. math::\n\n";
      str += fmt::format("{:<3}{}", "", vbc->getText(0).trim().str());
      for (int i = 1; i < vbc->getNumLines(); ++i) str += fmt::format("\n{:<3}{}", "", vbc->getText(i).trim().str());
      str += "\n\n";
      reset_src_loc_ = true;
    } else if (cmd_start == "f{") {
      // multiline math equation with latex environment
      add_new_lines(2, str);

      // get latex environment name (assuming the command looks like \f{env_name}{\n ...\f})
      auto first_line      = vbc->getText(0).trim().str();
      auto pos             = first_line.find('}');
      std::string env_name = first_line.substr(0, pos != std::string::npos ? pos : first_line.size());

      // make rst math environment
      str += ".. math::\n\n";
      str += fmt::format("{:<3}\\begin{{{}}}", "", env_name);
      for (int i = 1; i < vbc->getNumLines(); ++i) str += fmt::format("\n{:<3}{}", "", vbc->getText(i).trim().str());
      str += fmt::format("\n{:<3}\\end{{{}}}", "", env_name);
      str += "\n\n";
      reset_src_loc_ = true;
    } else {
      // code block or other verbatim block comment
      add_new_lines(2, str);
      str += "::\n\n";
      str += fmt::format("{:<3}{}", "", trim_single_ws(vbc->getText(0).str()));
      for (int i = 1; i < vbc->getNumLines(); ++i) str += fmt::format("\n{:<3}{}", "", trim_single_ws(vbc->getText(i).str()));
      str += "\n\n";
      reset_src_loc_ = true;
    }

    // update the last source location (llvm gets the source range wrong for inline math equations over multiple lines)
    src_loc_ = current_src_loc + vbc->getNumLines() - 1;
  }

  // Process a BlockCommandComment + subsequent inline math equations and ParagraphComment(s).
  // It can consist of consecutive ParagraphComments(s) and VerbatimBlockComments(s).
  // Only inline math equations are allowed as VerbatimBlockComments. Any other VerbatimBlockComment will terminate the block.
  // A blank line will also terminate the block.
  // The iterator after the call will point to the last processed comment that was part of the block.
  void doc_string_t::block_command(clang::comments::Comment::child_iterator &it, str_t &str) {
    // handle the first ParagraphComment
    auto *bcc = llvm::dyn_cast_or_null<clang::comments::BlockCommandComment>(*it);
    if (auto *pc = bcc->getParagraph()) paragraph_comment(pc, str);

    // loop over comments until the block is terminated
    auto *full_com = d_->getASTContext().getCommentForDecl(d_, nullptr);
    while (++it != full_com->child_end()) {
      // terminate if we encounter a blank line
      if (get_source_rg(*it).first - src_loc_ > 1) { break; }

      // handle different types of comments
      if (auto *pc = llvm::dyn_cast_or_null<clang::comments::ParagraphComment>(*it)) {
        // paragraph comments are appended to the string
        paragraph_comment(pc, str);
      } else if (is_inline_math(it)) {
        // inline math equations are appended to the string
        verbatim_block_comment(llvm::dyn_cast_or_null<clang::comments::VerbatimBlockComment>(*it), str);
      } else {
        // everything else terminates the paragraph
        break;
      }
    }
    --it;
  }

} // namespace clu
