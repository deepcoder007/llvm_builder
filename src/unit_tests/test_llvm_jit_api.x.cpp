//
// Created by vibhanshu on 2026-01-12
//

#include "gtest/gtest.h"
#include <cstdint>
#include "util/debug.h"
#include "llvm_builder/defines.h"

#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"

#include "common_llvm_test.h"

using namespace llvm_builder;
using namespace llvm_builder;

TEST(LLVM_CODEGEN_JIT_API, hello_api) {
    CODEGEN_LINE(Cursor l_cursor{"jit_api_v1"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo int32_arr5_type = TypeInfo::mk_array(int32_type, 5))
    CODEGEN_LINE(TypeInfo int32_arr5_ptr_type = int32_arr5_type.pointer_type())
    CODEGEN_LINE(TypeInfo int32_2d_type = TypeInfo::mk_array(int32_arr5_ptr_type, 5))
    CODEGEN_LINE(TypeInfo int32_2d_ptr_type = int32_2d_type.pointer_type())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    CODEGEN_LINE(TypeInfo l_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("field_1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_3", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_5", bool_type))
        CODEGEN_LINE(l_struct = TypeInfo::mk_struct("args", l_field_list, false))
    }
    // Create struct type for big_struct_test_fn arguments
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn("test_fn", FnContext{l_struct.pointer_type()}, l_module))

        {
            CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
            CODEGEN_LINE(l_fn_body.enter())

            CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
            LLVM_BUILDER_ALWAYS_ASSERT(l_struct.pointer_type() == ctx.type());
            CODEGEN_LINE(ctx.field("field_3").store(ctx.field("field_1").load()))
            CODEGEN_LINE(ctx.field("field_4").store(ctx.field("field_2").load()))
            CODEGEN_LINE(ctx.field("field_5").store(ValueInfo::from_constant(true)))
            CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        CODEGEN_LINE(jit_runner.process_module_fn(fn))
        {
            const std::string fn_name{"test_fn"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
        INIT_MODULE(l_module)
        CodeSectionContext::assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
    {
        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        const runtime::Struct& l_args = l_runtime_module.struct_info("args");
        const runtime::EventFn& test_fn = l_runtime_module.event_fn_info("test_fn");
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not test_fn.has_error())

        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args.num_fields(), 5);

        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Object l_args_obj = l_args.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

            CODEGEN_LINE(l_args_obj.set<int32_t>("field_1", i))
            CODEGEN_LINE(l_args_obj.set<int32_t>("field_2", i + 1))
            CODEGEN_LINE(l_args_obj.set<int32_t>("field_3", i + 2))
            CODEGEN_LINE(l_args_obj.set<int32_t>("field_4", i + 3))
            CODEGEN_LINE(l_args_obj.freeze())

            CODEGEN_LINE(test_fn.on_event(l_args_obj))

            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("field_1"), i);
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("field_2"), i + 1);
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("field_3"), i);
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("field_4"), i + 1);

            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
        }
    }
}

