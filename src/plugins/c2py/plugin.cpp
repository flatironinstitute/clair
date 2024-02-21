#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
//#include "clang/Basic/Version.h"
//#include "clang/Rewrite/Core/Rewriter.h"
//#include "clang/Rewrite/Frontend/Rewriters.h"
//#include "clang/AST/ASTConsumer.h"
//#include "clang/AST/ASTContext.h"
//#include "clang/ASTMatchers/ASTMatchers.h"
//#include "clang/Parse/ParseAST.h"

#include "./ast_consumer.hpp"
#include "clu/compile.hpp"
#include "codegen/module.hpp"
#include <fstream>
#include <regex>
#include <fmt/core.h>
#include <fmt/format.h>
#include "utility/macros.hpp"
#include "utility/stl_complement.hpp"
#include "./clang_format_config.hpp"

// -------------------------------------------------------------------------------------------
//
//  PluginASTAction
//
// -------------------------------------------------------------------------------------------

class custom_action : public clang::PluginASTAction {
  std::shared_ptr<worker_t> worker;

  public:
  using ASTConsumerPointer = std::unique_ptr<clang::ASTConsumer>;

  custom_action() = default;

  // --------------------------

  //  virtual bool PrepareToExecuteAction(clang::CompilerInstance & Compiler) override{}

  // skip function bodies. Gain in compiling time is small
  // virtual bool BeginInvocation(clang::CompilerInstance &CI) override {
  //   CI.getInvocation().getFrontendOpts().SkipFunctionBodies = 1;
  //   return true;
  // }

  // --------------------------

  ASTConsumerPointer CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) override {
    worker = std::make_unique<worker_t>(&compiler);
    return std::make_unique<ast_consumer>(worker);
  }

  // --------------------------

  bool ParseArgs(clang::CompilerInstance const &, const std::vector<std::string> &) override { return true; }

  // --------------------------

  PluginASTAction::ActionType getActionType() override { return ReplaceAction; }

  // --------------------------

  void EndSourceFileAction() override {
    auto &ci = this->getCompilerInstance();
    if (ci.getASTContext().getDiagnostics().hasErrorOccurred()) return;

    auto code     = codegen_module(worker->module_info);
    auto code_hxx = codegen_hxx(worker->module_info);

    auto outfilename     = worker->module_info.module_name + ".wrap.cxx";
    auto outfilename_hxx = worker->module_info.module_name + ".wrap.hxx";

    std::ofstream(outfilename) << code;
    std::ofstream(outfilename_hxx) << code_hxx;
    std::cerr << "Generated Python bindings " << outfilename << std::endl;

    if (not std::filesystem::exists(".clang-format")) std::ofstream(".clang-format") << clang_format_config;
    // Use a system call for clang-format

    system(fmt::format(R"RAW({0} -i {1})RAW", AS_STRING(CLANG_FORMAT), outfilename).c_str());
    system(fmt::format(R"RAW({0} -i {1})RAW", AS_STRING(CLANG_FORMAT), outfilename_hxx).c_str());

    code = util::read_txt_file(outfilename);

    std::cout << "Compiling Python module " << worker->module_info.module_name << ".so ..." << std::endl;
    clu::compile(ci, code);
    std::cout << "Done." << std::endl;
  }

  // --------------------------
  void PrintHelp(llvm::raw_ostream &ros) { ros << "Help \n"; }
};

// -------------------------------------------
// Registration
//-----------------------------------------------------------------------------
//NOLINTNEXTLINE
static clang::FrontendPluginRegistry::Add<custom_action> X(/*Name=*/"c2py", /*Description=*/"c2py plugin");
