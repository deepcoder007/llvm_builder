//
// Created by vibhanshu on 2024-07-14
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

using namespace llvm_builder;

TEST(LLVM_CODEGEN, type_test) {
    CODEGEN_LINE(Cursor l_cursor{"type_test"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo int32_type_alias = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    CODEGEN_LINE(TypeInfo int32_arr3_type = TypeInfo::mk_array(int32_type, 3))
    CODEGEN_LINE(TypeInfo int32_arr5_type = TypeInfo::mk_array(int32_type, 5))
    CODEGEN_LINE(TypeInfo int32_arr3_alias_type = TypeInfo::mk_array(int32_type_alias, 3))
    CODEGEN_LINE(TypeInfo int32_arr5_alias_type = TypeInfo::mk_array(int32_type_alias, 5))
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_arr3_type, int32_arr3_alias_type);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_arr5_type, int32_arr5_alias_type);
    LLVM_BUILDER_ALWAYS_ASSERT_NOT_EQ(int32_arr3_type, int32_arr5_type);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_type.size_in_bytes(), sizeof(int32_t));
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(bool_type.size_in_bytes(), 1);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_arr3_type.size_in_bytes(), sizeof(int32_t) * 3);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_arr5_type.size_in_bytes(), sizeof(int32_t) * 5);

    TypeInfo int32_vec3_type = TypeInfo::mk_vector(int32_type, 3);
    TypeInfo int32_vec5_type = TypeInfo::mk_vector(int32_type, 5);
    TypeInfo int32_vec3_alias_type = TypeInfo::mk_vector(int32_type_alias, 3);
    TypeInfo int32_vec5_alias_type = TypeInfo::mk_vector(int32_type_alias, 5);
    TypeInfo int32_vec5_ptr_type = int32_vec5_type.pointer_type();
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_vec3_type, int32_vec3_alias_type);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_vec5_type, int32_vec5_alias_type);
    LLVM_BUILDER_ALWAYS_ASSERT_NOT_EQ(int32_vec3_type, int32_vec5_type);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_vec3_type.size_in_bytes(), sizeof(int32_t) * 3u);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_vec5_type.size_in_bytes(), sizeof(int32_t) * 5u);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(int32_vec5_ptr_type.size_in_bytes(), sizeof(int32_t*))

    const std::string int32_arr3_str{"array[int32:3]"};
    const std::string int32_arr5_str{"array[int32:5]"};
    const std::string int32_vec3_str{"vector[int32:3]"};
    const std::string int32_vec5_str{"vector[int32:5]"};
    const std::string int32_vec5_ptr_str{"ptr[vector[int32:3]]"};
    const std::string bool_str{"bool"};
    LLVM_BUILDER_ALWAYS_ASSERT(int32_arr3_str == int32_arr3_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_arr5_str == int32_arr5_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_vec3_str == int32_vec3_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_vec5_str == int32_vec5_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_arr3_str == int32_arr3_alias_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_arr5_str == int32_arr5_alias_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_vec3_str == int32_vec3_alias_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_vec5_str == int32_vec5_alias_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(bool_str == bool_type.short_name());
    LLVM_BUILDER_ALWAYS_ASSERT(int32_vec5_ptr_str == int32_vec3_type.pointer_type().short_name());

    {
        std::vector<member_field_entry> l_field_list;
        l_field_list.emplace_back("field_1", int32_type);
        l_field_list.emplace_back("field_3", int32_vec5_ptr_type);
        l_field_list.emplace_back("field_2", int32_type);
        l_field_list.emplace_back("field_5", int32_vec5_ptr_type);
        TypeInfo l_struct_type = TypeInfo::mk_struct("struct_padding", l_field_list, false);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(4u, l_struct_type.num_elements());
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[0].offset(), 0u);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[0].type().size_in_bytes(), 4u);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[1].offset(), 8u);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[1].type().base_type().size_in_bytes(), 20u);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[2].offset(), 16u);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[2].type().size_in_bytes(), 4u);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[3].offset(), 24u);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_struct_type[3].type().base_type().size_in_bytes(), 20u);
    }
}

