//
// Created by vibhanshu on 2026-01-12
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

#include "common_llvm_test.h"

using namespace core;
using namespace core::llvm_proxy;

CORE_NS_BEGIN

template <typename T>
constexpr runtime_type_t get_runtime_type() {
    if constexpr (std::is_same_v<T, int32_t>) {
        return runtime_type_t::int32;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return runtime_type_t::int64;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return runtime_type_t::uint32;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return runtime_type_t::uint64;
    } else {
        static_assert(sizeof(T) == 0, "Unsupported type");
    }
}


RuntimeObject gen_ctx_object(runtime_type_t rt_type, const RuntimeStruct& ctx_struct) {
    CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
    constexpr uint32_t ARR_SIZE = 5;

    RuntimeObject ctx_obj = ctx_struct.mk_object();
    CORE_ALWAYS_ASSERT(not ctx_obj.has_error());

    RuntimeArray vec1_arr = RuntimeArray::from(rt_type, ARR_SIZE);
    RuntimeArray vec2_arr = RuntimeArray::from(rt_type, ARR_SIZE);
    RuntimeArray vec3_arr = RuntimeArray::from(rt_type, ARR_SIZE);
    CORE_ALWAYS_ASSERT(not vec1_arr.has_error());
    CORE_ALWAYS_ASSERT(not vec2_arr.has_error());
    CORE_ALWAYS_ASSERT(not vec3_arr.has_error());
    vec1_arr.try_freeze();
    vec2_arr.try_freeze();
    vec3_arr.try_freeze();
    ctx_obj.set_array("vec1", vec1_arr);
    ctx_obj.set_array("vec2", vec2_arr);
    ctx_obj.set_array("vec3", vec3_arr);

    RuntimeArray mat1_ptr_arr = RuntimeArray::from(runtime_type_t::pointer_array, ARR_SIZE);
    RuntimeArray mat2_ptr_arr = RuntimeArray::from(runtime_type_t::pointer_array, ARR_SIZE);
    RuntimeArray mat3_ptr_arr = RuntimeArray::from(runtime_type_t::pointer_array, ARR_SIZE);
    CORE_ALWAYS_ASSERT(not mat1_ptr_arr.has_error());
    CORE_ALWAYS_ASSERT(not mat2_ptr_arr.has_error());
    CORE_ALWAYS_ASSERT(not mat3_ptr_arr.has_error());

    for (uint32_t r = 0; r != ARR_SIZE; ++r) {
        RuntimeArray mat1_row = RuntimeArray::from(rt_type, ARR_SIZE);
        RuntimeArray mat2_row = RuntimeArray::from(rt_type, ARR_SIZE);
        RuntimeArray mat3_row = RuntimeArray::from(rt_type, ARR_SIZE);
        CORE_ALWAYS_ASSERT(not mat1_row.has_error());
        CORE_ALWAYS_ASSERT(not mat2_row.has_error());
        CORE_ALWAYS_ASSERT(not mat3_row.has_error());
        mat1_row.try_freeze();
        mat2_row.try_freeze();
        mat3_row.try_freeze();
        mat1_ptr_arr.set_array(r, mat1_row);
        mat2_ptr_arr.set_array(r, mat2_row);
        mat3_ptr_arr.set_array(r, mat3_row);
    }
    mat1_ptr_arr.try_freeze();
    mat2_ptr_arr.try_freeze();
    mat3_ptr_arr.try_freeze();

    ctx_obj.set_array("mat1", mat1_ptr_arr);
    ctx_obj.set_array("mat2", mat2_ptr_arr);
    ctx_obj.set_array("mat3", mat3_ptr_arr);
    return ctx_obj;
}


