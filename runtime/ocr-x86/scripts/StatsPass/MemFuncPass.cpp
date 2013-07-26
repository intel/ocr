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

#define DEBUG_TYPE "hello"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"  
#include <llvm/IR/Function.h>
#include <llvm/PassManager.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include "llvm/IR/IntrinsicInst.h"
#include <llvm/IR/LLVMContext.h>
#include "llvm/Support/raw_ostream.h"
//#include "llvm/Target/TargetData.h"
#include "llvm/DebugInfo.h"
#include <llvm/IR/DataLayout.h>

#define PROFILER_MAX_ID 3

using namespace llvm;

STATISTIC(HelloCounter, "Counts number of functions greeted");

namespace {

				//GlobalVariable* Var_global = NULL;
				//GlobalVariable* Struct_global = NULL;

				const char* PROFILER_names[PROFILER_MAX_ID] = {
								"load_callback",
								"store_callback",
								"build_addresstable_callback"
				};

				Function* PROFILER_funcs[PROFILER_MAX_ID] = {
								NULL,
								NULL,
								NULL
				};

				typedef Type* (*typeFunc_t)(LLVMContext&);

				Type* myGetInt8PtrTy(LLVMContext& c) {
								return Type::getInt8PtrTy(c, 0);
				}
				Type* myGetInt32PtrTy(LLVMContext& c) {
								return Type::getInt32PtrTy(c, 0);
				}
				Type* myGetInt64PtrTy(LLVMContext& c) {
								return Type::getInt64PtrTy(c, 0);
				}

				typeFunc_t PROFILER_args[PROFILER_MAX_ID][6] = {
								{myGetInt8PtrTy, (typeFunc_t)Type::getInt64Ty, NULL, NULL, NULL, NULL}, /* load */
								{myGetInt8PtrTy, (typeFunc_t)Type::getInt64Ty, NULL, NULL, NULL, NULL}, /* store */
								{NULL, NULL, NULL, NULL, NULL, NULL} /* address table */
				};

				int PROFILER_arg_size[PROFILER_MAX_ID] = {
								2,
								2,
								0
				};

				struct TransformArgFunc {
								TransformArgFunc(LLVMContext& c) : context(&c) {}

								Type* operator()(typeFunc_t func) {                                            
												return func(*context);
								}                                                                                                

								LLVMContext* context; 
				}; 

				// Hello - The first implementation, without getAnalysisUsage.
				class MemFuncPass : public llvm::FunctionPass {
								public:
												static char ID; // Pass identification, replacement for typeid
												static int inst_flag;
												static int count;

												MemFuncPass() : FunctionPass(ID), curTarget(NULL), curModule(NULL) {}
												virtual bool runOnFunction(llvm::Function &F); 
												virtual bool doInitialization(Module& M); 
												std::pair<Value*, Value*> getFileLineInfo(const Instruction* inst);
								private:
												Module *curModule;
												DataLayout *curTarget;  
												std::vector<std::pair<std::string, GlobalVariable*> > fileNames;
				};

