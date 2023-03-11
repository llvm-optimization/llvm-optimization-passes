// opt -load-pass-plugin ./lib/libCountPass.so  -passes="print-count-pass" test.bc
// opt-15 -load-pass-plugin ./libHelloWorld.so  -enable-new-pm=1  --passes='print-count-pass' ./test.bc

//#include "../include/countpass.h"
#include <iterator>
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
struct CountPass : public AnalysisInfoMixin<CountPass>
  {
    public:
      using ResultCountPass = SmallVector<unsigned int>;
    using Result = ResultCountPass;
    static bool isRequired() { return true; }

    static llvm::AnalysisKey Key;
    friend struct llvm::AnalysisInfoMixin<CountPass>;
PreservedAnalyses run(Function &F,
                      FunctionAnalysisManager &)
{

  unsigned int icount = 0;
  for (auto &basicb : F)
    icount += std::distance(basicb.begin(), basicb.end());

  SmallVector <unsigned int> test;
  test.push_back(icount);
   errs() << "function "<< F.getName() <<" nb instructions: "<< icount << "\n";
    return PreservedAnalyses::all();

}
};

  class CountPassPrinter : public PassInfoMixin<CountPassPrinter>
  {
    public:
        explicit CountPassPrinter(raw_ostream &OutS) : OS(OutS) {}

    static bool isRequired() { return true; }

       raw_ostream &OS;
PreservedAnalyses run(Function &F,
                                        FunctionAnalysisManager &FAM)
{
  auto &result = FAM.getResult<CountPass>(F);


  OS << format("Nb instructions: %lu\n", result.pop_back_val());

  return PreservedAnalyses::all();
}
  };

llvm::PassPluginLibraryInfo getCountPassPluginInfo()
{
  return {LLVM_PLUGIN_API_VERSION, "CountPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB)
          {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>)
                {
                  if (Name == "print-count-pass")
                  {
                    FPM.addPass(CountPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
  return getCountPassPluginInfo();
}