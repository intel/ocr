#define DEBUG_TYPE "OCR-Stats"

#include "OCRStatsPass.h"

#include <llvm/DebugInfo.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#define PROFILER_MAX_ID 2

using namespace llvm;

//STATISTIC(OCRStatsPass, "Instruments the code to notify the OCR statistics runtime of LD/ST");

namespace {


    const char* PROFILER_FuncNames[PROFILER_MAX_ID] = {
        "PROFILER_ocrStatsLoadCallback",
        "PROFILER_ocrStatsStoreCallback"
    };

    Function* PROFILER_Funcs[PROFILER_MAX_ID] = {
        NULL,
        NULL
    };

    typedef Type* (*typeFunc_t)(LLVMContext&);

    inline Type* myGetInt8PtrTy(LLVMContext& c) {
        return Type::getInt8PtrTy(c, 0);
    }

    // IMPORTANT NOTE: For all the functions that take instrCount, the value passed is
    // the value to *add* to _threadInstructionCount (ie: the instructions not yet recorded in
    // _threadInstructionCount)
    typeFunc_t PROFILER_FuncArgs[PROFILER_MAX_ID][4] = {
        {myGetInt8PtrTy, (typeFunc_t)Type::getInt64Ty, (typeFunc_t)Type::getInt64Ty, (typeFunc_t)Type::getInt64Ty}, /* load(void* addr, u64 size, u64 instrCount, u64 fpInstrCount) */
        {myGetInt8PtrTy, (typeFunc_t)Type::getInt64Ty, (typeFunc_t)Type::getInt64Ty, (typeFunc_t)Type::getInt64Ty} /* store(void* addr, u64 addr, u64 instrCount, u64 fpInstrCount) */
    };

    int PROFILER_FuncArgsSize[PROFILER_MAX_ID] = {
        4,
        4
    };

    struct TransformArgFunc {
        TransformArgFunc(LLVMContext& c) : context(&c) {}

        Type* operator()(typeFunc_t func) {
            return func(*context);
        }

        LLVMContext* context;
    };

    GlobalVariable* InstructionCount_global = NULL;
    GlobalVariable* FPInstructionCount_global = NULL;
    GlobalVariable* InstrumentOn_global = NULL;

    OCRStatsPass::~OCRStatsPass() {
        if(curTarget)
            delete curTarget;
    }


