//
// Created by vibhanshu on 2025-11-21
//

#include "gtest/gtest.h"

#include "util/debug.h"
#include "llvm_builder/defines.h"

#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"

using namespace llvm_builder;
using namespace llvm_builder;

TEST(API_OBJECT, basic_null_test) {
#define NULL_OBJECT_TEST(type_name)                                     \
     {                                                                  \
        type_name null_1 = type_name :: null();                         \
        type_name null_2 = type_name :: null();                         \
        LLVM_BUILDER_ALWAYS_ASSERT(null_1.has_error());                         \
        LLVM_BUILDER_ALWAYS_ASSERT(null_2.has_error());                         \
        LLVM_BUILDER_ALWAYS_ASSERT(null_1 == null_2);                          \
    }                                                                   \
 /**/

    NULL_OBJECT_TEST(Cursor)
    NULL_OBJECT_TEST(Module)

    NULL_OBJECT_TEST(TypeInfo)
    // TODO{vibhanshu}: null of field_entry is a bit different
    // NULL_OBJECT_TEST(TypeInfo::field_entry_t)
    NULL_OBJECT_TEST(ValueInfo)

    NULL_OBJECT_TEST(LinkSymbolName)
    NULL_OBJECT_TEST(LinkSymbol)

    NULL_OBJECT_TEST(FnContext)
    NULL_OBJECT_TEST(CodeSection)
    NULL_OBJECT_TEST(Function)


    NULL_OBJECT_TEST(runtime::Object)
    NULL_OBJECT_TEST(runtime::Array)
    NULL_OBJECT_TEST(runtime::Field)
    NULL_OBJECT_TEST(runtime::Struct)
    NULL_OBJECT_TEST(runtime::EventFn)
    NULL_OBJECT_TEST(runtime::Namespace)
    NULL_OBJECT_TEST(JustInTimeRunner)

#undef NULL_OBJECT_TEST
}
