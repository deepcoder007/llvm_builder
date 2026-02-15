//
// Created by vibhanshu on 2026-02-12
//

#include "gtest/gtest.h"
#include <cstdint>
#include "util/debug.h"
#include "util/cstring.h"
#include "llvm_builder/defines.h"

#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"
#include "llvm_builder/control_flow.h"

#include "common_llvm_test.h"
#include <format>

using namespace llvm_builder;
#include <format>

void code_fn() {
    CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
    CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
    CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
    CODEGEN_LINE(ValueInfo c1 = ValueInfo::from_constant(101))
    CODEGEN_LINE(ValueInfo c2 = ValueInfo::from_constant(999))
    CODEGEN_LINE(ValueInfo res = arg1.add(arg2))
    CODEGEN_LINE(ValueInfo res2 = res.add(c1))
    CODEGEN_LINE(ValueInfo res3 = res2.add(c2))
    CODEGEN_LINE(ValueInfo res4 = res2.add(ctx.field("arg3").load()))
    CODEGEN_LINE(ValueInfo res5 = res3.add(res4))
    CODEGEN_LINE(ValueInfo arg4_ptr = ctx.field("arg4"))
    CODEGEN_LINE(ValueInfo arg5_ptr = ctx.field("arg5"))
    CODEGEN_LINE(ValueInfo arg6_ptr = ctx.field("arg6"))
    CODEGEN_LINE(res5.push("res5"))
    CODEGEN_LINE(res5.add(res5).push("test_var"))
    CODEGEN_LINE(arg4_ptr.store(res5))
    CODEGEN_LINE(arg5_ptr.store(res5))
    CODEGEN_LINE(arg4_ptr.store(res5))
    CODEGEN_LINE(arg5_ptr.store(res5))
}

TEST(LLVM_CODEGEN, basic_graph_test) {
    CODEGEN_LINE(Cursor l_cursor{"basic_graph_test"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    CODEGEN_LINE(TypeInfo int32_arr_type = TypeInfo::mk_array(int32_type, 5))

    std::vector<member_field_entry> args3_fields;
    for (int i = 1; i != 10; ++i) {
        args3_fields.emplace_back(std::format("arg{}", i), int32_type);
    }
    CODEGEN_LINE(TypeInfo args3_type = TypeInfo::mk_struct("args3", args3_fields))

    std::vector<member_field_entry> abc_fields;
    abc_fields.emplace_back("a", int32_type);
    abc_fields.emplace_back("b", int32_type);
    abc_fields.emplace_back("c", int32_type);
    CODEGEN_LINE(TypeInfo abc_type = TypeInfo::mk_struct("abc_args", abc_fields))

    std::vector<member_field_entry> arr_args_fields;
    arr_args_fields.emplace_back("a", int32_type);
    arr_args_fields.emplace_back("b", int32_arr_type.pointer_type());
    CODEGEN_LINE(TypeInfo arr_args_type = TypeInfo::mk_struct("arr_args", arr_args_fields))

    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn("sample_fn_name", FnContext{args3_type.pointer_type()}, l_module))
        {
            CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
            CODEGEN_LINE(l_fn_body.enter())
            CODEGEN_LINE(CodeSectionContext::mk_ptr("test_var"_cs, int32_type, ValueInfo::from_constant(0)))
            CODEGEN_LINE(CodeSectionContext::mk_ptr("res5"_cs, int32_type, ValueInfo::from_constant(0)))
            code_fn();
            CODEGEN_LINE(CodeSectionContext::section_break("another_section"))
            {
                CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
                CODEGEN_LINE(ValueInfo arg4_ptr = ctx.field("arg4"))
                CODEGEN_LINE(ValueInfo arg5_ptr = ctx.field("arg5"))
                CODEGEN_LINE(ValueInfo arg6_ptr = ctx.field("arg6"))
                CODEGEN_LINE(ValueInfo res5 = CodeSectionContext::pop("res5"))
                CODEGEN_LINE(arg4_ptr.store(res5))
                CODEGEN_LINE(arg5_ptr.store(CodeSectionContext::pop("test_var")))
                CODEGEN_LINE(arg6_ptr.store(res5))
                CODEGEN_LINE(CodeSectionContext::set_return_value(res5))
            }
        }
        {
            const std::string fn_name{"sample_fn_name"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
    }
    l_module.write_to_file();
    {
        LLVM_BUILDER_ALWAYS_ASSERT(l_module.is_init());
        CODEGEN_LINE(JustInTimeRunner jit_runner)
        CODEGEN_LINE(jit_runner.add_module(l_cursor))
        LLVM_BUILDER_ALWAYS_ASSERT(not l_module.is_init());
        CODEGEN_LINE(jit_runner.bind())

        CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
        CODEGEN_LINE(const runtime::Struct& l_args3_struct = l_runtime_module.struct_info("args3"))
        CODEGEN_LINE(const runtime::Struct& l_abc_struct = l_runtime_module.struct_info("abc_args"))
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
        CODEGEN_LINE(const runtime::EventFn& sample_fn = l_runtime_module.event_fn_info("sample_fn_name"))
        LLVM_BUILDER_ALWAYS_ASSERT(not sample_fn.has_error());
        {
            for (int32_t i = 0; i != 10; ++i) {
                CODEGEN_LINE(runtime::Object l_args_obj = l_args3_struct.mk_object())

                l_args_obj.set<int32_t>("arg1", i);
                l_args_obj.set<int32_t>("arg2", i + 1);
                l_args_obj.set<int32_t>("arg3", i + 2);
                l_args_obj.freeze();

                int32_t c1 = 101;
                int32_t c2 = 999;
                int32_t res = i + (i + 1);
                int32_t res2 = res + c1;
                int32_t res3 = res2 + c2;
                int32_t res4 = res2 + (i + 2);
                int32_t res5 = res3 + res4;

                CODEGEN_LINE(int32_t result = sample_fn.on_event(l_args_obj))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(res5, result);
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(res5, l_args_obj.get<int32_t>("arg4"));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(res5*2, l_args_obj.get<int32_t>("arg5"));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(res5, l_args_obj.get<int32_t>("arg6"));
            }
        }
    }
}
