// opt -load-pass-plugin ./lib/libCountPass.so  -passes="print-count-pass" test.bc
// opt-15 -load-pass-plugin ./libHelloWorld.so  -enable-new-pm=1  --passes='print-count-pass' ./test.bc

// #include "../include/countpass.h"
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
#include "llvm/Transforms/Utils/SSAUpdater.h"

using namespace llvm;
struct SSAPass : public AnalysisInfoMixin<SSAPass>
{
public:
  static bool isRequired() { return true; }

  static llvm::AnalysisKey Key;
  friend struct llvm::AnalysisInfoMixin<SSAPass>;
  PreservedAnalyses run(Function &F,
                        FunctionAnalysisManager &)
  {

    /* SSAUpdater SSA;

      // Map from values to their definitions
      DenseMap<Value *, Instruction *> Defs;

      // For each basic block in the function
      for (BasicBlock &BB : F)
      {
        // For each instruction in the basic block
        for (Instruction &I : BB)
        {
          if (isa<AllocaInst>(&I))
          {
            printf("Oui\n");
            AllocaInst *alloca = dyn_cast<AllocaInst>(&I);
            SSA.Initialize(alloca->getType(), alloca->getName());
            SSA.AddAvailableValue(&BB, alloca);
          }
          // If the instruction is a definition
          if (isa<Instruction>(&I))
          {
            printf("Non\n");
            // Get the operand
            Value *Op = I.getOperand(0);

            // Check if the operand has a definition
            auto It = Defs.find(Op);
            if (It != Defs.end())
            {
              // If the operand has a definition, add the definition to the SSAUpdater
              SSA.AddAvailableValue(&BB, It->first);
              // Replace all uses of the operand with the SSA value
              if (I.getNumOperands() > 0)
                for (Use &Op2 : I.operands())
                {
                  SSA.RewriteUse(Op2);
                }
              // SSA.RewriteUse(Op);
              // Remove the operand from the map of definitions
              Defs.erase(Op);
            }
            // Add the instruction to the map of definitions
            Defs[&I] = &I;
          }
        }
      }*/
    /* // Create a map to store the mapping from values to their versions
     DenseMap<Value *, SmallVector<Value *>> valueMap;
     // Create a vector to store all the new instructions
     SmallVector<Instruction *, 8> newInstructions;

     for (auto &bb : F)
     {
       for (auto &inst : bb)
       {
         for (unsigned i = 0; i < inst.getNumOperands(); i++)
         {
           auto op = inst.getOperand(i);
           inst.operands();
           // Check if the operand is defined in the same basic block
           if (auto defInst = dyn_cast<Instruction>(op))
           {
             if (defInst->getParent() == &bb)
             {
               // If so, create a new version of the operand
               auto &versions = valueMap[op];
               auto newValue = defInst->clone();
               newValue->setName(op->getName() + ".v" + std::to_string(versions.size()));
               bb.getInstList().insert(inst.getIterator(), newValue);
               // bb.getInstList().deleteNode(&inst);
               versions.push_back(newValue);
               inst.setOperand(i, (Value *)newValue);
               defInst->eraseFromParent();
             }
           }
         }
       }
     }*/
    // Create an SSA updater
    SSAUpdater SSA;
    // Map from values to their definitions
    DenseMap<Value *, Instruction *> Defs;

    /*   // For each basic block in the function
       for (BasicBlock &BB : F)
       {
         // For each instruction in the basic block
         for (Instruction &I : BB)
         {
           // If the instruction is an alloca
           if (auto *alloca = dyn_cast<AllocaInst>(&I))
           {
             SSA.Initialize(alloca->getType(), alloca->getName());
             SSA.AddAvailableValue(&BB, alloca);
           }
           // If the instruction is a definition
           if (isa<Instruction>(&I))
           {
             if (I.getNumOperands() > 0)
             {
               // Get the operand
               Value *Op = I.getOperand(0);
               // Check if the operand has a definition
               auto It = Defs.find(Op);
               if (It != Defs.end())
               {
                 // If the operand has a definition, add the definition to the SSAUpdater
                 SSA.AddAvailableValue(&BB, It->second);
                 // Replace all uses of the operand with the SSA value
                 SSA.RewriteUse(I.getOperandUse(0));
                 // Remove the operand from the map of definitions
                 Defs.erase(Op);
               }
               // Add the instruction to the map of definitions
               Defs[&I] = &I;
             }
           }
         }
       }*/

    std::map<Value *, Value *> latestVersion;
    std::list<Instruction *> to_delete;

    for (Function::iterator bb = F.begin(), e = F.end(); bb != e; ++bb)
    {
      for (BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i)
      {
        Instruction *inst = &*i;
        // Only consider instructions that define a value
        if (!inst->getType()->isVoidTy())
        {
          Value *oldValue = inst;
          Value *newValue = latestVersion[oldValue];
          // If this is the first occurrence of the value, create a new version
          if (!newValue)
          {
            newValue = inst->clone();
            Instruction *newInst = dyn_cast<Instruction>(newValue);
            inst->getParent()->getInstList().insert(inst->getIterator(), newInst);
            // inst->deleteValue();
          }
          // Replace all uses of the old value with the new value
          oldValue->replaceAllUsesWith(newValue);
          latestVersion[oldValue] = newValue;
          to_delete.push_back(inst);
        }
      }
    }
    to_delete.sort();
    to_delete.unique();

    // Now erase replaced instructions, you can't do this in the basic block because it can segfault
    for (auto I : to_delete)
      I->eraseFromParent();
    /*  std::map<Instruction *, Value *> valueMap;

      for (auto &bb : F)
      {
        for (auto &inst : bb)
        {
         // if (!isa<PHINode>(inst))
           // continue;

          if(isa<PHINode>(inst))
          {
          auto phi = dyn_cast<PHINode>(&inst);
          // Get the PHI node's type and create a new alloca instruction
          Type *phiTy = phi->getType();
          IRBuilder<> builder(&bb);
          auto alloca = builder.CreateAlloca(phiTy);

          // Replace all uses of the PHI node with the new alloca
          phi->replaceAllUsesWith(alloca);

          // Create a new store instruction to store the incoming value
          for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i)
          {
            IRBuilder<> builder(phi->getIncomingBlock(i)->getTerminator());
            builder.CreateStore(phi->getIncomingValue(i), alloca);
          }

          // Remove the PHI node
          phi->eraseFromParent();
          }
          else if (inst.getType()->isVoidTy())
          {
            continue;
          }
          else
          {
            // If the instruction is not a PHI node and is not void, it must be a normal
            // instruction. Create a new alloca instruction for its result
            Type *instTy = inst.getType();
            IRBuilder<> builder(&bb);
            auto alloca = builder.CreateAlloca(instTy);

            // Replace all uses of the instruction's result with the new alloca
            inst.replaceUsesOfWith(valueMap[&inst] = alloca, alloca);
            inst.eraseFromParent();
          }
        }
      }

      // Iterate over the function's basic blocks again to add stores for each value
      for (auto &bb : F)
      {
        for (auto &inst : bb)
        {
          if (valueMap.count(&inst))
          {
            IRBuilder<> builder(&inst);
            builder.CreateStore(inst.clone(), valueMap[&inst]);
          }
        }
      }*/
    return PreservedAnalyses::all();
  }
};

llvm::PassPluginLibraryInfo getSSAPassPluginInfo()
{
  return {LLVM_PLUGIN_API_VERSION, "SSAPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB)
          {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>)
                {
                  if (Name == "print-ssa-pass")
                  {
                    FPM.addPass(SSAPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
  return getSSAPassPluginInfo();
}