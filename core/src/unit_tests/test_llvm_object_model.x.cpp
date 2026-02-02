//
// Created by vibhanshu on 2026-01-05
//

#include "gtest/gtest.h"
#include <cstdint>
#include "util/debug.h"
#include "core/util/defines.h"
#include "util/logger.h"

#include "core/llvm/module.h"
#include "core/llvm/type.h"
#include "core/llvm/jit.h"
#include "core/llvm/function.h"
#include "core/llvm/control_flow.h"

#include "common_llvm_test.h"

using namespace core;
using namespace core::llvm_proxy;

TEST(LLVM_CODEGEN_OBJECT, complex_struct_definitions) {
    CODEGEN_LINE(Cursor l_cursor{"struct_def_tests"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo void_type = TypeInfo::mk_void())
    CODEGEN_LINE(TypeInfo bool_type = TypeInfo::mk_bool())
    std::vector<member_field_entry> l_field_list;
    CODEGEN_LINE(l_field_list.emplace_back("field_1", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("field_2", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("field_3", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("field_4", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("field_5", int32_type))
    CODEGEN_LINE(TypeInfo l_struct_type = TypeInfo::mk_struct("test_struct", l_field_list, false))
    CODEGEN_LINE(l_field_list.emplace_back("field_6", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("field_7", l_struct_type.pointer_type()))
    CODEGEN_LINE(l_field_list.emplace_back("field_8", int32_type))
    CODEGEN_LINE(TypeInfo l_inner_struct = TypeInfo::mk_struct("inner_struct_2", l_field_list, false))
    CODEGEN_LINE(TypeInfo l_outer_struct)
    {
        std::vector<member_field_entry> l_field_list;
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_1", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_2", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_3", l_struct_type.pointer_type()))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_4", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_5", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_6", l_inner_struct.pointer_type()))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_7", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_8", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_9", l_inner_struct.pointer_type()))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_10", int32_type))
        CODEGEN_LINE(l_field_list.emplace_back("inner_field_11", l_inner_struct.pointer_type()))
        CODEGEN_LINE(l_outer_struct = TypeInfo::mk_struct("outer_struct", l_field_list, false))
    }
    std::vector<member_field_entry> big_struct_args_fields;
    big_struct_args_fields.emplace_back("arg_struct_type", l_struct_type.pointer_type());
    big_struct_args_fields.emplace_back("arg_inner_struct", l_inner_struct.pointer_type());
    big_struct_args_fields.emplace_back("arg_outer_struct", l_outer_struct.pointer_type());
    CODEGEN_LINE(TypeInfo big_struct_args_type = TypeInfo::mk_struct("big_struct_args", big_struct_args_fields))

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(Module l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    {
        CODEGEN_LINE(Function fn)
        {
            CODEGEN_LINE(FunctionBuilder fn_builder)
            CODEGEN_LINE(fn_builder.set_context(FnContext{big_struct_args_type.pointer_type()})
                .set_module(l_module)
                .set_name("big_struct_test_fn"))
            CODEGEN_LINE(fn = fn_builder.compile())

            {
                CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
                CODEGEN_LINE(l_fn_body.enter())

                CODEGEN_LINE(ValueInfo ctx = fn.context().value())
                CODEGEN_LINE(ValueInfo arg1a = ctx.field("arg_struct_type").load())
                CODEGEN_LINE(ValueInfo arg1b = ctx.field("arg_inner_struct").load())
                CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg_outer_struct").load())
                CORE_ALWAYS_ASSERT(not l_struct_type.has_error());
                CORE_ALWAYS_ASSERT(l_struct_type.pointer_type().check_sync(arg1a));
                CORE_ALWAYS_ASSERT(l_inner_struct.pointer_type().check_sync(arg1b));
                CORE_ALWAYS_ASSERT(l_outer_struct.pointer_type().check_sync(arg2));

                // inner_field_4 = inner_field_1
                // inner_field_5 = inner_field_2
                CODEGEN_LINE(arg2.field("inner_field_4").store(arg2.field("inner_field_1").load()))
                CODEGEN_LINE(arg2.field("inner_field_5").store(arg2.field("inner_field_2").load()))

                // arg1b.field_7 = arg1a
                {
                    CODEGEN_LINE(ValueInfo field_7 = arg1b.field("field_7").load())
                    for (const std::string& name : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4", "field_5"}}) {
                        CODEGEN_LINE(ValueInfo l_dst_ptr =  field_7.field(name))
                        CODEGEN_LINE(ValueInfo l_src_value = arg1a.field(name).load())
                        CODEGEN_LINE(l_dst_ptr.store(l_src_value))
                    }
                }

                // "inner_field_3" = arg1a
                {
                    CODEGEN_LINE(ValueInfo inner_field_3 = arg2.field("inner_field_3").load())
                    CORE_ALWAYS_ASSERT(inner_field_3.type().is_pointer());
                    CORE_ALWAYS_ASSERT(inner_field_3.type().base_type().is_struct());

                    for (const std::string& name : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4", "field_5"}}) {
                        CODEGEN_LINE(ValueInfo l_value_inner =  inner_field_3.field(name))
                        CODEGEN_LINE(ValueInfo l_src_value_ptr = arg1a.field(name))
                        CODEGEN_LINE(ValueInfo l_src_value = l_src_value_ptr)
                        CODEGEN_LINE(l_value_inner.store(l_src_value.load()))
                    }
                }

                // "inner_field_6" = arg1b
                {
                    CODEGEN_LINE(ValueInfo inner_field_6 = arg2.field("inner_field_6").load())
                    CORE_ALWAYS_ASSERT(inner_field_6.type().is_pointer());
                    CORE_ALWAYS_ASSERT(inner_field_6.type().base_type().is_struct());

                    for (const std::string& name : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4", "field_5", "field_6", "field_8"}}) {
                        CODEGEN_LINE(ValueInfo l_value_inner =  inner_field_6.field(name))
                        CODEGEN_LINE(ValueInfo l_src_value_ptr = arg1b.field(name))
                        CODEGEN_LINE(ValueInfo l_src_value = l_src_value_ptr)
                        CODEGEN_LINE(l_value_inner.store(l_src_value.load()))
                    }
                }

                // "inner_field_11" = "inner_field_9"
                {
                    CODEGEN_LINE(ValueInfo inner_field_9 =  arg2.field("inner_field_9").load())
                    CODEGEN_LINE(ValueInfo inner_field_11 = arg2.field("inner_field_11").load())
                    CORE_ALWAYS_ASSERT(inner_field_9.type().is_pointer())
                    CORE_ALWAYS_ASSERT(inner_field_9.type().base_type().is_struct())
                    CORE_ALWAYS_ASSERT(inner_field_11.type().is_pointer())
                    CORE_ALWAYS_ASSERT(inner_field_11.type().base_type().is_struct())
                    for (const std::string& name : std::initializer_list<std::string>{{"field_1", "field_2", "field_3", "field_4", "field_5", "field_6", "field_8"}}) {
                        CODEGEN_LINE(ValueInfo l_value_inner = inner_field_11.field(name));
                        CODEGEN_LINE(ValueInfo l_src_value = inner_field_9.field(name).load());
                        CODEGEN_LINE(l_value_inner.store(l_src_value));

                    }
                }
                CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
            }
            CODEGEN_LINE(jit_runner.process_module_fn(fn))
            {
                const std::string fn_name{"big_struct_test_fn"};
                Function f2 = l_module.get_function(fn_name);
                f2.verify();
            }
        }
        INIT_MODULE(l_module)
        CodeSectionContext::assert_no_context();
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    {
        const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace();
        const RuntimeStruct& l_big_struct_args = l_runtime_module.struct_info("big_struct_args");
        const RuntimeStruct& l_outer_struct = l_runtime_module.struct_info("outer_struct");
        const RuntimeStruct& l_inner_struct = l_runtime_module.struct_info("inner_struct_2");
        const RuntimeStruct& l_test_struct = l_runtime_module.struct_info("test_struct");
        const RuntimeEventFn& test_fn = l_runtime_module.event_fn_info("big_struct_test_fn");
        CORE_ALWAYS_ASSERT(not l_runtime_module.has_error())
        CORE_ALWAYS_ASSERT(not l_big_struct_args.has_error())
        CORE_ALWAYS_ASSERT(not l_outer_struct.has_error())
        CORE_ALWAYS_ASSERT(not l_inner_struct.has_error())
        CORE_ALWAYS_ASSERT(not l_test_struct.has_error())
        CORE_ALWAYS_ASSERT(not test_fn.has_error())

        CORE_ALWAYS_ASSERT_EQ(l_big_struct_args.num_fields(), 3);
        CORE_ALWAYS_ASSERT_EQ(l_outer_struct.num_fields(), 11);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct.num_fields(), 8);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct.num_fields(), 5);

        // struct big_struct_args_struct {
        //     void *arg_struct_type;
        //     void *arg_inner_struct;
        //     void *arg_outer_struct;
        // } ;
        // using raw_fn_t = int32_t(big_struct_args_struct*);
        // raw_fn_t* fn = jit_runner.get_fn<raw_fn_t>("struct_def_tests_big_struct_test_fn");
        // CORE_ALWAYS_ASSERT(fn);

        // Notes
        // 1. test_struct ->
        // 2. inner_struct_2  ->  [field_7:test_struct]
        // 3. outer_struct  ->  [inner_field_3:test_struct,
        //                       inner_field_6:inner_struct_2,
        //                       inner_field_9:inner_struct_2,
        //                       inner_field_11:inner_struct_2 ]
        //

        auto gen_inner_struct_obj = [&]() -> RuntimeObject {
            RuntimeObject l_inner_struct_field7_obj = l_test_struct.mk_object();
            l_inner_struct_field7_obj.try_freeze();
            RuntimeObject l_inner_struct_obj = l_inner_struct.mk_object();
            l_inner_struct_obj.set_object("field_7", l_inner_struct_field7_obj);
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            return l_inner_struct_obj;
        };

        auto gen_outer_struct_base = [&]() -> RuntimeObject {
            RuntimeObject l_field_3_obj = l_test_struct.mk_object();
            l_field_3_obj.try_freeze();
            RuntimeObject l_field_6_obj = gen_inner_struct_obj();
            l_field_6_obj.try_freeze();
            RuntimeObject l_outer_struct_obj = l_outer_struct.mk_object();
            l_outer_struct_obj.set_object("inner_field_3", l_field_3_obj);
            l_outer_struct_obj.set_object("inner_field_6", l_field_6_obj);
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            return l_outer_struct_obj;
        };


        for (int32_t i = 0; i != 3; ++i) {
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            CODEGEN_LINE(RuntimeObject l_test_struct_obj = l_test_struct.mk_object())
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            CODEGEN_LINE(RuntimeObject l_inner_struct_obj = gen_inner_struct_obj())
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            CODEGEN_LINE(RuntimeObject l_outer_struct_obj = gen_outer_struct_base())
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            CODEGEN_LINE(RuntimeObject l_big_struct_args_obj = l_big_struct_args.mk_object())
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            {
                l_test_struct_obj.set<int32_t>("field_1", i);
                l_test_struct_obj.set<int32_t>("field_2", i + 1);
                l_test_struct_obj.set<int32_t>("field_3", i + 2);
                l_test_struct_obj.set<int32_t>("field_4", i + 3);
                l_test_struct_obj.set<int32_t>("field_5", i + 4);
                l_test_struct_obj.try_freeze();
            }
            {
                l_inner_struct_obj.set<int32_t>("field_1", i + 5);
                l_inner_struct_obj.set<int32_t>("field_2", i + 6);
                l_inner_struct_obj.set<int32_t>("field_3", i + 7);
                l_inner_struct_obj.set<int32_t>("field_4", i + 8);
                l_inner_struct_obj.set<int32_t>("field_5", i + 9);
                l_inner_struct_obj.set<int32_t>("field_6", i + 10);
                l_inner_struct_obj.set<int32_t>("field_8", i + 12);
                l_inner_struct_obj.try_freeze();
            }
            RuntimeObject l_field_9_obj = gen_inner_struct_obj();
            {
                l_field_9_obj.set<int32_t>("field_1", i + 105);
                l_field_9_obj.set<int32_t>("field_2", i + 106);
                l_field_9_obj.set<int32_t>("field_3", i + 107);
                l_field_9_obj.set<int32_t>("field_4", i + 108);
                l_field_9_obj.set<int32_t>("field_5", i + 109);
                l_field_9_obj.set<int32_t>("field_6", i + 1010);
                l_field_9_obj.set<int32_t>("field_8", i + 1012);
                l_field_9_obj.try_freeze();
            }
            RuntimeObject l_field_11_obj = gen_inner_struct_obj();
            l_field_11_obj.try_freeze();
            {
                CODEGEN_LINE(l_outer_struct_obj.set<int32_t>("inner_field_1", i + 1111))
                CODEGEN_LINE(l_outer_struct_obj.set<int32_t>("inner_field_2", i + 1112))
                l_outer_struct_obj.set_object("inner_field_9", l_field_9_obj);
                l_outer_struct_obj.set_object("inner_field_11", l_field_11_obj);
                l_outer_struct_obj.try_freeze();
            }
            {
                l_big_struct_args_obj.set_object("arg_struct_type", l_test_struct_obj);
                l_big_struct_args_obj.set_object("arg_inner_struct", l_inner_struct_obj);
                l_big_struct_args_obj.set_object("arg_outer_struct", l_outer_struct_obj);
                l_big_struct_args_obj.try_freeze();
            }
            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            test_fn.on_event(l_big_struct_args_obj);
            {
                CODEGEN_LINE(RuntimeObject inner_field_7 = l_inner_struct_obj.get_object("field_7"))
                CORE_ALWAYS_ASSERT_EQ(inner_field_7.get<int32_t>("field_1"), i)
                CORE_ALWAYS_ASSERT_EQ(inner_field_7.get<int32_t>("field_2"), i + 1)
                CORE_ALWAYS_ASSERT_EQ(inner_field_7.get<int32_t>("field_3"), i + 2)
                CORE_ALWAYS_ASSERT_EQ(inner_field_7.get<int32_t>("field_4"), i + 3)
                CORE_ALWAYS_ASSERT_EQ(inner_field_7.get<int32_t>("field_5"), i + 4)
            }
            {
                CORE_ALWAYS_ASSERT_EQ(l_outer_struct_obj.get<int32_t>("inner_field_4"), i+1111)
                CORE_ALWAYS_ASSERT_EQ(l_outer_struct_obj.get<int32_t>("inner_field_5"), i+1112)
            }
            {
                CODEGEN_LINE(RuntimeObject inner_field_3 = l_outer_struct_obj.get_object("inner_field_3"))
                CORE_ALWAYS_ASSERT_EQ(inner_field_3.get<int32_t>("field_1"), i)
                CORE_ALWAYS_ASSERT_EQ(inner_field_3.get<int32_t>("field_2"), i + 1)
                CORE_ALWAYS_ASSERT_EQ(inner_field_3.get<int32_t>("field_3"), i + 2)
                CORE_ALWAYS_ASSERT_EQ(inner_field_3.get<int32_t>("field_4"), i + 3)
                CORE_ALWAYS_ASSERT_EQ(inner_field_3.get<int32_t>("field_5"), i + 4)
            }
            {
                CODEGEN_LINE(RuntimeObject inner_field_6 = l_outer_struct_obj.get_object("inner_field_6"))
                CORE_ALWAYS_ASSERT_EQ(inner_field_6.get<int32_t>("field_1"), i + 5)
                CORE_ALWAYS_ASSERT_EQ(inner_field_6.get<int32_t>("field_2"), i + 6)
                CORE_ALWAYS_ASSERT_EQ(inner_field_6.get<int32_t>("field_3"), i + 7)
                CORE_ALWAYS_ASSERT_EQ(inner_field_6.get<int32_t>("field_4"), i + 8)
                CORE_ALWAYS_ASSERT_EQ(inner_field_6.get<int32_t>("field_5"), i + 9)
                CORE_ALWAYS_ASSERT_EQ(inner_field_6.get<int32_t>("field_6"), i + 10)
                CORE_ALWAYS_ASSERT_EQ(inner_field_6.get<int32_t>("field_8"), i + 12)
            }
            {
                CODEGEN_LINE(RuntimeObject inner_field_11 = l_outer_struct_obj.get_object("inner_field_11"))
                CORE_ALWAYS_ASSERT_EQ(inner_field_11.get<int32_t>("field_1"), i + 105)
                CORE_ALWAYS_ASSERT_EQ(inner_field_11.get<int32_t>("field_2"), i + 106)
                CORE_ALWAYS_ASSERT_EQ(inner_field_11.get<int32_t>("field_3"), i + 107)
                CORE_ALWAYS_ASSERT_EQ(inner_field_11.get<int32_t>("field_4"), i + 108)
                CORE_ALWAYS_ASSERT_EQ(inner_field_11.get<int32_t>("field_5"), i + 109)
                CORE_ALWAYS_ASSERT_EQ(inner_field_11.get<int32_t>("field_6"), i + 1010)
                CORE_ALWAYS_ASSERT_EQ(inner_field_11.get<int32_t>("field_8"), i + 1012)
            }
        }
    }
    {
        const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace();
        const RuntimeStruct& l_test_struct = l_runtime_module.struct_info("test_struct");
        CORE_ALWAYS_ASSERT(l_runtime_module.is_bind());
        CORE_ALWAYS_ASSERT(not l_runtime_module.has_error());
        CORE_ALWAYS_ASSERT(not l_test_struct.has_error());
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_1"].offset(), 0);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_2"].offset(), 4);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_3"].offset(), 8);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_4"].offset(), 12);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_5"].offset(), 16);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_5"].offset(), 16);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_6"].offset(), std::numeric_limits<int32_t>::max());
        CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
    }
    {
        const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace();
        const RuntimeStruct& l_test_struct = l_runtime_module.struct_info("inner_struct_2");
        CORE_ALWAYS_ASSERT(l_runtime_module.is_bind());
        CORE_ALWAYS_ASSERT(not l_runtime_module.has_error());
        CORE_ALWAYS_ASSERT(not l_test_struct.has_error());
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_1"].offset(), 0);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_2"].offset(), 4);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_3"].offset(), 8);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_4"].offset(), 12);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_5"].offset(), 16);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_6"].offset(), 20);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_7"].offset(), 24);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_8"].offset(), 32);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["field_9"].offset(), std::numeric_limits<int32_t>::max());
        CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
    }
    {
        const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace();
        const RuntimeStruct& l_test_struct = l_runtime_module.struct_info("outer_struct");
        CORE_ALWAYS_ASSERT_EQ(l_test_struct.size_in_bytes(), 64);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_1"].offset(), 0);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_2"].offset(), 4);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_3"].offset(), 8);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_4"].offset(), 16);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_5"].offset(), 20);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_6"].offset(), 24);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_7"].offset(), 32);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_8"].offset(), 36);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_9"].offset(), 40);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_10"].offset(), 48);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_11"].offset(), 56);
        CORE_ALWAYS_ASSERT_EQ(l_test_struct["inner_field_12"].offset(), std::numeric_limits<int32_t>::max());
    }
}

