//
// Created by vibhanshu on 2024-08-10
//

#include "ds/fixed_string.h"
#include "util/debug.h"

#include "gtest/gtest.h"
#include <cstdint>

using namespace llvm_builder;

TEST(LLVM_BUILDER_DS_FIXED_STRING, basic_test) {
    fixed_string str1{"fixed-str-1"};
    fixed_string str2{"fixed-str-2"};
    fixed_string str3{"fixed-str-3"};
    fixed_string str1_alias{"fixed-str-1"};
    fixed_string str_null1{""};
    fixed_string str_null2{""};

    LLVM_BUILDER_ALWAYS_ASSERT(str1 != str2);
    LLVM_BUILDER_ALWAYS_ASSERT(str1 != str3);
    LLVM_BUILDER_ALWAYS_ASSERT(str1 == str1_alias);
    LLVM_BUILDER_ALWAYS_ASSERT(str_null1 == str_null2);

    fixed_string_map<int32_t> str_map;
    str_map[str1] = 1;
    str_map[str2] = 2;
    str_map[str3] = 3;
    str_map[str_null1] = 10;
    LLVM_BUILDER_ALWAYS_ASSERT(str_map.at(str1) == 1);
    LLVM_BUILDER_ALWAYS_ASSERT(str_map.at(str2) == 2);
    LLVM_BUILDER_ALWAYS_ASSERT(str_map.at(str3) == 3);
    LLVM_BUILDER_ALWAYS_ASSERT(str_map.at(str_null2) == 10);
    str_map[str1_alias] = 4;
    LLVM_BUILDER_ALWAYS_ASSERT(str_map.at(str1) == 4);
    LLVM_BUILDER_ALWAYS_ASSERT(str_map.at(str2) == 2);
    LLVM_BUILDER_ALWAYS_ASSERT(str_map.at(str3) == 3);
}