auto gen_ctx(const std::string& name, const TypeInfo &type) -> FnContext {
    CODEGEN_LINE(TypeInfo arr_type = TypeInfo::mk_array(type, 5))
    CODEGEN_LINE(TypeInfo arr_ptr_type = arr_type.pointer_type())
    CODEGEN_LINE(TypeInfo matrix_type = TypeInfo::mk_array(arr_ptr_type, 5))
    CODEGEN_LINE(TypeInfo matrix_ptr_type = matrix_type.pointer_type())
    std::vector<member_field_entry> l_field_list;
    l_field_list.emplace_back("arg1", type);
    l_field_list.emplace_back("arg2", type);
    l_field_list.emplace_back("arg3", type);
    l_field_list.emplace_back("res", type);
    l_field_list.emplace_back("vec1", arr_ptr_type);
    l_field_list.emplace_back("vec2", arr_ptr_type);
    l_field_list.emplace_back("vec3", arr_ptr_type);
    l_field_list.emplace_back("mat1", matrix_ptr_type);
    l_field_list.emplace_back("mat2", matrix_ptr_type);
    l_field_list.emplace_back("mat3", matrix_ptr_type);
    CODEGEN_LINE(TypeInfo l_struct = TypeInfo::mk_struct(name, l_field_list, false))
    CODEGEN_LINE(return FnContext{l_struct.pointer_type()})
};

template <typename T>
void gen_sample_fn(Function &fn) {
    CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
    CODEGEN_LINE(l_fn_body.enter())
    CODEGEN_LINE(ValueInfo ctx = fn.context().value())
    CODEGEN_LINE(ValueInfo arg1 = ctx.field("arg1").load())
    CODEGEN_LINE(ValueInfo arg2 = ctx.field("arg2").load())
    CODEGEN_LINE(ValueInfo arg3 = ctx.field("arg3").load())
    CODEGEN_LINE(ValueInfo output = ctx.field("res"))
    CODEGEN_LINE(ValueInfo c1 = ValueInfo::from_constant<T>(101))
    CODEGEN_LINE(ValueInfo c2 = ValueInfo::from_constant<T>(999))
    CODEGEN_LINE(ValueInfo res = arg1.add(arg2, "construct_add"))
    CODEGEN_LINE(ValueInfo res2 = res.add(c1, "add_const1"))
    CODEGEN_LINE(ValueInfo res3 = res2.add(c2, "add_const1"))
    CODEGEN_LINE(ValueInfo res4 = res2.add(arg3, "add_arg"))
    CODEGEN_LINE(ValueInfo res5 = res3.add(res4, "accum_res"))
    CODEGEN_LINE(output.store(res5));
    CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)));
    if (ErrorContext::has_error()) {
        ErrorContext::print(std::cout, 6);
    }
    CORE_ASSERT(not ErrorContext::has_error())
}

void vector_add(Function &fn) {
    CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
    CODEGEN_LINE(l_fn_body.enter())
    CODEGEN_LINE(ValueInfo ctx = fn.context().value())
    CODEGEN_LINE(ValueInfo vec1 = ctx.field("vec1").load())
    CODEGEN_LINE(ValueInfo vec2 = ctx.field("vec2").load())
    CODEGEN_LINE(ValueInfo vec3 = ctx.field("vec3").load())
    CODEGEN_LINE(ValueInfo output = ctx.field("res"))
    CORE_ASSERT(vec1.type().is_pointer());
    CORE_ASSERT(vec2.type().is_pointer());
    CORE_ASSERT(vec3.type().is_pointer());
    CORE_ASSERT(vec1.type().base_type().is_array());
    CORE_ASSERT(vec2.type().base_type().is_array());
    CORE_ASSERT(vec3.type().base_type().is_array());

    for (int i = 0; i != 5; ++i) {
        CodeSectionContext::section_break(CORE_CONCAT << "iter_" << i);
        CODEGEN_LINE(ValueInfo sum = vec1.entry(i).load() + vec2.entry(i).load())
        CODEGEN_LINE(vec3.entry(i).store(sum));
        CODEGEN_LINE(output.store(sum));
    }

    CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)));
    if (ErrorContext::has_error()) {
        ErrorContext::print(std::cout, 5);
    }
    CORE_ASSERT(not ErrorContext::has_error())
}

