#include "./compile.hpp"
#include "utility/macros.hpp"
#include <algorithm>
#include <iostream>
#include <filesystem>
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "llvm/Support/MemoryBuffer.h"

namespace fs = std::filesystem;

void clu::compile(clang::CompilerInstance &ci, std::string const &code) {
  auto &CodeGenOpts = ci.getCodeGenOpts();
  auto &Target      = ci.getTarget();
  auto &Diagnostics = ci.getDiagnostics();

  auto filename             = ci.getInvocation().getFrontendOpts().Inputs[0].getFile().str();
  std::string filename_wrap = fs::path{filename}.replace_extension("wrap.cxx").c_str();

  auto output = ci.getInvocation().getFrontendOpts().OutputFile;

  // create new compiler instance
  auto cinvNew = std::make_shared<clang::CompilerInvocation>();

  auto clargs1 = CodeGenOpts.CommandLineArgs;
  for (auto &x : clargs1)
    if (x == filename) x = filename_wrap; // += ".cxx"; // std::cout  << x <<std::endl;

  std::vector<const char *> clargs2(clargs1.size());
  std::transform(clargs1.begin(), clargs1.end(), clargs2.begin(), [](auto &&x) { return x.c_str(); });

  bool cinvNewCreated = clang::CompilerInvocation::CreateFromArgs(*cinvNew, clargs2, Diagnostics);
  EXPECTS(cinvNewCreated);

  clang::CompilerInstance ciNew;
  ciNew.setInvocation(cinvNew);
  ciNew.setTarget(&Target);
  ciNew.createDiagnostics();

  // create rewrite buffer
  auto FileMemoryBuffer = llvm::MemoryBuffer::getMemBufferCopy(code);

  // create "virtual" input file
  auto &PreprocessorOpts = ciNew.getPreprocessorOpts();
  PreprocessorOpts.addRemappedFile(filename_wrap, FileMemoryBuffer.get());

  //ciNew.getFrontendOpts ().OutputFile = "bb.o";

  // generate code
  clang::EmitObjAction EmitObj;
  ciNew.ExecuteAction(EmitObj);

  // clean up rewrite buffer
  FileMemoryBuffer.release();
}
