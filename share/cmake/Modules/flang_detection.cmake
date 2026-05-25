#=========================
# Locate Flang and LLVM
#=========================

find_package(Flang REQUIRED CONFIG HINTS ${LLVM_ROOT} $ENV{LLVM_ROOT} ${Flang_ROOT} $ENV{Flang_ROOT})

message(STATUS "FLANG_INSTALL_PREFIX  : ${FLANG_INSTALL_PREFIX}")
message(STATUS "FLANG_INCLUDE_DIRS    : ${FLANG_INCLUDE_DIRS}")

#===============================================================
# Create an Interface target for the Flang and LLVM Libraries
#===============================================================
add_library(flang_llvm INTERFACE)
if(LLVM_LINK_LLVM_DYLIB)
  target_link_libraries(flang_llvm INTERFACE flangFrontend MLIR $<$<PLATFORM_ID:Linux>:LLVM>)
else()
  target_link_libraries(flang_llvm INTERFACE flangFrontend MLIR $<$<PLATFORM_ID:Linux>:LLVMSupport>)
endif()

if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(flang_llvm INTERFACE -fno-rtti)
endif()