    bool OCRStatsPass::runOnFunction(Function &F) {
        if(F.isDeclaration()) return false;
        if(!considerFunction(&F)) return false;

        errs()<< "--- Function: " << F.getName().str() << " ---\n";


        for(Function::iterator bb = F.begin(), be = F.end(); bb != be; ++bb){

            MDNode* blockInfo = NULL;
            uint64_t instrCount = 0;
            uint64_t fpInstrCount = 0;
            unsigned char skipInstructions = 0;
            unsigned char skipAndCountInstructions = 0;
            unsigned char blockType = 'n';

            blockInfo = bb->front().getMetadata("OCRP");

            if(blockInfo) {
                ConstantInt* mdValue = cast<ConstantInt>(blockInfo->getOperand(0));
                uint64_t v = mdValue->getValue().getZExtValue();
                blockType = (v & 0xFF); // 8 bits for the block type
                skipInstructions = (v & 0xFF00) >> 8; // 8 bits for the skip Instructions
                skipAndCountInstructions = (v & 0xFF0000) >> 16; // 8 bits for the skipAndCount
                instrCount = (v & 0xFFFFF000000) >> 24; // 20 bits for instrCount
                fpInstrCount =  v >> 44; // 20 bits for fpInstrCount
            }

            switch(blockType) {
            case 'n': /* normal block */
                assert(instrCount == 0 && "Instruction count not zero at start of normal block");
                assert(fpInstrCount == 0 && "FP Instruction count not zero at start of normal block");
                assert(skipInstructions == 0 && "Normal blocks should have no instructions to skip");
                assert(skipAndCountInstructions == 0 && "Normal blocks should have no instructions to skip");
                assert(!(bb->getName().startswith("OCRP")) && "Unexpected special block");
                break;
            case 'p': /* profiling block */
                assert(bb->getName().startswith("OCRP_PB") && "Expected a profiling block");
                assert(skipInstructions == 0 && skipAndCountInstructions == 0
                       && "Profiling blocks should have not instructions to skip");
                break;
            case 'f': /* fallthrough block */
                assert(bb->getName().startswith("OCRP_FT") && "Expected a fall-through block");
                break;
            default:
                assert(0 && "Unexpected block type");
            };
            for (BasicBlock::iterator bbit = bb->begin(), bbie = bb->end(); bbit != bbie;
                 ++bbit) {

                if(skipInstructions) {
                    --skipInstructions;
                    continue;
                }
                if(skipAndCountInstructions) {
                    ++instrCount;
                    --skipAndCountInstructions;
                    Instruction *i = bbit;
                    int opcode = i->getOpcode();
                    if(opcode == Instruction::FAdd || opcode == Instruction::FSub ||
                       opcode == Instruction::FMul || opcode == Instruction::FDiv ||
                       opcode == Instruction::FPToUI || opcode == Instruction::FPToSI ||
                       opcode == Instruction::UIToFP || opcode == Instruction::SIToFP ||
                       opcode == Instruction::FPTrunc || opcode == Instruction::FPExt ||
                       opcode == Instruction::FCmp)
                        ++fpInstrCount;
                    continue;
                }

                bool breakForLoop = false;
                bool instrCountReset = false;
                Instruction *i = bbit;

                int opcode = i->getOpcode();

                switch(opcode) {
                case Instruction::FAdd: case Instruction::FSub: case Instruction::FMul:
                case Instruction::FDiv: case Instruction::FPToUI: case Instruction::FPToSI:
                case Instruction::UIToFP: case Instruction::SIToFP:
                case Instruction::FPTrunc: case Instruction::FPExt: case Instruction::FCmp:
                {
                    ++fpInstrCount;
                    break;
                }
                case Instruction::Load:
                {
                    assert(isa<LoadInst>(i));
                    breakForLoop = insertProfilerLoad(&*bb, instrCount, fpInstrCount, bbit);
                    break;
                }
                case Instruction::Store:
                {
                    assert(isa<StoreInst>(i));
                    breakForLoop = insertProfilerStore(&*bb, instrCount, fpInstrCount, bbit);
                    break;
                }
                case Instruction::Invoke:
                {
                    assert(isa<InvokeInst>(i));
                    // This is a terminating instruction so we need to update the instruction count
                    insertProfilerUpdateInstrCount(instrCount + 1, fpInstrCount, bbit);
                    instrCountReset = true;
                    break;
                }
                case Instruction::Call:
                {
                    assert(isa<CallInst>(i));
                    Function *calledFunc = (cast<CallInst>(i))->getCalledFunction();
                    // If calledFunc is NULL, we can ignore it (indirect call??)
                    if(calledFunc) {
                        if(calledFunc->getIntrinsicID() != Intrinsic::not_intrinsic) {
                            insertProfilerUpdateInstrCount(instrCount + 1, fpInstrCount, bbit);
                            instrCountReset = true;
                        }
                    }
                    break;
                }
                default:
                    if(i->isTerminator()) {
                        insertProfilerUpdateInstrCount(instrCount + 1, fpInstrCount, bbit);
                        instrCountReset = true;
                    }
                    break;
                }

                if(breakForLoop) {
                    // This means that we inserted a profiling block and therefore we need
                    // to stop looking at this block (as the iterator is no longer valid)
                    // and go to the next block (which is the profiling block which will get skipped)
                    // and then the other block (which will be the fallthrough branch). We
                    // will propagate the value of instrCount to save on useless store
                    // and load
                    break;
                }
                if(instrCountReset) {
                    instrCount = 0;
                    fpInstrCount = 0;
                } else
                    ++instrCount;
            }
        }
        return true;
    }

