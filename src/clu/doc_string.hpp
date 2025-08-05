#include "utility/string_tools.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Comment.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Rewrite/Frontend/Rewriters.h>
#include <utility>
#include <vector>

namespace clu {

  str_t get_raw_comment(const clang::Decl *d);

  // Representation of a documentation string
  struct doc_string_t {
    str_t brief_str, details_str, return_str;
    std::vector<std::pair<str_t, str_t>> params_vec, tparams_vec, misc_vec;

    doc_string_t() = default;
    doc_string_t(const clang::Decl *d);

    private:
    // Check if a given child_iterator (clang::comments::Comment *const*) points to an inline math equation.
    bool is_inline_math(clang::comments::Comment::child_iterator it) const;

    // Get the source range of a Comment as a pair of ints, i.e. (begin line #, end line #).
    std::pair<long, long> get_source_rg(const clang::comments::Comment *cmt) const;

    // Process a ParagraphComment.
    void paragraph_comment(const clang::comments::ParagraphComment *pc, str_t &str);

    // Process a VerbatimBlockComment.
    void verbatim_block_comment(const clang::comments::VerbatimBlockComment *vbc, str_t &str);

    // Process a BlockCommandComment + subsequent inline math equations and ParagraphComment(s).
    void block_command(clang::comments::Comment::child_iterator &it, str_t &str);

    private:
    long src_loc_                 = 0;
    const clang::Decl *d_         = nullptr;
    const clang::ASTContext *ctx_ = nullptr;
    bool reset_src_loc_           = false;
  };

} // namespace clu
