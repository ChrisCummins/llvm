//===- MemoryCounter.cpp - Memory instrumentation -------------------------===//

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

// Global variable names
#define GLOBAL_STORE     "$store$"
#define GLOBAL_LOAD      "$load$"
#define GLOBAL_STORE_STR  GLOBAL_STORE"str"
#define GLOBAL_LOAD_STR   GLOBAL_LOAD"str"

// Debug statistics
#define DEBUG_TYPE ""
STATISTIC(BlocksCount,    "no of basic blocks");
STATISTIC(InstCount,      "no of instructions");
STATISTIC(StoreInstCount, "no of STORE instructions instrumented");
STATISTIC(LoadInstCount,  "no of LOAD instructions instrumented");
#undef  DEBUG_TYPE

namespace {

  // Returns whether the current module has a main execution entry
  // point or not.
  bool hasMainMethod(Module &M) {
    return M.getFunction("main");
  }

  // Type conversion wrappers
  ConstantInt *int2const(Module &M, const int value, const size_t nbits) {
    return ConstantInt::get(M.getContext(), APInt(nbits, value, false));
  }

  ConstantInt *int2const32(Module &M, const int value) {
    return int2const(M, value, 32);
  }

  ConstantInt *int2const64(Module &M, const int value) {
    return int2const(M, value, 64);
  }

  // Create a new 64 bit global variable with name "name" and initial
  // value "value".
  GlobalVariable *createGVarInt64(Module &M, const char *name,
                                  const int value) {
    GlobalVariable *const var
        = new GlobalVariable(/*Module=*/M,
                             /*Type=*/IntegerType::get(M.getContext(), 64),
                             /*isConstant=*/false,
                             /*Linkage=*/GlobalValue::ExternalLinkage,
                             /*Initializer=*/0, // has initializer, specified below
                             /*Name=*/name);

    // Only define the variable once. This prevents
    // multiple-definition errors when linking multiple files which
    // have been instrumented.
    if (hasMainMethod(M)) {
        var->setAlignment(8);
        var->setInitializer(int2const64(M, value));
    }
    return var;
  }

  // Append instruction "i" with a set of instructions that increments
  // a global 64 bit integer variable "gvar" by amount "value".
  void addGVarInt64andConst64(Module &M, Instruction *const i,
                              GlobalVariable *const gvar, const int value) {
    ConstantInt *const const_value = int2const64(M, value);
    LoadInst *const load = new LoadInst(gvar, "", false, i);
    BinaryOperator *const add = BinaryOperator::Create(Instruction::Add,
                                                       load, const_value,
                                                       "", i);
    StoreInst *const store = new StoreInst(add, gvar, false, i);

    load->setAlignment(8);
    store->setAlignment(8);
  }

  // Prepend "I" with instructions to update a global counter such that:
  //     global_counter += counter;
  //     counter = 0
  void updateGlobalCounter(Module &M, Instruction *const I,
                           GlobalVariable *const global_counter, int *const counter) {
      if (*counter) {
          addGVarInt64andConst64(M, I, global_counter, *counter);
          *counter = 0;
      }
  }

  // Prepend "I" with instructions to update both global counters.
  void updateGlobalCounters(Module &M, Instruction *const I,
                            GlobalVariable *const store, int *const store_count,
                            GlobalVariable *const load, int *const load_count) {
      updateGlobalCounter(M, I, store, store_count);
      updateGlobalCounter(M, I, load, load_count);
  }

  Instruction *LLVMGetPreviousInstruction(Instruction *Instr) {
    BasicBlock::iterator I = *Instr;
    if (I == Instr->getParent()->begin())
      return nullptr;
    return --I;
  }

