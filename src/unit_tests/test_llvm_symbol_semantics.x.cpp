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

#include "common_llvm_test.h"
#include <format>

using namespace llvm_builder;

void code_fn() {
    CODEGEN_LINE(ValueInfo ctx = ValueInfo::from_context())
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

TEST(LLVM_CODEGEN_SYMBOL_SEMANTICS, basic_graph_test) {
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
    arr_args_fields.emplace_back("b", int32_arr_type.mk_ptr());
    CODEGEN_LINE(TypeInfo arr_args_type = TypeInfo::mk_struct("arr_args", arr_args_fields))

    CODEGEN_LINE(l_cursor.bind(args3_type.mk_ptr()))
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn("sample_fn_name"))
        {
            CODEGEN_LINE(FunctionContext l_fn_ctx{fn})
            CODEGEN_LINE(FunctionContext::mk_ptr("test_var"_cs, int32_type, ValueInfo::from_constant(0)))
            CODEGEN_LINE(FunctionContext::mk_ptr("res5"_cs, int32_type, ValueInfo::from_constant(0)))
            code_fn();
            CODEGEN_LINE(FunctionContext::section_break("another_section"))
            {
                CODEGEN_LINE(ValueInfo ctx = ValueInfo::from_context())
                CODEGEN_LINE(ValueInfo arg4_ptr = ctx.field("arg4"))
                CODEGEN_LINE(ValueInfo arg5_ptr = ctx.field("arg5"))
                CODEGEN_LINE(ValueInfo arg6_ptr = ctx.field("arg6"))
                CODEGEN_LINE(ValueInfo res5 = FunctionContext::pop("res5"))
                CODEGEN_LINE(arg4_ptr.store(res5))
                CODEGEN_LINE(arg5_ptr.store(FunctionContext::pop("test_var")))
                CODEGEN_LINE(arg6_ptr.store(res5))
                CODEGEN_LINE(FunctionContext::set_return_value(res5))
            }
        }
        {
            const std::string fn_name{"sample_fn_name"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
    }
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

static int32_t test_callback_fn(void* arg) {
    return 42;
}

TEST(LLVM_CODEGEN_SYMBOL_SEMANTICS, lazy_load_logic) {
    // NOTE{vibhanshu}: testing if:
    //                   1. load are ordered correctly at the start of function and before any write to them.
    //                   2. load after store *should* read pre-fn call value
    //                   3. multiple store of same expression should be filtered by graph
    //                   4. graph calculation uniqueness filtering are per section (*not* per function)
    CODEGEN_LINE(Cursor l_cursor{"lazy_load_logic"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo fn_type = TypeInfo::mk_fn_type())
    CODEGEN_LINE(TypeInfo fn_ptr_type = fn_type.mk_ptr())

    LLVM_BUILDER_ALWAYS_ASSERT(not fn_type.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn_ptr_type.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(fn_ptr_type.is_valid_struct_field());

    CODEGEN_LINE(TypeInfo l_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("fn_ptr_field", fn_ptr_type))
        CODEGEN_LINE(l_field_list.emplace_back("result", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg3", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg5", int32_type))
        CODEGEN_LINE(l_struct = TypeInfo::mk_struct("fn_ptr_args", l_field_list))
    }
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind(l_struct.mk_ptr()))
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn_double("double_arg"))
        {
            CODEGEN_LINE(FunctionContext l_fn_ctx{fn_double})
            CODEGEN_LINE(ValueInfo ctx = ValueInfo::from_context())
            CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
            CODEGEN_LINE(ctx.field("arg1").store(arg1.add(arg1)))
            CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
            CODEGEN_LINE(ctx.field("arg2").store(arg2.add(arg2)))
            CODEGEN_LINE(ValueInfo arg3 = ctx.field("arg3").load())
            CODEGEN_LINE(ctx.field("arg3").store(arg3.add(arg3)))
            CODEGEN_LINE(ValueInfo arg4 = ctx.field("arg4").load())
            CODEGEN_LINE(ctx.field("arg4").store(arg4.add(arg4)))
            for (int i = 0; i != 5; ++i) {
                CODEGEN_LINE(ValueInfo arg5 = ctx.field("arg5").load())
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5)))
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5).add(arg5)))
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5)))
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5).add(arg5)))
            }
            CODEGEN_LINE(FunctionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        CODEGEN_LINE(Function fn_double_section("double_arg_section"))
        {
            CODEGEN_LINE(FunctionContext l_fn_ctx{fn_double_section})
            for (int i = 0; i != 5; ++i) {
                std::stringstream ss;
                ss << "section_" << i;
                FunctionContext::section_break(ss.str());
                CODEGEN_LINE(ValueInfo ctx = ValueInfo::from_context())
                CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
                CODEGEN_LINE(ctx.field("arg1").store(arg1.add(arg1)))
                CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
                CODEGEN_LINE(ctx.field("arg2").store(arg2.add(arg2)))
                CODEGEN_LINE(ValueInfo arg3 = ctx.field("arg3").load())
                CODEGEN_LINE(ctx.field("arg3").store(arg3.add(arg3)))
                CODEGEN_LINE(ValueInfo arg4 = ctx.field("arg4").load())
                CODEGEN_LINE(ctx.field("arg4").store(arg4.add(arg4)))
                CODEGEN_LINE(ValueInfo arg5 = ctx.field("arg5").load())
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5)))
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5).add(arg5)))
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5)))
                CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5).add(arg5)))
            }
            CODEGEN_LINE(FunctionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        FunctionContext::function().assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
    {
        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        const runtime::Struct& l_args_def = l_runtime_module.struct_info("fn_ptr_args");
        const runtime::EventFn& double_arg = l_runtime_module.event_fn_info("double_arg");
        const runtime::EventFn& double_arg_section = l_runtime_module.event_fn_info("double_arg_section");
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args_def.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not double_arg.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not double_arg_section.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_def.num_fields(), 7);

        const runtime::Field& fn_field = l_args_def["fn_ptr_field"];
        LLVM_BUILDER_ALWAYS_ASSERT(not fn_field.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(fn_field.is_fn_pointer());

        for (int32_t i = 0; i != 10;  ++i) {
            CODEGEN_LINE(runtime::Object l_args_obj = l_args_def.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

            CODEGEN_LINE(l_args_obj.set_fn_ptr("fn_ptr_field", test_callback_fn))
            CODEGEN_LINE(l_args_obj.set<int32_t>("result", 0))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg1", i+1))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg2", i+2))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg3", i+3))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg4", i+4))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg5", i+5))
            CODEGEN_LINE(l_args_obj.freeze())

            CODEGEN_LINE(double_arg.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg1") == 2*(i + 1));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg2") == 2*(i + 2));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg3") == 2*(i + 3));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg4") == 2*(i + 4));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg5") == 3*(i + 5));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("result") == 0);
        }
        for (int32_t i = 0; i != 10;  ++i) {
            CODEGEN_LINE(runtime::Object l_args_obj = l_args_def.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

            CODEGEN_LINE(l_args_obj.set_fn_ptr("fn_ptr_field", test_callback_fn))
            CODEGEN_LINE(l_args_obj.set<int32_t>("result", 0))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg1", i+1))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg2", i+2))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg3", i+3))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg4", i+4))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg5", i+5))
            CODEGEN_LINE(l_args_obj.freeze())

            CODEGEN_LINE(double_arg_section.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg1") == 32*(i + 1));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg2") == 32*(i + 2));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg3") == 32*(i + 3));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg4") == 32*(i + 4));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("arg5") == 243*(i + 5));
            LLVM_BUILDER_ALWAYS_ASSERT(l_args_obj.get<int32_t>("result") == 0);
        }
    }
}