void matrix_add(Function &fn) {
    CODEGEN_LINE(CodeSection l_fn_body = fn.mk_section("test_fn_body"))
    CODEGEN_LINE(l_fn_body.enter())
    CODEGEN_LINE(ValueInfo ctx = fn.context().value())
    CODEGEN_LINE(ValueInfo mat1 = ctx.field("mat1").load())
    CODEGEN_LINE(ValueInfo mat2 = ctx.field("mat2").load())
    CODEGEN_LINE(ValueInfo mat3 = ctx.field("mat3").load())
    CORE_ASSERT(mat1.type().is_pointer());
    CORE_ASSERT(mat2.type().is_pointer());
    CORE_ASSERT(mat3.type().is_pointer());
    CORE_ASSERT(mat1.type().base_type().is_array());
    CORE_ASSERT(mat2.type().base_type().is_array());
    CORE_ASSERT(mat3.type().base_type().is_array());
    CORE_ASSERT(mat1.type().base_type().base_type().is_pointer());
    CORE_ASSERT(mat2.type().base_type().base_type().is_pointer());
    CORE_ASSERT(mat3.type().base_type().base_type().is_pointer());
    CORE_ASSERT(mat1.type().base_type().base_type().base_type().is_array());
    CORE_ASSERT(mat2.type().base_type().base_type().base_type().is_array());
    CORE_ASSERT(mat3.type().base_type().base_type().base_type().is_array());

    for (int i = 0; i != 5; ++i) {
        for (int j = 0; j != 5; ++j) {
            CODEGEN_LINE(ValueInfo sum = mat1.entry(i).load().entry(j).load() + mat2.entry(i).load().entry(j).load())
            CODEGEN_LINE(mat3.entry(i).load().entry(j).store(sum));
        }
    }

    CODEGEN_LINE(CodeSectionContext::set_return_value(ValueInfo::from_constant(0)));
    if (ErrorContext::has_error()) {
        ErrorContext::print(std::cout, 5);
    }
    CORE_ASSERT(not ErrorContext::has_error())
}

template <typename T>
void gen_for_type(const TypeInfo& int_type, const FnContext int_context) {
    CORE_ALWAYS_ASSERT(int_type.is_integer());
    CORE_ALWAYS_ASSERT(Module::Context::has_value());
    const std::string type_name = int_type.short_name();
    CODEGEN_LINE(Module l_module = Module::Context::value());
    {
        const std::string fn_name{CORE_CONCAT << "sample_fn_name_" << type_name};
        CODEGEN_LINE(FunctionBuilder fn_builder)
        CODEGEN_LINE(fn_builder.set_context(int_context)
            .set_module(l_module)
            .set_name(fn_name))
        CODEGEN_LINE(Function fn = fn_builder.compile())
        gen_sample_fn<T>(fn);
        Function f2 = l_module.get_function(fn_name);
        f2.verify();
    }
    {
        const std::string fn_name{CORE_CONCAT << "vector_addition_" << type_name};
        CODEGEN_LINE(FunctionBuilder fn_builder)
        CODEGEN_LINE(fn_builder.set_context(int_context)
            .set_module(l_module)
            .set_name(fn_name))
        CODEGEN_LINE(Function fn = fn_builder.compile())
        vector_add(fn);
        Function f2 = l_module.get_function(fn_name);
        f2.verify();
    }
    {
        const std::string fn_name{CORE_CONCAT << "matrix_addition_" << type_name};
        CODEGEN_LINE(FunctionBuilder fn_builder)
        CODEGEN_LINE(fn_builder.set_context(int_context)
            .set_module(l_module)
            .set_name(fn_name))
        CODEGEN_LINE(Function fn = fn_builder.compile())
        matrix_add(fn);
        Function f2 = l_module.get_function(fn_name);
        f2.verify();
    }
}

