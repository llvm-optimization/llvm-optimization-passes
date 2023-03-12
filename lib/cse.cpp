#include "../include/csepass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
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
// --passes='print-cse-pass' ./test_Fiona_opt.ll -o testfin.ll
// clang -o test testfin.ll

using namespace llvm;

// Search in the list if the instruction already exists
bool find_inst(std::list<Instruction *> list, Instruction const *B) {
  for (Instruction *A : list) {
    if (A->getValueID() == B->getValueID())
      return true;
  }
  return false;
}

// Create binary operator and replace j++ with the position
void SwitchCase(int position, Instruction *I, Instruction *j) {
  switch (I->getOpcode()) {
  case Instruction::Add: {
    j->replaceAllUsesWith(&*I);

    j->getNextNode()->replaceAllUsesWith(BinaryOperator::Create(
        BinaryOperator::Add, (Instruction *)&*I, j->getOperand(position), "new",
        (Instruction *)&*j));
    break;
  }
  case Instruction::Sub: {
    j->replaceAllUsesWith(&*I);
    j->getNextNode()->replaceAllUsesWith(BinaryOperator::Create(
        BinaryOperator::Sub, (Instruction *)&*I, j->getOperand(position), "new",
        (Instruction *)&*j));
    break;
  }

  case Instruction::FAdd: {
    j->replaceAllUsesWith(&*I);
    j->getNextNode()->replaceAllUsesWith(BinaryOperator::Create(
        BinaryOperator::FAdd, (Instruction *)&*I, j->getOperand(position),
        "new", (Instruction *)&*j));
    break;
  }

  case Instruction::FSub: {
    j->replaceAllUsesWith(&*I);
    j->getNextNode()->replaceAllUsesWith(BinaryOperator::Create(
        BinaryOperator::FSub, (Instruction *)&*I, j->getOperand(position),
        "new", (Instruction *)&*j));
    break;
  }

  case Instruction::Mul: {
    j->replaceAllUsesWith(&*I);
    j->getNextNode()->replaceAllUsesWith(BinaryOperator::Create(
        BinaryOperator::Mul, (Instruction *)&*I, j->getOperand(position), "new",
        (Instruction *)&*j));
    break;
  }
  case Instruction::FMul: {
    j->replaceAllUsesWith(&*I);

    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::FMul, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::UDiv: {
    j->replaceAllUsesWith(&*I);

    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::UDiv, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::SDiv: {
    j->replaceAllUsesWith(&*I);

    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::SDiv, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::FDiv: {
    j->replaceAllUsesWith(&*I);

    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::FDiv, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::URem: {
    j->replaceAllUsesWith(&*I);

    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::URem, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::SRem: {
    j->replaceAllUsesWith(&*I);

    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::SRem, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::FRem: {
    j->replaceAllUsesWith(&*I);

    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::FRem, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::Shl: {
    j->replaceAllUsesWith(&*I);
    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::Shl, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::LShr: {
    j->replaceAllUsesWith(&*I);
    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::LShr, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::AShr: {
    j->replaceAllUsesWith(&*I);
    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::AShr, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::And: {
    j->replaceAllUsesWith(&*I);
    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::And, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::Or: {
    j->replaceAllUsesWith(&*I);
    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::Or, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  case Instruction::Xor: {
    j->replaceAllUsesWith(&*I);
    (j->getNextNode())
        ->replaceAllUsesWith(BinaryOperator::Create(
            BinaryOperator::Xor, (Instruction *)&*I, j->getOperand(position),
            "new", (Instruction *)&*j));
    break;
  }
  }
}

PreservedAnalyses CSEPass::run(Function &F, FunctionAnalysisManager &) {

  llvm::DominatorTree DT = llvm::DominatorTree();
  DT.recalculate(F);

  std::list<Instruction *> to_delete;

  for (auto basicb = F.begin(); basicb != F.end(); basicb++) {

    for (auto I = basicb->begin(); I != basicb->end(); I++) {

      // Only if binary operator
      if ((I->getOpcode() >= Instruction::Add) &&
          (I->getOpcode() <= Instruction::Xor)) {

        // Iterate over the instructions of the bb
        for (auto j = basicb->begin(); j != basicb->end(); j++) {

          // Only if i is not j
          if (I != j) {

            // If I dominates J then it can replace it
            if (DT.dominates((Instruction *)&*I, (Instruction *)&*j))

              // If I and J same operand, num operands
              if ((I->getOpcode() == j->getOpcode()) &&
                  (I->getType() == j->getType()) &&
                  (I->getNumOperands() == j->getNumOperands())) {

                //  Now compare if already in list
                /*  if (find_inst(to_delete, (Instruction *)&*j))
                    continue;*/

                // Case A+B = A+B
                if ((I->getOperand(0) == j->getOperand(0)) &&
                    (I->getOperand(1) == j->getOperand(1))) {
                  to_delete.push_back((Instruction *)&*j);

                  j->replaceAllUsesWith((Instruction *)&*I);
                  continue;
                }

                // Case A+B = B+A but A*B =/= B*A and A/B =/= B/A (It needs to
                // be commutative)
                if ((I->getOperand(1) == j->getOperand(0)) &&
                    (I->getOperand(0) == j->getOperand(1)) &&
                    Instruction::isCommutative(I->getOpcode())) {
                  to_delete.push_back((Instruction *)&*j);

                  j->replaceAllUsesWith((Instruction *)&*I);
                  continue;
                }

                // If j is the final instruction of the basic block, don't
                // search for j++
                if (j == basicb->end())
                  continue;

                if ((j->getNextNode())->getNumOperands() == 2)

                  // I = 3 + 2 J =  3+ 4 / J = 2 + 4, J++ = J + 2 / J++ = J+3

                  if (((j->getOperand(0) == I->getOperand(1)) &&
                       ((j->getNextNode())->getOperand(1) ==
                        I->getOperand(0))) ||
                      ((j->getOperand(0) == I->getOperand(0)) &&
                       ((j->getNextNode())->getOperand(1) ==
                        I->getOperand(1)))) {

                    // Delete J & J++ and replace J++ to I+ ...
                    to_delete.push_back((Instruction *)&*j->getNextNode());
                    Instruction *tmp1 = (Instruction *)(&*j);
                    SwitchCase(1, &*I, &*j);
                    j++;

                    // Delete J
                    tmp1->eraseFromParent();
                  }

                // I = 3 + 2 J = 4 + 3 / J = 4 + 2, J++ = J + 2 / J++ = J+3
                if (((j->getOperand(1) == I->getOperand(0)) &&
                     ((j->getNextNode())->getOperand(0) == I->getOperand(1))) ||
                    ((j->getOperand(1) == I->getOperand(1)) &&
                     ((j->getNextNode())->getOperand(0) == I->getOperand(0)))) {

                  to_delete.push_back((Instruction *)&*j->getNextNode());

                  Instruction *tmp1 = (Instruction *)(&*j);

                  SwitchCase(0, &*I, &*j);
                  j++;

                  // Delete J
                  tmp1->eraseFromParent();
                }
              }
          }

          // Same with unary operators
          if ((I->getOpcode() < Instruction::Add)) {
            if (DT.dominates((Instruction *)&*I, (Instruction *)&*j) &&
                (I->getOpcode() == j->getOpcode()) &&
                (I->getType() == j->getType()) &&
                (I->getNumOperands() == j->getNumOperands())) {

              if (I->getOperand(0) == j->getOperand(1)) {
                j->replaceAllUsesWith((Instruction *)&*I);
                to_delete.push_back((Instruction *)&*j);
              }
            }
          }
        }
      }
    }
  }

  // Delete dupplicated instructions, it can cause a segfault if you try
  // delete 1+ times the same instruction
  to_delete.sort();
  to_delete.unique();

  // Now erase replaced instructions, you can't do this in the basic block
  // because it can segfault
  for (auto I : to_delete) {

    // Check: The instruction that you will delete, dominate every use ?
    bool dominate = true;
    for (Use &U : I->uses()) {
      Instruction *User = cast<Instruction>(U.getUser());
      if (!DT.dominates(I, User)) {
        // I does not dominate User
        dominate = false;
      }
    }
    if (dominate)
      I->eraseFromParent();
  }

  // Uncomment if you want to print the output
  /* for (auto basicb = F.begin(); basicb != F.end(); basicb++) {

     for (auto I = basicb->begin(); I != basicb->end(); I++) {
       errs() << "APRES opcode   " << I->getOpcode() << " " << *I << "\n";
     }
   }*/

  return PreservedAnalyses::none();
}

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
