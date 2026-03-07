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
#include "llvm_builder/event.h"

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
    Cursor l_cursor{"hierarchical_def"};
    {
        Cursor::Context l_cursor_ctx{l_cursor};
        TypeInfo int32_type = TypeInfo::mk_int32();

        Event ev1 = Event::from_name("ev1");
        Event ev2 = Event::from_name("ev2");
        Event ev3 = Event::from_name("ev3");

        EventSet ev_set1;
        EventSet ev_set2;
        EventSet ev_set3;
        {
            ev_set1.add(ev1);
            ev_set1.add(ev2);
            ev_set2.add(ev2);
            ev_set2.add(ev3);
            ev_set3.add(ev1);
            ev_set3.add(ev3);
        }

        l_cursor.add_field("arg1", int32_type, ev_set1);
        l_cursor.add_field("arg2", int32_type, ev_set2);
        l_cursor.add_field("arg1_fn_bkp", int32_type, ev_set3);
        l_cursor.add_field("arg2_fn_bkp", int32_type, ev_set1);
        l_cursor.add_field("arr_idx", int32_type, ev_set2);
        l_cursor.add_field("arg_arr1", int32_type.mk_arr(5).mk_ptr(), ev_set3);
        l_cursor.add_field("arg_arr2", int32_type.mk_arr(5).mk_ptr(), ev_set1);
        {
            std::vector<field_entry_t> l_field_list;
            l_field_list.emplace_back("field_1", int32_type);
            l_field_list.emplace_back("field_2", int32_type);
            l_field_list.emplace_back("field_3", int32_type);
            l_field_list.emplace_back("field_4", int32_type);
            TypeInfo l_inner_struct_type = TypeInfo::mk_struct("inner_struct", l_field_list, false).mk_ptr();
            l_cursor.add_field("inner_struct", l_inner_struct_type, ev_set2);
        }
        l_cursor.bind("context_st");
        std::cout << " >> context_st :" << l_cursor.error_log() << std::endl;
        LLVM_BUILDER_ALWAYS_ASSERT(not l_cursor.has_error());

        //
        // computation graph BEGIN
        //
        ValueInfo ctx = ValueInfo::from_context();
        ValueInfo arr_idx = ctx.field("arr_idx").load();

        ValueInfo arg1_ptr = ctx.field("arg1");
        ValueInfo arg2_ptr = ctx.field("arg2");
        ValueInfo arg1_fn_bkp_ptr = ctx.field("arg1_fn_bkp");
        ValueInfo arg2_fn_bkp_ptr = ctx.field("arg2_fn_bkp");
        // ValueInfo arr_idx = ctx.field("arr_idx").load();
        ValueInfo arg_arr1_ptr = ctx.field("arg_arr1").load();
        ValueInfo arg_arr2_ptr = ctx.field("arg_arr2").load();
        ValueInfo inner_struct_ptr = ctx.field("inner_struct").load();
        ValueInfo arg1 = arg1_ptr.load();
        ValueInfo arg2 = arg2_ptr.load();
        ValueInfo inner_field_1 = inner_struct_ptr.field("field_1").load();
        ValueInfo inner_field_2 = inner_struct_ptr.field("field_2").load();

        LLVM_BUILDER_ASSERT(not ctx.has_error());
        LLVM_BUILDER_ASSERT(not arr_idx.has_error());
        LLVM_BUILDER_ASSERT(not arg1_ptr.has_error());
        LLVM_BUILDER_ASSERT(not arg2_ptr.has_error());

        //
        //  computation graph END
        //

        Module l_module = l_cursor.main_module();
        Module::Context l_module_ctx{l_module};
        Function on_event_fn("on_big_event");
        Function fibo_fn("fibonacci_calc");
        Function arr_copy_fn("arr_copy_fn");

        FunctionContext::set_fn(on_event_fn);

        auto log_event = [] (const std::string& name, const ValueInfo& value) {
            std::cout << " events in:" << name << ":[";
            for (const Event& l_ev : value.events().values()) {
                std::cout << ", " << l_ev.name();
            }
            std::cout << "]" << std::endl;
        };

        {
            FunctionContext l_local_fibo_ctx{fibo_fn};
            log_event("arg1_ptr", arg1_ptr);
            log_event("arg2_ptr", arg2_ptr);
            log_event("arg2", arg2);
            arg1_ptr.store(arg2);
            log_event("arg1+arg2", arg1.add(arg2));
            arg2_ptr.store(arg1.add(arg2));
            arg1_fn_bkp_ptr.store(arg1);
            arg2_fn_bkp_ptr.store(arg2);
            FunctionContext::set_return_value(ValueInfo::from_constant(0));
        }
        {
            FunctionContext l_local_fn_ctx{arr_copy_fn};
            arg_arr2_ptr.entry(arr_idx).store(arg_arr1_ptr.entry(arr_idx).load());
            log_event("arg2[arr_idx]=arg1[arr_idx]", arg_arr1_ptr.entry(arr_idx).load());
            FunctionContext::set_return_value(ValueInfo::from_constant(0));
        }
        fibo_fn.call_fn();
        arr_copy_fn.call_fn();

        FunctionContext::set_return_value(inner_field_1.add(inner_field_2));
        // FunctionContext::set_return_value(arg1_ptr.load());
        l_module.write_to_file();
        {
            const std::string fn_name{"on_big_event"};
            Function f2 = l_module.get_function(fn_name);
            f2.verify();
        }
    }

    LLVM_BUILDER_ALWAYS_ASSERT(not ErrorContext::has_error())
    {
        JustInTimeRunner jit_runner;
        jit_runner.add_module(l_cursor);
        jit_runner.bind();
        LLVM_BUILDER_ALWAYS_ASSERT(not jit_runner.has_error())

        const runtime::Namespace& l_runtime_module = jit_runner.get_global_namespace();
        LLVM_BUILDER_ALWAYS_ASSERT(not l_runtime_module.has_error());
        const runtime::Struct& context_st = l_runtime_module.struct_info("context_st");
        LLVM_BUILDER_ALWAYS_ASSERT(not context_st.has_error())
        const runtime::Struct& inner_struct = l_runtime_module.struct_info("inner_struct");
        LLVM_BUILDER_ALWAYS_ASSERT(not inner_struct.has_error())
        const runtime::EventFn& sample_fn = l_runtime_module.event_fn_info("on_big_event");
        LLVM_BUILDER_ALWAYS_ASSERT(not sample_fn.has_error());

        {
            runtime::Object l_args_obj = context_st.mk_object();
            runtime::Object inner_struct_obj = inner_struct.mk_object();
            runtime::Array l_arg_arr1 = runtime::Array::from(runtime::type_t::int32, 5);
            runtime::Array l_arg_arr2 = runtime::Array::from(runtime::type_t::int32, 5);

            inner_struct_obj.freeze();
            l_arg_arr1.freeze();
            l_arg_arr2.freeze();
            l_args_obj.set_array("arg_arr1", l_arg_arr1);
            l_args_obj.set_array("arg_arr2", l_arg_arr2);
            l_args_obj.set_object("inner_struct", inner_struct_obj);
            bool is_frozen = l_args_obj.freeze();
            if (not is_frozen) {
                for (const auto& l_field : l_args_obj.null_fields()) {
                    std::cout << " > field:" << l_field.name() << std::endl;
                }
            }
            std::cout << " > " << l_args_obj.error_log() << std::endl;
            LLVM_BUILDER_ASSERT(not l_args_obj.has_error());
            LLVM_BUILDER_ASSERT(is_frozen);
            l_args_obj.set<int32_t>("arg1", 1);
            l_args_obj.set<int32_t>("arg2", 1);
            for (int32_t i = 0; i != 20; ++i) {
                inner_struct_obj.set<int32_t>("field_1", i+1);
                inner_struct_obj.set<int32_t>("field_2", i+2);
                inner_struct_obj.set<int32_t>("field_3", i+3);
                inner_struct_obj.set<int32_t>("field_4", i+4);

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
                int32_t result = sample_fn.on_event(l_args_obj);
                    (void)result;
                std::cout << " i:" << i << " -> " << (i+1) << ":" << (i+2) << " = " << result << std::endl;
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