TEST(LLVM_CODEGEN, struct_array_type_test) {
    CODEGEN_LINE(Cursor l_cursor{"type_test"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo void_type = TypeInfo::mk_void())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())

    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    LLVM_BUILDER_ALWAYS_ASSERT(int32_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo int32_ptr_type = int32_type.pointer_type())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_ptr_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_ptr_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo int32_arr3_type = TypeInfo::mk_array(int32_type, 3))
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_arr3_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_arr3_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_arr3_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo int32_arr3_ptr_type = int32_arr3_type.pointer_type())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_arr3_ptr_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(int32_arr3_ptr_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(int32_arr3_ptr_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo int32_vec3_type = TypeInfo::mk_vector(int32_type, 3))
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_vec3_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_vec3_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_vec3_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo int32_vec3_ptr_type = int32_vec3_type.pointer_type())
    LLVM_BUILDER_ALWAYS_ASSERT(not int32_vec3_ptr_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(int32_vec3_ptr_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(int32_vec3_ptr_type.is_valid_pointer_field())


    TypeInfo l_struct_type = [&] {
                                std::vector<member_field_entry> l_field_list;
                                l_field_list.emplace_back("field_1", int32_type);
                                l_field_list.emplace_back("field_2", int32_arr3_ptr_type);
                                l_field_list.emplace_back("field_3", int32_vec3_ptr_type);
                                return TypeInfo::mk_struct("struct_padding", l_field_list, false);
    } ();
    LLVM_BUILDER_ALWAYS_ASSERT(not l_struct_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(not l_struct_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(not l_struct_type.is_valid_pointer_field())
    CODEGEN_LINE(TypeInfo struct_ptr_type = l_struct_type.pointer_type())
    LLVM_BUILDER_ALWAYS_ASSERT(not struct_ptr_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(struct_ptr_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(struct_ptr_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo struct_ptr_arr3_type = TypeInfo::mk_array(struct_ptr_type, 3))
    LLVM_BUILDER_ALWAYS_ASSERT(not struct_ptr_arr3_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(not struct_ptr_arr3_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(not struct_ptr_arr3_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo struct_ptr_arr3_ptr_type = struct_ptr_arr3_type.pointer_type())
    LLVM_BUILDER_ALWAYS_ASSERT(not struct_ptr_arr3_ptr_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(struct_ptr_arr3_ptr_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(struct_ptr_arr3_ptr_type.is_valid_pointer_field())

    TypeInfo l_outer_struct_type = [&] {
                                std::vector<member_field_entry> l_field_list;
                                l_field_list.emplace_back("outer_field_1", int32_type);
                                l_field_list.emplace_back("outer_field_2", int32_arr3_ptr_type);
                                l_field_list.emplace_back("outer_field_3", int32_vec3_ptr_type);
                                l_field_list.emplace_back("outer_field_4", struct_ptr_type);
                                l_field_list.emplace_back("outer_field_5", struct_ptr_arr3_ptr_type);
                                return TypeInfo::mk_struct("struct_padding", l_field_list, false);
    } ();
    LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_struct_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_struct_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(not l_outer_struct_type.is_valid_pointer_field())

    CODEGEN_LINE(TypeInfo outer_struct_ptr_type = l_outer_struct_type.pointer_type())
    LLVM_BUILDER_ALWAYS_ASSERT(not outer_struct_ptr_type.has_error())
    LLVM_BUILDER_ALWAYS_ASSERT(outer_struct_ptr_type.is_valid_struct_field())
    LLVM_BUILDER_ALWAYS_ASSERT(outer_struct_ptr_type.is_valid_pointer_field())
}

TEST(LLVM_CODEGEN, basic_test) {
    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error())
    CODEGEN_LINE(Cursor l_cursor{"basic_test"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    CODEGEN_LINE(TypeInfo int32_arr_type = TypeInfo::mk_array(int32_type, 5))

    std::vector<member_field_entry> args3_fields;
    args3_fields.emplace_back("arg1", int32_type);
    args3_fields.emplace_back("arg2", int32_type);
    args3_fields.emplace_back("arg3", int32_type);
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
            CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
            LLVM_BUILDER_ALWAYS_ASSERT(ctx == CodeSectionContext::current_context());
            CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
            LLVM_BUILDER_ALWAYS_ASSERT(ctx.field("arg1").load() == ctx.field("arg1").load())
            LLVM_BUILDER_ALWAYS_ASSERT(ctx.field("arg3").load() != ctx.field("arg1").load())
            LLVM_BUILDER_ALWAYS_ASSERT(arg1 == ctx.field("arg1").load())
            LLVM_BUILDER_ALWAYS_ASSERT(arg1 != ctx.field("arg2").load())
            CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
            LLVM_BUILDER_ALWAYS_ASSERT(arg2 != ctx.field("arg1").load())
            LLVM_BUILDER_ALWAYS_ASSERT(arg2 == ctx.field("arg2").load())
            CODEGEN_LINE(ValueInfo c1 = ValueInfo::from_constant(101))
            CODEGEN_LINE(ValueInfo c2 = ValueInfo::from_constant(999))
            LLVM_BUILDER_ALWAYS_ASSERT(c1 != ValueInfo::from_constant(100))
            LLVM_BUILDER_ALWAYS_ASSERT(c1 == ValueInfo::from_constant(101))
            LLVM_BUILDER_ALWAYS_ASSERT(c2 != ValueInfo::from_constant(1000))
            LLVM_BUILDER_ALWAYS_ASSERT(c2 == ValueInfo::from_constant(1000 - 1))
            CODEGEN_LINE(ValueInfo res = arg1.add(arg2))
            LLVM_BUILDER_ALWAYS_ASSERT(arg1.add(arg2) == res)
            CODEGEN_LINE(ValueInfo res2 = res.add(c1))
            LLVM_BUILDER_ALWAYS_ASSERT(res.add(c1) == res2)
            CODEGEN_LINE(ValueInfo res3 = res2.add(c2))
            LLVM_BUILDER_ALWAYS_ASSERT(res2.add(c2) == res3)
            CODEGEN_LINE(ValueInfo res4 = res2.add(ctx.field("arg3").load()))
            LLVM_BUILDER_ALWAYS_ASSERT(res2.add(ctx.field("arg3").load()) == res4)
            CODEGEN_LINE(ValueInfo res5 = res3.add(res4))
            LLVM_BUILDER_ALWAYS_ASSERT(res3.add(res4) == res5)
            CODEGEN_LINE(CodeSectionContext::set_return_value(res5))
        }
        {
            const std::string fn_name{"sample_fn_name"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
    }
    {
        CODEGEN_LINE(Function fn_if_else("foo_if_else", FnContext{abc_type.pointer_type()}, l_module))

        LLVM_BUILDER_ASSERT(not int32_type.has_error());
        {
            CODEGEN_LINE(CodeSection l_fn_body = fn_if_else.mk_section("if_else_fn_begin"))
            CODEGEN_LINE(l_fn_body.enter())
            CODEGEN_LINE(ValueInfo ctx_if_else = CodeSectionContext::current_context())
            CODEGEN_LINE(ValueInfo arg_a = ctx_if_else.field("a").load())
            CODEGEN_LINE(ValueInfo arg_b = ctx_if_else.field("b").load())
            CODEGEN_LINE(ValueInfo c_5 = ValueInfo::from_constant(5))
            CODEGEN_LINE(ValueInfo compare_1 = arg_b.less_than(c_5))
            CODEGEN_LINE(ValueInfo compare_2 = compare_1.not_equal(ValueInfo::from_constant(false)))
            CODEGEN_LINE(IfElseCond if_else_branch{"cond_block", compare_2})
            CODEGEN_LINE(CodeSectionContext::mk_ptr("test_value_fwd"_cs, int32_type, ValueInfo::from_constant(0)))
            CODEGEN_LINE(CodeSectionContext::mk_ptr("test_value_fwd_2"_cs, int32_type, ValueInfo::from_constant(0)))
            if_else_branch.then_branch([&ctx_if_else] {
                ValueInfo arg_a = ctx_if_else.field("a").load();
                ValueInfo then_val = arg_a.add(ValueInfo::from_constant(999));
                then_val.push("test_value_fwd"_cs);
                ValueInfo then_val_2 = then_val.add(then_val);
                then_val_2.push("test_value_fwd_2"_cs);
            });
            if_else_branch.else_branch([&ctx_if_else] {
                ValueInfo arg_a = ctx_if_else.field("a").load();
                ValueInfo else_val = arg_a.add(ValueInfo::from_constant(8080));
                else_val.push("test_value_fwd"_cs);
                ValueInfo else_val_2 = else_val.add(else_val);
                else_val_2.push("test_value_fwd_2"_cs);
            });
            CODEGEN_LINE(if_else_branch.bind())
            CODEGEN_LINE(ValueInfo l_test_value_fwd = CodeSectionContext::pop("test_value_fwd_2"_cs))
            CODEGEN_LINE(CodeSectionContext::set_return_value(l_test_value_fwd))
        }
        {
            const std::string fn_name{"foo_if_else"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
    }
    {
        CODEGEN_LINE(Function fn_load("foo_loop_v2", FnContext{arr_args_type.pointer_type()}, l_module))

        {
            CODEGEN_LINE(CodeSection l_fn_body = fn_load.mk_section("fn_load_begin"))
            CODEGEN_LINE(l_fn_body.enter())
            CODEGEN_LINE(ValueInfo ctx_load = CodeSectionContext::current_context())
            CODEGEN_LINE(ValueInfo arg_a = ctx_load.field("a").load())
            CODEGEN_LINE(ValueInfo arg_b = ctx_load.field("b").load())
            CODEGEN_LINE(ValueInfo arr_entry_value = arg_b.entry(2).load())
            CODEGEN_LINE(arg_b.entry(0).store(ValueInfo::from_constant(1987)))
            CODEGEN_LINE(CodeSectionContext::set_return_value(arr_entry_value))
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
        CODEGEN_LINE(const runtime::EventFn& if_else_fn = l_runtime_module.event_fn_info("foo_if_else"))
        LLVM_BUILDER_ALWAYS_ASSERT(not if_else_fn.has_error());

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
            }
        }
        {
            for (int32_t i = 0; i != 10; ++i) {
                CODEGEN_LINE(runtime::Object l_abc_obj = l_abc_struct.mk_object())

                l_abc_obj.set<int32_t>("a", i);
                l_abc_obj.set<int32_t>("b", i + 1);
                l_abc_obj.set<int32_t>("c", i + 2);
                l_abc_obj.freeze();

                int32_t c_5 = 5;
                bool compare_1 = (i + 1) < c_5;
                bool compare_2 = compare_1 != false;
                int32_t test_value_fwd;
                int32_t test_value_fwd_2;
                if (compare_2) {
                    test_value_fwd = i + 999;
                    test_value_fwd_2 = test_value_fwd + test_value_fwd;
                } else {
                    test_value_fwd = i + 8080;
                    test_value_fwd_2 = test_value_fwd + test_value_fwd;
                }
                CODEGEN_LINE(int32_t result = if_else_fn.on_event(l_abc_obj))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(test_value_fwd_2, result);
            }
        }
    }
}

TEST(LLVM_CODEGEN, lexical_context) {
    Cursor l_cursor{"lexical_ctx"};
    Cursor::Context l_cursor_ctx{l_cursor};
    TypeInfo int32_type = TypeInfo::mk_int32();
    TypeInfo bool_type = TypeInfo::mk_bool();

    std::vector<member_field_entry> lexical_args_fields;
    lexical_args_fields.emplace_back("arg1", int32_type);
    lexical_args_fields.emplace_back("arg2", int32_type);
    lexical_args_fields.emplace_back("arg3", int32_type);
    TypeInfo lexical_args_type = TypeInfo::mk_struct("lexical_args", lexical_args_fields);

    Cursor::Context::value().bind();
    Module l_module = Cursor::Context::value().main_module();
    Module::Context l_module_ctx{l_module};
    auto gen_fn = [&lexical_args_type, &int32_type] (const std::string& fn_name, Module& mod, int version) -> Function {
        Function fn(fn_name, FnContext{lexical_args_type.pointer_type()}, mod);
        {
            CodeSection l_fn_body = fn.mk_section("test_fn_body");
            l_fn_body.enter();
            ValueInfo ctx = CodeSectionContext::current_context();
            ValueInfo arg1 = ctx.field("arg1").load();
            ValueInfo arg2 = ctx.field("arg2").load();
            ValueInfo arg3 = ctx.field("arg3").load();
            arg1.push("arg_in"_cs);
            CodeSectionContext::push_var_context();
            arg2.push("arg_in"_cs);
            CodeSectionContext::push_var_context();
            arg3.push("arg_in"_cs);
            CodeSectionContext::push_var_context();

            if (version == 0) {
                CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs));
            }
            CodeSectionContext::pop_var_context();
            if (version == 1) {
                CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs));
            }
            CodeSectionContext::pop_var_context();
            if (version == 2) {
                CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs));
            }
            CodeSectionContext::pop_var_context();
            if (version == 3) {
                CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs));
            }
        }
        return fn;
    };

    gen_fn("fn1", l_module, 0);
    gen_fn("fn2", l_module, 1);
    gen_fn("fn3", l_module, 2);
    gen_fn("fn4", l_module, 3);
    LLVM_BUILDER_ASSERT(not ErrorContext::has_error());
    {
        LLVM_BUILDER_ALWAYS_ASSERT(l_module.is_init());
        CODEGEN_LINE(JustInTimeRunner jit_runner)
        CODEGEN_LINE(jit_runner.add_module(l_cursor))
        CODEGEN_LINE(jit_runner.bind())

        CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
        CODEGEN_LINE(const runtime::Struct& l_lexical_struct = l_runtime_module.struct_info("lexical_args"))
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());

        CODEGEN_LINE(const runtime::EventFn& fn1 = l_runtime_module.event_fn_info("fn1"))
        CODEGEN_LINE(const runtime::EventFn& fn2 = l_runtime_module.event_fn_info("fn2"))
        CODEGEN_LINE(const runtime::EventFn& fn3 = l_runtime_module.event_fn_info("fn3"))
        CODEGEN_LINE(const runtime::EventFn& fn4 = l_runtime_module.event_fn_info("fn4"))
        LLVM_BUILDER_ALWAYS_ASSERT(not fn1.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not fn2.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not fn3.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not fn4.has_error());

        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Object l_args_obj = l_lexical_struct.mk_object())

            l_args_obj.set<int32_t>("arg1", i);
            l_args_obj.set<int32_t>("arg2", i + 1);
            l_args_obj.set<int32_t>("arg3", i + 2);
            l_args_obj.freeze();

            LLVM_BUILDER_ALWAYS_ASSERT_EQ(i + 2, fn1.on_event(l_args_obj));
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(i + 2, fn2.on_event(l_args_obj));
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(i + 1, fn3.on_event(l_args_obj));
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(i, fn4.on_event(l_args_obj));
        }
    }
}

