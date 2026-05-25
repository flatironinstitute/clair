#include "custom_action.hpp"

#include <filesystem>
#include <fstream>

#include "flang/Frontend/CompilerInstance.h"
#include "flang/Frontend/FrontendOptions.h"
#include "flang/Semantics/scope.h"
#include "flang/Semantics/symbol.h"
#include "../codegen/module.hpp"
#include "clu/clang_format.hpp"
#include "utility/logger.hpp"
#include "traversal.hpp"

namespace sema = Fortran::semantics;
namespace fs   = std::filesystem;

void custom_action::executeAction() {
  wdata = std::make_shared<wdata_t>(&getInstance());

  sema::Scope &global    = getInstance().getSemanticsContext().globalScope();
  auto const &inputs     = getInstance().getFrontendOpts().inputs;
  std::string sourcefile = inputs.empty() ? "" : inputs[0].getFile().str();
  fs::path source_dir    = fs::absolute(sourcefile).parent_path();

  for (auto const &[name, symRef] : global) {
    sema::Symbol const &sym = symRef.get();
    if (!sym.has<sema::ModuleDetails>()) continue;

    sema::Scope const *modScope = sym.get<sema::ModuleDetails>().scope();
    if (!modScope) continue;

    module_info_t mi;
    mi.module_name          = sym.name().ToString();
    mi.sourcefile           = sourcefile;
    mi.sourcefile_full_stem = (source_dir / mi.module_name).string();

    process_module_scope(*modScope, mi.module_name, mi);

    wdata->intern(std::move(mi));
  }

  // Codegen
  util::logger log = util::logger{"-- ", "", 1};
  for (auto const &mi : wdata->modules) {
    auto code     = codegen_module(*mi);
    auto code_hxx = codegen_hxx(*mi);

    auto outfilename     = mi->sourcefile_full_stem + ".wrap.cxx";
    auto outfilename_hxx = mi->sourcefile_full_stem + ".wrap.hxx";

    log(fmt::format("\033[1;34m[Success] \033[0m"));
    log(fmt::format("   Generated Python bindings: {} [included in {}]", outfilename, mi->sourcefile));
    if (not code_hxx.empty())
      log(fmt::format("             headers        : {} [to be used with other modules, cf doc]", outfilename_hxx));

    if (not std::getenv("CLAIR_SKIP_CLANG_FORMAT")) {
      log("Running clang-format ...");
      auto style = clang::format::getStyle("file", mi->sourcefile, "LLVM").get();
      code       = clu::clang_format(code, style);
      if (not code_hxx.empty()) code_hxx = clu::clang_format(code_hxx, style);
      log("Done.");
    }

    std::ofstream(outfilename) << code;
    if (not code_hxx.empty()) std::ofstream(outfilename_hxx) << code_hxx;
  }
}
