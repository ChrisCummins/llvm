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
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "hello"

STATISTIC(BlockCount, "Number of basic blocks");
STATISTIC(InstCount, "Number of instructions");
STATISTIC(StoreInstCount, "Number of store instructions");
STATISTIC(LoadInstCount, "Number of load instructions");

namespace {
  // Hello - The first implementation, without getAnalysisUsage.
  struct Hello : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Hello() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      for (Function::iterator bb = F.begin(), bbe = F.end(); bb != bbe; ++bb) {
        BasicBlock &block = *bb;
        BlockCount++;

        for (BasicBlock::iterator instruction = block.begin();
             instruction != block.end(); instruction++) {
          InstCount++;

          if (dyn_cast<StoreInst>(instruction))
            StoreInstCount++;
          else if (dyn_cast<LoadInst>(instruction))
            LoadInstCount++;
        }
      }

      return true;
    }
  };
}

char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass");
