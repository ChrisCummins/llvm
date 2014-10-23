//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "hello"

#define GLOBAL_STORE "$store$"
#define GLOBAL_LOAD "$load$"

STATISTIC(BlockCount, "Number of basic blocks");
STATISTIC(InstCount, "Number of instructions");
STATISTIC(StoreInstCount, "Number of store instructions instrumented");
STATISTIC(LoadInstCount, "Number of load instructions instrumented");

namespace {

  // Create a new 32 bit global variable with name "name" and initial
  // value "value".
  GlobalVariable *createGVarInt32(Module &M, const char *name, int value) {
    ConstantInt* init_val = ConstantInt::get(M.getContext(),
                                             APInt(32, value, false));
    GlobalVariable* var = new GlobalVariable(/*Module=*/M,
                                             /*Type=*/IntegerType::get(M.getContext(), 32),
                                             /*isConstant=*/false,
                                             /*Linkage=*/GlobalValue::ExternalLinkage,
                                             /*Initializer=*/0, // has initializer, specified below
                                             /*Name=*/name);
    var->setAlignment(4);
    var->setInitializer(init_val);
    return var;
  }

  // Append instruction "i" with a set of instructions that increments
  // a global 32 bit integer variable "gvar" by amount "value".
  void incrementGVarInt32ByConst32(Module &M, Instruction *i,
                                   GlobalVariable *gvar, int value) {
    ConstantInt* const_value = ConstantInt::get(M.getContext(),
                                                APInt(32, value, false));
    LoadInst* load_gvar = new LoadInst(gvar, "", false, i);
    load_gvar->setAlignment(4);
    BinaryOperator* inc_gvar = BinaryOperator::Create(Instruction::Add,
                                                      load_gvar, const_value,
                                                      "", i);
    StoreInst* store_gvar = new StoreInst(inc_gvar, gvar, false, i);
    store_gvar->setAlignment(4);
  }

  // Instrument a function so that it counts how many load and store
  // instructions there are in it, and adds those values to the global
  // load and store counters.
  void instrumentFunction(Module &M, Function &F, GlobalVariable *store,
                          GlobalVariable *load) {
    for (Function::iterator bb = F.begin(), bbe = F.end(); bb != bbe; ++bb) {
      BasicBlock &block = *bb;
      int store_count = 0, load_count = 0;

      // Increment basic block counter
      BlockCount++;

      for (BasicBlock::iterator instruction = block.begin();
           instruction != block.end(); instruction++) {
        // Increment instruction counter
        InstCount++;

        if (dyn_cast<StoreInst>(instruction)) {
          // Increment store instruction counters
          StoreInstCount++;
          store_count++;
        } else if (dyn_cast<LoadInst>(instruction)) {
          // Increment load instruction counters
          LoadInstCount++;
          load_count++;
        }
      }

      // Add store and load instruction counters to global counters
      TerminatorInst *terminator = block.getTerminator();
      if (store_count)
        incrementGVarInt32ByConst32(M, terminator, store, store_count);
      if (load_count)
        incrementGVarInt32ByConst32(M, terminator, load, load_count);
    }
  }

  // Hello - The first implementation, without getAnalysisUsage.
  struct Hello : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    Hello() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {

      // Add global load/store counters
      GlobalVariable *store = createGVarInt32(M, GLOBAL_STORE, 0);
      GlobalVariable *load = createGVarInt32(M, GLOBAL_LOAD, 0);

      for(Module::FunctionListType::iterator it = M.getFunctionList().begin();
          it != M.getFunctionList().end(); it++) {
        // Iterate over each function
        Function &function = *it;

        instrumentFunction(M, function, store, load);
      }

      return true;
    }

  };
}

char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass");