TEST(LLVM_CODEGEN, struct_type_test) {
    CODEGEN_LINE(Cursor l_cursor{"struct_type_test"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo void_type = TypeInfo::mk_void())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    std::vector<member_field_entry> l_field_list;
    l_field_list.emplace_back("field_1", int32_type);
    l_field_list.emplace_back("field_2", int32_type);
    l_field_list.emplace_back("field_3", int32_type);
    l_field_list.emplace_back("field_4", int32_type);
    l_field_list.emplace_back("field_5", int32_type);
    CODEGEN_LINE(TypeInfo l_struct_type = TypeInfo::mk_struct("test_struct", l_field_list))
    LLVM_BUILDER_ALWAYS_ASSERT(not l_struct_type.has_error());

    std::vector<member_field_entry> struct_test_args_fields;
    struct_test_args_fields.emplace_back("arg1", int32_type);
    struct_test_args_fields.emplace_back("arg2", int32_type);
    struct_test_args_fields.emplace_back("arg3", int32_type);
    struct_test_args_fields.emplace_back("arg4", int32_type);
    struct_test_args_fields.emplace_back("arg5", l_struct_type.pointer_type());
    CODEGEN_LINE(TypeInfo struct_test_args_type = TypeInfo::mk_struct("struct_test_args", struct_test_args_fields))
    LLVM_BUILDER_ALWAYS_ASSERT(not struct_test_args_type.has_error());

    CODEGEN_LINE(l_cursor.bind())
    {
        CODEGEN_LINE(Module l_module = l_cursor.main_module())
        CODEGEN_LINE(Module::Context l_module_ctx{l_module})
        {
            CODEGEN_LINE(Function fn("struct_fn", FnContext{struct_test_args_type.pointer_type()}, l_module))
            {
                CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
                CODEGEN_LINE(l_fn_body.enter())
                CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
                CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
                CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
                CODEGEN_LINE(ValueInfo arg3 = ctx.field("arg3").load())
                CODEGEN_LINE(ValueInfo arg4 = ctx.field("arg4").load())
                CODEGEN_LINE(ValueInfo arg5 = ctx.field("arg5").load())
                CODEGEN_LINE(arg5.field("field_1").store(arg1))
                CODEGEN_LINE(arg5.field("field_2").store(arg2))
                CODEGEN_LINE(arg5.field("field_3").store(arg3))
                CODEGEN_LINE(arg5.field("field_4").store(arg4))
                CODEGEN_LINE(CodeSectionContext::set_return_value(arg5.field("field_2").load()))
            }
            {
                const std::string fn_name{"struct_fn"};
                Function f2 = l_module.get_function(fn_name);
                f2.verify();
            }
        }
        INIT_MODULE(l_module)
        LLVM_BUILDER_ALWAYS_ASSERT(l_module.is_init())
    }
    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(jit_runner.add_module(l_cursor))
    LLVM_BUILDER_ALWAYS_ASSERT(not jit_runner.has_error());
    CODEGEN_LINE(jit_runner.bind())
    LLVM_BUILDER_ALWAYS_ASSERT(not jit_runner.has_error());

    CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
    CODEGEN_LINE(const runtime::Struct& l_test_struct_rt = l_runtime_module.struct_info("test_struct"))
    CODEGEN_LINE(const runtime::Struct& l_args_struct = l_runtime_module.struct_info("struct_test_args"))
    CODEGEN_LINE(const runtime::EventFn& struct_fn = l_runtime_module.event_fn_info("struct_fn"))
    LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not l_test_struct_rt.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not l_args_struct.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not struct_fn.has_error());

    LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_struct_rt.size_in_bytes(), 20);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_struct_rt["field_1"].offset(), 0);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_struct_rt["field_2"].offset(), 4);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_struct_rt["field_3"].offset(), 8);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_struct_rt["field_4"].offset(), 12);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_struct_rt["field_5"].offset(), 16);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_struct_rt.num_fields(), 5);

    CODEGEN_LINE(runtime::Field field_1 = l_test_struct_rt["field_1"])
    CODEGEN_LINE(runtime::Field field_2 = l_test_struct_rt["field_2"])
    LLVM_BUILDER_ALWAYS_ASSERT(not field_1.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not field_2.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(field_1.idx(), 0);
    LLVM_BUILDER_ALWAYS_ASSERT_EQ(field_2.idx(), 1);
    {
        CODEGEN_LINE(runtime::Object l_test_object = l_test_struct_rt.mk_object())
        l_test_object.set<int32_t>("field_1", 100);
        l_test_object.set<int32_t>("field_2", 200);
        l_test_object.set<int32_t>("field_3", 300);
        l_test_object.set<int32_t>("field_4", 400);
        l_test_object.freeze();

        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_object.get<int32_t>("field_1"), 100);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_object.get<int32_t>("field_2"), 200);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_object.get<int32_t>("field_3"), 300);
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_object.get<int32_t>("field_4"), 400);
    }
    {
        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Object l_test_obj = l_test_struct_rt.mk_object())
            l_test_obj.freeze();
            CODEGEN_LINE(runtime::Object l_args_obj = l_args_struct.mk_object())
            LLVM_BUILDER_ALWAYS_ASSERT(not l_test_obj.has_error());
            LLVM_BUILDER_ALWAYS_ASSERT(l_test_obj.is_instance_of(l_test_struct_rt));
            int32_t result = 0;
            {
                l_args_obj.set<int32_t>("arg1", i);
                l_args_obj.set<int32_t>("arg2", i + 2);
                l_args_obj.set<int32_t>("arg3", i + 4);
                l_args_obj.set<int32_t>("arg4", i + 6);
                l_args_obj.set_object("arg5", l_test_obj);
                l_args_obj.freeze();
                CODEGEN_LINE(result = struct_fn.on_event(l_args_obj))
            }
            {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_obj.get<int32_t>("field_1"), i);
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_obj.get<int32_t>("field_2"), i + 2);
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_obj.get<int32_t>("field_3"), i + 4);
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_test_obj.get<int32_t>("field_4"), i + 6);
            }
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(result, 0);
        }
    }
}