TEST(LLVM_CODEGEN_SYMBOL_SEMANTICS, value_sink_ordering_logic) {
    // NOTE{vibhanshu}: testing if:
    //                   1. store and fn_call are acting as sink of value.
    //                   2. only the last instance of store on a variable is useful
    //                   3. using output of function call should not trigger function re-eval
    //                   4. all the load will happen before first fn-call and first store
    //                   5. any load even after fun call which changes it's value will load pre-fn value
    CODEGEN_LINE(Cursor l_cursor{"value_sink_ordering_logic"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo fn_type = TypeInfo::mk_fn_type())
    CODEGEN_LINE(TypeInfo fn_ptr_type = fn_type.mk_ptr())

    LLVM_BUILDER_ALWAYS_ASSERT(not fn_type.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn_ptr_type.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(fn_ptr_type.is_valid_struct_field());

    CODEGEN_LINE(TypeInfo l_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("fn_ptr_field", fn_ptr_type))
        CODEGEN_LINE(l_field_list.emplace_back("result", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg3", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("arg5", int32_type))
        CODEGEN_LINE(l_struct = TypeInfo::mk_struct("fn_ptr_args", l_field_list))
    }
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind(l_struct.mk_ptr()))
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn_double("double_arg"))
        {
            CODEGEN_LINE(FunctionContext l_fn_ctx{fn_double})
            CODEGEN_LINE(ValueInfo ctx = ValueInfo::from_context())
            CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
            CODEGEN_LINE(ctx.field("arg1").store(arg1.add(arg1)))
            CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
            CODEGEN_LINE(ctx.field("arg2").store(arg2.add(arg2)))
            CODEGEN_LINE(ValueInfo arg3 = ctx.field("arg3").load())
            CODEGEN_LINE(ctx.field("arg3").store(arg3.add(arg3)))
            CODEGEN_LINE(ValueInfo arg4 = ctx.field("arg4").load())
            CODEGEN_LINE(ctx.field("arg4").store(arg4.add(arg4)))
            CODEGEN_LINE(ValueInfo arg5 = ctx.field("arg5").load())
            CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5)))
            CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5).add(arg5)))
            CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5)))
            CODEGEN_LINE(ctx.field("arg5").store(arg5.add(arg5).add(arg5)))
            CODEGEN_LINE(FunctionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        {
            CODEGEN_LINE(Function fn("call_fn_ptr"))
            CODEGEN_LINE(FunctionContext l_fn_ctx{fn})

            CODEGEN_LINE(ValueInfo ctx = ValueInfo::from_context())
            CODEGEN_LINE(ValueInfo arg4 = ctx.field("arg4").load())
            CODEGEN_LINE(ctx.field("arg4").store(arg4.add(ValueInfo::from_constant(1000))))
            CODEGEN_LINE(ValueInfo call_fn_1 = fn_double.call_fn())
            CODEGEN_LINE(ValueInfo call_fn_2 = fn_double.call_fn())
            CODEGEN_LINE(ValueInfo call_fn_3 = fn_double.call_fn())
            CODEGEN_LINE(ctx.field("arg3").store(call_fn_1.add(call_fn_2)))
            CODEGEN_LINE(ctx.field("arg3").store(call_fn_1.add(call_fn_2)))
            CODEGEN_LINE(ctx.field("arg3").store(call_fn_1.add(call_fn_2)))
            CODEGEN_LINE(ValueInfo fn_ptr_val = ctx.field("fn_ptr_field").load())
            CODEGEN_LINE(ValueInfo call_result_1 = fn_ptr_val.call_fn())
            CODEGEN_LINE(ValueInfo call_result_2 = fn_ptr_val.call_fn())
            CODEGEN_LINE(ctx.field("result").store(call_result_1))
            CODEGEN_LINE(ctx.field("arg2").store(ValueInfo::from_constant(100)))
            for (int i = 0; i != 10; ++i) {
                CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
                CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
                CODEGEN_LINE(ValueInfo sum2 = arg1 + arg2)
                CODEGEN_LINE(ctx.field("arg2").store(sum2))
            }
            CODEGEN_LINE(FunctionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        {
            const std::string fn_name{"call_fn_ptr"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
        FunctionContext::function().assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
    {
        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        const runtime::Struct& l_args_def = l_runtime_module.struct_info("fn_ptr_args");
        const runtime::EventFn& double_arg = l_runtime_module.event_fn_info("double_arg");
        const runtime::EventFn& call_fn_ptr = l_runtime_module.event_fn_info("call_fn_ptr");
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args_def.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not double_arg.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not call_fn_ptr.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_def.num_fields(), 7);

        const runtime::Field& fn_field = l_args_def["fn_ptr_field"];
        LLVM_BUILDER_ALWAYS_ASSERT(not fn_field.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(fn_field.is_fn_pointer());

        for (int32_t i = 0; i != 5; ++i) {
            CODEGEN_LINE(runtime::Object l_args_obj = l_args_def.mk_object())
            CODEGEN_LINE(l_args_obj.set_fn_ptr("fn_ptr_field", test_callback_fn))
            CODEGEN_LINE(l_args_obj.set<int32_t>("result", 0))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg1", i+1))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg2", i+2))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg3", i+3))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg4", i+4))
            CODEGEN_LINE(l_args_obj.set<int32_t>("arg5", i+5))
            CODEGEN_LINE(l_args_obj.freeze())

            auto* retrieved_fn = l_args_obj.get_fn_ptr("fn_ptr_field");
            LLVM_BUILDER_ALWAYS_ASSERT_EQ((void*)retrieved_fn, (void*)test_callback_fn);

            const int32_t l_pre_arg1 = l_args_obj.get<int32_t>("arg1");
            const int32_t l_pre_arg2 = l_args_obj.get<int32_t>("arg2");
            const int32_t l_pre_arg3 = l_args_obj.get<int32_t>("arg3");
            const int32_t l_pre_arg4 = l_args_obj.get<int32_t>("arg4");
            const int32_t l_pre_arg5 = l_args_obj.get<int32_t>("arg5");
            const int32_t l_sum = l_pre_arg1 + l_pre_arg2;
            CODEGEN_LINE(call_fn_ptr.on_event(l_args_obj))
            const int32_t l_post_arg1 = l_args_obj.get<int32_t>("arg1");
            const int32_t l_post_arg2 = l_args_obj.get<int32_t>("arg2");
            const int32_t l_post_arg3 = l_args_obj.get<int32_t>("arg3");
            const int32_t l_post_arg4 = l_args_obj.get<int32_t>("arg4");
            const int32_t l_post_arg5 = l_args_obj.get<int32_t>("arg5");
            LLVM_BUILDER_ALWAYS_ASSERT(l_post_arg1 == 8 * l_pre_arg1);
            LLVM_BUILDER_ALWAYS_ASSERT(l_post_arg2 == l_sum);
            LLVM_BUILDER_ALWAYS_ASSERT(l_post_arg3 == 0);
            LLVM_BUILDER_ALWAYS_ASSERT(l_post_arg4 == 8 *(l_pre_arg4 + 1000));
            LLVM_BUILDER_ALWAYS_ASSERT(l_post_arg5 == 27 * l_pre_arg5);
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("result"), 42);
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
        }
    }
}

