#ifndef LLVM_EXTERNAL_INCLUDE_H
#define LLVM_EXTERNAL_INCLUDE_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wredundant-move"

#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Triple.h"

#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MathExtras.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IRReader/IRReader.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/DebugUtils.h"
#include "llvm/ExecutionEngine/Orc/Debugging/DebuggerSupport.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/ObjectTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/SymbolStringPool.h"
#include "llvm/ExecutionEngine/Orc/TargetProcess/TargetExecutionUtils.h"

#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"

#pragma GCC diagnostic pop

#endif // LLVM_EXTERNAL_INCLUDE_H