TEST(LLVM_CODEGEN_OBJECT, complex_struct_definitions_v2) {
    CODEGEN_LINE(Cursor l_cursor{"struct_type_test_v2"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo int32_arr7_type = TypeInfo::mk_array(int32_type, 7))
    CODEGEN_LINE(TypeInfo int32_arr7_ptr_type = int32_arr7_type.pointer_type())
    CODEGEN_LINE(TypeInfo int32_vec7_type = TypeInfo::mk_vector(int32_type, 7))
    CODEGEN_LINE(TypeInfo int32_vec7_ptr_type = int32_vec7_type.pointer_type())
    CODEGEN_LINE(TypeInfo double_type = TypeInfo::mk_float64())
    CODEGEN_LINE(TypeInfo double_arr7_type = TypeInfo::mk_array(double_type, 7))
    CODEGEN_LINE(TypeInfo double_arr7_ptr_type = double_arr7_type.pointer_type())
    CODEGEN_LINE(TypeInfo double_vec7_type = TypeInfo::mk_vector(double_type, 7))
    CODEGEN_LINE(TypeInfo double_vec7_ptr_type = double_vec7_type.pointer_type())
    std::vector<member_field_entry> l_field_list;
    CODEGEN_LINE(l_field_list.emplace_back("int_field_1", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("int_field_2", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("int_arr_field_1", int32_arr7_ptr_type))
    CODEGEN_LINE(l_field_list.emplace_back("int_arr_field_2", int32_arr7_ptr_type))
    CODEGEN_LINE(TypeInfo l_inner_struct_type{})
    {
        std::vector<member_field_entry> l_inner_field_list;
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_1", double_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_2", int32_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_3", int32_arr7_ptr_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_3a", int32_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_4", int32_arr7_ptr_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_5", int32_arr7_ptr_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_6", int32_arr7_ptr_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_7", double_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_8", int32_type))
        CODEGEN_LINE(l_inner_field_list.emplace_back("field_9", double_type))
        CODEGEN_LINE(l_inner_struct_type = TypeInfo::mk_struct("inner_struct_v2", l_inner_field_list, false))
    }
    CODEGEN_LINE(TypeInfo l_inner_struct_ptr_type = l_inner_struct_type.pointer_type())
    CODEGEN_LINE(l_field_list.emplace_back("struct_field_1", l_inner_struct_ptr_type))
    CODEGEN_LINE(l_field_list.emplace_back("struct_field_2", l_inner_struct_ptr_type))
    CODEGEN_LINE(TypeInfo l_inner_struct_ptr_arr7_type = TypeInfo::mk_array(l_inner_struct_ptr_type, 7))
    CODEGEN_LINE(l_field_list.emplace_back("struct_arr_field_1", l_inner_struct_ptr_arr7_type.pointer_type()))
    CODEGEN_LINE(l_field_list.emplace_back("struct_arr_field_2", l_inner_struct_ptr_arr7_type.pointer_type()))
    CODEGEN_LINE(l_field_list.emplace_back("struct_field_3", l_inner_struct_ptr_type))
    CODEGEN_LINE(l_field_list.emplace_back("struct_field_5", l_inner_struct_ptr_type))
    CODEGEN_LINE(l_field_list.emplace_back("struct_field_6", l_inner_struct_ptr_type))
    CODEGEN_LINE(l_field_list.emplace_back("struct_field_4", l_inner_struct_ptr_type))
    CODEGEN_LINE(l_field_list.emplace_back("struct_field_7", l_inner_struct_ptr_type))
    CODEGEN_LINE(l_field_list.emplace_back("int_field_3", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("int_field_4", int32_type))
    CODEGEN_LINE(l_field_list.emplace_back("int_field_5", int32_type))
    CODEGEN_LINE(TypeInfo l_struct_type = TypeInfo::mk_struct("test_struct_v2", l_field_list, false))

    std::vector<member_field_entry> inner_copy_args_fields;
    inner_copy_args_fields.emplace_back("arg1", l_inner_struct_type.pointer_type());
    CODEGEN_LINE(TypeInfo inner_copy_args_type = TypeInfo::mk_struct("inner_copy_args_v2", inner_copy_args_fields))

    std::vector<member_field_entry> outer_copy_args_fields;
    outer_copy_args_fields.emplace_back("arg1", l_struct_type.pointer_type());
    CODEGEN_LINE(TypeInfo outer_copy_args_type = TypeInfo::mk_struct("outer_copy_args_v2", outer_copy_args_fields))
    CORE_ALWAYS_ASSERT(not ErrorContext::has_error())

    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(l_cursor.bind())
    {
        CODEGEN_LINE(Module l_module = l_cursor.main_module())
        CODEGEN_LINE(Module::Context l_module_ctx{l_module})
        {
            CODEGEN_LINE(FunctionBuilder fn_builder)
            CODEGEN_LINE(fn_builder.set_context(FnContext{inner_copy_args_type.pointer_type()})
                .set_module(l_module)
                .set_name("test_inner_field_copy"))
            CODEGEN_LINE(Function fn = fn_builder.compile())
            {
                CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
                CODEGEN_LINE(l_fn_body.enter())
                CODEGEN_LINE(ValueInfo ctx = fn.context().value())
                CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
                CODEGEN_LINE(ValueInfo field_1 = arg1.field("field_1"))
                CODEGEN_LINE(ValueInfo field_2 = arg1.field("field_2"))
                CODEGEN_LINE(ValueInfo field_3 = arg1.field("field_3").load())
                CODEGEN_LINE(ValueInfo field_4 = arg1.field("field_4").load())
                CODEGEN_LINE(ValueInfo field_5 = arg1.field("field_5").load())
                CODEGEN_LINE(ValueInfo field_6 = arg1.field("field_6"))
                CODEGEN_LINE(ValueInfo field_7 = arg1.field("field_7"))
                CODEGEN_LINE(ValueInfo field_8 = arg1.field("field_8"))

                CODEGEN_LINE(arg1.field("field_3a").store(field_2.load()))

                CORE_ALWAYS_ASSERT(field_5.type().is_pointer());
                CORE_ALWAYS_ASSERT(field_3.type().is_pointer());
                CORE_ALWAYS_ASSERT(field_5.type().base_type().is_array());
                CORE_ALWAYS_ASSERT(field_3.type().base_type().is_array());
                for (int i = 0; i != 5;  ++i) {
                    CODEGEN_LINE(ValueInfo l_value_ptr = field_5.entry(i));
                    CODEGEN_LINE(ValueInfo l_src_value = field_3.entry(i))
                    CODEGEN_LINE(l_value_ptr.store(l_src_value.load()))
                }

                CODEGEN_LINE(field_5.entry(2).store(field_2.load()))
                CODEGEN_LINE(field_7.store(field_1.load()))
                CODEGEN_LINE(ValueInfo field_4_3_value = field_4.entry(3).load())
                CODEGEN_LINE(field_8.store(field_4_3_value))
                CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
            }
        }
        {
            CODEGEN_LINE(FunctionBuilder fn_builder)
            CODEGEN_LINE(fn_builder.set_context(FnContext{outer_copy_args_type.pointer_type()})
                .set_module(l_module)
                .set_name("test_outer_field_copy"))
            CODEGEN_LINE(Function fn = fn_builder.compile())
            {
                CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
                CODEGEN_LINE(l_fn_body.enter())
                CODEGEN_LINE(ValueInfo outer_ctx = fn.context().value())
                CODEGEN_LINE(ValueInfo arg1 = outer_ctx.field("arg1").load())
                CORE_ALWAYS_ASSERT(arg1.type().is_pointer())
                // ValueInfo copy_inner_struct = arg1.struct_field("struct_field_1");
                // arg1.struct_field("struct_field_2").store_value(copy_inner_struct);

                auto copy_inner_st = [](ValueInfo v1, ValueInfo v2) {
                    CORE_ALWAYS_ASSERT(v1.type().is_pointer());
                    CORE_ALWAYS_ASSERT(v2.type().is_pointer());
                    CORE_ALWAYS_ASSERT(v1.type().base_type().is_struct());
                    CORE_ALWAYS_ASSERT(v2.type().base_type().is_struct());

                    for (const std::string& name : std::initializer_list<std::string>{{std::string{"field_1"}, std::string{"field_2"}, std::string{"field_3a"},
                                                                                    std::string{"field_7"}, std::string{"field_8"}, std::string{"field_9"}}}) {
                        CODEGEN_LINE(ValueInfo dst_ptr = v2.field(name))
                        CODEGEN_LINE(ValueInfo src_ptr = v1.field(name).load())
                        CODEGEN_LINE(dst_ptr.store(src_ptr))
                    }
                    for (const std::string& name : std::initializer_list<std::string>{{std::string{"field_3"}, std::string{"field_5"}}}) {
                        CODEGEN_LINE(ValueInfo dst_ptr = v2.field(name).load())
                        CODEGEN_LINE(ValueInfo src_ptr = v1.field(name).load())
                        CORE_ALWAYS_ASSERT(dst_ptr.type().is_pointer());
                        CORE_ALWAYS_ASSERT(src_ptr.type().is_pointer());
                        for (int i = 0; i != 7; ++i) {
                            CODEGEN_LINE(ValueInfo src_value = src_ptr.entry(i).load())
                            CODEGEN_LINE(ValueInfo dst_idx = dst_ptr.entry(i))
                            CODEGEN_LINE(dst_idx.store(src_value))
                        }
                    }
                    // TODO{vibhanshu}: fix vector copy operation
                    // std::string{"field_4"}, std::string{"field_6"}}}) {
                };

                // 1. struct_field_3 = struct_field_1
                // 2. struct_field_4 = struct_field_2
                // 3. struct_ptr_arr_field_1[0, 2, 4, 6] = struct_field_1
                // 4. struct_ptr_arr_field_1[1, 3, 5] = struct_field_2
                // 5. struct_ptr_arr_field_2[0, 2, 4, 6] = struct_field_2
                // 6. struct_ptr_arr_field_2[1, 3, 5] = struct_field_1
                // 7. struct_ptr_field_1 = struct_field_1
                // 8. struct_ptr_field_2 = struct_field_2
                // 9. struct_ptr_field_3 = struct_field_1

                CODEGEN_LINE(ValueInfo struct_field_1 = arg1.field("struct_field_1").load())
                ValueInfo struct_field_2 = arg1.field("struct_field_2").load();
                copy_inner_st(struct_field_1, arg1.field("struct_field_3").load());
                copy_inner_st(struct_field_2, arg1.field("struct_field_4").load());
                CODEGEN_LINE(ValueInfo struct_ptr_arr_field_1 = arg1.field("struct_arr_field_1").load())

                CORE_ALWAYS_ASSERT(not struct_field_1.has_error());
                copy_inner_st(struct_field_1, struct_ptr_arr_field_1.entry(0).load());
                copy_inner_st(struct_field_1, struct_ptr_arr_field_1.entry(2).load());
                copy_inner_st(struct_field_1, struct_ptr_arr_field_1.entry(4).load());
                copy_inner_st(struct_field_1, struct_ptr_arr_field_1.entry(6).load());
                copy_inner_st(struct_field_2, struct_ptr_arr_field_1.entry(1).load());
                copy_inner_st(struct_field_2, struct_ptr_arr_field_1.entry(3).load());
                copy_inner_st(struct_field_2, struct_ptr_arr_field_1.entry(5).load());

                ValueInfo struct_ptr_arr_field_2 = arg1.field("struct_arr_field_2").load();
                copy_inner_st(struct_field_2, struct_ptr_arr_field_2.entry(0).load());
                copy_inner_st(struct_field_2, struct_ptr_arr_field_2.entry(2).load());
                copy_inner_st(struct_field_2, struct_ptr_arr_field_2.entry(4).load());
                copy_inner_st(struct_field_2, struct_ptr_arr_field_2.entry(6).load());
                copy_inner_st(struct_field_1, struct_ptr_arr_field_2.entry(1).load());
                copy_inner_st(struct_field_1, struct_ptr_arr_field_2.entry(3).load());
                copy_inner_st(struct_field_1, struct_ptr_arr_field_2.entry(5).load());

                copy_inner_st(struct_field_1, arg1.field("struct_field_5").load());
                copy_inner_st(struct_field_2, arg1.field("struct_field_6").load());
                copy_inner_st(struct_field_1, arg1.field("struct_field_7").load());
                CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)))
            }
        }
        INIT_MODULE(l_module)
    }
    jit_runner.add_module(l_cursor);
    jit_runner.bind();
    CORE_ALWAYS_ASSERT(not ErrorContext::has_error())
    {
        const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace();
        const RuntimeStruct& l_inner_struct = l_runtime_module.struct_info("inner_struct_v2");
        CORE_ALWAYS_ASSERT(l_runtime_module.is_bind());
        CORE_ALWAYS_ASSERT(not l_runtime_module.has_error());
        CORE_ALWAYS_ASSERT(not l_inner_struct.has_error());

        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_1"].offset(), 0);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_2"].offset(), 8);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_3"].offset(), 16);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_3a"].offset(), 24);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_4"].offset(), 32);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_5"].offset(), 40);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_6"].offset(), 48);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_7"].offset(), 56);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_8"].offset(), 64);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_9"].offset(), 72);
        CORE_ALWAYS_ASSERT_EQ(l_inner_struct["field_10"].offset(), std::numeric_limits<int32_t>::max());
    }
    {
        const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace();
        const RuntimeStruct& l_test_struct = l_runtime_module.struct_info("test_struct_v2");
        CORE_ALWAYS_ASSERT(l_runtime_module.is_bind());
        CORE_ALWAYS_ASSERT(not l_runtime_module.has_error());
        CORE_ALWAYS_ASSERT(not l_test_struct.has_error());

        CORE_ALWAYS_ASSERT_EQ(112, l_test_struct.size_in_bytes());
        CORE_ALWAYS_ASSERT_EQ(16, l_test_struct.num_fields());

        CORE_ALWAYS_ASSERT_EQ(0, l_test_struct["int_field_1"].offset());
        CORE_ALWAYS_ASSERT_EQ(4, l_test_struct["int_field_2"].offset());
        CORE_ALWAYS_ASSERT_EQ(8, l_test_struct["int_arr_field_1"].offset());
        CORE_ALWAYS_ASSERT_EQ(16, l_test_struct["int_arr_field_2"].offset());
        CORE_ALWAYS_ASSERT_EQ(24, l_test_struct["struct_field_1"].offset());
        CORE_ALWAYS_ASSERT_EQ(32, l_test_struct["struct_field_2"].offset());
        CORE_ALWAYS_ASSERT_EQ(40, l_test_struct["struct_arr_field_1"].offset());
        CORE_ALWAYS_ASSERT_EQ(48, l_test_struct["struct_arr_field_2"].offset());
        CORE_ALWAYS_ASSERT_EQ(56, l_test_struct["struct_field_3"].offset());
        CORE_ALWAYS_ASSERT_EQ(64, l_test_struct["struct_field_5"].offset());
        CORE_ALWAYS_ASSERT_EQ(72, l_test_struct["struct_field_6"].offset());
        CORE_ALWAYS_ASSERT_EQ(80, l_test_struct["struct_field_4"].offset());
        CORE_ALWAYS_ASSERT_EQ(88, l_test_struct["struct_field_7"].offset());
        CORE_ALWAYS_ASSERT_EQ(96, l_test_struct["int_field_3"].offset());
        CORE_ALWAYS_ASSERT_EQ(100, l_test_struct["int_field_4"].offset());
        CORE_ALWAYS_ASSERT_EQ(104, l_test_struct["int_field_5"].offset());
    }
    {
        const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace();
        const RuntimeStruct& l_inner_struct_def = l_runtime_module.struct_info("inner_struct_v2");
        const RuntimeStruct& l_inner_copy_args_def = l_runtime_module.struct_info("inner_copy_args_v2");
        const RuntimeEventFn& fn = l_runtime_module.event_fn_info("test_inner_field_copy");
        CORE_ALWAYS_ASSERT(not fn.has_error());
        CORE_ALWAYS_ASSERT(not l_inner_struct_def.has_error());
        CORE_ALWAYS_ASSERT(not l_inner_copy_args_def.has_error());

        RuntimeObject l_inner_obj = l_inner_struct_def.mk_object();
        RuntimeArray field_3_arr = RuntimeArray::from(runtime_type_t::int32, 7);
        RuntimeArray field_4_arr = RuntimeArray::from(runtime_type_t::int32, 7);
        RuntimeArray field_5_arr = RuntimeArray::from(runtime_type_t::int32, 7);
        RuntimeArray field_6_arr = RuntimeArray::from(runtime_type_t::int32, 7);

        for (int32_t i = 0; i != 7; ++i) {
            field_3_arr.set<int32_t>(static_cast<uint32_t>(i), 10000 + i);
            field_4_arr.set<int32_t>(static_cast<uint32_t>(i), 20000 + i);
            field_6_arr.set<int32_t>(static_cast<uint32_t>(i), 30000 + i);
        }
        field_3_arr.try_freeze();
        field_4_arr.try_freeze();
        field_5_arr.try_freeze();
        field_6_arr.try_freeze();

        {
            l_inner_obj.set<double>("field_1", 109.109);
            l_inner_obj.set<int32_t>("field_2", 10101);
            l_inner_obj.set_array("field_3", field_3_arr);
            l_inner_obj.set_array("field_4", field_4_arr);
            l_inner_obj.set_array("field_5", field_5_arr);
            l_inner_obj.set_array("field_6", field_6_arr);
            l_inner_obj.try_freeze();
        }

        RuntimeObject l_args_obj = l_inner_copy_args_def.mk_object();
        {
            l_args_obj.set_object("arg1", l_inner_obj);
            l_args_obj.try_freeze();
        }

        fn.on_event(l_args_obj);

        CORE_ALWAYS_ASSERT_EQ(field_5_arr.get<int32_t>(0), 10000);
        CORE_ALWAYS_ASSERT_EQ(field_5_arr.get<int32_t>(1), 10001);
        CORE_ALWAYS_ASSERT_EQ(field_5_arr.get<int32_t>(2), 10101);
        CORE_ALWAYS_ASSERT_EQ(field_5_arr.get<int32_t>(3), 10003);
        CORE_ALWAYS_ASSERT_EQ(field_5_arr.get<int32_t>(4), 10004);
        CORE_ALWAYS_ASSERT_EQ(field_5_arr.get<int32_t>(5), 0);
        CORE_ALWAYS_ASSERT_EQ(field_5_arr.get<int32_t>(6), 0);
        {
            CORE_ALWAYS_ASSERT_EQ(l_inner_obj.get<int32_t>("field_3a"), 10101);
            CORE_ALWAYS_ASSERT_EQ(l_inner_obj.get<double>("field_7"), 109.109);
            CORE_ALWAYS_ASSERT_EQ(l_inner_obj.get<int32_t>("field_8"), 20003);
        }
    }
    {
        CODEGEN_LINE(const RuntimeNamespace& l_runtime_module = jit_runner.get_global_namespace())
        CODEGEN_LINE(const RuntimeStruct& l_inner_struct_def = l_runtime_module.struct_info("inner_struct_v2"))
        CODEGEN_LINE(const RuntimeStruct& l_test_struct_def = l_runtime_module.struct_info("test_struct_v2"))
        CODEGEN_LINE(const RuntimeStruct& l_outer_copy_args_def = l_runtime_module.struct_info("outer_copy_args_v2"))
        CODEGEN_LINE(const RuntimeEventFn& fn = l_runtime_module.event_fn_info("test_outer_field_copy"))
        CORE_ALWAYS_ASSERT(not fn.has_error());
        CORE_ALWAYS_ASSERT(not l_inner_struct_def.has_error());
        CORE_ALWAYS_ASSERT(not l_test_struct_def.has_error());
        CORE_ALWAYS_ASSERT(not l_outer_copy_args_def.has_error());

        auto new_inner_st_fn = [&]() -> RuntimeObject {
            CODEGEN_LINE(RuntimeObject obj = l_inner_struct_def.mk_object())
            CODEGEN_LINE(RuntimeArray f3 = RuntimeArray::from(runtime_type_t::int32, 7))
            CODEGEN_LINE(RuntimeArray f4 = RuntimeArray::from(runtime_type_t::int32, 7))
            CODEGEN_LINE(RuntimeArray f5 = RuntimeArray::from(runtime_type_t::int32, 7))
            CODEGEN_LINE(RuntimeArray f6 = RuntimeArray::from(runtime_type_t::int32, 7))
            CODEGEN_LINE(f3.try_freeze())
            CODEGEN_LINE(f4.try_freeze())
            CODEGEN_LINE(f5.try_freeze())
            CODEGEN_LINE(f6.try_freeze())
            CODEGEN_LINE(obj.set_array("field_3", f3))
            CODEGEN_LINE(obj.set_array("field_4", f4))
            CODEGEN_LINE(obj.set_array("field_5", f5))
            CODEGEN_LINE(obj.set_array("field_6", f6))
            return obj;
        };

        auto compare_st_fn = [&](const RuntimeObject& s1, const RuntimeObject& s2) {
            std::vector<std::string>  double_fields{{"field_1", "field_7", "field_9"}};
            std::vector<std::string>  int32_fields{{"field_2", "field_3a", "field_8"}};
            for (const std::string& v : double_fields) {
                CORE_ALWAYS_ASSERT_EQ(s1.get<double>(v), s2.get<double>(v));
            }
            for (const std::string& v : int32_fields) {
                CORE_ALWAYS_ASSERT_EQ(s1.get<int32_t>(v), s2.get<int32_t>(v));
            }

            RuntimeArray s1_f3 = s1.get_array("field_3");
            RuntimeArray s1_f5 = s1.get_array("field_5");
            RuntimeArray s2_f3 = s2.get_array("field_3");
            RuntimeArray s2_f5 = s2.get_array("field_5");
            for (int k = 0; k != 7; ++k) {
                CORE_ALWAYS_ASSERT_EQ(s1_f3.get<int32_t>(k), s2_f3.get<int32_t>(k))
                CORE_ALWAYS_ASSERT_EQ(s1_f5.get<int32_t>(k), s2_f5.get<int32_t>(k))
            }
        };


        for (int i = 0; i != 100; ++i) {
            CODEGEN_LINE(RuntimeObject l_struct_obj = l_test_struct_def.mk_object())
            CODEGEN_LINE(RuntimeArray int_arr_field_1_arr = RuntimeArray::from(runtime_type_t::int32, 7))
            CODEGEN_LINE(int_arr_field_1_arr.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_array("int_arr_field_1", int_arr_field_1_arr))
            CODEGEN_LINE(RuntimeArray int_arr_field_2_arr = RuntimeArray::from(runtime_type_t::int32, 7))
            CODEGEN_LINE(int_arr_field_2_arr.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_array("int_arr_field_2", int_arr_field_2_arr))

            CODEGEN_LINE(RuntimeObject struct_field_1 = new_inner_st_fn())
            CODEGEN_LINE(struct_field_1.set<int32_t>("field_2", i + 1))
            CODEGEN_LINE(struct_field_1.set<int32_t>("field_3a", i + 9))
            CODEGEN_LINE(struct_field_1.set<int32_t>("field_8", i + 49))
            {
                CODEGEN_LINE(RuntimeArray f3 = RuntimeArray::from(runtime_type_t::int32, 7))
                CODEGEN_LINE(RuntimeArray f4 = RuntimeArray::from(runtime_type_t::int32, 7))
                CODEGEN_LINE(RuntimeArray f5 = RuntimeArray::from(runtime_type_t::int32, 7))
                CODEGEN_LINE(RuntimeArray f6 = RuntimeArray::from(runtime_type_t::int32, 7))
                for (int j = 0; j < 7; ++j) {
                    CODEGEN_LINE(f3.set<int32_t>(j, i + 2 + j))
                    CODEGEN_LINE(f4.set<int32_t>(j, i + 12 + j))
                    CODEGEN_LINE(f5.set<int32_t>(j, i + 22 + j))
                    CODEGEN_LINE(f6.set<int32_t>(j, i + 32 + j))
                }
                CODEGEN_LINE(f3.try_freeze())
                CODEGEN_LINE(f4.try_freeze())
                CODEGEN_LINE(f5.try_freeze())
                CODEGEN_LINE(f6.try_freeze())
                CODEGEN_LINE(struct_field_1.set_array("field_3", f3))
                CODEGEN_LINE(struct_field_1.set_array("field_4", f4))
                CODEGEN_LINE(struct_field_1.set_array("field_5", f5))
                CODEGEN_LINE(struct_field_1.set_array("field_6", f6))
            }
            CODEGEN_LINE(struct_field_1.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_object("struct_field_1", struct_field_1))
            CODEGEN_LINE(RuntimeObject struct_field_2 = new_inner_st_fn())
            CODEGEN_LINE(struct_field_2.set<int32_t>("field_2", 100 * i + 1))
            CODEGEN_LINE(struct_field_2.set<int32_t>("field_3a", 100 * i + 900))
            CODEGEN_LINE(struct_field_2.set<int32_t>("field_8", 100 * i + 4900))
            {
                CODEGEN_LINE(RuntimeArray f3 = RuntimeArray::from(runtime_type_t::int32, 7))
                CODEGEN_LINE(RuntimeArray f4 = RuntimeArray::from(runtime_type_t::int32, 7))
                CODEGEN_LINE(RuntimeArray f5 = RuntimeArray::from(runtime_type_t::int32, 7))
                CODEGEN_LINE(RuntimeArray f6 = RuntimeArray::from(runtime_type_t::int32, 7))
                for (int j = 0; j < 7; ++j) {
                    CODEGEN_LINE(f3.set<int32_t>(j, i + 200 + j))
                    CODEGEN_LINE(f4.set<int32_t>(j, i + 1200 + j))
                    CODEGEN_LINE(f5.set<int32_t>(j, i + 2200 + j))
                    CODEGEN_LINE(f6.set<int32_t>(j, i + 3200 + j))
                }
                CODEGEN_LINE(f3.try_freeze())
                CODEGEN_LINE(f4.try_freeze())
                CODEGEN_LINE(f5.try_freeze())
                CODEGEN_LINE(f6.try_freeze())
                CODEGEN_LINE(struct_field_2.set_array("field_3", f3))
                CODEGEN_LINE(struct_field_2.set_array("field_4", f4))
                CODEGEN_LINE(struct_field_2.set_array("field_5", f5))
                CODEGEN_LINE(struct_field_2.set_array("field_6", f6))
            }
            CODEGEN_LINE(struct_field_2.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_object("struct_field_2", struct_field_2))
            CODEGEN_LINE(RuntimeObject struct_field_3 = new_inner_st_fn())
            CODEGEN_LINE(struct_field_3.try_freeze())
            CORE_ALWAYS_ASSERT(struct_field_3.is_frozen())
            CODEGEN_LINE(l_struct_obj.set_object("struct_field_3", struct_field_3))
            CODEGEN_LINE(RuntimeObject struct_field_4 = new_inner_st_fn())
            CODEGEN_LINE(struct_field_4.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_object("struct_field_4", struct_field_4))
            CODEGEN_LINE(RuntimeObject struct_field_5 = new_inner_st_fn())
            CODEGEN_LINE(struct_field_5.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_object("struct_field_5", struct_field_5))
            CODEGEN_LINE(RuntimeObject struct_field_6 = new_inner_st_fn())
            CODEGEN_LINE(struct_field_6.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_object("struct_field_6", struct_field_6))
            CODEGEN_LINE(RuntimeObject struct_field_7 = new_inner_st_fn())
            CODEGEN_LINE(struct_field_7.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_object("struct_field_7", struct_field_7))

            CODEGEN_LINE(RuntimeArray struct_arr_field_1 = RuntimeArray::from(runtime_type_t::pointer_struct, 7))
            CODEGEN_LINE(RuntimeArray struct_arr_field_2 = RuntimeArray::from(runtime_type_t::pointer_struct, 7))
            for (uint32_t j = 0; j != 7; ++j) {
                CODEGEN_LINE(RuntimeObject arr1_object = new_inner_st_fn())
                CODEGEN_LINE(RuntimeObject arr2_object = new_inner_st_fn())
                CODEGEN_LINE(arr1_object.try_freeze())
                CODEGEN_LINE(arr2_object.try_freeze())
                CODEGEN_LINE(struct_arr_field_1.set_object(j, arr1_object))
                CODEGEN_LINE(struct_arr_field_2.set_object(j, arr2_object))
            }
            CODEGEN_LINE(struct_arr_field_1.try_freeze())
            CODEGEN_LINE(struct_arr_field_2.try_freeze())
            CODEGEN_LINE(l_struct_obj.set_array("struct_arr_field_1", struct_arr_field_1))
            CODEGEN_LINE(l_struct_obj.set_array("struct_arr_field_2", struct_arr_field_2))
            CODEGEN_LINE(l_struct_obj.try_freeze())

            CODEGEN_LINE(RuntimeObject l_args_obj = l_outer_copy_args_def.mk_object())
            CODEGEN_LINE(l_args_obj.set_object("arg1", l_struct_obj))
            CODEGEN_LINE(l_args_obj.try_freeze())

            CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
            CODEGEN_LINE(fn.on_event(l_args_obj))

            compare_st_fn(struct_field_3, struct_field_1);
            compare_st_fn(struct_field_4, struct_field_2);
            compare_st_fn(struct_field_5, struct_field_1);
            compare_st_fn(struct_field_6, struct_field_2);
            compare_st_fn(struct_field_7, struct_field_1);

            compare_st_fn(struct_arr_field_1.get_object(0), struct_field_1);
            compare_st_fn(struct_arr_field_1.get_object(2), struct_field_1);
            compare_st_fn(struct_arr_field_1.get_object(4), struct_field_1);
            compare_st_fn(struct_arr_field_1.get_object(6), struct_field_1);
            compare_st_fn(struct_arr_field_1.get_object(1), struct_field_2);
            compare_st_fn(struct_arr_field_1.get_object(3), struct_field_2);
            compare_st_fn(struct_arr_field_1.get_object(5), struct_field_2);

            compare_st_fn(struct_arr_field_2.get_object(0), struct_field_2);
            compare_st_fn(struct_arr_field_2.get_object(2), struct_field_2);
            compare_st_fn(struct_arr_field_2.get_object(4), struct_field_2);
            compare_st_fn(struct_arr_field_2.get_object(6), struct_field_2);
            compare_st_fn(struct_arr_field_2.get_object(1), struct_field_1);
            compare_st_fn(struct_arr_field_2.get_object(3), struct_field_1);
            compare_st_fn(struct_arr_field_2.get_object(5), struct_field_1);
        }
    }



}
