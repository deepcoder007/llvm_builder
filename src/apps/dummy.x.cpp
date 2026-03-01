#include "llvm_builder/value.h"
#include "util/debug.h"
#include "llvm_builder/defines.h"
#include <iostream>
#include <string>

#include "util/string_util.h"
#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"

#include <format>
#include <iomanip>

LLVM_BUILDER_NS_BEGIN

int32_t test_callback_fn(void* arg) {
    return 42;
}

LLVM_BUILDER_NS_END

int32_t main(int32_t argc, char **argv) {
    using namespace llvm_builder;
    (void)argc;
    (void)argv;

    std::cout << " ======= APP BEGIN ======= " << std::endl;
    CODEGEN_LINE(Cursor l_cursor{"hierarchical_def"})
    {
        CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
        CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())

        CODEGEN_LINE(TypeInfo args3_type)
        {
            std::vector<member_field_entry> args3_fields;
            args3_fields.emplace_back("arg1", int32_type);
            args3_fields.emplace_back("arg2", int32_type);
            args3_fields.emplace_back("arg1_fn_bkp", int32_type);
            args3_fields.emplace_back("arg2_fn_bkp", int32_type);
            args3_fields.emplace_back("arr_idx", int32_type);
            args3_fields.emplace_back("arg_arr1", TypeInfo::mk_array(int32_type, 5).mk_ptr());
            args3_fields.emplace_back("arg_arr2", TypeInfo::mk_array(int32_type, 5).mk_ptr());
            CODEGEN_LINE(args3_type = TypeInfo::mk_struct("args3", args3_fields))

        }
        LLVM_BUILDER_ASSERT(not args3_type.has_error());

        CODEGEN_LINE(const TypeInfo int32_base = int32_type.base_type())
        std::cout << " base int32 : " << int32_base.short_name() << std::endl;
        std::cout << " base int32 log : [" << int32_base.error_log() << "]" << std::endl;

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
        CODEGEN_LINE(fibo_fn.call_fn())
        CODEGEN_LINE(arr_copy_fn.call_fn())

        CODEGEN_LINE(FunctionContext::set_return_value(arg1_ptr.load()))
        l_module.write_to_file();
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
                    (void)result;
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg2_prev, l_args_obj.get<int32_t>("arg1"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(new_value, l_args_obj.get<int32_t>("arg2"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg1_prev, l_args_obj.get<int32_t>("arg1_fn_bkp"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg2_prev, l_args_obj.get<int32_t>("arg2_fn_bkp"))
                LLVM_BUILDER_ALWAYS_ASSERT_EQ(l_arg_arr1.get<int32_t>(i % 5), l_arg_arr2.get<int32_t>(i % 5))
            }
        }
    }

    std::cout << " ======= APP END ======= " << std::endl;
    return 0;
}
