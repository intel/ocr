#ifndef __OCRSTATSPASS_H__
#define __OCRSTATSPASS_H__

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <string>
#include <utility>

using namespace llvm;

namespace {

extern GlobalVariable* InstructionCount_global; /* Count instructions in an EDT */
extern GlobalVariable* InstrumentOn_global; /* Will be non zero if LD/ST should be reported */

class OCRStatsPass : public FunctionPass {
  private:
    DataLayout *curTarget;
    Module *curModule;

  public:
    static char ID;

    OCRStatsPass() : FunctionPass(ID), curTarget(NULL), curModule(NULL) { }
    ~OCRStatsPass();

    virtual bool runOnFunction(Function& F);
    virtual bool doInitialization(Module& M);

  protected:
    /**
     * @brief Returns true if the function should be
     * considered and annotated and false otherwise.
     *
     * This mostly excludes the profiling functions
     */
    bool considerFunction(const Function* func) const;

    /**
     * @brief Returns true if v is one of the globals
     * we use for tracking
     *
     * We will never track loads and stores to these
     * and can safely ignore anything that happens to them
     */
    bool isIgnoredGlobal(const Value* v) const;

    /**
     * @brief Returns true if the value v has been allocated
     * with alloca
     *
     * Given the structure we have, we never track memory that
     * has been stack allocated as all objects come in through a
     * parameter
     */
    bool isAlloca(const Value* v) const;

    /*
     * @brief Insert a profiling block
     *
     * This will insert a profiling block BEFORE instruction i in block orig
     * based on the value of condVariable (if non-null, the control-flow
     * will go to the profiling block, otherwise it will go to orig).
     *
     * Returns a pair of basic-blocks: the first is the fall-through block (first
     * instruction is i) and the second is an empty profiling block.
     *
     * @param orig         Original basic block to split
     * @param i            Instruction to insert before
     * @param skipValue    Number of instructions that should be skipped in the fall-through block
     * @param instrCount   Starting point for instrCount for the fall-through block
     * @param condVariable Variable that determines whether or not the profiling block is called
     */
    std::pair<BasicBlock*, BasicBlock*> insertProfilingBlock(BasicBlock *orig,
            BasicBlock::iterator i,
            uint32_t skipValue, uint64_t instrCount,
            uint64_t fpInstrCount,
            GlobalVariable *condVariable);
    /**
     * @brief Determines if instrumentation is required for
     * the load instruction 'i' and if so inserts it.
     *
     * @param block Basic block in which to insert the instructions
     * @param count Current instruction count
     * @param i     Load instruction before which to insert
     * @return True if instrumentation was inserted and false otherwise
     */
    bool insertProfilerLoad(BasicBlock* block, uint64_t count, uint64_t fpInstrCount,
                            BasicBlock::iterator i);

    /**
     * @brief Same as insertProfilerLoad() except for stores
     *
     * @param block Basic block in which to insert the instructions
     * @param count Current instruction count
     * @param i     Store instruction before which to insert
     * @return True if instrumentation was inserted and false otherwise
     */
    bool insertProfilerStore(BasicBlock* block, uint64_t count, uint64_t fpInstrCount,
                             BasicBlock::iterator i);

    /**
     * @brief Inserts an update to the instruction count
     *
     * @param count Current instruction count
     * @param inst  Instruction to insert before
     * @return True if instrumentation was inserted and false otherwise
     */
    bool insertProfilerUpdateInstrCount(uint64_t count, uint64_t fpCount, BasicBlock::iterator i);

}; /* class OCRStatsPass */

} /*  anonymous namespace */

#endif /* __OCRSTATSPASS_H__ */