TEST(LLVM_CODEGEN_SYMBOL_SEMANTICS, hierarchical_def) {
    CODEGEN_LINE(Cursor l_cursor{"hierarchical_def"})
    {
        CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
        CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())

        std::vector<member_field_entry> args3_fields;
        args3_fields.emplace_back("arg1", int32_type);
        args3_fields.emplace_back("arg2", int32_type);
        args3_fields.emplace_back("arg1_fn_bkp", int32_type);
        args3_fields.emplace_back("arg2_fn_bkp", int32_type);
        args3_fields.emplace_back("arg1_bkp_pre", int32_type);
        args3_fields.emplace_back("arg2_bkp_pre", int32_type);
        args3_fields.emplace_back("arg1_bkp_post", int32_type);
        args3_fields.emplace_back("arg2_bkp_post", int32_type);
        args3_fields.emplace_back("arg1_bkp_section_post", int32_type);
        args3_fields.emplace_back("arg2_bkp_section_post", int32_type);
        args3_fields.emplace_back("arr_idx", int32_type);
        args3_fields.emplace_back("arg_arr1", TypeInfo::mk_array(int32_type, 5).mk_ptr());
        args3_fields.emplace_back("arg_arr2", TypeInfo::mk_array(int32_type, 5).mk_ptr());
        CODEGEN_LINE(TypeInfo args3_type = TypeInfo::mk_struct("args3", args3_fields))

        CODEGEN_LINE(l_cursor.bind(args3_type.mk_ptr()))

        //
        // computation graph BEGIN
        //
        CODEGEN_LINE(ValueInfo ctx = ValueInfo::from_context())
        CODEGEN_LINE(ValueInfo arr_idx = ctx.field("arr_idx").load())

        CODEGEN_LINE(ValueInfo arg1_ptr = ctx.field("arg1"))
        CODEGEN_LINE(ValueInfo arg2_ptr = ctx.field("arg2"))
        CODEGEN_LINE(ValueInfo arg1_fn_bkp_ptr = ctx.field("arg1_fn_bkp"))
        CODEGEN_LINE(ValueInfo arg2_fn_bkp_ptr = ctx.field("arg2_fn_bkp"))
        CODEGEN_LINE(ValueInfo arg1_bkp_pre_ptr = ctx.field("arg1_bkp_pre"))
        CODEGEN_LINE(ValueInfo arg2_bkp_pre_ptr = ctx.field("arg2_bkp_pre"))
        CODEGEN_LINE(ValueInfo arg1_bkp_post_ptr = ctx.field("arg1_bkp_post"))
        CODEGEN_LINE(ValueInfo arg2_bkp_post_ptr = ctx.field("arg2_bkp_post"))
        CODEGEN_LINE(ValueInfo arg1_bkp_section_post_ptr = ctx.field("arg1_bkp_section_post"))
        CODEGEN_LINE(ValueInfo arg2_bkp_section_post_ptr = ctx.field("arg2_bkp_section_post"))
        // CODEGEN_LINE(ValueInfo arr_idx = ctx.field("arr_idx").load())
        CODEGEN_LINE(ValueInfo arg_arr1_ptr = ctx.field("arg_arr1").load())
        CODEGEN_LINE(ValueInfo arg_arr2_ptr = ctx.field("arg_arr2").load())
        CODEGEN_LINE(ValueInfo arg1 = arg1_ptr.load())
        CODEGEN_LINE(ValueInfo arg2 = arg2_ptr.load())
        //
        //  computation graph END
        //

        CODEGEN_LINE(Module l_module = l_cursor.main_module())
        CODEGEN_LINE(Module::Context l_module_ctx{l_module})
        CODEGEN_LINE(Function on_event_fn("on_big_event"))
        CODEGEN_LINE(Function fibo_fn("fibonacci_calc"))
        CODEGEN_LINE(Function arr_copy_fn("arr_copy_fn"))

        CODEGEN_LINE(FunctionContext::set_fn(on_event_fn))

        {
            CODEGEN_LINE(FunctionContext l_local_fibo_ctx{fibo_fn})
            CODEGEN_LINE(arg1_ptr.store(arg2))
            CODEGEN_LINE(arg2_ptr.store(arg1.add(arg2)))
            CODEGEN_LINE(arg1_fn_bkp_ptr.store(arg1))
            CODEGEN_LINE(arg2_fn_bkp_ptr.store(arg2))
            CODEGEN_LINE(FunctionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        {
            CODEGEN_LINE(FunctionContext l_local_fn_ctx{arr_copy_fn})
            CODEGEN_LINE(arg_arr2_ptr.entry(arr_idx).store(arg_arr1_ptr.entry(arr_idx).load()))
            CODEGEN_LINE(FunctionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        CODEGEN_LINE(arg1_bkp_pre_ptr.store(arg1))
        CODEGEN_LINE(arg2_bkp_pre_ptr.store(arg2))
        CODEGEN_LINE(fibo_fn.call_fn())
        CODEGEN_LINE(arr_copy_fn.call_fn())
        CODEGEN_LINE(arg1_bkp_post_ptr.store(arg1))
        CODEGEN_LINE(arg2_bkp_post_ptr.store(arg2))

        CODEGEN_LINE(FunctionContext::section_break("section_break"))

        CODEGEN_LINE(arg1_bkp_section_post_ptr.store(arg1))
        CODEGEN_LINE(arg2_bkp_section_post_ptr.store(arg2))

        CODEGEN_LINE(FunctionContext::set_return_value(arg1_ptr.load()))
        // CODEGEN_LINE(FunctionContext::set_return_value(ValueInfo::from_constant(0)))
        {
            const std::string fn_name{"on_big_event"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
    }

    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error())
    {
        CODEGEN_LINE(JustInTimeRunner jit_runner)
        CODEGEN_LINE(jit_runner.add_module(l_cursor))
        CODEGEN_LINE(jit_runner.bind())
        LLVM_BUILDER_ALWAYS_ASSERT(not jit_runner.has_error())

        CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
        CODEGEN_LINE(const runtime::Struct& l_args3_struct = l_runtime_module.struct_info("args3"))
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
        CODEGEN_LINE(const runtime::EventFn& sample_fn = l_runtime_module.event_fn_info("on_big_event"))
        LLVM_BUILDER_ALWAYS_ASSERT(not sample_fn.has_error());

        {
            CODEGEN_LINE(runtime::Object l_args_obj = l_args3_struct.mk_object())
            CODEGEN_LINE(runtime::Array l_arg_arr1 = runtime::Array::from(runtime::type_t::int32, 5))
            CODEGEN_LINE(runtime::Array l_arg_arr2 = runtime::Array::from(runtime::type_t::int32, 5))
            CODEGEN_LINE(l_arg_arr1.freeze())
            CODEGEN_LINE(l_arg_arr2.freeze())
            CODEGEN_LINE(l_args_obj.set_array("arg_arr1", l_arg_arr1))
            CODEGEN_LINE(l_args_obj.set_array("arg_arr2", l_arg_arr2))
            bool is_frozen = l_args_obj.freeze();
            LLVM_BUILDER_ASSERT(is_frozen);
            l_args_obj.set<int32_t>("arg1", 1);
            l_args_obj.set<int32_t>("arg2", 1);
            for (int32_t i = 0; i != 30; ++i) {

                LLVM_BUILDER_ASSERT(not l_arg_arr1.has_error());
                LLVM_BUILDER_ASSERT(not l_arg_arr2.has_error());
                const int32_t l_arg1_prev = l_args_obj.get<int32_t>("arg1");
                const int32_t l_arg2_prev = l_args_obj.get<int32_t>("arg2");
                l_args_obj.set<int32_t>("arg1_fn_bkp", i + 2);
                l_args_obj.set<int32_t>("arr_idx", i % 5);
                for (uint32_t j = 0; j != 5; ++j) {
                    l_arg_arr1.set<int32_t>(j, i * 10 + 1);
                    l_arg_arr2.set<int32_t>(j, -1);
                }
                const int32_t new_value = l_arg1_prev + l_arg2_prev;
                LLVM_BUILDER_ALWAYS_ASSERT(not l_args_obj.has_error())
                CODEGEN_LINE(int32_t result = sample_fn.on_event(l_args_obj))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg2_prev, l_args_obj.get<int32_t>("arg1"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(new_value, l_args_obj.get<int32_t>("arg2"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg1_prev, l_args_obj.get<int32_t>("arg1_fn_bkp"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg2_prev, l_args_obj.get<int32_t>("arg2_fn_bkp"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg1_prev, l_args_obj.get<int32_t>("arg1_bkp_pre"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg2_prev, l_args_obj.get<int32_t>("arg2_bkp_pre"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg1_prev, l_args_obj.get<int32_t>("arg1_bkp_post"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg2_prev, l_args_obj.get<int32_t>("arg2_bkp_post"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("arg1"), l_args_obj.get<int32_t>("arg1_bkp_section_post"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("arg2"), l_args_obj.get<int32_t>("arg2_bkp_section_post"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg_arr1.get<int32_t>(i % 5), l_arg_arr2.get<int32_t>(i % 5))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(result, l_args_obj.get<int32_t>("arg1"));
            }
        }
    }
}