    bool OCRStatsPass::doInitialization(Module& M) {

        curModule = &M;
        curTarget = new DataLayout(&M);

        //create globals
        Constant *t;
        t = curModule->getOrInsertGlobal("_threadInstructionCount", Type::getInt64Ty(M.getContext()));
        assert(isa<GlobalVariable>(t) && "_threadInstructionCount of the wrong type");
        InstructionCount_global = cast<GlobalVariable>(t);
        InstructionCount_global->setThreadLocal(true);
        //assert(InstructionCount_global->isThreadLocal() && "_threadInstructionCount not thread local");

        t = curModule->getOrInsertGlobal("_threadFPInstructionCount", Type::getInt64Ty(M.getContext()));
        assert(isa<GlobalVariable>(t) && "_threadFPInstructionCount of the wrong type");
        FPInstructionCount_global = cast<GlobalVariable>(t);
        FPInstructionCount_global->setThreadLocal(true);
        //assert(FPInstructionCount_global->isThreadLocal() && "_threadFPInstructionCount not thread local");

        t = curModule->getOrInsertGlobal("_threadInstrumentOn", Type::getInt8Ty(M.getContext()));
        assert(isa<GlobalVariable>(t) && "_threadInstrumentOn of the wrong type");
        InstrumentOn_global = cast<GlobalVariable>(t);
        InstrumentOn_global->setThreadLocal(true);
        //assert(InstrumentOn_global->isThreadLocal() && "_threadInstrumentOn not thread local");

        //create functions
        std::vector<Type*> argTypes;
        std::vector<typeFunc_t> argFuncs;
        TransformArgFunc myTransformer(curModule->getContext());
        for(unsigned int i = 0, e = (unsigned)PROFILER_MAX_ID; i<e; ++i) {
            argFuncs.assign(PROFILER_FuncArgs[i], PROFILER_FuncArgs[i] + PROFILER_FuncArgsSize[i]);

            argTypes.resize(argFuncs.size());
            std::transform(argFuncs.begin(), argFuncs.end(), argTypes.begin(), myTransformer);

            Constant *tt = curModule->getOrInsertFunction(
                PROFILER_FuncNames[i], FunctionType::get(
                    Type::getVoidTy(curModule->getContext()), argTypes, false));

            assert(tt && isa<Function>(tt) && "Could not create function in module");
            PROFILER_Funcs[i] = cast<Function>(tt);
        }

        return true;
    }

    bool OCRStatsPass::considerFunction(const Function* func) const {
        StringRef fName(func->getName());

        if(fName.startswith("PROFILER_")) return false;

        return true;
    }

    bool OCRStatsPass::isIgnoredGlobal(const Value* v) const {
	return (v == InstructionCount_global || v == FPInstructionCount_global ||
                v == InstrumentOn_global);
    }

    bool OCRStatsPass::isAlloca(const Value* v) const {

	return (isa<AllocaInst>(v) ||
		(isa<ConstantExpr>(v) &&
                 cast<ConstantExpr>(v)->getOpcode() == Instruction::Alloca));
    }

