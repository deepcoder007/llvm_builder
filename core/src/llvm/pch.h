//
// Precompiled header for LLVM and common STL includes
// This file is used to speed up compilation by precompiling heavy headers
//

#ifndef CORE_LLVM_PCH_H
#define CORE_LLVM_PCH_H

// Suppress warnings from LLVM headers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wredundant-move"
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

// ============================================================================
// Common STL headers
// ============================================================================
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// ============================================================================
// LLVM ADT and Support
// ============================================================================
#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Triple.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetSelect.h"

// ============================================================================
// LLVM IR
// ============================================================================
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"

// ============================================================================
// LLVM ExecutionEngine / ORC JIT
// ============================================================================
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#include "llvm/ExecutionEngine/Orc/DebugUtils.h"
#include "llvm/ExecutionEngine/Orc/Debugging/DebuggerSupport.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ObjectTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/SymbolStringPool.h"
#include "llvm/ExecutionEngine/Orc/TargetProcess/TargetExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"

// ============================================================================
// LLVM Transforms and Passes
// ============================================================================
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"

#pragma GCC diagnostic pop

#endif // CORE_LLVM_PCH_H