  // Instrument a function so that it counts how many load and store
  // instructions there are in it, and adds those values to the global
  // load and store counters.
  void instrumentFunction(Module &M, Function &F,
                          GlobalVariable *const store,
                          GlobalVariable *const load) {
    for (Function::iterator block = F.begin(); block != F.end(); block++) {
      int store_count = 0, load_count = 0;
      BlocksCount++;

      // Iterate over basic blocks
      for (BasicBlock::iterator instruction = block->begin();
           instruction != block->end(); instruction++) {
        InstCount++;

        // Increment the instruction type counters
        if (isa<StoreInst>(instruction)) {
          StoreInstCount++;
          store_count++;
        } else if (isa<LoadInst>(instruction)) {
          LoadInstCount++;
          load_count++;
        }
      }

      // Add store and load instruction counters to global counters
      Instruction *terminator = block->getTerminator();

      // If the terminator is unreachable, then instrument the
      // preceding instruction.
      if (isa<UnreachableInst>(terminator))
        terminator = LLVMGetPreviousInstruction(terminator);

      updateGlobalCounters(M, terminator,
                           store, &store_count, load, &load_count);
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
        = FunctionType::get(/*Result=*/IntegerType::get(M.getContext(), 64),
                            /*Params=*/FuncTy_6_args,
                            /*isVarArg=*/true);
    Function *func_printf = M.getFunction("printf");
    if (!func_printf) {
      func_printf = Function::Create(/*Type=*/FuncTy_6,
                                     /*Linkage=*/GlobalValue::ExternalLinkage,
                                     /*Name=*/"printf", &M); // (external, no body)
      func_printf->setCallingConv(CallingConv::C);
    }
    AttributeSet func_printf_PAL, PAS;
    SmallVector<AttributeSet, 4> Attrs;
    AttrBuilder B;
    PAS = AttributeSet::get(M.getContext(), ~0U, B);

    Attrs.push_back(PAS);
    func_printf_PAL = AttributeSet::get(M.getContext(), Attrs);
    func_printf->setAttributes(func_printf_PAL);

    return func_printf;
  }

  // Append instruction "i" with printf call with params "params".
  template<typename T>
  void callPrintf(Module &M, T params, Instruction *const i) {
    AttributeSet printf_PAL;
    Function *const func_printf = getPrintfFunction(M);
    CallInst *const printf = CallInst::Create(func_printf, params, "", i);

    printf->setCallingConv(CallingConv::C);
    printf->setTailCall(false);
    printf->setAttributes(printf_PAL);
  }

  // Print the results line.
  // GENERATED BY LLVM STATIC COMPILER
  void printResultsLine(Module &M, Instruction *const i) {
    ArrayType *const arrayTy = ArrayType::get(IntegerType::get(M.getContext(), 8), 39);
    GlobalVariable *const str
        = new GlobalVariable(/*Module=*/M,
                             /*Type=*/arrayTy,
                             /*isConstant=*/true,
                             /*Linkage=*/GlobalValue::PrivateLinkage,
                             /*Initializer=*/0, // has initializer, specified below
                             /*Name=*/".$RESULTSLINE$");
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

    callPrintf(M, const_ptr_12, i);
  }

  // Prepend "i" with instructions to print the 64 bit unsigned
  // integer variable "gvar".
  void printGVar64(Module &M, Instruction *const i,
                   GlobalVariable *const gvar, const char *name,
                   const char *fmt, size_t fmt_len) {
    // Initialise string.
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
    std::vector<Constant *> indices;
    indices.push_back(const0);
    indices.push_back(const0);
    Constant *const fmt_str = ConstantExpr::getGetElementPtr(str, indices);
    str->setInitializer(const_array);

    // Initialise variable.
    LoadInst *const gvar64 = new LoadInst(gvar, "", false, i);
    gvar64->setAlignment(8);

    // Call printf.
    std::vector<Value *> params;
    params.push_back(fmt_str);
    params.push_back(gvar64);
    callPrintf(M, params, i);
  }

  // Add printf calls to display load and store counters.
  void instrumentMain(Module &M,
                      GlobalVariable *const store,
                      GlobalVariable *const load) {
    Function *const main = M.getFunction("main");

    // Iterate over basic blocks.
    for (Function::iterator bb = main->begin(); bb != main->end(); bb++) {
        TerminatorInst *const terminator = bb->getTerminator();

        // If we have a return (i.e. program exit) instruction, then
        // add in the debug printouts.
        if (dyn_cast<ReturnInst>(terminator)) {
            printResultsLine(M, terminator);
            printGVar64(M, terminator, store, GLOBAL_STORE_STR,
                        "Number of STORE instructions executed: %d\x0A", 43);
            printGVar64(M, terminator, load, GLOBAL_LOAD_STR,
                        "Number of LOAD instructions executed:  %d\x0A", 43);
        }
    }
  }

  // MemoryCounter.
  struct MemoryCounter : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    MemoryCounter() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
      Module::FunctionListType &funcs = M.getFunctionList();

      // Get global load/store counter variables
      GlobalVariable *const store = createGVarInt64(M, GLOBAL_STORE, 0);
      GlobalVariable *const load = createGVarInt64(M, GLOBAL_LOAD, 0);

      // Instrument functions
      for (Module::FunctionListType::iterator function = funcs.begin();
           function != funcs.end(); function++) {
        instrumentFunction(M, *function, store, load);
      }

      // Print instrumentation results
      if (hasMainMethod(M))
        instrumentMain(M, store, load);

      return true;
    }
  };
}  // namespace

char MemoryCounter::ID = 0;
static RegisterPass<MemoryCounter> X("memorycounter",
                                     "Memory Instrumentation Pass");
