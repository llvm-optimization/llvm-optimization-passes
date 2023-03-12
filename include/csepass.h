#pragma once
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <iterator>
using namespace llvm;

struct CSEPass : public AnalysisInfoMixin<CSEPass> {
public:
  static bool isRequired() { return true; }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &);

private:
};

llvm::PassPluginLibraryInfo getCSEPassPluginInfo();

void SwitchCase(int position, Instruction *I, Instruction *j);