				bool MemFuncPass::runOnFunction(Function &F){
								//if(F.isDeclaration()) return false;
								bool retval = true;
								//int counter = 0;
								//if(F.getName() != NULL) 
								{
									errs()<< "Func: " << F.getName() << " called\n" ;
								}
								if(F.getName() == "ocrDbCreate") {
									errs()<< "ocrDbCreate called\n";
									std::vector<Value*> load_args;
									CallInst * loadCall = CallInst::Create(PROFILER_funcs[2], load_args);
									loadCall->insertBefore(F.begin()->begin());
								}

								for(Function::iterator bb = F.begin(), be = F.end(); bb != be; ++bb){

								for (BasicBlock::iterator bbit = bb->begin(), bbie = bb->end(); bbit != bbie;
																++bbit) { // Make loop work given updates
												Instruction *i = bbit;
												//Mem::count++;
												int opcode = i->getOpcode();

												struct globalStruct{
																int i;
																int j;
												} *myStruct;
												myStruct = (struct globalStruct*)malloc(sizeof(struct globalStruct));
												myStruct->i = 1;
												myStruct->j = 2;

												switch(opcode) {
																case Instruction::Load:
																				{
																								//Value* loadCounter = new LoadInst(Var_global, "load_counter", true, i);
																								//ConstantInt* counterVal = ConstantInt::get(Type::getInt32Ty(curModule->getContext()), MemFuncPass::count++);
																								//Value* addResult = BinaryOperator::Create(Instruction::Add, loadCounter, counterVal, "add_counter", i);
																								//new StoreInst(counterVal, Var_global, true, i);

																								StructType *ourTypeInfoType;
																								std::vector<Type*> arrayTypes;
																								arrayTypes.clear();
																								arrayTypes.push_back(Type::getInt32Ty(curModule->getContext()));
																								arrayTypes.push_back(Type::getInt32Ty(curModule->getContext()));
																								ourTypeInfoType = StructType::get(curModule->getContext(),arrayTypes); 
																								std::vector<Constant*> structVals;
																								structVals.clear();
																								structVals.push_back(ConstantInt::get(Type::getInt32Ty(curModule->getContext()), 1));
																								structVals.push_back(ConstantInt::get(Type::getInt32Ty(curModule->getContext()), 2));
																								Constant* structVal = ConstantStruct::get(ourTypeInfoType, structVals);

																								//ConstantInt* structVal = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), (long long)myStruct);
																								//new StoreInst(structVal, Struct_global, true, i);

																								//errs()<<"here load\n";
																								Value* loadLocation = i->getOperand(LoadInst::getPointerOperandIndex());

																								//no stack variable load instrumented 
																								if(isa<AllocaInst>(loadLocation)) break;

																								// Get file/line information for this instruction
																								//std::pair<Value*, Value*> fileLine(getFileLineInfo(instr));

																								const PointerType* locationPtrType = cast<PointerType>(loadLocation->getType());
																								Type* locationType = locationPtrType->getElementType();
																								assert(locationPtrType->isSized() && "Pointer type is not sized!!");
																								assert(locationType->isSized() && "Load type is not sized!!");

																								std::vector<Value*> load_args(2, NULL);
																								load_args[0] = new BitCastInst(loadLocation, Type::getInt8PtrTy(curModule->getContext()), "", i);
																								load_args[1] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), curTarget->getTypeAllocSize(locationType));					  

																								CallInst * loadCall = CallInst::Create(PROFILER_funcs[0], load_args);
																								loadCall->insertBefore(i);
																								break;
																				}
																case Instruction::Store:
																				{
																								//Value* loadCounter = new LoadInst(Var_global, "load_counter", true, i);
																								//ConstantInt* counterVal = ConstantInt::get(Type::getInt32Ty(curModule->getContext()), MemFuncPass::count++);
																								//Value* addResult = BinaryOperator::Create(Instruction::Add, loadCounter, counterVal, "add_counter", i);
																								//new StoreInst(counterVal, Var_global, true, i);

																								//errs()<<"here store\n";
																								Value* storeLocation = i->getOperand(StoreInst::getPointerOperandIndex());

																								//no global and stack variable store instrumented 
																								if(isa<AllocaInst>(storeLocation)) break;

																								// Get file/line information for this instruction
																								//std::pair<Value*, Value*> fileLine(getFileLineInfo(instr));

																								const PointerType* locationPtrType = cast<PointerType>(storeLocation->getType());
																								Type* locationType = locationPtrType->getElementType();
																								assert(locationPtrType->isSized() && "Pointer type is not sized!!");
																								assert(locationType->isSized() && "Load type is not sized!!");

																								std::vector<Value*> store_args(2, NULL);
																								store_args[0] = new BitCastInst(storeLocation, Type::getInt8PtrTy(curModule->getContext()), "", i);
																								store_args[1] = ConstantInt::get(Type::getInt64Ty(curModule->getContext()), curTarget->getTypeAllocSize(locationType));					  

																								CallInst * storeCall = CallInst::Create(PROFILER_funcs[1], store_args);
																								storeCall->insertBefore(i);
																								break;
																				}
												}

												/*if (!i->isTerminator()) {
													CallInst * afterCall = // INSERT THIS
													afterCall->insertAfter(i);
													}*/
												/*if(!Mem::inst_flag)
													errs() << "instruction: " << counter++ <<"\n";*/
								}
								}
								return retval;
				}