template <typename T>
void test_for_type(const std::string& suffix, const std::string& ctx_name, JustInTimeRunner& jit_runner) {
    CORE_ALWAYS_ASSERT(not ErrorContext::has_error());
    const RuntimeNamespace& ns = jit_runner.get_global_namespace();
    const RuntimeStruct& ctx_struct = ns.struct_info(ctx_name);
    CORE_ALWAYS_ASSERT(not ctx_struct.has_error());

    {
        const RuntimeEventFn& fn = ns.event_fn_info(CORE_CONCAT << "sample_fn_name_" << suffix);
        CORE_ALWAYS_ASSERT(not fn.has_error());

        for (int32_t i = 0; i != 10; ++i) {
            RuntimeObject ctx_obj = gen_ctx_object(get_runtime_type<T>(), ctx_struct);
            CORE_ALWAYS_ASSERT(not ctx_obj.has_error());

            T arg1 = static_cast<T>(i);
            T arg2 = static_cast<T>(i + 1);
            T arg3 = static_cast<T>(i + 2);
            T c1 = 101;
            T c2 = 999;
            T res = arg1 + arg2;
            T res2 = res + c1;
            T res3 = res2 + c2;
            T res4 = res2 + arg3;
            T res5 = res3 + res4;

            CODEGEN_LINE(ctx_obj.set<T>("arg1", arg1))
            CODEGEN_LINE(ctx_obj.set<T>("arg2", arg2))
            CODEGEN_LINE(ctx_obj.set<T>("arg3", arg3))
            CODEGEN_LINE(ctx_obj.try_freeze())
            CODEGEN_LINE(fn.on_event(ctx_obj))
            T result = ctx_obj.get<T>("res");
            CORE_ALWAYS_ASSERT_EQ(res5, result);
        }
    }

    constexpr uint32_t ARR_SIZE = 5;
    {
        const RuntimeEventFn& fn = ns.event_fn_info(CORE_CONCAT << "vector_addition_" << suffix);
        CORE_ALWAYS_ASSERT(not fn.has_error());

        for (int32_t i = 0; i != 5; ++i) {
            RuntimeObject ctx_obj = gen_ctx_object(get_runtime_type<T>(), ctx_struct);
            CORE_ALWAYS_ASSERT(not ctx_obj.has_error());

            RuntimeArray vec1_arr = RuntimeArray::from(get_runtime_type<T>(), ARR_SIZE);
            RuntimeArray vec2_arr = RuntimeArray::from(get_runtime_type<T>(), ARR_SIZE);
            RuntimeArray vec3_arr = RuntimeArray::from(get_runtime_type<T>(), ARR_SIZE);
            CORE_ALWAYS_ASSERT(not vec1_arr.has_error());
            CORE_ALWAYS_ASSERT(not vec2_arr.has_error());
            CORE_ALWAYS_ASSERT(not vec3_arr.has_error());

            for (uint32_t j = 0; j != 5; ++j) {
                vec1_arr.set<T>(j, static_cast<T>(i * 100 + j));
                vec2_arr.set<T>(j, static_cast<T>(i * 100 + 10 * j));
                vec3_arr.set<T>(j, static_cast<T>(0));
            }
            vec1_arr.try_freeze();
            vec2_arr.try_freeze();
            vec3_arr.try_freeze();

            ctx_obj.set_array("vec1", vec1_arr);
            ctx_obj.set_array("vec2", vec2_arr);
            ctx_obj.set_array("vec3", vec3_arr);
            ctx_obj.try_freeze();

            fn.on_event(ctx_obj);

            for (uint32_t j = 0; j != 5; ++j) {
                T expected = vec1_arr.get<T>(j) + vec2_arr.get<T>(j);
                CORE_ALWAYS_ASSERT_EQ(vec3_arr.get<T>(static_cast<uint32_t>(j)), expected);
            }
        }
    }

    {
        runtime_type_t elem_type = get_runtime_type<T>();
        const RuntimeEventFn& fn = ns.event_fn_info(CORE_CONCAT << "matrix_addition_" << suffix);
        CORE_ALWAYS_ASSERT(not fn.has_error());

        for (int32_t i = 0; i != 5; ++i) {
            RuntimeObject ctx_obj = gen_ctx_object(elem_type, ctx_struct);
            CORE_ALWAYS_ASSERT(not ctx_obj.has_error());

            RuntimeArray mat1_ptr_arr = RuntimeArray::from(runtime_type_t::pointer_array, ARR_SIZE);
            RuntimeArray mat2_ptr_arr = RuntimeArray::from(runtime_type_t::pointer_array, ARR_SIZE);
            RuntimeArray mat3_ptr_arr = RuntimeArray::from(runtime_type_t::pointer_array, ARR_SIZE);
            CORE_ALWAYS_ASSERT(not mat1_ptr_arr.has_error());
            CORE_ALWAYS_ASSERT(not mat2_ptr_arr.has_error());
            CORE_ALWAYS_ASSERT(not mat3_ptr_arr.has_error());

            for (uint32_t j = 0; j != ARR_SIZE; ++j) {
                RuntimeArray mat1_row = RuntimeArray::from(elem_type, ARR_SIZE);
                RuntimeArray mat2_row = RuntimeArray::from(elem_type, ARR_SIZE);
                RuntimeArray mat3_row = RuntimeArray::from(elem_type, ARR_SIZE);
                for (uint32_t k = 0; k != ARR_SIZE; ++k) {
                    mat1_row.set<T>(k, static_cast<T>(k * 100 + j));
                    mat2_row.set<T>(k, static_cast<T>(k * 100 + 10 * j));
                    mat3_row.set<T>(k, static_cast<T>(0));
                }
                mat1_row.try_freeze();
                mat2_row.try_freeze();
                mat3_row.try_freeze();
                mat1_ptr_arr.set_array(j, mat1_row);
                mat2_ptr_arr.set_array(j, mat2_row);
                mat3_ptr_arr.set_array(j, mat3_row);
            }

            mat1_ptr_arr.try_freeze();
            mat2_ptr_arr.try_freeze();
            mat3_ptr_arr.try_freeze();

            ctx_obj.set_array("mat1", mat1_ptr_arr);
            ctx_obj.set_array("mat2", mat2_ptr_arr);
            ctx_obj.set_array("mat3", mat3_ptr_arr);
            ctx_obj.try_freeze();

            fn.on_event(ctx_obj);

            for (int j = 0; j != 5; ++j) {
                RuntimeArray mat1_row = mat1_ptr_arr.get_array(static_cast<uint32_t>(j));
                RuntimeArray mat2_row = mat2_ptr_arr.get_array(static_cast<uint32_t>(j));
                RuntimeArray mat3_row = mat3_ptr_arr.get_array(static_cast<uint32_t>(j));
                for (uint32_t k = 0; k != 5; ++k) {
                    T expected = mat1_row.get<T>(k) + mat2_row.get<T>(k);
                    CORE_ALWAYS_ASSERT_EQ(mat3_row.get<T>(k), expected);
                }
            }
        }
    }
}

