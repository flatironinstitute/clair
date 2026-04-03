#include "clang/AST/ASTConsumer.h"
#include "./wdata.hpp"

class ast_consumer : public clang::ASTConsumer {
  std::shared_ptr<wdata_t> wdata_sp;
  wdata_t *wdata;

  public:
  explicit ast_consumer(std::shared_ptr<wdata_t> const &wdata_in) : wdata_sp{wdata_in}, wdata{wdata_sp.get()} {}
  void HandleTranslationUnit(clang::ASTContext &ctx) override;
};
