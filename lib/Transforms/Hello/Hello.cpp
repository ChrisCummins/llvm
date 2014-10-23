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

#include <vector>

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

#define DEBUG_TYPE "instructions"

#define GLOBAL_STORE "$store$"
#define GLOBAL_LOAD  "$load$"

STATISTIC(StoreInstCount, "Number of instrumented STOREs");
STATISTIC(LoadInstCount,  "Number of instrumented LOADs");

namespace {

  // Convert an int to 32 bit unsigned ConstantInt.
  ConstantInt *int2const32(Module &M, const int value) {
    return ConstantInt::get(M.getContext(), APInt(32, value, false));
  }

  // Create a new 32 bit global variable with name "name" and initial
  // value "value".
  GlobalVariable *createGVarInt32(Module &M, const char *name,
                                  const int value) {
    GlobalVariable *const var
        = new GlobalVariable(/*Module=*/M,
                             /*Type=*/IntegerType::get(M.getContext(), 32),
                             /*isConstant=*/false,
                             /*Linkage=*/GlobalValue::ExternalLinkage,
                             /*Initializer=*/0, // has initializer, specified below
                             /*Name=*/name);
    var->setAlignment(4);
    var->setInitializer(int2const32(M, value));
    return var;
  }

  // Append instruction "i" with a set of instructions that increments
  // a global 32 bit integer variable "gvar" by amount "value".
  void addGVarInt32andConst32(Module &M, Instruction *const i,
                              GlobalVariable *const gvar, const int value) {
    ConstantInt *const const_value = int2const32(M, value);
    LoadInst *const load = new LoadInst(gvar, "", false, i);
    BinaryOperator *const add = BinaryOperator::Create(Instruction::Add,
                                                       load, const_value,
                                                       "", i);
    StoreInst *const store = new StoreInst(add, gvar, false, i);

    load->setAlignment(4);
    store->setAlignment(4);
  }

  // Instrument a function so that it counts how many load and store
  // instructions there are in it, and adds those values to the global
  // load and store counters.
  void instrumentFunction(Module &M, Function &F,
                          GlobalVariable *const store,
                          GlobalVariable *const load) {
    for (Function::iterator block = F.begin(); block != F.end(); block++) {
      int store_count = 0, load_count = 0;

      // Iterate over basic blocks
      for (BasicBlock::iterator instruction = block->begin();
           instruction != block->end(); instruction++) {
        // Check the instruction type
        if (dyn_cast<StoreInst>(instruction)) {
          StoreInstCount++;
          store_count++;
        } else if (dyn_cast<LoadInst>(instruction)) {
          LoadInstCount++;
          load_count++;
        }
      }

      // Add store and load instruction counters to global counters
      TerminatorInst *const terminator = block->getTerminator();
      if (store_count)
        addGVarInt32andConst32(M, terminator, store, store_count);
      if (load_count)
        addGVarInt32andConst32(M, terminator, load, load_count);
    }
  }

  // Get (or create) a printf function.
  // GENERATED BY LLVM STATIC COMPILER
  Function *getPrintfFunction(Module &M) {
    PointerType *const PointerTy_4
        = PointerType::get(IntegerType::get(M.getContext(), 8), 0);
    std::vector<Type *>FuncTy_6_args;
    FuncTy_6_args.push_back(PointerTy_4);
    FunctionType *const FuncTy_6
        = FunctionType::get(/*Result=*/IntegerType::get(M.getContext(), 32),
                            /*Params=*/FuncTy_6_args,
                            /*isVarArg=*/true);

    Function *func_printf = M.getFunction("printf");
    if (!func_printf) {
      func_printf = Function::Create(
          /*Type=*/FuncTy_6,
          /*Linkage=*/GlobalValue::ExternalLinkage,
          /*Name=*/"printf", &M); // (external, no body)
      func_printf->setCallingConv(CallingConv::C);
    }
    AttributeSet func_printf_PAL;
    {
      SmallVector<AttributeSet, 4> Attrs;
      AttributeSet PAS;
      {
        AttrBuilder B;
        PAS = AttributeSet::get(M.getContext(), ~0U, B);
      }

      Attrs.push_back(PAS);
      func_printf_PAL = AttributeSet::get(M.getContext(), Attrs);
    }
    func_printf->setAttributes(func_printf_PAL);

    return func_printf;
  }

