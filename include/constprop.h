#pragma once
#include <vector>
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {

struct ConstantPropagation : PassInfoMixin<ConstantPropagation> {

  // Check if constants have the right size
  bool is_constant(Value *op, int &constant);

  //Run function for new pass manager
  llvm::PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

  //
  bool constProp(llvm::BasicBlock *b);

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired();
};

llvm::PassPluginLibraryInfo getConstantPropagationPluginInfo();
}