TEST(LLVM_CODEGEN, multi_type) {
    Cursor l_cursor{"multi_type"};
    Cursor::Context l_cursor_ctx{l_cursor};
    TypeInfo int16_type = TypeInfo::mk_int16();
    TypeInfo int32_type = TypeInfo::mk_int32();
    TypeInfo int64_type = TypeInfo::mk_int64();
    TypeInfo uint32_type = TypeInfo::mk_uint32();
    TypeInfo uint16_type = TypeInfo::mk_uint16();
    TypeInfo uint64_type = TypeInfo::mk_uint64();
    TypeInfo float32_type = TypeInfo::mk_float32();
    TypeInfo float64_type = TypeInfo::mk_float64();
    std::vector<member_field_entry> l_field_list;
    l_field_list.emplace_back("int_arg1", int32_type);
    l_field_list.emplace_back("int_arg2", int32_type);
    l_field_list.emplace_back("int16_arg1", int16_type);
    l_field_list.emplace_back("int16_arg2", int16_type);
    l_field_list.emplace_back("int64_arg1", int64_type);
    l_field_list.emplace_back("int64_arg2", int64_type);
    l_field_list.emplace_back("uint_arg1", uint32_type);
    l_field_list.emplace_back("uint_arg2", uint32_type);
    l_field_list.emplace_back("uint16_arg1", uint16_type);
    l_field_list.emplace_back("uint16_arg2", uint16_type);
    l_field_list.emplace_back("uint64_arg1", uint64_type);
    l_field_list.emplace_back("uint64_arg2", uint64_type);
    l_field_list.emplace_back("float32_arg1", float32_type);
    l_field_list.emplace_back("float32_arg2", float32_type);
    l_field_list.emplace_back("float64_arg1", float64_type);
    l_field_list.emplace_back("float64_arg2", float64_type);
    TypeInfo l_struct_type = TypeInfo::mk_struct("multi_type_struct", l_field_list);

    std::vector<member_field_entry> multi_type_args_fields;
    multi_type_args_fields.emplace_back("arg1", int32_type);
    multi_type_args_fields.emplace_back("arg1_neg", int32_type);
    multi_type_args_fields.emplace_back("arg2", uint32_type);
    multi_type_args_fields.emplace_back("arg3", float32_type);
    multi_type_args_fields.emplace_back("arg4", float64_type);
    multi_type_args_fields.emplace_back("output", l_struct_type.pointer_type());
    TypeInfo multi_type_args_type = TypeInfo::mk_struct("multi_type_args", multi_type_args_fields);

    l_cursor.bind();
    JustInTimeRunner jit_runner;
    {
        Module l_module = l_cursor.main_module();
        Module::Context l_module_ctx{l_module};
        {
            Function fn("basic_multi_type_test", FnContext{multi_type_args_type.pointer_type()}, l_module);
            {
                CodeSection l_fn_body = fn.mk_section("test_fn_body");
                l_fn_body.enter();
                ValueInfo ctx = CodeSectionContext::current_context();
                ValueInfo arg1 = ctx.field("arg1").load();
                ValueInfo arg1_neg = ctx.field("arg1_neg").load();
                ValueInfo arg2 = ctx.field("arg2").load();
                ValueInfo arg3 = ctx.field("arg3").load();
                ValueInfo arg4 = ctx.field("arg4").load();
                ValueInfo arg5 = ctx.field("output").load();
                arg5.field("int_arg1").store(arg1);
                arg5.field("uint_arg1").store(arg2);
                arg5.field("float32_arg1").store(arg3);
                arg5.field("float64_arg1").store(arg4);
                arg5.field("int16_arg1").store(arg1.cast(int16_type));
                arg5.field("int64_arg1").store(arg1.cast(int64_type));
                arg5.field("int16_arg2").store(arg1_neg.cast(int16_type));
                arg5.field("int64_arg2").store(arg1_neg.cast(int64_type));
                arg5.field("uint16_arg2").store(arg1_neg.cast(uint16_type));
                arg5.field("uint64_arg2").store(arg1_neg.cast(uint64_type));
                arg5.field("float32_arg2").store(arg4.cast(float32_type));
                arg5.field("float64_arg2").store(arg3.cast(float64_type));
                CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
            }
            {
                const std::string fn_name{"basic_multi_type_test"};
                Function f2 = l_module.get_function(fn_name);
                f2.verify();
            }
        }
        INIT_MODULE(l_module)
    }
    CODEGEN_LINE(jit_runner.add_module(l_cursor))
    CODEGEN_LINE(jit_runner.bind())

    CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
    CODEGEN_LINE(const runtime::Struct& l_output_struct = l_runtime_module.struct_info("multi_type_struct"))
    CODEGEN_LINE(const runtime::Struct& l_args_struct = l_runtime_module.struct_info("multi_type_args"))
    CODEGEN_LINE(const runtime::EventFn& multi_type_fn = l_runtime_module.event_fn_info("basic_multi_type_test"))
    LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not multi_type_fn.has_error());

    {
        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Object l_output_obj = l_output_struct.mk_object())
            l_output_obj.freeze();
            CODEGEN_LINE(runtime::Object l_args_obj = l_args_struct.mk_object())
            {
                l_args_obj.set<int32_t>("arg1", i);
                l_args_obj.set<int32_t>("arg1_neg", i * -1);
                l_args_obj.set<uint32_t>("arg2", (uint32_t)(i + 2));
                l_args_obj.set<float32_t>("arg3", (float32_t)(i + 4));
                l_args_obj.set<float64_t>("arg4", (float64_t)(i + 6));
                l_args_obj.set_object("output", l_output_obj);
                l_args_obj.freeze();

                CODEGEN_LINE(multi_type_fn.on_event(l_args_obj))
            }
            {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<int32_t>("int_arg1"), i);
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<uint32_t>("uint_arg1"), (uint32_t)(i + 2));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<float32_t>("float32_arg1"), (float32_t)(i + 4));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<float64_t>("float64_arg1"), (float64_t)(i + 6));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<int16_t>("int16_arg1"), (int16_t)i);
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<int64_t>("int64_arg1"), (int64_t)i);
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<int16_t>("int16_arg2"), (int16_t)(i * -1));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<int64_t>("int64_arg2"), (int64_t)(i * -1));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<uint16_t>("uint16_arg2"), (uint16_t)(i * -1));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<uint64_t>("uint64_arg2"), (uint64_t)((uint32_t)(i * -1)));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<float32_t>("float32_arg2"), (float32_t)(float64_t)(i + 6));
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_output_obj.get<float64_t>("float64_arg2"), (float64_t)(float32_t)(i + 4));
            }
        }
    }
}

