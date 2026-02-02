#include "core/llvm/value.h"
#include "util/debug.h"
#include "core/util/defines.h"
#include "util/logger.h"
#include "core/ds/fixed_vector.h"
#include "core/util/dtype_util.h"
#include <iostream>
#include <string>

#include "util/string_util.h"
#include "core/llvm/module.h"
#include "core/llvm/type.h"
#include "core/llvm/jit.h"
#include "core/llvm/function.h"
#include "core/llvm/control_flow.h"

#include "util/sys_util.h"
#include <filesystem>

CORE_NS_BEGIN

CORE_NS_END

int32_t main(int32_t argc, char **argv) {
    using namespace core;
    using namespace core::llvm_proxy;
    (void)argc;
    (void)argv;

    CORE_INFO << " ======= APP BEGIN ======= ";

    CORE_INFO << " ======= APP END ======= ";
    return 0;
}
