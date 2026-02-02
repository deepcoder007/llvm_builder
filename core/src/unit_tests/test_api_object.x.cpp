//
// Created by vibhanshu on 2025-11-21
//

#include "gtest/gtest.h"

#include "util/debug.h"
#include "core/util/defines.h"

#include "core/llvm/module.h"
#include "core/llvm/type.h"
#include "core/llvm/jit.h"
#include "core/llvm/function.h"

using namespace core;
using namespace core::llvm_proxy;

TEST(API_OBJECT, basic_null_test) {
#define NULL_OBJECT_TEST(type_name)                                     \
     {                                                                  \
        type_name null_1 = type_name :: null();                         \
        type_name null_2 = type_name :: null();                         \
        CORE_ALWAYS_ASSERT(null_1.has_error());                         \
        CORE_ALWAYS_ASSERT(null_2.has_error());                         \
        CORE_ALWAYS_ASSERT_EQ(null_1, null_2);                          \
    }                                                                   \
 /**/

    NULL_OBJECT_TEST(Cursor)
    NULL_OBJECT_TEST(Module)
    NULL_OBJECT_TEST(PackagedModule)

    NULL_OBJECT_TEST(TypeInfo)
    // TODO{vibhanshu}: null of field_entry is a bit different
    // NULL_OBJECT_TEST(TypeInfo::field_entry_t)
    NULL_OBJECT_TEST(ValueInfo)

    NULL_OBJECT_TEST(LinkSymbolName)
    NULL_OBJECT_TEST(LinkSymbol)

    NULL_OBJECT_TEST(FnContext)
    NULL_OBJECT_TEST(CodeSection)
    NULL_OBJECT_TEST(Function)
    NULL_OBJECT_TEST(FunctionBuilder)

    NULL_OBJECT_TEST(RuntimeObject)
    NULL_OBJECT_TEST(RuntimeArray)
    NULL_OBJECT_TEST(RuntimeField)
    NULL_OBJECT_TEST(RuntimeStruct)
    NULL_OBJECT_TEST(RuntimeEventFn)
    NULL_OBJECT_TEST(RuntimeNamespace)
    NULL_OBJECT_TEST(JustInTimeRunner)

#undef NULL_OBJECT_TEST
}