TEST(LLVM_CODEGEN_JIT_API, inner_struct) {
    CODEGEN_LINE(Cursor l_cursor{"jit_api_v2"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    CODEGEN_LINE(TypeInfo l_inner_struct)
    CODEGEN_LINE(TypeInfo l_outer_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("field_1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_3", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_5", bool_type))
        CODEGEN_LINE(l_inner_struct = TypeInfo::mk_struct("inner_struct", l_field_list, false))
    }
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_3", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_5", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_6", l_inner_struct.pointer_type()))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_7", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_8", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_9", l_inner_struct.pointer_type()))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_10", int32_type))
        CODEGEN_LINE(l_outer_struct = TypeInfo::mk_struct("outer_struct", l_field_list, false))
    }
    // Create struct type for big_struct_test_fn arguments
    std::vector<member_field_entry> args_fields;
    args_fields.emplace_back("arg_inner", l_inner_struct.pointer_type());
    args_fields.emplace_back("arg_outer", l_outer_struct.pointer_type());
    CODEGEN_LINE(TypeInfo args_type = TypeInfo::mk_struct("args", args_fields))
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn("test_fn", FnContext{args_type.pointer_type()}, l_module))

        {
            CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
            CODEGEN_LINE(l_fn_body.enter())

            CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
            CODEGEN_LINE(ValueInfo arg_inner = ctx.field("arg_inner").load())
            CODEGEN_LINE(ValueInfo arg_outer = ctx.field("arg_outer").load())
            LLVM_BUILDER_ALWAYS_ASSERT(l_inner_struct.pointer_type() == arg_inner.type());
            LLVM_BUILDER_ALWAYS_ASSERT(l_outer_struct.pointer_type() == arg_outer.type());

            CODEGEN_LINE(ValueInfo inner_field_6 = arg_outer.field("inner_field_6").load())
            for (const std::string& fname : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4"}}) {
                CODEGEN_LINE(ValueInfo l_output = inner_field_6.field(fname))
                CODEGEN_LINE(ValueInfo l_input = arg_inner.field(fname).load())
                CODEGEN_LINE(l_output.store(l_input))
                CODEGEN_LINE(arg_outer.field(LLVM_BUILDER_CONCAT << "inner_" << fname).store(l_input));
            }
            CODEGEN_LINE(inner_field_6.field("field_5").store(ValueInfo::from_constant(true)))
            CODEGEN_LINE(arg_inner.field("field_3").store(arg_inner.field("field_1").load()))
            CODEGEN_LINE(arg_inner.field("field_4").store(arg_inner.field("field_2").load()))
            CODEGEN_LINE(ValueInfo inner_field_9 = arg_outer.field("inner_field_9").load())
            for (const std::string& fname : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4"}}) {
                CODEGEN_LINE(ValueInfo l_output = inner_field_9.field(fname))
                CODEGEN_LINE(ValueInfo l_input = arg_inner.field(fname).load())
                CODEGEN_LINE(l_output.store(l_input))
            }
            CODEGEN_LINE(inner_field_9.field("field_5").store(ValueInfo::from_constant(true)))
            CODEGEN_LINE(arg_inner.field("field_5").store(ValueInfo::from_constant(true)))
            CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        CODEGEN_LINE(jit_runner.process_module_fn(fn))
        {
            const std::string fn_name{"test_fn"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
        INIT_MODULE(l_module)
        CodeSectionContext::assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
    {
        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        const runtime::Struct& l_args = l_runtime_module.struct_info("args");
        const runtime::Struct& l_outer_struct = l_runtime_module.struct_info("outer_struct");
        const runtime::Struct& l_inner_struct = l_runtime_module.struct_info("inner_struct");
        const runtime::Struct& l_args_struct = l_runtime_module.struct_info("args");
        const runtime::EventFn& test_fn = l_runtime_module.event_fn_info("test_fn");
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_struct.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_inner_struct.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args_struct.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not test_fn.has_error())

        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args.num_fields(), 2);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_struct.num_fields(), 10);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_struct.num_fields(), 5);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_struct.num_fields(), 2);

        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Object l_inner_obj = l_inner_struct.mk_object())
            CODEGEN_LINE(runtime::Object l_inner_obj_pre = l_inner_struct.mk_object())
            CODEGEN_LINE(runtime::Object l_outer_obj = l_outer_struct.mk_object())
            CODEGEN_LINE(runtime::Object l_args_obj = l_args.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_1", i))
            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_2", i + 1))
            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_3", i + 2))
            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_4", i + 3))
            CODEGEN_LINE(l_inner_obj.freeze())
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_1", i))
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_2", i + 1))
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_3", i + 2))
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_4", i + 3))
            CODEGEN_LINE(l_inner_obj_pre.freeze())


            CODEGEN_LINE(runtime::Object l_outer_field_6_obj = l_inner_struct.mk_object())
            CODEGEN_LINE(l_outer_field_6_obj.freeze());
            CODEGEN_LINE(l_outer_obj.set_object("inner_field_6", l_outer_field_6_obj))
            CODEGEN_LINE(runtime::Object l_outer_field_9_obj = l_inner_struct.mk_object())
            CODEGEN_LINE(l_outer_field_9_obj.freeze())
            CODEGEN_LINE(l_outer_obj.set_object("inner_field_9", l_outer_field_9_obj))

            CODEGEN_LINE(l_outer_obj.freeze())
            LLVM_BUILDER_ALWAYS_ASSERT(l_inner_obj.is_frozen());
            LLVM_BUILDER_ALWAYS_ASSERT(l_outer_obj.is_frozen());
            CODEGEN_LINE(l_args_obj.set_object("arg_inner", l_inner_obj))
            CODEGEN_LINE(l_args_obj.set_object("arg_outer", l_outer_obj))
            CODEGEN_LINE(l_args_obj.freeze())

            CODEGEN_LINE(test_fn.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj.get<int32_t>("field_1"), l_inner_obj.get<int32_t>("field_3"))
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj.get<int32_t>("field_2"), l_inner_obj.get<int32_t>("field_4"))
            std::vector<std::string> field_list;
            field_list.emplace_back("field_1");
            field_list.emplace_back("field_2");
            field_list.emplace_back("field_3");
            field_list.emplace_back("field_4");
            for (const std::string& fname : field_list) {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj_pre.get<int32_t>(fname), l_outer_field_6_obj.get<int32_t>(fname))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj_pre.get<int32_t>(fname), l_outer_field_9_obj.get<int32_t>(fname))
            }
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(true, l_inner_obj.get<bool>("field_5"));

            runtime::Object l_outer_field_9_obj_v2 = l_outer_obj.get_object("inner_field_9");
            LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_field_9_obj_v2.has_error());
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_field_9_obj.struct_def(), l_outer_field_9_obj_v2.struct_def());
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_field_9_obj.ref(), l_outer_field_9_obj_v2.ref());
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_field_9_obj, l_outer_field_9_obj_v2);

            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
        }
    }
}