#if 0
				std::pair<Value*, Value*> Mem::getFileLineInfo(const Instruction* inst) {
								const DebugLoc& myLoc = inst->getDebugLoc();
								unsigned int lNum = myLoc.getLine();
								DIScope scope(myLoc.getScope(curModule->getContext()));

								// Get a GlobalVariable representation of the filename. We need this so that
								// we can actually address it and pass its address around.
								Value * fNameValue = NULL;
								std::string fNameStr("<unknown>");
								if(!scope.getFilename().empty())
												fNameStr = std::string(scope.getFilename().data());

								for(unsigned int i = 0, e = fileNames.size(); i < e; ++i) {
												if(fileNames[i].first == fNameStr) {
																fNameValue = fileNames[i].second;
																break;
												}
								}

								if(!fNameValue) {
												// This means we have to create a global variable
												Constant* fNameArray = ConstantArray::get(ArrayType::get(),
																				fNameStr);

												fNameValue = new GlobalVariable(*curModule, fNameArray->getType(),
																				true, GlobalValue::PrivateLinkage, fNameArray, "");
												fileNames.push_back(std::pair<std::string, GlobalVariable*>(fNameStr,
																								cast<GlobalVariable>(fNameValue)));
								}
								// Convert lNum and fName to constants
								ConstantInt* lNumValue = ConstantInt::get(Type::getInt32Ty(curModule->getContext()),
																lNum);
								return std::pair<Value*, Value*>(fNameValue, lNumValue);
				}
#endif

				bool MemFuncPass::doInitialization(Module& M) {

								curModule = &M;
								if(curTarget)
												delete curTarget;
								curTarget = new DataLayout(&M);

								//create globals
								/*Constant *t;
								t = curModule->getOrInsertGlobal("_globalVar", Type::getInt32Ty(M.getContext()));
								assert(isa<GlobalVariable>(t) && "_globalVar of the wrong type");
								Var_global = cast<GlobalVariable>(t);

								t = curModule->getOrInsertGlobal("_globalStruct", StructType::create(curModule->getContext()));
								assert(isa<GlobalVariable>(t) && "_globalVar of the wrong type");
								Struct_global = cast<GlobalVariable>(t);*/

								//create functions
								std::vector<Type*> argTypes;                                                               
								std::vector<typeFunc_t> argFuncs;
								TransformArgFunc myTransformer(curModule->getContext());                                         
								for(unsigned int i=0, e = (unsigned)PROFILER_MAX_ID; i<e; ++i) {                                 
												argFuncs.assign(PROFILER_args[i], PROFILER_args[i]+PROFILER_arg_size[i]);                
												argTypes.resize(argFuncs.size());
												std::transform(argFuncs.begin(), argFuncs.end(), argTypes.begin(), myTransformer);       

												Constant *tt = curModule->getOrInsertFunction(PROFILER_names[i],                         
																				FunctionType::get(Type::getVoidTy(curModule->getContext()), argTypes, false));   

												assert(tt && isa<Function>(tt) && "Could not create function in module");
												PROFILER_funcs[i] = cast<Function>(tt);                                                  
								}               

								fileNames.clear(); // Reset the file names for this module
								return true;
				}

				int myfunc()
				{
								if(!MemFuncPass::inst_flag)
												errs() << "instruction called\n" ;
								return 0;

				}

}

int MemFuncPass::inst_flag = 0;
char MemFuncPass::ID = 0;
int MemFuncPass::count = 0;

static RegisterPass<MemFuncPass> X("mem", "Memory Access Pass");

