#include <unordered_map>
#include "utility/string_tools.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/ASTContext.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"

#include "clu/misc.hpp"

namespace clu {

  str_t get_raw_comment(const clang::Decl *d);

  // Representation of a documentation string
  struct doc_string_t {
    str_t brief, content;
    std::vector<std::pair<str_t, str_t>> params, tparams;
    std::unordered_map<str_t, str_t> misc;

    doc_string_t() = default;
    doc_string_t(const clang::Decl *d);
  };

} // namespace clu
