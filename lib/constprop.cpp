#include "../include/constprop.h"
#include <vector>
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

// USAGE: New PM
     // opt -load-pass-plugin=build/libConstantPropagation.so -passes="const-prop" -disable-output main.ll


  // Check if constants have the right size
  bool is_constant(Value *op, int &constant)
  {
     Value *op0 = op;
     if (Instruction *CI = dyn_cast<Instruction>(op))
        op0 = CI->getOperand(0);

     if (ConstantInt *CI = dyn_cast<ConstantInt>(op0)) {
        if (CI->getBitWidth() <= 32) {
           constant = CI->getSExtValue();
           return true;
        }
     }
     return false;
  }

  //
  bool constProp(llvm::BasicBlock *b){

    std::vector<Value*> ptrs;
    std::vector<Value*> vals;

    Value* val_op = nullptr;
    Value* ptr_op = nullptr;

    for(auto &inst : *b){

      //Store instruction
      if(isa<llvm::StoreInst>(&inst)){
        llvm::StoreInst *store = (llvm::StoreInst*)&inst;
        val_op = store->getValueOperand();
        ptr_op = store->getPointerOperand();

        int c = 0;
        if(is_constant(val_op,c)){  //Verify val_op is a constant
          bool in = false;
          int i = 0;

          for(auto &p : ptrs){  //Verify that pointer is already stored
            if(ptr_op == p){
              vals[i] = val_op;
              in = true;
              break;
            }
            ++i;
          }
          if(in == false){
            ptrs.push_back(ptr_op);
            vals.push_back(val_op);
          }
        }
        
        else{ //val_op is an expression
          int i = 0;
          for(auto &p : ptrs){  //Verify that pointer is already stored
            if(ptr_op == p){
              ptrs.erase(ptrs.begin() + i);
              vals.erase(vals.begin() + i);
              break;
            }
            ++i;
          }
        }
      }

      //Load
      if(isa<llvm::LoadInst>(&inst)){
        llvm::LoadInst *load = (llvm::LoadInst*)&inst;
        int i=0;
        for(auto &p : ptrs){
          if(load->getPointerOperand() == p){
            (&inst)->replaceAllUsesWith(vals[i]);  
            break;
          }
          i++;
        }
      }

      /*// Constant propagate to all users of the call
      while (!WorkList.empty()) {
        Instruction *i = *WorkList.begin();
        WorkList.erase(WorkList.begin());
        if (Constant *c = ConstantFoldInstruction(i, DL)) {
          for (auto use : i->users())
            WorkList.insert(cast<Instruction>(use));
          i->replaceAllUsesWith(c);
          i->eraseFromParent();
        }
      }*/
    }
    return true;
  }

  //Run function for new pass manager
  llvm::PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    //
    for(auto& basicb : F){
      constProp(&basicb);
    }

    return llvm::PreservedAnalyses::all();
  }

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getConstantPropagationPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ConstantPropagation", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "const-prop") {
                    FPM.addPass(ConstantPropagation());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getConstantPropagationPluginInfo();
}