TEST(LLVM_CODEGEN, array_type_test) {
    Cursor l_cursor{"array_type_test"};
    Cursor::Context l_cursor_ctx{l_cursor};
    TypeInfo int32_type = TypeInfo::mk_int32();
    TypeInfo int32_arr5_type = TypeInfo::mk_array(int32_type, 5);
    TypeInfo int32_vec5_type = TypeInfo::mk_vector(int32_type, 5);

    std::vector<member_field_entry> arr_load_args_fields;
    arr_load_args_fields.emplace_back("a", int32_type);
    arr_load_args_fields.emplace_back("b", int32_arr5_type.pointer_type());
    TypeInfo arr_load_args_type = TypeInfo::mk_struct("arr_load_args", arr_load_args_fields, false);

    std::vector<member_field_entry> arr_assign_args_fields;
    arr_assign_args_fields.emplace_back("idx", int32_type);
    arr_assign_args_fields.emplace_back("arr", int32_arr5_type.pointer_type());
    arr_assign_args_fields.emplace_back("value", int32_type);
    TypeInfo arr_assign_args_type = TypeInfo::mk_struct("arr_assign_args", arr_assign_args_fields, false);

    std::vector<member_field_entry> vec_load_args_fields;
    vec_load_args_fields.emplace_back("a", int32_type);
    vec_load_args_fields.emplace_back("b", int32_vec5_type.pointer_type());
    TypeInfo vec_load_args_type = TypeInfo::mk_struct("vec_load_args", vec_load_args_fields, false);

    std::vector<member_field_entry> vec_assign_args_fields;
    vec_assign_args_fields.emplace_back("idx", int32_type);
    vec_assign_args_fields.emplace_back("vec", int32_vec5_type.pointer_type());
    vec_assign_args_fields.emplace_back("value", int32_type);
    TypeInfo vec_assign_args_type = TypeInfo::mk_struct("vec_assign_args", vec_assign_args_fields, false);

    l_cursor.bind();
    Module l_module = l_cursor.main_module();
    Module::Context l_module_ctx{l_module};

    {
        Function fn_load("array_idx_load", FnContext{arr_load_args_type.pointer_type()}, l_module);

        Function fn_load_lazy("array_idx_load_lazy", FnContext{arr_load_args_type.pointer_type()}, l_module);
        {
            CodeSection l_fn_body = fn_load.mk_section("fn_load_begin");
            l_fn_body.enter();
            ValueInfo ctx = CodeSectionContext::current_context();
            ValueInfo arg_a = ctx.field("a").load();
            ValueInfo arg_b = ctx.field("b").load();
            ValueInfo arr_entry_value = arg_b.entry(arg_a).load();
            CodeSectionContext::set_return_value(arr_entry_value);
        }
        {
            CodeSection l_fn_body = fn_load_lazy.mk_section("fn_load_begin");
            l_fn_body.enter();
            ValueInfo ctx_lazy = CodeSectionContext::current_context();
            ValueInfo arg_a = ctx_lazy.field("a").load();
            ValueInfo arg_b = ctx_lazy.field("b").load();
            ValueInfo arr_entry_ptr = arg_b.entry(arg_a);
            ValueInfo arr_entry_value = arr_entry_ptr.load();
            CodeSectionContext::set_return_value(arr_entry_value);
        }
    }
    {
        Function fn_assign("array_idx_assign", FnContext{arr_assign_args_type.pointer_type()}, l_module);
        {
            CodeSection l_fn_body = fn_assign.mk_section("fn_assign_begin");
            l_fn_body.enter();
            ValueInfo ctx = CodeSectionContext::current_context();
            ValueInfo arg_idx = ctx.field("idx").load();
            ValueInfo arg_arr = ctx.field("arr").load();
            ValueInfo arg_value = ctx.field("value").load();
            arg_arr.entry(arg_idx).store(arg_value);
            CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
        }
        Function fn_assign_lazy("array_idx_assign_lazy", FnContext{arr_assign_args_type.pointer_type()}, l_module);
        {
            CodeSection l_fn_body = fn_assign_lazy.mk_section("fn_assign_begin");
            l_fn_body.enter();
            ValueInfo ctx_lazy = CodeSectionContext::current_context();
            ValueInfo arg_idx = ctx_lazy.field("idx").load();
            ValueInfo arg_arr = ctx_lazy.field("arr").load();
            ValueInfo arg_value = ctx_lazy.field("value").load();
            ValueInfo target_ptr = arg_arr.entry(arg_idx);
            target_ptr.store(arg_value);
            CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
        }
    }
    {
        Function fn_load("vector_idx_load", FnContext{vec_load_args_type.pointer_type()}, l_module);
        {
            CodeSection l_fn_body = fn_load.mk_section("fn_load_begin");
            l_fn_body.enter();
            ValueInfo ctx = CodeSectionContext::current_context();
            ValueInfo arg_a = ctx.field("a").load();
            ValueInfo arg_b = ctx.field("b").load().load();
            ValueInfo vec_entry_value = arg_b.load_vector_entry(arg_a);
            CodeSectionContext::set_return_value(vec_entry_value);
        }
    }
    {
        Function fn_assign("vector_idx_assign", FnContext{vec_assign_args_type.pointer_type()}, l_module);
        {
            CodeSection l_fn_body = fn_assign.mk_section("fn_assign_begin");
            l_fn_body.enter();
            ValueInfo ctx = CodeSectionContext::current_context();
            ValueInfo arg_idx = ctx.field("idx").load();
            ValueInfo arg_vec_ptr = ctx.field("vec").load();
            ValueInfo arg_vec = arg_vec_ptr.load();
            ValueInfo arg_value = ctx.field("value").load();
            ValueInfo l_final_vec = arg_vec.store_vector_entry(arg_idx, arg_value);
            arg_vec_ptr.store(l_final_vec);
            CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
        }
    }
    INIT_MODULE(l_module)
    {
        LLVM_BUILDER_ALWAYS_ASSERT(l_module.is_init());
        CODEGEN_LINE(JustInTimeRunner jit_runner)
        CODEGEN_LINE(jit_runner.add_module(l_cursor))
        CODEGEN_LINE(jit_runner.bind())

        CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
        CODEGEN_LINE(const runtime::Struct& l_arr_load_struct = l_runtime_module.struct_info("arr_load_args"))
        CODEGEN_LINE(const runtime::Struct& l_arr_assign_struct = l_runtime_module.struct_info("arr_assign_args"))
        CODEGEN_LINE(const runtime::Struct& l_vec_load_struct = l_runtime_module.struct_info("vec_load_args"))
        CODEGEN_LINE(const runtime::Struct& l_vec_assign_struct = l_runtime_module.struct_info("vec_assign_args"))
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());

        CODEGEN_LINE(const runtime::EventFn& arr_idx_load_fn = l_runtime_module.event_fn_info("array_idx_load"))
        CODEGEN_LINE(const runtime::EventFn& arr_idx_load_lazy_fn = l_runtime_module.event_fn_info("array_idx_load_lazy"))
        CODEGEN_LINE(const runtime::EventFn& arr_idx_assign_fn = l_runtime_module.event_fn_info("array_idx_assign"))
        CODEGEN_LINE(const runtime::EventFn& arr_idx_assign_lazy_fn = l_runtime_module.event_fn_info("array_idx_assign_lazy"))
        CODEGEN_LINE(const runtime::EventFn& vec_idx_load_fn = l_runtime_module.event_fn_info("vector_idx_load"))
        CODEGEN_LINE(const runtime::EventFn& vec_idx_assign_fn = l_runtime_module.event_fn_info("vector_idx_assign"))
        LLVM_BUILDER_ALWAYS_ASSERT(not arr_idx_load_fn.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not arr_idx_load_lazy_fn.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not arr_idx_assign_fn.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not arr_idx_assign_lazy_fn.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not vec_idx_load_fn.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(not vec_idx_assign_fn.has_error());

        {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            for (uint32_t i = 0; i != 10; ++i) {
                arr_obj.set<int32_t>(i, (int32_t)(i+1) * 10);
            }
            arr_obj.freeze();
            for (int32_t i = 0; i != 5; ++i) {
                CODEGEN_LINE(runtime::Object l_args_obj = l_arr_load_struct.mk_object())
                l_args_obj.set<int32_t>("a", i);
                l_args_obj.set_array("b", arr_obj);
                l_args_obj.freeze();
                CODEGEN_LINE(int32_t result = arr_idx_load_fn.on_event(l_args_obj))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(result, arr_obj.get<int32_t>(i));
            }
        }
        {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            for (uint32_t i = 0; i != 10; ++i) {
                arr_obj.set<int32_t>(i, (int32_t)(i+1) * 10);
            }
            arr_obj.freeze();
            for (int32_t i = 0; i != 5; ++i) {
                CODEGEN_LINE(runtime::Object l_args_obj = l_arr_load_struct.mk_object())
                l_args_obj.set<int32_t>("a", i);
                l_args_obj.set_array("b", arr_obj);
                l_args_obj.freeze();
                CODEGEN_LINE(int32_t result = arr_idx_load_lazy_fn.on_event(l_args_obj))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(result, arr_obj.get<int32_t>(i));
            }
        }
        {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            arr_obj.freeze();
            for (int32_t i = 0; i != 5; ++i) {
                CODEGEN_LINE(runtime::Object l_args_obj = l_arr_assign_struct.mk_object())
                l_args_obj.set<int32_t>("idx", i);
                l_args_obj.set_array("arr", arr_obj);
                l_args_obj.set<int32_t>("value", i * 100);
                l_args_obj.freeze();
                CODEGEN_LINE(arr_idx_assign_fn.on_event(l_args_obj))
            }
            for (int32_t i = 0; i != 5; ++i) {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(arr_obj.get<int32_t>(static_cast<uint32_t>(i)), i * 100);
            }
        }
        {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            arr_obj.freeze();
            for (int32_t i = 0; i != 5; ++i) {
                CODEGEN_LINE(runtime::Object l_args_obj = l_arr_assign_struct.mk_object())
                l_args_obj.set<int32_t>("idx", i);
                l_args_obj.set_array("arr", arr_obj);
                l_args_obj.set<int32_t>("value", i * 100);
                l_args_obj.freeze();
                CODEGEN_LINE(arr_idx_assign_lazy_fn.on_event(l_args_obj))
            }
            for (int32_t i = 0; i != 5; ++i) {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(arr_obj.get<int32_t>(static_cast<uint32_t>(i)), i * 100);
            }
        }
        {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            for (int i = 0; i != 10; ++i) {
                arr_obj.set<int32_t>(static_cast<uint32_t>(i), (i+1) * 10);
            }
            arr_obj.freeze();
            for (int32_t i = 0; i != 5; ++i) {
                CODEGEN_LINE(runtime::Object l_args_obj = l_vec_load_struct.mk_object())
                l_args_obj.set<int32_t>("a", i);
                l_args_obj.set_array("b", arr_obj);
                l_args_obj.freeze();
                // TODO{vibhanshu}: enable and debug this test on vector
                CODEGEN_LINE(int32_t result = vec_idx_load_fn.on_event(l_args_obj))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(result, arr_obj.get<int32_t>(static_cast<uint32_t>(i)));
            }
        }
        {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            arr_obj.freeze();
            for (int32_t i = 0; i != 5; ++i) {
                CODEGEN_LINE(runtime::Object l_args_obj = l_vec_assign_struct.mk_object())
                l_args_obj.set<int32_t>("idx", i);
                l_args_obj.set_array("vec", arr_obj);
                l_args_obj.set<int32_t>("value", i * 100);
                l_args_obj.freeze();
                CODEGEN_LINE(vec_idx_assign_fn.on_event(l_args_obj))
            }
            for (int32_t i = 0; i != 5; ++i) {
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(arr_obj.get<int32_t>(static_cast<uint32_t>(i)), i * 100);
            }
        }
    }
}

