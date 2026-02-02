
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wredundant-move"


// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/IR/IRBuilder.h"
// #include "llvm/IR/Verifier.h"
// #include "llvm/Support/raw_ostream.h"
#include "llvm/ext_include.h"
#include <vector>
#include <iostream>
#include <memory>

#pragma GCC diagnostic pop

using namespace llvm;
using namespace llvm::orc;

int binary_fn(int a, int b) {
    std::cout << " Received a: " << a << std::endl;
    std::cout << " Received b: " << a << std::endl;
    std::cout << " Return c: " << a + b << std::endl;
    return a + b;
}

int main(int argc, char** argv) {

    InitLLVM X(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();


    std::unique_ptr<LLVMContext> context = std::make_unique<LLVMContext>();
    std::unique_ptr<Module> mod = std::make_unique<Module>("CallbackModule", *context);
    IRBuilder<> builder(*context);

    // ---------------------------------------------------------
    // 1. Define Basic Types
    // ---------------------------------------------------------
    Type* int32Type = Type::getInt32Ty(*context);
    Type* ptrType = PointerType::get(*context, 0); // Opaque pointer

    // ---------------------------------------------------------
    // 2. Define the Callback Function Signature
    // ---------------------------------------------------------
    // We need to strictly define what the callback looks like so LLVM knows
    // how to call it, even though we pass it as a generic pointer.
    // Signature: int(int, int)
    std::vector<Type*> callbackArgs = { int32Type, int32Type };
    FunctionType* callbackFuncType = FunctionType::get(
        int32Type,      // Return type
        callbackArgs,   // Argument types
        false           // Is VarArg?
    );

    // ---------------------------------------------------------
    // 3. Define the 'Wrapper' Function Signature
    // ---------------------------------------------------------
    // Signature: int apply_callback(int, int, ptr)
    // Note: In modern LLVM, the function pointer argument is just 'ptr'.
    std::vector<Type*> wrapperArgs = { int32Type, int32Type, ptrType };
    FunctionType* wrapperFuncType = FunctionType::get(
        int32Type,
        wrapperArgs,
        false
    );

    Function* wrapperFunc = Function::Create(
        wrapperFuncType,
        Function::ExternalLinkage,
        "apply_callback",
        *mod
    );

    // ---------------------------------------------------------
    // 4. Implement the Function Body
    // ---------------------------------------------------------
    BasicBlock* entry = BasicBlock::Create(*context, "entry", wrapperFunc);
    builder.SetInsertPoint(entry);

    // Get arguments
    auto args = wrapperFunc->arg_begin();
    Value* argA = args++;
    argA->setName("a");
    Value* argB = args++;
    argB->setName("b");
    Value* callbackPtr = args++;
    callbackPtr->setName("callback_fn");

    // ---------------------------------------------------------
    // 5. Call the Callback Pointer
    // ---------------------------------------------------------
    // CRITICAL STEP: When calling a pointer, LLVM needs to know the
    // FunctionType of the callee. We pass 'callbackFuncType' here.
    Value* result = builder.CreateCall(
        callbackFuncType,   // The signature of the function we are calling
        callbackPtr,        // The pointer to the function
        { argA, argB },     // The arguments to pass to the callback
        "call_result"
    );

    builder.CreateRet(result);

    // ---------------------------------------------------------
    // 6. Verify and Print
    // ---------------------------------------------------------
    verifyFunction(*wrapperFunc);
    mod->print(outs(), nullptr);

    // JIT
    std::cout << " ----------- JIT BELOW ------------------- " << std::endl;
    auto JIT = ExitOnError()(LLJITBuilder().create());
    if (not JIT) {
        errs() << "Failed to create LLJIT\n";
        return 1;
    }

    typedef int (*apply_cb)(int, int, void*);
    ThreadSafeModule TSM{std::move(mod), std::move(context)};
    ExitOnError()(JIT->addIRModule(std::move(TSM)));
    ExitOnError()(JIT->initialize(JIT->getMainJITDylib()));

    auto apply_callback_fn = ExitOnError()(JIT->lookup("apply_callback"));
    if (not apply_callback_fn) {
        errs() << "Failed to find add function\n";
        return 1;
    }
    uint64_t fn_addr = apply_callback_fn.getValue();
    apply_cb fn_ptr = reinterpret_cast<apply_cb>(fn_addr);
    int res = fn_ptr(10, 11333, (void*)binary_fn);
    std::cout << " res:  " << res << std::endl;

    return 0;
}