  // Print the results line.
  // GENERATED BY LLVM STATIC COMPILER
  void printResultsLine(Module &M, Instruction *const i) {
    Function *const func_printf = getPrintfFunction(M);
    ArrayType *const arrayTy = ArrayType::get(IntegerType::get(M.getContext(), 8), 39);
    GlobalVariable *const str
        = new GlobalVariable(/*Module=*/M,
                             /*Type=*/arrayTy,
                             /*isConstant=*/true,
                             /*Linkage=*/GlobalValue::PrivateLinkage,
                             /*Initializer=*/0, // has initializer, specified below
                             /*Name=*/"$RESULTSLINE$");
    str->setAlignment(1);
    Constant *const const_array_9
        = ConstantDataArray::getString(M.getContext(),
                                       "---------- INSTRUMENTATION ----------\x0A",
                                       true);
    ConstantInt *const const_int32_11 = int2const32(M, 0);
    std::vector<Constant *> const_ptr_12_indices;
    const_ptr_12_indices.push_back(const_int32_11);
    const_ptr_12_indices.push_back(const_int32_11);
    Constant *const const_ptr_12
        = ConstantExpr::getGetElementPtr(str, const_ptr_12_indices);
    str->setInitializer(const_array_9);

    // Create instructions
    CallInst *const printf = CallInst::Create(func_printf, const_ptr_12, "", i);
    printf->setCallingConv(CallingConv::C);
    printf->setTailCall(false);
    AttributeSet printf_PAL;
    printf->setAttributes(printf_PAL);
  }

  // Print a global 32 bit integer variable.
  void printGVar32(Module &M, Instruction *const i,
                   GlobalVariable *const gvar, const char *name,
                   const char *fmt, size_t fmt_len) {
    Function *const func_printf = getPrintfFunction(M);
    ArrayType *const ArrayTy_1 = ArrayType::get(IntegerType::get(M.getContext(),
                                                                 8), fmt_len);
    GlobalVariable *const str = new GlobalVariable(/*Module=*/M,
                                                   /*Type=*/ArrayTy_1,
                                                   /*isConstant=*/true,
                                                   /*Linkage=*/GlobalValue::PrivateLinkage,
                                                   /*Initializer=*/0, // has initializer, specified below
                                                   /*Name=*/name);
    str->setAlignment(1);
    ConstantInt *const const0 = int2const32(M, 0);
    Constant *const const_array = ConstantDataArray::getString(M.getContext(),
                                                               fmt, true);
    std::vector<Constant *> const_ptr_indices;
    const_ptr_indices.push_back(const0);
    const_ptr_indices.push_back(const0);
    Constant *const const_ptr = ConstantExpr::getGetElementPtr(str,
                                                               const_ptr_indices);
    str->setInitializer(const_array);

    // Add instructions:
    LoadInst *const load = new LoadInst(gvar, "", false, i);
    load->setAlignment(4);
    std::vector<Value *> printf_params;
    printf_params.push_back(const_ptr);
    printf_params.push_back(load);
    CallInst *const printf = CallInst::Create(func_printf, printf_params,
                                              "", i);
    printf->setCallingConv(CallingConv::C);
    printf->setTailCall(false);
    AttributeSet printf_PAL;
    printf->setAttributes(printf_PAL);
  }

  // Add printf calls to display load and store counters.
  void instrumentMain(Module &M,
                      GlobalVariable *const store,
                      GlobalVariable *const load) {
    Function *const main = M.getFunction("main");

    if (main) {
      for (Function::iterator bb = main->begin(); bb != main->end(); bb++) {
        TerminatorInst *const terminator = bb->getTerminator();

        // If we have a return (i.e. program exit) instruction, then
        // add in the debug printouts.
        if (dyn_cast<ReturnInst>(terminator)) {
          printResultsLine(M, terminator);
          printGVar32(M, terminator, store, GLOBAL_STORE"str",
                      "Number of STORE instructions executed: %d\x0A", 43);
          printGVar32(M, terminator, load, GLOBAL_LOAD"str",
                      "Number of LOAD instructions executed:  %d\x0A", 43);
        }
      }

    } else {
      errs() << "No main method!\n";
      return;
    }
  }

  // Hello - The first implementation, without getAnalysisUsage.
  struct Hello : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    Hello() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
      // Add global load/store counters
      GlobalVariable *const store = createGVarInt32(M, GLOBAL_STORE, 0);
      GlobalVariable *const load = createGVarInt32(M, GLOBAL_LOAD, 0);

      // Instrument functions
      for (Module::FunctionListType::iterator function = M.getFunctionList().begin();
           function != M.getFunctionList().end(); function++) {
        instrumentFunction(M, *function, store, load);
      }

      // Print instrumentation results
      instrumentMain(M, store, load);

      return true;
    }
  };
}  // namespace

char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass");