CORE_NS_END


TEST(LLVM_CODEGEN_LINALG, addition) {
    CODEGEN_LINE(Cursor l_cursor{"pointer_op"})
    CODEGEN_LINE(Module l_module)
    CODEGEN_LINE(Cursor::Context::set_value(l_cursor));

    CODEGEN_LINE(TypeInfo int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(TypeInfo int64_type = TypeInfo::mk_int64())
    CODEGEN_LINE(TypeInfo uint32_type = TypeInfo::mk_uint32())
    CODEGEN_LINE(TypeInfo uint64_type = TypeInfo::mk_uint64())

    CODEGEN_LINE(FnContext l_int32_context = gen_ctx("int32_ctx", int32_type))
    CODEGEN_LINE(FnContext l_int64_context = gen_ctx("int64_ctx", int64_type))
    CODEGEN_LINE(FnContext l_uint32_context = gen_ctx("uint32_ctx", uint32_type))
    CODEGEN_LINE(FnContext l_uint64_context = gen_ctx("uint64_ctx", uint64_type))

    CODEGEN_LINE(l_cursor.bind())
    CODEGEN_LINE(l_module = l_cursor.main_module())
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    if (ErrorContext::has_error()) {
        ErrorContext::print(std::cout, 5);
    }
    CORE_ASSERT(not ErrorContext::has_error());
    gen_for_type<int32_t>(int32_type, l_int32_context);
    gen_for_type<int64_t>(int64_type, l_int64_context);
    gen_for_type<uint32_t>(uint32_type, l_uint32_context);
    gen_for_type<uint64_t>(uint64_type, l_uint64_context);
    INIT_MODULE(l_module);
    CORE_ALWAYS_ASSERT(l_module.is_init());
    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(jit_runner.add_module(l_cursor))
    if (ErrorContext::has_error()) {
        ErrorContext::print(std::cout, 5);
    }
    CORE_ALWAYS_ASSERT(not l_module.is_init());
    CODEGEN_LINE(jit_runner.bind())
    if (jit_runner.has_error()) {
        ErrorContext::print(std::cout, 5);
    }
    CORE_ALWAYS_ASSERT(not jit_runner.has_error());

    test_for_type<int32_t>("int32", "int32_ctx", jit_runner);
    test_for_type<int64_t>("int64", "int64_ctx", jit_runner);
    test_for_type<uint32_t>("uint32", "uint32_ctx", jit_runner);
    test_for_type<uint64_t>("uint64", "uint64_ctx", jit_runner);

    CODEGEN_LINE(Cursor::Context::clear_value())
}
