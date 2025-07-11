#include "clang/AST/ASTConsumer.h"
#include "./worker.hpp"

// -----------------------------------------------------------
//  AST consumer
// -----------------------------------------------------------

class ast_consumer : public clang::ASTConsumer {
  std::shared_ptr<worker_t> worker;

  public:
  explicit ast_consumer(std::shared_ptr<worker_t> const &worker_in) : worker{worker_in} {}

  //void Initialize(clang::ASTContext &ctx) override;
  void HandleTranslationUnit(clang::ASTContext &ctx) override;
};