TEST(LLVM_CODEGEN_JIT_API, array_basic_1d) {
    CODEGEN_LINE(Cursor l_cursor{"jit_api_v3"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo int32_arr5_type = TypeInfo::mk_array(int32_type, 5))
    CODEGEN_LINE(TypeInfo int32_arr5_ptr_type = int32_arr5_type.pointer_type())
    CODEGEN_LINE(TypeInfo int32_2d_type = TypeInfo::mk_array(int32_arr5_ptr_type, 5))
    CODEGEN_LINE(TypeInfo int32_2d_ptr_type = int32_2d_type.pointer_type())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    CODEGEN_LINE(TypeInfo l_inner_struct)
    CODEGEN_LINE(TypeInfo l_outer_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("field_1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_3", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("field_5", bool_type))
        CODEGEN_LINE(l_inner_struct = TypeInfo::mk_struct("inner_struct", l_field_list, false))
    }
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_3", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_5", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_6", l_inner_struct.pointer_type()))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_7", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_8", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_9", l_inner_struct.pointer_type()))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_10", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_11", int32_arr5_ptr_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_12", int32_arr5_ptr_type))
        CODEGEN_LINE(l_outer_struct = TypeInfo::mk_struct("outer_struct", l_field_list, false))
    }
    // Create struct type for big_struct_test_fn arguments
    std::vector<member_field_entry> args_fields;
    args_fields.emplace_back("arg_inner", l_inner_struct.pointer_type());
    args_fields.emplace_back("arg_outer", l_outer_struct.pointer_type());
    CODEGEN_LINE(TypeInfo args_type = TypeInfo::mk_struct("args", args_fields))
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn("big_struct_test_fn", FnContext{args_type.pointer_type()}, l_module))

        {
            CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
            CODEGEN_LINE(l_fn_body.enter())

            CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
            CODEGEN_LINE(ValueInfo arg_inner = ctx.field("arg_inner").load())
            CODEGEN_LINE(ValueInfo arg_outer = ctx.field("arg_outer").load())
            LLVM_BUILDER_ALWAYS_ASSERT(l_inner_struct.pointer_type() == arg_inner.type());
            LLVM_BUILDER_ALWAYS_ASSERT(l_outer_struct.pointer_type() == arg_outer.type());

            CODEGEN_LINE(ValueInfo inner_field_6 = arg_outer.field("inner_field_6").load())
            for (const std::string& fname : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4"}}) {
                CODEGEN_LINE(ValueInfo l_output = inner_field_6.field(fname))
                CODEGEN_LINE(ValueInfo l_input = arg_inner.field(fname).load())
                CODEGEN_LINE(l_output.store(l_input))
                CODEGEN_LINE(arg_outer.field(LLVM_BUILDER_CONCAT << "inner_" << fname).store(l_input));
            }
            CODEGEN_LINE(inner_field_6.field("field_5").store(ValueInfo::from_constant(true)))
            CODEGEN_LINE(arg_inner.field("field_3").store(arg_inner.field("field_1").load()))
            CODEGEN_LINE(arg_inner.field("field_4").store(arg_inner.field("field_2").load()))
            CODEGEN_LINE(ValueInfo inner_field_9 = arg_outer.field("inner_field_9").load())
            for (const std::string& fname : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4"}}) {
                CODEGEN_LINE(ValueInfo l_output = inner_field_9.field(fname))
                CODEGEN_LINE(ValueInfo l_input = arg_inner.field(fname).load())
                CODEGEN_LINE(l_output.store(l_input))
            }
            CODEGEN_LINE(ValueInfo inner_field_11 = arg_outer.field("inner_field_11").load())
            CODEGEN_LINE(ValueInfo inner_field_12 = arg_outer.field("inner_field_12").load())
            for (int i = 0; i != 5; ++i) {
                CODEGEN_LINE(ValueInfo l_src_entry = inner_field_11.entry(i).load())
                CODEGEN_LINE(ValueInfo l_dst_entry = inner_field_12.entry(i))
                CODEGEN_LINE(l_dst_entry.store(l_src_entry))
            }
            CODEGEN_LINE(inner_field_9.field("field_5").store(ValueInfo::from_constant(true)))
            CODEGEN_LINE(arg_inner.field("field_5").store(ValueInfo::from_constant(true)))
            CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        CODEGEN_LINE(jit_runner.process_module_fn(fn))
        {
            const std::string fn_name{"big_struct_test_fn"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
        INIT_MODULE(l_module)
        CodeSectionContext::assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
    {
        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        const runtime::Struct& l_args = l_runtime_module.struct_info("args");
        const runtime::Struct& l_outer_struct = l_runtime_module.struct_info("outer_struct");
        const runtime::Struct& l_inner_struct = l_runtime_module.struct_info("inner_struct");
        const runtime::Struct& l_args_struct = l_runtime_module.struct_info("args");
        const runtime::EventFn& test_fn = l_runtime_module.event_fn_info("big_struct_test_fn");
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_struct.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_inner_struct.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args_struct.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not test_fn.has_error())

        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args.num_fields(), 2);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_struct.num_fields(), 12);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_struct.num_fields(), 5);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_struct.num_fields(), 2);

        for (int32_t i = 0; i != 30; ++i) {
            CODEGEN_LINE(runtime::Object l_inner_obj = l_inner_struct.mk_object())
            CODEGEN_LINE(runtime::Object l_inner_obj_pre = l_inner_struct.mk_object())
            CODEGEN_LINE(runtime::Object l_outer_obj = l_outer_struct.mk_object())
            CODEGEN_LINE(runtime::Object l_args_obj = l_args.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_1", i))
            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_2", i + 1))
            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_3", i + 2))
            CODEGEN_LINE(l_inner_obj.set<int32_t>("field_4", i + 3))
            CODEGEN_LINE(l_inner_obj.freeze())
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_1", i))
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_2", i + 1))
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_3", i + 2))
            CODEGEN_LINE(l_inner_obj_pre.set<int32_t>("field_4", i + 3))
            CODEGEN_LINE(l_inner_obj_pre.freeze())

            CODEGEN_LINE(runtime::Object l_outer_field_6_obj = l_inner_struct.mk_object())
            CODEGEN_LINE(l_outer_field_6_obj.freeze());
            CODEGEN_LINE(l_outer_obj.set_object("inner_field_6", l_outer_field_6_obj))
            CODEGEN_LINE(runtime::Object l_outer_field_9_obj = l_inner_struct.mk_object())
            CODEGEN_LINE(l_outer_field_9_obj.freeze())
            CODEGEN_LINE(l_outer_obj.set_object("inner_field_9", l_outer_field_9_obj))
            CODEGEN_LINE(runtime::Array l_outer_field_11_obj = runtime::Array::from(runtime::type_t::int32, 10))
            for (uint32_t i = 0; i != 10; ++i) {
                CODEGEN_LINE(l_outer_field_11_obj.set<int32_t>(i, i*10));
            }
            CODEGEN_LINE(l_outer_field_11_obj.freeze())
            CODEGEN_LINE(l_outer_obj.set_array("inner_field_11", l_outer_field_11_obj))
            CODEGEN_LINE(runtime::Array l_outer_field_12_obj = runtime::Array::from(runtime::type_t::int32, 10))
            CODEGEN_LINE(l_outer_field_12_obj.freeze())
            CODEGEN_LINE(l_outer_obj.set_array("inner_field_12", l_outer_field_12_obj))
            CODEGEN_LINE(l_outer_obj.freeze())

            LLVM_BUILDER_ALWAYS_ASSERT(l_inner_obj.is_frozen());
            LLVM_BUILDER_ALWAYS_ASSERT(l_outer_obj.is_frozen());
            CODEGEN_LINE(l_args_obj.set_object("arg_inner", l_inner_obj))
            CODEGEN_LINE(l_args_obj.set_object("arg_outer", l_outer_obj))
            CODEGEN_LINE(l_args_obj.freeze())
            CODEGEN_LINE(test_fn.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj.get<int32_t>("field_1"), l_inner_obj.get<int32_t>("field_3"))
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj.get<int32_t>("field_2"), l_inner_obj.get<int32_t>("field_4"))
            std::vector<std::string> field_list;
            field_list.emplace_back("field_1");
            field_list.emplace_back("field_2");
            field_list.emplace_back("field_3");
            field_list.emplace_back("field_4");
            for (const std::string& fname : field_list) {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj_pre.get<int32_t>(fname), l_outer_field_6_obj.get<int32_t>(fname))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_inner_obj_pre.get<int32_t>(fname), l_outer_field_9_obj.get<int32_t>(fname))
            }
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(true, l_inner_obj.get<bool>("field_5"));

            runtime::Object l_outer_field_9_obj_v2 = l_outer_obj.get_object("inner_field_9");
            LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_field_9_obj_v2.has_error());
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_field_9_obj.struct_def(), l_outer_field_9_obj_v2.struct_def());
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_field_9_obj.ref(), l_outer_field_9_obj_v2.ref());
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_field_9_obj, l_outer_field_9_obj_v2);

            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
        }
    }
}

