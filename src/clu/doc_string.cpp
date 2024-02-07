#include "doc_string.hpp"
#include "clang/AST/Comment.h"
#include "clang/Basic/SourceManager.h"
#include <clang/AST/Attr.h>
#include <itertools/itertools.hpp>
namespace clu {

  str_t get_raw_comment(const clang::Decl *d) {
    auto &ctx = d->getASTContext();
    if (const clang::RawComment *rc = ctx.getRawCommentForDeclNoCache(d)) {
      return str_t{rc->getRawText(ctx.getSourceManager())};
    } else
      return {}; // no comment is not an error
  }

  doc_string_t::doc_string_t(const clang::Decl *d) {

    const clang::ASTContext *ctx = &d->getASTContext();

    // Register a few additional Block Command \xxx that are not by default in doxygen
    static bool command_are_registered = false;
    if (!command_are_registered) {
      command_are_registered = true;
      auto &cmd              = ctx->getCommentCommandTraits();
      //cmd.registerBlockCommand("tag");
      //cmd.registerBlockCommand("merge");
      cmd.registerBlockCommand("return");
    }

    // Grabs one paragraph into a string. Need to iterate on childs (??).
    auto get_one_paragraph = [](clang::comments::ParagraphComment const *p) -> std::string { // can be null
      std::string r;
      if (!p) return r;
      for (auto it = p->child_begin(); it != p->child_end(); ++it)
        if (auto *text = llvm::dyn_cast_or_null<clang::comments::TextComment>(*it))
          if (not text->isWhitespace()) r += text->getText().str();
      return util::trim(r);
    };

    if (clang::comments::FullComment *com = ctx->getCommentForDecl(d, nullptr)) {
      int i = 0;
      for (auto *b : com->getBlocks()) {

        // A paragraph
        if (auto *p = llvm::dyn_cast_or_null<clang::comments::ParagraphComment>(b)) {

          for (auto it = p->child_begin(); it != p->child_end(); ++it) {

            if (auto *c = llvm::dyn_cast_or_null<clang::comments::TextComment>(*it)) {
              auto s = util::trim(str_t{c->getText()});
              if (i++ == 0) {
                brief = s;
              } else {
                content += s + (!c->hasTrailingNewline() ? '\n' : ' ');
              }
            }

            if (auto *c = llvm::dyn_cast_or_null<clang::comments::InlineCommandComment>(*it)) {
              auto command = util::trim(clu::get_source_range_as_string(c->getCommandNameRange(), ctx));
              misc.emplace(command.substr(1, str_t::npos), "");
            }
          }
        }

        // \param
        else if (auto *param = llvm::dyn_cast_or_null<clang::comments::ParamCommandComment>(b))
          params.emplace_back(param->getParamNameAsWritten().str(), get_one_paragraph(param->getParagraph()));

        // \tparam
        else if (auto *tparam = llvm::dyn_cast_or_null<clang::comments::TParamCommandComment>(b))
          tparams.emplace_back(tparam->getParamNameAsWritten().str(), get_one_paragraph(tparam->getParagraph()));

        // \something_else  (have a bit less information)
        // else if (auto *other = llvm::dyn_cast_or_null<clang::comments::InlineCommandComment>(b)) {
        //   auto name = other->getCommandName(ctx->getCommentCommandTraits()).str();
        //   //std::cout << "COMMAND name " << name << "\n";
        //   //for (auto i : itertools::range(other->getNumArgs())) {
        //   //  auto value = std::string{other->getArgText(i)};
        //   //  std::cout << "arg  " << i << " " << value << "\n";
        //   //}
        // }

        else if (auto *other = llvm::dyn_cast_or_null<clang::comments::BlockCommandComment>(b)) {
          auto command_name = other->getCommandName(ctx->getCommentCommandTraits()).str();
          if (auto *verb = llvm::dyn_cast_or_null<clang::comments::VerbatimLineComment>(other)) {
            misc.emplace(command_name, util::trim(verb->getText().str()));
          } else if (auto *verb = llvm::dyn_cast_or_null<clang::comments::VerbatimBlockComment>(other)) {
            misc.emplace(command_name, util::trim(verb->getText(0).str()));
          } else
            misc.emplace(command_name, get_one_paragraph(other->getParagraph()));
        }
      }
    }
    content = util::trim(content);
    brief   = util::trim(brief);
  }

} // namespace clu
