
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wredundant-move"

#include "llvm/ext_include.h"
#include <iostream>
#include <memory>

#pragma GCC diagnostic pop

using namespace llvm;
using namespace llvm::orc;

// Build a simple function: int add_and_double(int a, int b) { return (a + b) * 2; }
Function* buildAddAndDoubleFunction(Module& mod, LLVMContext& ctx) {
    Type* int32Type = Type::getInt32Ty(ctx);

    FunctionType* funcType = FunctionType::get(
        int32Type,
        {int32Type, int32Type},
        false
    );

    Function* func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        "add_and_double",
        mod
    );

    BasicBlock* entry = BasicBlock::Create(ctx, "entry", func);
    IRBuilder<> builder(entry);

    auto args = func->arg_begin();
    Value* a = args++;
    a->setName("a");
    Value* b = args++;
    b->setName("b");

    // sum = a + b
    Value* sum = builder.CreateAdd(a, b, "sum");
    // result = sum * 2
    Value* two = ConstantInt::get(int32Type, 2);
    Value* result = builder.CreateMul(sum, two, "result");

    builder.CreateRet(result);

    return func;
}

int main(int argc, char** argv) {
    InitLLVM X(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    auto context = std::make_unique<LLVMContext>();
    auto mod = std::make_unique<Module>("OrcJitWithInstrumentationExample", *context);

    // ---------------------------------------------------------
    // 1. Build the function
    // ---------------------------------------------------------
    Function* func = buildAddAndDoubleFunction(*mod, *context);
    verifyFunction(*func);

    std::cout << "=== Original IR ===" << std::endl;
    mod->print(outs(), nullptr);

    // ---------------------------------------------------------
    // 2. Set up StandardInstrumentations and Pass Managers
    // ---------------------------------------------------------
    // PassInstrumentationCallbacks - holds callbacks that get invoked during pass execution
    PassInstrumentationCallbacks PIC;

    // Register CUSTOM callbacks to see pass execution
    // These fire for every pass that runs through the pass manager
    PIC.registerBeforeNonSkippedPassCallback(
        [](StringRef PassID, Any IR) {
            std::cout << "[Pass] Running BeforeNonSkipped: " << PassID.str() << std::endl;
        });

    PIC.registerAfterPassCallback(
        [](StringRef PassID, Any IR, const PreservedAnalyses& PA) {
            std::cout << "[Pass] Finished: " << PassID.str() << std::endl;
        });

    PIC.registerAfterAnalysisCallback(
        [](StringRef AnalysisID, Any IR) {
            std::cout << "[Analysis] Computed: " << AnalysisID.str() << std::endl;
        });

    // StandardInstrumentations - provides built-in instrumentation (timing, IR printing, verification)
    // Second parameter 'true' enables debug logging (but requires LLVM flags to actually print)
    StandardInstrumentations SI(*context, /*DebugLogging=*/true);

    // Register the standard callbacks with PIC
    // This connects SI's callbacks (timing, printing, etc.) to the pass manager
    // The ModuleAnalysisManager pointer allows SI to access cached analyses
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    SI.registerCallbacks(PIC, &MAM);

    // ---------------------------------------------------------
    // 3. Set up PassBuilder and register analyses
    // ---------------------------------------------------------
    // IMPORTANT: Pass the PIC to PassBuilder so it wires up the callbacks
    // to the analysis managers
    PassBuilder PB(/*TargetMachine=*/nullptr, PipelineTuningOptions(), std::nullopt, &PIC);

    // Register all the standard analyses (this also registers PassInstrumentation)
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);

    // Cross-register proxies so passes can request analyses from other managers
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // ---------------------------------------------------------
    // 4. Create and run optimization passes
    // ---------------------------------------------------------
    FunctionPassManager FPM;
    FPM.addPass(InstCombinePass());      // Combine instructions
    FPM.addPass(ReassociatePass());      // Reorder associative operations
    FPM.addPass(GVNPass());              // Global Value Numbering

    std::cout << "\n=== Running Optimization Passes ===" << std::endl;

    // Run passes on our function
    // The StandardInstrumentations will log pass execution if debug is enabled
    FPM.run(*func, FAM);

    std::cout << "\n=== Optimized IR ===" << std::endl;
    mod->print(outs(), nullptr);

    // ---------------------------------------------------------
    // 5. Set up ORC JIT with custom JITTargetMachineBuilder
    // ---------------------------------------------------------
    std::cout << "\n=== Setting up ORC JIT ===" << std::endl;

    // Detect host target machine
    auto JTMB = JITTargetMachineBuilder::detectHost();
    if (!JTMB) {
        errs() << "Failed to detect host: " << toString(JTMB.takeError()) << "\n";
        return 1;
    }

    // Configure optimization level (use None to see the effect of our manual passes)
    JTMB->setCodeGenOptLevel(CodeGenOptLevel::None);

    // Create the JIT using LLJITBuilder
    auto JIT = LLJITBuilder()
        .setJITTargetMachineBuilder(std::move(*JTMB))
        .create();

    if (!JIT) {
        errs() << "Failed to create LLJIT: " << toString(JIT.takeError()) << "\n";
        return 1;
    }

    // ---------------------------------------------------------
    // 5b. Set up JIT compilation logging via ObjectTransformLayer
    // ---------------------------------------------------------
    // The ObjectTransformLayer is called after IR is compiled to object code.
    // We can set a transform function to log when compilation happens.
    (*JIT)->getObjTransformLayer().setTransform(
        [](std::unique_ptr<MemoryBuffer> Obj) -> Expected<std::unique_ptr<MemoryBuffer>> {
            std::cout << "[JIT] Compiling module to object code ("
                      << Obj->getBufferSize() << " bytes)" << std::endl;
            return Obj;
        });

    // We can also add an IR transform to log before compilation
    (*JIT)->getIRTransformLayer().setTransform(
        [](ThreadSafeModule TSM, const MaterializationResponsibility& R)
            -> Expected<ThreadSafeModule> {
            TSM.withModuleDo([](Module& M) {
                std::cout << "[JIT] Compiling IR module: " << M.getName().str() << std::endl;
                for (const Function& F : M) {
                    if (!F.isDeclaration()) {
                        std::cout << "[JIT]   - function: " << F.getName().str() << std::endl;
                    }
                }
            });
            return TSM;
        });

    // ---------------------------------------------------------
    // 6. Add the module and compile
    // ---------------------------------------------------------
    ThreadSafeModule TSM{std::move(mod), std::move(context)};

    if (Error err = (*JIT)->addIRModule(std::move(TSM))) {
        errs() << "Failed to add module: " << toString(std::move(err)) << "\n";
        return 1;
    }

    // Initialize the JIT dylib (runs static constructors, etc.)
    if (Error err = (*JIT)->initialize((*JIT)->getMainJITDylib())) {
        errs() << "Failed to initialize JIT: " << toString(std::move(err)) << "\n";
        return 1;
    }

    // ---------------------------------------------------------
    // 7. Look up and execute the function
    // ---------------------------------------------------------
    auto sym = (*JIT)->lookup("add_and_double");
    if (!sym) {
        errs() << "Failed to find symbol: " << toString(sym.takeError()) << "\n";
        return 1;
    }

    // Cast to function pointer and call
    using AddAndDoubleFn = int (*)(int, int);
    AddAndDoubleFn fn = reinterpret_cast<AddAndDoubleFn>(sym->getValue());

    std::cout << "\n=== Executing JIT-compiled function ===" << std::endl;

    int a = 5, b = 7;
    int result = fn(a, b);

    std::cout << "add_and_double(" << a << ", " << b << ") = " << result << std::endl;
    std::cout << "Expected: (" << a << " + " << b << ") * 2 = " << (a + b) * 2 << std::endl;

    // ---------------------------------------------------------
    // 8. Cleanup
    // ---------------------------------------------------------
    if (Error err = (*JIT)->deinitialize((*JIT)->getMainJITDylib())) {
        errs() << "Failed to deinitialize JIT: " << toString(std::move(err)) << "\n";
        return 1;
    }

    std::cout << "\nDone!" << std::endl;
    return 0;
}