TEST(LLVM_CODEGEN_JIT_API, full_test) {
    CODEGEN_LINE(Cursor l_cursor{"jit_api_2D_arr"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo int32_arr5_type = TypeInfo::mk_array(int32_type, 5))
    CODEGEN_LINE(TypeInfo int32_arr5_ptr_type = int32_arr5_type.pointer_type())
    CODEGEN_LINE(TypeInfo int32_2d_type = TypeInfo::mk_array(int32_arr5_ptr_type, 5))
    CODEGEN_LINE(TypeInfo int32_2d_ptr_type = int32_2d_type.pointer_type())
    CODEGEN_LINE(TypeInfo l_outer_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_2", int32_arr5_ptr_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_3", int32_2d_ptr_type))
        CODEGEN_LINE(l_outer_struct = TypeInfo::mk_struct("outer_struct", l_field_list, false))
    }
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn("test_fn", FnContext{l_outer_struct.pointer_type()}, l_module))

        {
            CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
            CODEGEN_LINE(l_fn_body.enter())

            CODEGEN_LINE(ValueInfo arg_outer = CodeSectionContext::current_context())
            LLVM_BUILDER_ALWAYS_ASSERT(l_outer_struct.pointer_type() == arg_outer.type());

            CODEGEN_LINE(ValueInfo inner_field_2 = arg_outer.field("inner_field_2").load())
            CODEGEN_LINE(ValueInfo inner_field_3 = arg_outer.field("inner_field_3").load())
            for (int i = 0; i != 5; ++i) {
                CODEGEN_LINE(ValueInfo l_src_entry = inner_field_3.entry(1).load().entry(i).load())
                CODEGEN_LINE(ValueInfo l_dst_entry = inner_field_2.entry(i))
                CODEGEN_LINE(l_dst_entry.store(l_src_entry))
                CODEGEN_LINE(ValueInfo l_dst_entry_2 = inner_field_3.entry(3).load().entry(i))
                CODEGEN_LINE(l_dst_entry_2.store(l_src_entry))
            }
            CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        CODEGEN_LINE(jit_runner.process_module_fn(fn))
        {
            const std::string fn_name{"test_fn"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
        INIT_MODULE(l_module)
        CodeSectionContext::assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
    {
        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        const runtime::Struct& l_outer_struct = l_runtime_module.struct_info("outer_struct");
        const runtime::EventFn& test_fn = l_runtime_module.event_fn_info("test_fn");
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_struct.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not test_fn.has_error())

        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_outer_struct.num_fields(), 3);

        for (int32_t i = 0; i != 100; ++i) {
            CODEGEN_LINE(runtime::Object l_outer_obj = l_outer_struct.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

            CODEGEN_LINE(runtime::Array l_outer_field_2_obj = runtime::Array::from(runtime::type_t::int32, 10))
            CODEGEN_LINE(l_outer_field_2_obj.freeze())
            CODEGEN_LINE(l_outer_obj.set_array("inner_field_2", l_outer_field_2_obj))
            CODEGEN_LINE(runtime::Array l_outer_field_3_obj = runtime::Array::from(runtime::type_t::pointer_array, 10))
            for (int i = 0; i != 10; ++i) {
                CODEGEN_LINE(runtime::Array l_arr = runtime::Array::from(runtime::type_t::int32, 10))
                for (int j = 0; j != 10; ++j) {
                    CODEGEN_LINE(l_arr.set<int32_t>(j, i * j * 10));
                }
                CODEGEN_LINE(l_arr.freeze())
                CODEGEN_LINE(l_outer_field_3_obj.set_array(i, l_arr))
            }
            CODEGEN_LINE(l_outer_field_3_obj.freeze())
            LLVM_BUILDER_ALWAYS_ASSERT(l_outer_field_3_obj.is_frozen())
            CODEGEN_LINE(l_outer_obj.set_array("inner_field_3", l_outer_field_3_obj))
            CODEGEN_LINE(l_outer_obj.freeze())

            LLVM_BUILDER_ALWAYS_ASSERT(l_outer_obj.is_frozen());
            CODEGEN_LINE(test_fn.on_event(l_outer_obj))

            runtime::Array src_arr = l_outer_field_3_obj.get_array(1);
            runtime::Array dst_arr = l_outer_field_3_obj.get_array(3);
            for (uint32_t i = 0; i != 5; ++i) {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(src_arr.get<int32_t>(i), l_outer_field_2_obj.get<int32_t>(i));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(src_arr.get<int32_t>(i), dst_arr.get<int32_t>(i));
            }
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
        }
    }


}

static int32_t test_callback_fn(void* /*arg*/) {
    return 42;
}

TEST(LLVM_CODEGEN_JIT_API, fn_pointer) {
    CODEGEN_LINE(Cursor l_cursor{"jit_api_fn_ptr"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo fn_type = TypeInfo::mk_fn_type())
    CODEGEN_LINE(TypeInfo fn_ptr_type = fn_type.pointer_type())

    LLVM_BUILDER_ALWAYS_ASSERT(not fn_type.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn_ptr_type.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(fn_ptr_type.is_valid_struct_field());

    CODEGEN_LINE(TypeInfo l_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("fn_ptr_field", fn_ptr_type))
        CODEGEN_LINE(l_field_list.emplace_back("result", int32_type))
        CODEGEN_LINE(l_struct = TypeInfo::mk_struct("fn_ptr_args", l_field_list))
    }
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn("call_fn_ptr", FnContext{l_struct.pointer_type()}, l_module))
        {
            CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("body"))
            CODEGEN_LINE(l_fn_body.enter())

            CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
            CODEGEN_LINE(ValueInfo fn_ptr_val = ctx.field("fn_ptr_field").load())
            CODEGEN_LINE(ValueInfo call_result = fn_ptr_val.call_fn(ctx))
            CODEGEN_LINE(ctx.field("result").store(call_result))
            CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
        }
        CODEGEN_LINE(jit_runner.process_module_fn(fn))
        {
            const std::string fn_name{"call_fn_ptr"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
        INIT_MODULE(l_module)
        CodeSectionContext::assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
    {
        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        const runtime::Struct& l_args_def = l_runtime_module.struct_info("fn_ptr_args");
        const runtime::EventFn& call_fn_ptr = l_runtime_module.event_fn_info("call_fn_ptr");
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not l_args_def.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT(not call_fn_ptr.has_error())
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_def.num_fields(), 2);

        // Verify field type detection
        const runtime::Field& fn_field = l_args_def["fn_ptr_field"];
        LLVM_BUILDER_ALWAYS_ASSERT(not fn_field.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(fn_field.is_fn_pointer());

        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Object l_args_obj = l_args_def.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());

            CODEGEN_LINE(l_args_obj.set_fn_ptr("fn_ptr_field", test_callback_fn))
            CODEGEN_LINE(l_args_obj.set<int32_t>("result", 0))
            CODEGEN_LINE(l_args_obj.freeze())

            // Verify get_fn_ptr returns the same pointer we set
            auto* retrieved_fn = l_args_obj.get_fn_ptr("fn_ptr_field");
            LLVM_BUILDER_ALWAYS_ASSERT_EQ((void*)retrieved_fn, (void*)test_callback_fn);

            CODEGEN_LINE(call_fn_ptr.on_event(l_args_obj))

            LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_args_obj.get<int32_t>("result"), 42);
            LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error());
        }
    }
}
