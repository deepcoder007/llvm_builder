#include "llvm_builder/value.h"
#include "util/debug.h"
#include "llvm_builder/defines.h"
#include <iostream>
#include <string>

#include "util/string_util.h"
#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"
#include "llvm_builder/control_flow.h"

#include <format>

LLVM_BUILDER_NS_BEGIN

LLVM_BUILDER_NS_END

int32_t main(int32_t argc, char **argv) {
    using namespace llvm_builder;
    (void)argc;
    (void)argv;

    std::cout << " ======= APP BEGIN ======= " << std::endl;

    std::cout << " ======= APP END ======= " << std::endl;
    return 0;
}
