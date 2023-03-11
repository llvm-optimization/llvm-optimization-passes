#ifndef LLVM_TRANSFORMS_HELLONEW_OPTIMIZATIONCOA_H
#define LLVM_TRANSFORMS_HELLONEW_OPTIMIZATIONCOA_H

#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

  using ResultCountPass = SmallVector<unsigned int>;

  struct CountPass : public AnalysisInfoMixin<CountPass>
  {
    using Result = ResultCountPass;

    Result run(Function &F, FunctionAnalysisManager &AM);
    static bool isRequired() { return true; }

  private:
    static llvm::AnalysisKey Key;
    friend struct llvm::AnalysisInfoMixin<CountPass>;
  };

  class CountPassPrinter : public PassInfoMixin<CountPassPrinter>
  {
  public:
    explicit CountPassPrinter(raw_ostream &OutS) : OS(OutS) {}
    //llvm::raw_ostream CounPassPrinter(llvm::raw_fd_ostream &OutS){}
    llvm::PreservedAnalyses run(Function &Func,
                                FunctionAnalysisManager &FAM);

    static bool isRequired() { return true; }

    private:
       raw_ostream &OS;

  };

 // namespace llvm

#endif // LLVM_TRANSFORMS_HELLONEW_OPTIMIZATIONCOA_H