    std::pair<BasicBlock*, BasicBlock*> OCRStatsPass::insertProfilingBlock(
        BasicBlock *orig, BasicBlock::iterator i, uint32_t skipValue, uint64_t instrCount,
        uint64_t fpInstrCount, GlobalVariable *condVariable) {

        assert(((skipValue & ~(0xFFFFULL)) == 0) && "skipValue too big");
	assert(((instrCount & ~(0xFFFFFULL)) == 0) && "instrCount too big");
        assert(((fpInstrCount & ~(0xFFFFFULL)) == 0) && "fpInstrCount too big");

	// This function
	//    - splits the original block (this creates the fall-through block)
	//        + renames the fall-through block appropriately
	//    - adds instructions to test if condVariable is non zero
	//    - creates a profiling block
	//	  + renames it appropriately
	//    - creates a terminating instruction of profiling block to fall-through block
	//    - replaces terminating instruction of original block (unconditional
	//      branch leading to fall-through block) with a conditional one based on test
	//	- returns the profiling block and FT block

	assert(i->getParent() == orig && "Iterator not part of original basic block");

	// Split block
	BasicBlock *fallThroughBlock = orig->splitBasicBlock(i, "OCRP_FT");

	// Create new instruction for comparison
	Value *loadValue = new LoadInst(condVariable, "", orig->getTerminator());
	ICmpInst *compareInst = new ICmpInst(orig->getTerminator(), CmpInst::ICMP_UGT, loadValue,
                                             ConstantInt::get(loadValue->getType(), 0));

	// Create new profiling basic block
	BasicBlock *profilingBlock = BasicBlock::Create(curModule->getContext(),
                                                        "OCRP_PB", orig->getParent(), fallThroughBlock);
	BranchInst::Create(fallThroughBlock, profilingBlock);

	// Replace old branch instruction
	ReplaceInstWithInst(orig->getTerminator(),
                            BranchInst::Create(profilingBlock, fallThroughBlock, compareInst));

	// Tag the first instructions correctly. First of the fall-through block
	uint64_t tagValue = 'f';
	tagValue |= skipValue<<8;
	tagValue |= instrCount<<24; // fall-through instruction count should continue
        tagValue |= fpInstrCount<<44;
	Value* mdData[1] = { ConstantInt::get(Type::getInt64Ty(curModule->getContext()), tagValue) };
	fallThroughBlock->front().setMetadata("OCRP", MDNode::get(curModule->getContext(), mdData));

	// Take care of the profiling block
	tagValue = 'p';
	mdData[0] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), tagValue);
	profilingBlock->front().setMetadata("OCRP", MDNode::get(curModule->getContext(), mdData));

	return std::make_pair(fallThroughBlock, profilingBlock);
    }


    bool OCRStatsPass::insertProfilerLoad(BasicBlock *block, uint64_t instrCount, uint64_t fpInstrCount, BasicBlock::iterator i) {
        Instruction *instr = &*i;

        Value *loadLocation = instr->getOperand(LoadInst::getPointerOperandIndex());

        // Make sure that this is not one of the globals or stack allocated thing
        if(isIgnoredGlobal(loadLocation) || isAlloca(loadLocation)) return false;

        const PointerType *locationPtrType = cast<PointerType>(loadLocation->getType());
        Type* locationType = locationPtrType->getElementType();
        assert(locationPtrType->isSized() && "Pointer type is not sized!!");
        assert(locationType->isSized() && "Load type is not sized!!");

        // This returns the newly created profiling block before i
        BasicBlock *profBlock = insertProfilingBlock(block, i, 1<<8, instrCount,
                                                     fpInstrCount, InstrumentOn_global).second;

        Instruction *firstInstr = &(profBlock->front());

        std::vector<Value*> args(4, NULL);
        args[0] = new BitCastInst(loadLocation, Type::getInt8PtrTy(curModule->getContext()), "",
                                  profBlock->getTerminator());
        args[1] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()),
                                   curTarget->getTypeAllocSize(locationType));
        args[2] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), instrCount);
        args[3] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), fpInstrCount);

        CallInst *loadCall = CallInst::Create(PROFILER_Funcs[0], args);
        loadCall->insertBefore(profBlock->getTerminator());

        // Move the metadata information
        cast<Instruction>(args[0])->setMetadata("OCRP", firstInstr->getMetadata("OCRP"));
        firstInstr->setMetadata("OCRP", NULL);

        return true;

    }

    bool OCRStatsPass::insertProfilerStore(BasicBlock *block, uint64_t instrCount, uint64_t fpInstrCount,
                                           BasicBlock::iterator i) {
        Instruction *instr = &*i;

        Value *storeLocation = instr->getOperand(StoreInst::getPointerOperandIndex());

        // Make sure that this is not one of the globals or stack allocated thing
        if(isIgnoredGlobal(storeLocation) || isAlloca(storeLocation)) return false;

        const PointerType* locationPtrType = cast<PointerType>(storeLocation->getType());
        Type* locationType = locationPtrType->getElementType();
        assert(locationPtrType->isSized() && "Pointer type is not sized!!");
        assert(locationType->isSized() && "Store type is not sized!!");

        // This returns the newly created profiling block before i
        BasicBlock *profBlock = insertProfilingBlock(block, i, 1<<8, instrCount,
                                                     fpInstrCount, InstrumentOn_global).second;

        Instruction *firstInstr = &(profBlock->front());

        std::vector<Value*> args(4, NULL);
        args[0] = new BitCastInst(storeLocation, Type::getInt8PtrTy(curModule->getContext()), "",
                                  profBlock->getTerminator());
        args[1] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()),
                                   curTarget->getTypeAllocSize(locationType));
        args[2] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), instrCount);
        args[3] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), fpInstrCount);

        CallInst *storeCall = CallInst::Create(PROFILER_Funcs[1], args);
        storeCall->insertBefore(profBlock->getTerminator());

        // Move the metadata information
        cast<Instruction>(args[0])->setMetadata("OCRP", firstInstr->getMetadata("OCRP"));
        firstInstr->setMetadata("OCRP", NULL);

        return true;

    }

    bool OCRStatsPass::insertProfilerUpdateInstrCount(uint64_t count, uint64_t fpCount, BasicBlock::iterator i) {

        Instruction *inst = &*i;
        Value* loadCounter = cast<Value>(new LoadInst(InstructionCount_global, "load_counter", true, inst));
        ConstantInt* counterAdd = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), count);
        Value* addResult = BinaryOperator::Create(Instruction::Add, loadCounter, counterAdd, "add_counter",
                                                  inst);

        new StoreInst(addResult, InstructionCount_global, true, inst);

        loadCounter = cast<Value>(new LoadInst(FPInstructionCount_global, "load_fpcounter", true, inst));
        counterAdd = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), fpCount);
        addResult = BinaryOperator::Create(Instruction::Add, loadCounter, counterAdd, "add_fpcounter",
                                                  inst);

        new StoreInst(addResult, FPInstructionCount_global, true, inst);
        return true;
    }

    char OCRStatsPass::ID = 0;

    static RegisterPass<OCRStatsPass> X("OCRStats", "OCR Profiling Pass");

} /* anonymous namespace */


