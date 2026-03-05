//
// Created by vibhanshu on 2025-11-21
//

#include "gtest/gtest.h"

#include "util/debug.h"
#include "llvm_builder/defines.h"

#include "llvm_builder/module.h"
#include "llvm_builder/event.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"

using namespace llvm_builder;
using namespace llvm_builder;

TEST(API_OBJECT, basic_null_test) {
#define NULL_OBJECT_TEST(type_name, sz)                                 \
     {                                                                  \
        type_name null_1 = type_name :: null();                         \
        type_name null_2 = type_name :: null();                         \
        LLVM_BUILDER_ALWAYS_ASSERT(null_1.has_error());                 \
        LLVM_BUILDER_ALWAYS_ASSERT(null_2.has_error());                 \
        LLVM_BUILDER_ALWAYS_ASSERT(null_1 == null_2);                   \
        static_assert(sizeof(type_name) == sz);                         \
    }                                                                   \
 /**/

    NULL_OBJECT_TEST(Cursor, 56)
    NULL_OBJECT_TEST(Module, 56)

    NULL_OBJECT_TEST(TypeInfo, 56)
    NULL_OBJECT_TEST(Event, 56)
    NULL_OBJECT_TEST(field_entry_t, 144)
    NULL_OBJECT_TEST(ValueInfo, 56)

    NULL_OBJECT_TEST(LinkSymbolName, 144)
    NULL_OBJECT_TEST(LinkSymbol, 272)

    NULL_OBJECT_TEST(CodeSection, 56)
    NULL_OBJECT_TEST(Function, 56)

    NULL_OBJECT_TEST(runtime::Object, 56)
    NULL_OBJECT_TEST(runtime::Array, 56)
    NULL_OBJECT_TEST(runtime::Field, 56)
    NULL_OBJECT_TEST(runtime::Struct, 56)
    NULL_OBJECT_TEST(runtime::EventFn, 56)
    NULL_OBJECT_TEST(runtime::Namespace, 56)
    NULL_OBJECT_TEST(JustInTimeRunner, 56)

#undef NULL_OBJECT_TEST
}
