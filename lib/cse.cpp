#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <algorithm>
#include <iterator>

// opt-15 -S -load-pass-plugin ./libCSE.so  -enable-new-pm=1
// --passes='print-cse-pass' ./testopt.ll -o testfin.ll clang -o test testfin.ll

using namespace llvm;

bool find_inst(std::list<Instruction *> list, Instruction const *B) {
  for (Instruction *A : list) {
    if (A->getName() == B->getName())
      return true;
  }
  return false;
}

struct CSEPass : public AnalysisInfoMixin<CSEPass> {
public:
  static bool isRequired() { return true; }

  static llvm::AnalysisKey Key;
  friend struct llvm::AnalysisInfoMixin<CSEPass>;
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

    llvm::DominatorTree DT = llvm::DominatorTree();
    DT.recalculate(F);

    std::list<Instruction *> to_delete;

    // for (auto basicb : F)
    for (auto basicb = F.begin(); basicb != F.end(); basicb++) {

      // for (auto &I : basicb)

      for (auto I = basicb->begin(); I != basicb->end(); I++) {
        errs() << "opcode   " << I->getOpcode() << " " << *I << "\n";

        if ((I->getOpcode() >= Instruction::Add) &&
            (I->getOpcode() <= Instruction::FDiv)) {

          for (auto j = basicb->begin(); j != basicb->end(); j++) {
            if (I != j) {

              if (DT.dominates((Instruction *)&*I, (Instruction *)&*j) &&
                  (I->getOpcode() == j->getOpcode()) &&
                  (I->getType() == j->getType()) &&
                  (I->getNumOperands() == j->getNumOperands())) {

                unsigned nbOp = I->getNumOperands();
                errs() << "ok ici I = " << I->getOpcode() << " "
                       << I->getNumOperands() << " " << *I << "\n";
                errs() << "ok ici J = " << j->getOpcode() << " "
                       << j->getNumOperands() << " " << *j << "\n";
                // errs() << "ok ici J++ = " << j++->getOpcode() << " " <<
                // j++->getNumOperands() << " " << *j++ << "\n";
                //  Now compare operands
                if (find_inst(to_delete, &*j))
                  continue;
                bool sim = true, sim2 = true;
                // Case A+B = A+B
                if ((I->getOperand(0) != j->getOperand(0)) ||
                    (I->getOperand(1) != j->getOperand(1)))
                  sim = false;

                // Case A+B = B+A but A*B =/= B*A and A/B =/= B/A
                if (((I->getOperand(1) != j->getOperand(0)) ||
                     (I->getOperand(0) != j->getOperand(1)))) {
                  errs() << "rentre dedans TROUVER I = " << I->getNumOperands()
                         << " " << *I << "\n";
                  errs() << "rentre dedans TROUVER J = " << j->getOpcode()
                         << " " << *j << "\n";
                  sim2 = false;
                }

                if (sim ||
                    (Instruction::isCommutative(I->getOpcode()) && sim2)) {
                  errs() << sim << " " << sim2 << "\n";
                  errs() << "TROUVER I = " << I->getOpcode() << " " << *I
                         << "\n";
                  errs() << "TROUVER J = " << j->getOpcode() << " " << *j
                         << "\n";

                  j->replaceAllUsesWith((Instruction *)&*I);
                  // Delete after
                  to_delete.push_back((Instruction *)&*j);
                  // j->removeFromParent();
                  continue;
                }
                if (j == basicb->end())
                  continue;
                if (j++->getNumOperands() == 2)

                  if ((Instruction::Store == j++->getOpcode()) &&
                      ((j->getOperand(0) == I->getOperand(1)) &&
                       (j++->getOperand(1) == I->getOperand(0)))) {
                    errs() << " CAVAICI"
                           << "\n";

                    j->replaceAllUsesWith((Instruction *)&*I);
                    switch (I->getOpcode()) {
                    case Instruction::Add: {
                      j++->replaceAllUsesWith(BinaryOperator::Create(
                          BinaryOperator::Add, (Instruction *)&*I,
                          j->getOperand(1), "new", (Instruction *)&*j));
                      break;
                    }
                    case Instruction::Sub: {
                      j++->replaceAllUsesWith(BinaryOperator::Create(
                          BinaryOperator::Sub, (Instruction *)&*I,
                          j->getOperand(1), "new", (Instruction *)&*j));
                      break;
                    }
                    }
                  }
                // j++->replaceAllUsesWith(BinaryOperator::Create(BinaryOperator::CastOps(j++->getType()),
                // (Instruction *)&I, j->getOperand(1), "new", &F)); Delete
                // after
                to_delete.push_back((Instruction *)&*j);
                to_delete.push_back((Instruction *)&*j++);
              }
            }
            /*
            // CASE A+B = tmp A+C+B = tmp+C
            if ((I->getOpcode() == Instruction::Sub) || (I->getOpcode() ==
            Instruction::Add))
            {
                if (DT.dominates((Instruction *)&*I, (Instruction *)&*j) &&
            (I->getOpcode() == j->getOpcode()) && (I->getType() == j->getType())
            && (I->getNumOperands() == j->getNumOperands()))
                {

                    unsigned op = I->getNumOperands();
                    // Now compare operands
                    unsigned sim1 = 0, sim2 = 0;
                    for (unsigned c = 0; c < op; c++)
                    {
                        if (I->getOperand(c) == j->getOperand(c))
                            sim1++;

                        if (I->getOperand(c) != j->getOperand(op - (c + 1)))
                            sim2++;
                    }
                    if (sim1 || sim2)
                    {
                        errs() << "TROUVER I = " << I->getNumOperands() << " "
            << *I << "\n"; errs() << "TROUVER J = " << j->getOpcode() << " " <<
            *j << "\n";

                        j->replaceAllUsesWith((Instruction *)&*I);
                        to_delete.push_back((Instruction *)&*j);
                        // j->removeFromParent();
                    }
                }
            }*/
          }
        }
      }

      // Delete dupplicated instructions, it can cause a segfault if you try
      // delete 1+ times the same instruction
      to_delete.sort();
      to_delete.unique();

      // Now erase replaced instructions, you can't do this in the basic block
      // because it can segfault
      for (auto I : to_delete)
        I->eraseFromParent();

      for (auto basicb = F.begin(); basicb != F.end(); basicb++) {

        for (auto I = basicb->begin(); I != basicb->end(); I++) {
          errs() << "APRES opcode   " << I->getOpcode() << " " << *I << "\n";
        }
      }

      return PreservedAnalyses::none();
    }
  }
};

llvm::PassPluginLibraryInfo getCSEPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "CSEPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "print-cse-pass") {
                    FPM.addPass(CSEPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getCSEPassPluginInfo();
}