TEST(LLVM_CODEGEN, vector_type_test) {
    Cursor l_cursor{"vector_type_test"};
    Cursor::Context l_cursor_ctx{l_cursor};
    TypeInfo int32_type = TypeInfo::mk_int32();
    TypeInfo int32_vec5_type = TypeInfo::mk_vector(int32_type, 5);
    TypeInfo int32_vec5_pointer_type = int32_vec5_type.pointer_type();

    std::vector<member_field_entry> vec_add_args_fields;
    vec_add_args_fields.emplace_back("arg1", int32_vec5_pointer_type);
    vec_add_args_fields.emplace_back("arg2", int32_vec5_pointer_type);
    vec_add_args_fields.emplace_back("arg3", int32_vec5_pointer_type);
    TypeInfo vec_add_args_type = TypeInfo::mk_struct("vec_add_args", vec_add_args_fields);

    std::vector<member_field_entry> vec_set_args_fields;
    vec_set_args_fields.emplace_back("arg1", int32_vec5_pointer_type);
    vec_set_args_fields.emplace_back("v", int32_type);
    TypeInfo vec_set_args_type = TypeInfo::mk_struct("vec_set_args", vec_set_args_fields);

    std::vector<member_field_entry> vec_load_args_fields;
    vec_load_args_fields.emplace_back("arg1", int32_vec5_pointer_type);
    TypeInfo vec_load_args_type = TypeInfo::mk_struct("vec_load_args", vec_load_args_fields);

    l_cursor.bind();
    JustInTimeRunner jit_runner;
    {
        Module l_module = l_cursor.main_module();
        Module::Context l_module_ctx{l_module};
        {
            Function fn("sample_vec_add_op", FnContext{vec_add_args_type.pointer_type()}, l_module);

            {
                CodeSection l_fn_body = fn.mk_section("test_fn_body");
                l_fn_body.enter();
                ValueInfo ctx = CodeSectionContext::current_context();
                ValueInfo arg1 = ctx.field("arg1").load().load();
                ValueInfo arg2 = ctx.field("arg2").load().load();
                ValueInfo arg3 = ctx.field("arg3").load();
                ValueInfo l_sum_vec = arg1.add(arg2);
                ValueInfo l_sum_vec_2 = l_sum_vec.add(l_sum_vec);
                arg3.store(l_sum_vec_2);
                CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
            }
            {
                const std::string fn_name{"sample_vec_add_op"};
                Function f2 = l_module.get_function(fn_name);
                f2.verify();
            }
        }
        {
            Function fn("set_vector_value_idx1", FnContext{vec_set_args_type.pointer_type()}, l_module);
            {
                CodeSection l_fn_body = fn.mk_section("test_fn_body");
                l_fn_body.enter();
                ValueInfo ctx = CodeSectionContext::current_context();
                ValueInfo arg1_ptr = ctx.field("arg1").load();
                ValueInfo arg1 = arg1_ptr.load();
                ValueInfo v = ctx.field("v").load();
                ValueInfo new_arg1 = arg1.store_vector_entry(1, v);
                arg1_ptr.store(new_arg1);
                CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
            }
        }
        {
            Function fn("load_vector_value_idx1", FnContext{vec_load_args_type.pointer_type()}, l_module);
            {
                CodeSection l_fn_body = fn.mk_section("test_fn_body");
                l_fn_body.enter();
                ValueInfo ctx = CodeSectionContext::current_context();
                ValueInfo arg1 = ctx.field("arg1").load().load();
                ValueInfo r = arg1.load_vector_entry(1);
                CodeSectionContext::set_return_value(r);
            }
        }
        INIT_MODULE(l_module)
    }
    CODEGEN_LINE(jit_runner.add_module(l_cursor))
    CODEGEN_LINE(jit_runner.bind())

    CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
    CODEGEN_LINE(const runtime::Struct& l_vec_add_struct = l_runtime_module.struct_info("vec_add_args"))
    CODEGEN_LINE(const runtime::Struct& l_vec_set_struct = l_runtime_module.struct_info("vec_set_args"))
    CODEGEN_LINE(const runtime::Struct& l_vec_load_struct = l_runtime_module.struct_info("vec_load_args"))
    CODEGEN_LINE(const runtime::EventFn& vec_add_fn = l_runtime_module.event_fn_info("sample_vec_add_op"))
    CODEGEN_LINE(const runtime::EventFn& vec_set_fn = l_runtime_module.event_fn_info("set_vector_value_idx1"))
    CODEGEN_LINE(const runtime::EventFn& vec_load_fn = l_runtime_module.event_fn_info("load_vector_value_idx1"))
    LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not vec_add_fn.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not vec_set_fn.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not vec_load_fn.has_error());

    {
        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Array arr1_obj = runtime::Array::from(runtime::type_t::int32, 10))
            CODEGEN_LINE(runtime::Array arr2_obj = runtime::Array::from(runtime::type_t::int32, 10))
            CODEGEN_LINE(runtime::Array arr3_obj = runtime::Array::from(runtime::type_t::int32, 10))
            for (int32_t j = 0; j != 10; ++j) {
                CODEGEN_LINE(arr1_obj.set<int32_t>(static_cast<uint32_t>(j), i + j))
                CODEGEN_LINE(arr2_obj.set<int32_t>(static_cast<uint32_t>(j), i + j + 6))
                CODEGEN_LINE(arr3_obj.set<int32_t>(static_cast<uint32_t>(j), 0))
            }
            CODEGEN_LINE(arr1_obj.freeze())
            CODEGEN_LINE(arr2_obj.freeze())
            CODEGEN_LINE(arr3_obj.freeze())
            CODEGEN_LINE(runtime::Object l_args_obj = l_vec_add_struct.mk_object())
            CODEGEN_LINE(l_args_obj.set_array("arg1", arr1_obj))
            CODEGEN_LINE(l_args_obj.set_array("arg2", arr2_obj))
            CODEGEN_LINE(l_args_obj.set_array("arg3", arr3_obj))
            CODEGEN_LINE(l_args_obj.freeze())
            CODEGEN_LINE(vec_add_fn.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(arr3_obj.get<int32_t>(0), (4 * (i + 0) + 12));
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(1) == (4 * (i + 1) + 12));
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(2) == (4 * (i + 2) + 12));
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(3) == (4 * (i + 3) + 12));
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(4) == (4 * (i + 4) + 12));
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(5) == 0);
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(6) == 0);
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(7) == 0);
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(8) == 0);
            LLVM_BUILDER_ALWAYS_ASSERT(arr3_obj.get<int32_t>(9) == 0);
        }
    }
    {
        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            for (int32_t j = 0; j != 10; ++j) {
                arr_obj.set<int32_t>(static_cast<uint32_t>(j), 0);
            }
            arr_obj.freeze();
            CODEGEN_LINE(runtime::Object l_args_obj = l_vec_set_struct.mk_object())
            l_args_obj.set_array("arg1", arr_obj);
            l_args_obj.set<int32_t>("v", i * 10);
            l_args_obj.freeze();
            CODEGEN_LINE(vec_set_fn.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT(arr_obj.get<int32_t>(1) == i * 10);
        }
    }
    {
        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Array arr_obj = runtime::Array::from(runtime::type_t::int32, 10))
            for (int32_t j = 0; j != 10; ++j) {
                arr_obj.set<int32_t>(static_cast<uint32_t>(j), i * 100 + j);
            }
            arr_obj.freeze();
            CODEGEN_LINE(runtime::Object l_args_obj = l_vec_load_struct.mk_object())
            l_args_obj.set_array("arg1", arr_obj);
            l_args_obj.freeze();
            CODEGEN_LINE(int32_t result = vec_load_fn.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(result, i * 100 + 1);
        }
    }
}

TEST(LLVM_CODEGEN, multi_module) {
    CODEGEN_LINE(Cursor l_cursor{"module"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(JustInTimeRunner jit_runner)

    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    std::vector<member_field_entry> multi_args_fields;
    multi_args_fields.emplace_back("arg1", int32_type);
    multi_args_fields.emplace_back("arg2", int32_type);
    multi_args_fields.emplace_back("arg3", int32_type);
    CODEGEN_LINE(TypeInfo multi_args_type = TypeInfo::mk_struct("multi_args", multi_args_fields))

    CODEGEN_LINE(l_cursor.bind())
    {
        Module l_module = l_cursor.gen_module();
        Module::Context l_module_ctx{l_module};
        {
            Function fn_ext("fn2", FnContext{int32_type}, l_module);

            Function fn_local_ext("fn2_local", FnContext{int32_type}, l_module);

            Function fn("fn1", FnContext{multi_args_type.pointer_type()}, l_module);
            {
                CodeSection l_fn_body = fn.mk_section("test_fn_body");
                l_fn_body.enter();
                ValueInfo ctx = CodeSectionContext::current_context();
                ValueInfo arg1 = ctx.field("arg1").load();
                ValueInfo arg2 = ctx.field("arg2").load();
                ValueInfo c1 = ValueInfo::from_constant(101);
                ValueInfo c2 = ValueInfo::from_constant(999);
                ValueInfo res = arg1.add(arg2);
                ValueInfo res2 = res.add(c1);
                ValueInfo res3 = res2.add(c2);
                ValueInfo res4 = res2.add(ctx.field("arg3").load());
                ValueInfo res5 = res3.add(res4);
                ValueInfo res6 = fn_ext.call_fn(res5);
                ValueInfo res7 = fn_local_ext.call_fn(res6);
                CodeSectionContext::set_return_value(res7);
            }
            jit_runner.process_module_fn(fn);
            {
                const std::string fn_name{"fn1"};
                Function f2 = l_module.get_function(fn_name);
                f2.verify();
            }
        }
        INIT_MODULE(l_module)
    }
    {
        Module l_module = l_cursor.gen_module();
        Module::Context l_module_ctx{l_module};
        TypeInfo float32_type = TypeInfo::mk_float32();
        {
            Function fn("fn2", FnContext{int32_type}, l_module);
            {
                CodeSection l_fn_body = fn.mk_section("test_fn_body");
                l_fn_body.enter();
                ValueInfo arg = CodeSectionContext::current_context();
                ValueInfo res = arg.add(arg);
                CodeSectionContext::set_return_value(res);
            }
            jit_runner.process_module_fn(fn);
            {
                const std::string fn_name{"fn2"};
                Function f2 = l_module.get_function(fn_name);
                f2.verify();
            }
        }
        {
            Function fn("fn2_local", FnContext{int32_type}, l_module);
            {
                CodeSection l_fn_body = fn.mk_section("test_fn_body");
                l_fn_body.enter();
                ValueInfo arg = CodeSectionContext::current_context();
                ValueInfo res = arg.add(arg);
                CodeSectionContext::set_return_value(res);
            }
            jit_runner.process_module_fn(fn);
            {
                const std::string fn_name{"fn2"};
                Function f2 = l_module.get_function(fn_name);
                f2.verify();
            }
        }
        INIT_MODULE(l_module)
    }
    CODEGEN_LINE(jit_runner.add_module(l_cursor))
    CODEGEN_LINE(jit_runner.bind())

    CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
    CODEGEN_LINE(const runtime::Struct& l_multi_struct = l_runtime_module.struct_info("multi_args"))
    CODEGEN_LINE(const runtime::EventFn& fn1_fn = l_runtime_module.event_fn_info("fn1"))
    LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn1_fn.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(fn1_fn.is_init());

    {
        const int32_t golden_set[10] = { 2410, 2420, 2430, 2440, 2450,
                                      2460, 2470, 2480, 2490, 2500};
        for (int32_t i = 0; i != 10; ++i) {
            CODEGEN_LINE(runtime::Object l_args_obj = l_multi_struct.mk_object())
            l_args_obj.set<int32_t>("arg1", i);
            l_args_obj.set<int32_t>("arg2", i + 1);
            l_args_obj.set<int32_t>("arg3", i + 2);
            l_args_obj.freeze();
            CODEGEN_LINE(int32_t result = fn1_fn.on_event(l_args_obj))
            LLVM_BUILDER_ALWAYS_ASSERT_EQ(golden_set[i] * 2, result);
        }
    }
}

TEST(LLVM_CODEGEN, resilient_api) {
    CODEGEN_LINE(Function fn_save);
    CODEGEN_LINE(Cursor l_cursor{"lexical_ctx"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())

    auto gen_fn = [&int32_type] (const FnContext& fn_context, const std::string& fn_name, Module& mod, int version) -> Function {
        CODEGEN_LINE(Function fn(fn_name, fn_context, mod))
        {
            CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
            CODEGEN_LINE(l_fn_body.enter())
            CODEGEN_LINE(ValueInfo ctx = CodeSectionContext::current_context())
            CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
            CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
            CODEGEN_LINE(ValueInfo arg3 = ctx.field("arg3").load())
            CODEGEN_LINE(arg1.push("arg_in"_cs))
            CODEGEN_LINE(CodeSectionContext::push_var_context())
            CODEGEN_LINE(arg2.push("arg_in"_cs))
            CODEGEN_LINE(CodeSectionContext::push_var_context())
            CODEGEN_LINE(arg3.push("arg_in"_cs))
            CODEGEN_LINE(CodeSectionContext::push_var_context())


            if (version == 0) {
                LLVM_BUILDER_ASSERT(not l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(not l_fn_body.is_commit());
                CODEGEN_LINE(CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs)))
                LLVM_BUILDER_ASSERT(l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(l_fn_body.is_commit());
            }
            CODEGEN_LINE(CodeSectionContext::pop_var_context())
            if (version == 1) {
                LLVM_BUILDER_ASSERT(not l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(not l_fn_body.is_commit());
                CODEGEN_LINE(CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs)))
                LLVM_BUILDER_ASSERT(l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(l_fn_body.is_commit());
            }
            CODEGEN_LINE(CodeSectionContext::pop_var_context())
            if (version == 2) {
                LLVM_BUILDER_ASSERT(not l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(not l_fn_body.is_commit());
                CODEGEN_LINE(CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs)))
                LLVM_BUILDER_ASSERT(l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(l_fn_body.is_commit());
            }
            CODEGEN_LINE(CodeSectionContext::pop_var_context())
            if (version == 3) {
                LLVM_BUILDER_ASSERT(not l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(not l_fn_body.is_commit());
                CODEGEN_LINE(CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs)))
                LLVM_BUILDER_ASSERT(l_fn_body.is_sealed());
                LLVM_BUILDER_ASSERT(l_fn_body.is_commit());
            }
        }
        return fn;
    };

    std::vector<member_field_entry> resilient_args_fields;
    resilient_args_fields.emplace_back("arg1", int32_type);
    resilient_args_fields.emplace_back("arg2", int32_type);
    resilient_args_fields.emplace_back("arg3", int32_type);
    CODEGEN_LINE(TypeInfo resilient_args_type = TypeInfo::mk_struct("resilient_args", resilient_args_fields))
    CODEGEN_LINE(FnContext fn_context{resilient_args_type.pointer_type()});

    CODEGEN_LINE(Cursor::Context::value().bind())
    CODEGEN_LINE(Module l_module = Cursor::Context::value().main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    LLVM_BUILDER_ASSERT(fn_save.has_error());
    fn_save = gen_fn(fn_context, "fn1", l_module, 0);
    LLVM_BUILDER_ASSERT(not fn_save.has_error());
    gen_fn(fn_context, "fn2", l_module, 1);
    gen_fn(fn_context, "fn3", l_module, 2);
    gen_fn(fn_context, "fn4", l_module, 3);
    LLVM_BUILDER_ALWAYS_ASSERT(l_module.is_init());
    LLVM_BUILDER_ASSERT(not fn_save.has_error());
    LLVM_BUILDER_ASSERT(not ErrorContext::has_error());
    LLVM_BUILDER_ASSERT(not fn_save.has_error());

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(jit_runner.add_module(l_cursor))
    CODEGEN_LINE(jit_runner.bind())

    CODEGEN_LINE(const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace())
    CODEGEN_LINE(const runtime::Struct& l_resilient_struct = l_runtime_module.struct_info("resilient_args"))
    CODEGEN_LINE(const runtime::EventFn& fn1_fn = l_runtime_module.event_fn_info("fn1"))
    CODEGEN_LINE(const runtime::EventFn& fn2_fn = l_runtime_module.event_fn_info("fn2"))
    CODEGEN_LINE(const runtime::EventFn& fn3_fn = l_runtime_module.event_fn_info("fn3"))
    CODEGEN_LINE(const runtime::EventFn& fn4_fn = l_runtime_module.event_fn_info("fn4"))
    LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn1_fn.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn2_fn.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn3_fn.has_error());
    LLVM_BUILDER_ALWAYS_ASSERT(not fn4_fn.has_error());

    for (int32_t i = 0; i != 10; ++i) {
        CODEGEN_LINE(runtime::Object l_args_obj = l_resilient_struct.mk_object())
        l_args_obj.set<int32_t>("arg1", i);
        l_args_obj.set<int32_t>("arg2", i + 1);
        l_args_obj.set<int32_t>("arg3", i + 2);
        l_args_obj.freeze();

        LLVM_BUILDER_ALWAYS_ASSERT_EQ(i + 2, fn1_fn.on_event(l_args_obj));
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(i + 2, fn2_fn.on_event(l_args_obj));
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(i + 1, fn3_fn.on_event(l_args_obj));
        LLVM_BUILDER_ALWAYS_ASSERT_EQ(i, fn4_fn.on_event(l_args_obj));
    }
}
