#include "llvm_builder/value.h"
#include "llvm_builder/analyze.h"
#include "util/debug.h"
#include "llvm_builder/defines.h"
#include <iostream>
#include <string>

#include "util/string_util.h"
#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"
#include "llvm_builder/control_flow.h"
#include "llvm/context_impl.h"


LLVM_BUILDER_NS_BEGIN

struct test_struct {
    int32_t v1;
    int32_t v2;
    int32_t v3;
    int32_t v4;
public:
    void print(std::ostream& os) const {
      os << "{test_struct:v1=" << v1
         << ", v2=" << v2
         << ", v3=" << v3
         << ", v4=" << v4
         <<"}";
    }
    OSTREAM_FRIEND(test_struct)
};

class StdOutCallback : public object::Callback {
    using object_t = typename object::Callback::object_t;
private:
    void M_on_new(object_t type, uint64_t id, const std::string& name) override  {
        if (type == object_t::CURSOR) {
            std::cout << "         NEW cursor:" << id << " : " << name << std::endl;
        } else if (type == object_t::MODULE) {
            std::cout << "         NEW module:" << id << " : " << name << std::endl;
        } else if (type == object_t::PACKAGED_MODULE) {
            std::cout << "         NEW packaged module:" << id << " : " << name << std::endl;
        } else if (type == object_t::LINK_SYMBOL) {
            // std::cout << " NEW link_symbol:" << id << " : " << name << std::endl;
        } else if (type == object_t::FUNCTION) {
            std::cout << "         NEW function:" << id << " : " << name << std::endl;
        } else if (type == object_t::CODE_SECTION) {
            std::cout << "         NEW code section:" << id << " : " << name << std::endl;
        } else if (type == object_t::TYPE) {
            std::cout << "         NEW type:" << id << " : " << name << std::endl;
        } else if (type == object_t::VALUE) {
            // std::cout << " NEW value:" << id << " : " << name << std::endl;
        } else {
            std::cout << "         NEW unidentified:" << id << " : " << name << std::endl;
        }
    }
    void M_on_delete(object_t type, uint64_t id, const std::string& name) override {
        if (type == object_t::CURSOR) {
            std::cout << "         DELETE cursor:" << id << " : " << name << std::endl;
        } else if (type == object_t::MODULE) {
            std::cout << "         DELETE module:" << id << " : " << name << std::endl;
        } else if (type == object_t::PACKAGED_MODULE) {
            std::cout << "         DELETE packaged module:" << id << " : " << name << std::endl;
        } else if (type == object_t::LINK_SYMBOL) {
            // std::cout << " DELETE link_symbol:" << id << " : " << name << std::endl;
        } else if (type == object_t::FUNCTION) {
            std::cout << "         DELETE function:" << id << " : " << name << std::endl;
        } else if (type == object_t::CODE_SECTION) {
            std::cout << "         DELETE code section:" << id << " : " << name << std::endl;
        } else if (type == object_t::TYPE) {
            std::cout << "         DELETE type:" << id << " : " << name << std::endl;
        } else if (type == object_t::VALUE) {
            // std::cout << " DELETE value:" << id << " : " << name << std::endl;
        } else {
            std::cout << "         DELETE unidentified:" << id << " : " << name << std::endl;
        }
    }
};

LLVM_BUILDER_NS_END

using namespace llvm_builder;
using namespace llvm_builder;

int32_t main(int32_t argc, char **argv) {
    using namespace llvm_builder;
    using namespace llvm_builder;
    (void)argc;
    (void)argv;

    std::cout << " ======= APP BEGIN ======= " << std::endl;
    // object::Counter::singleton().add_callback(std::make_unique<StdOutCallback>());
    CODEGEN_LINE(PackagedModule pkg_module)
    CODEGEN_LINE(FunctionBuilder fn_builder)
    CODEGEN_LINE(Function fn)
    CODEGEN_LINE(Function fn_struct)
    CODEGEN_LINE(Function sample_fn)
    CODEGEN_LINE(Function fn_if_else)
    CODEGEN_LINE(Function fn_loop)
    CODEGEN_LINE(Function fn_load)
    CODEGEN_LINE(Function fn_get_global)
    CODEGEN_LINE(CodeSection fn_section)
    CODEGEN_LINE(CodeSection l_fn_body)
    CODEGEN_LINE(Module l_module)
    CODEGEN_LINE(TypeInfo int32_type)
    CODEGEN_LINE(TypeInfo bool_type)
    CODEGEN_LINE(TypeInfo int32_arr_type)
    CODEGEN_LINE(TypeInfo void_type)
    CODEGEN_LINE(ValueInfo gVarX)
    CODEGEN_LINE(ValueInfo gVarY)
    CODEGEN_LINE(ValueInfo arg1)
    CODEGEN_LINE(ValueInfo arg2)
    CODEGEN_LINE(ValueInfo arg_a)
    CODEGEN_LINE(ValueInfo arg_b)
    CODEGEN_LINE(ValueInfo c_5)
    CODEGEN_LINE(ValueInfo compare_1)
    LLVM_BUILDER_ASSERT(pkg_module.has_error())
    LLVM_BUILDER_ASSERT(fn.has_error())
    LLVM_BUILDER_ASSERT(l_module.has_error())
    LLVM_BUILDER_ASSERT(int32_type.has_error())
    LLVM_BUILDER_ASSERT(void_type.has_error())
    CODEGEN_LINE(Cursor l_cursor{"if_else_test"})
    CODEGEN_LINE(Cursor::Context l_cursor_ctx{l_cursor})
    CODEGEN_LINE(JustInTimeRunner jit_runner)
    CODEGEN_LINE(int32_type = TypeInfo::mk_int32())
    CODEGEN_LINE(bool_type = TypeInfo::mk_bool())
    CODEGEN_LINE(int32_arr_type = TypeInfo::mk_array(int32_type, 5))
    CODEGEN_LINE(void_type = TypeInfo::mk_void())
    std::vector<member_field_entry> l_field_list;
    l_field_list.emplace_back("field_1", int32_type);
    // l_field_list.emplace_back("field_1", int32_type);
    l_field_list.emplace_back("field_2", int32_type);
    l_field_list.emplace_back("field_3", int32_type);
    l_field_list.emplace_back("field_4", int32_type);
    l_field_list.emplace_back("field_5", int32_type);
    CODEGEN_LINE(TypeInfo l_struct_type = TypeInfo::mk_struct("test_struct", l_field_list))

    // Create struct types for function arguments
    std::vector<member_field_entry> fn1_args_fields;
    fn1_args_fields.emplace_back("arg1", int32_type);
    fn1_args_fields.emplace_back("arg2", int32_type);
    fn1_args_fields.emplace_back("arg3", int32_type);
    CODEGEN_LINE(TypeInfo fn1_args_type = TypeInfo::mk_struct("fn1_args", fn1_args_fields))

    std::vector<member_field_entry> if_else_args_fields;
    if_else_args_fields.emplace_back("a", int32_type);
    if_else_args_fields.emplace_back("b", int32_type);
    if_else_args_fields.emplace_back("c", int32_type);
    CODEGEN_LINE(TypeInfo if_else_args_type = TypeInfo::mk_struct("if_else_args", if_else_args_fields))

    std::vector<member_field_entry> fn_load_args_fields;
    fn_load_args_fields.emplace_back("a", int32_type);
    fn_load_args_fields.emplace_back("b", int32_arr_type.pointer_type());
    CODEGEN_LINE(TypeInfo fn_load_args_type = TypeInfo::mk_struct("fn_load_args", fn_load_args_fields))

    CODEGEN_LINE(Cursor::Context::value().bind())
    CODEGEN_LINE(l_module = Cursor::Context::value().main_module())
    std::cout << " MAIN module name: [" << l_module.name() << "]" << std::endl;
    CODEGEN_LINE(Module::Context l_module_ctx{l_module})
    CODEGEN_LINE(fn_builder.set_context(FnContext{fn1_args_type.pointer_type()})
        .set_module(l_module)
        .set_name("fn1"))
    CODEGEN_LINE(fn = fn_builder.compile())
    // {
    //     CODEGEN_LINE(FunctionBuilder fn_builder)
    //     CODEGEN_LINE(fn_builder.add_arg(FnContext{l_struct_type.pointer_type(), "arg1"})
    //         .add_arg(FnContext{l_struct_type.pointer_type(), "arg2"})
    //         .set_return_type(void_type)
    //         .set_module(l_module)
    //         .set_name("fn_struct"))
    //     CODEGEN_LINE(fn_struct = fn_builder.compile())
    // }
    CODEGEN_LINE(fn_builder = FunctionBuilder{});
    CODEGEN_LINE(fn_builder.set_context(FnContext{fn1_args_type.pointer_type()})
        .set_module(l_module)
        .set_name("sample_fn_name"))
    CODEGEN_LINE(sample_fn = fn_builder.compile())
    CODEGEN_LINE(FunctionBuilder fn_builder_if_else)
    CODEGEN_LINE(fn_builder_if_else.set_context(FnContext{if_else_args_type.pointer_type()})
        .set_module(l_module)
        .set_name("foo_if_else"))
    CODEGEN_LINE(fn_if_else = fn_builder_if_else.compile())
    CODEGEN_LINE(FunctionBuilder fn_builder_loop)
    CODEGEN_LINE(fn_builder_loop.set_context(FnContext{if_else_args_type.pointer_type()})
        .set_module(l_module)
        .set_name("foo_loop"))
    CODEGEN_LINE(fn_loop = fn_builder_loop.compile())
    CODEGEN_LINE(FunctionBuilder fn_builder_load)
    CODEGEN_LINE(fn_builder_load.set_context(FnContext{fn_load_args_type.pointer_type()})
        .set_module(l_module)
        .set_name("foo_loop_v2"))
    CODEGEN_LINE(fn_load = fn_builder_load.compile())
    ErrorContext::print(std::cout, 5);
    LLVM_BUILDER_ASSERT(fn_section.has_error())
    LLVM_BUILDER_ASSERT(l_fn_body.has_error())
#if 1
    CODEGEN_LINE(fn_section = fn.mk_section("test_fn_body"))
    CODEGEN_LINE(fn_section.enter())
    CODEGEN_LINE(ValueInfo fn_ctx = fn.context().value())
    CODEGEN_LINE(arg1 = fn_ctx.field("arg1").load())
    CODEGEN_LINE(arg2 = fn_ctx.field("arg2").load())
    CODEGEN_LINE(ValueInfo arg3 = fn_ctx.field("arg3").load())
    CODEGEN_LINE(arg1.push("arg_in"_cs))
    CODEGEN_LINE(CodeSectionContext::section_break("break1"))
    CODEGEN_LINE(CodeSectionContext::push_var_context())
    CODEGEN_LINE(arg2.push("arg_in"_cs))
    // CodeSectionContext::mk_ptr("arg1_in"_cs, arg1.type(), arg1);
    CODEGEN_LINE(CodeSectionContext::section_break("break2"))
    CODEGEN_LINE(CodeSectionContext::push_var_context())
    CODEGEN_LINE(arg3.push("arg_in"_cs))
    CODEGEN_LINE(CodeSectionContext::section_break("break3"))
    CODEGEN_LINE(CodeSectionContext::push_var_context())

    LLVM_BUILDER_ASSERT(not CodeSectionContext::current_section().is_sealed());
    LLVM_BUILDER_ASSERT(not CodeSectionContext::current_section().is_commit());
    CODEGEN_LINE(CodeSectionContext::set_return_value(CodeSectionContext::pop("arg_in"_cs)))
    if (CodeSectionContext::current_section().is_valid()) {
        LLVM_BUILDER_ASSERT(CodeSectionContext::current_section().is_sealed());
        LLVM_BUILDER_ASSERT(CodeSectionContext::current_section().is_commit());
    }

    CODEGEN_LINE(CodeSectionContext::pop_var_context())
    CODEGEN_LINE(CodeSectionContext::pop_var_context())
    CODEGEN_LINE(CodeSectionContext::pop_var_context())
    // Next function
    CODEGEN_LINE(l_fn_body = sample_fn.mk_section("test_fn_body"))
    CODEGEN_LINE(l_fn_body.enter())
    CODEGEN_LINE(ValueInfo sample_fn_ctx = sample_fn.context().value())
    CODEGEN_LINE(arg1 = sample_fn_ctx.field("arg1").load())
    CODEGEN_LINE(arg2 = sample_fn_ctx.field("arg2").load())
    CODEGEN_LINE(ValueInfo c1 = ValueInfo::from_constant(101))
    CODEGEN_LINE(ValueInfo c2 = ValueInfo::from_constant(999))
    CODEGEN_LINE(ValueInfo res = arg1.add(arg2, "construct_add"))
    CODEGEN_LINE(ValueInfo res2 = res.add(c1, "add_const1"))
    CODEGEN_LINE(ValueInfo res3 = res2.add(c2, "add_const1"))
    CODEGEN_LINE(ValueInfo res4 = res2.add(sample_fn_ctx.field("arg3").load(), "add_arg"))
    CODEGEN_LINE(ValueInfo res5 = res3.add(res4, "accum_res"))
    CODEGEN_LINE(CodeSectionContext::set_return_value(res5))
    // Next function
    std::cout << "------------- Function BEGIN ------------------------- " << std::endl;
    CODEGEN_LINE(l_fn_body = fn_if_else.mk_section("if_else_fn_begin"))
    CODEGEN_LINE(l_fn_body.enter())
    CODEGEN_LINE(ValueInfo if_else_ctx = fn_if_else.context().value())
    CODEGEN_LINE(arg_a = if_else_ctx.field("a").load())
    CODEGEN_LINE(arg_b = if_else_ctx.field("b").load())
    CODEGEN_LINE(c_5 = ValueInfo::from_constant(5))
    CODEGEN_LINE(compare_1 = arg_b.less_than(c_5, "comp_test"))
    LLVM_BUILDER_ALWAYS_ASSERT(bool_type.check_sync(compare_1));
    CODEGEN_LINE(ValueInfo compare_2 = compare_1.not_equal(ValueInfo::from_constant(false), "if_cond"))
    LLVM_BUILDER_ALWAYS_ASSERT(bool_type.check_sync(compare_2));
    CODEGEN_LINE(IfElseCond if_else_branch{"cond_block", compare_2})
    CODEGEN_LINE(CodeSectionContext::mk_ptr("test_value_fwd"_cs, int32_type, ValueInfo::from_constant(0)))
    CODEGEN_LINE(CodeSectionContext::mk_ptr("test_value_fwd_2"_cs, int32_type, ValueInfo::from_constant(0)))
    // if_else_branch.then_branch([] {
    CODEGEN_LINE(if_else_branch.then_branch().enter())
    CODEGEN_LINE(arg_a = if_else_ctx.field("a").load())
    CODEGEN_LINE(ValueInfo then_val = arg_a.add(ValueInfo::from_constant(999), "then_add_tmp"))
    CODEGEN_LINE(then_val.push("test_value_fwd"_cs))
    CODEGEN_LINE(ValueInfo then_val_2 = then_val.add(then_val, "double_then"))
    CODEGEN_LINE(then_val_2.push("test_value_fwd_2"_cs))
    CODEGEN_LINE(if_else_branch.then_branch().exit())
    CODEGEN_LINE(if_else_branch.else_branch().enter())
    CODEGEN_LINE(arg_a = if_else_ctx.field("a").load())
    CODEGEN_LINE(ValueInfo else_val = arg_a.add(ValueInfo::from_constant(8080), "else_add_tmp"))
    CODEGEN_LINE(else_val.push("test_value_fwd"_cs))
    CODEGEN_LINE(ValueInfo else_val_2 = else_val.add(else_val, "double_then"))
#if 0
    std::cout << " -------------- if_else_2 ---------- " << std::endl;
    CODEGEN_LINE(IfElseCond if_else_branch_2{"cond_block_2", compare_1})
    std::cout << " >>>> if_else_2 : enter then " << std::endl;
    CODEGEN_LINE(if_else_branch_2.then_branch().enter())
    CODEGEN_LINE(else_val_2.push("test_value_fwd_2"_cs))
    std::cout << " >>>> if_else_2 : exit then " << std::endl;
    CODEGEN_LINE(if_else_branch_2.then_branch().exit())
    std::cout << " >>>> if_else_2 : enter else " << std::endl;
    CODEGEN_LINE(if_else_branch_2.else_branch().enter())
    CODEGEN_LINE(else_val.push("test_value_fwd_2"_cs))
    std::cout << " >>>> if_else_2 : exit else " << std::endl;
    CODEGEN_LINE(if_else_branch_2.else_branch().exit())
    std::cout << " >>>> if_else_2 : bind " << std::endl;
    CODEGEN_LINE(if_else_branch_2.bind())
    std::cout << " -------------- if_else_2 ---------- " << std::endl;
#else
    CODEGEN_LINE(else_val_2.push("test_value_fwd_2"_cs))
#endif
    CODEGEN_LINE(if_else_branch.else_branch().exit())
    // if_else_branch.then_branch([] {
    //     CODEGEN_LINE(ValueInfo arg_a = CodeSectionContext::pop("a"_cs))
    //     CODEGEN_LINE(ValueInfo then_val = arg_a.add(ValueInfo::from_constant(999), "then_add_tmp"))
    //     CODEGEN_LINE(then_val.push("test_value_fwd"_cs))
    //     CODEGEN_LINE(ValueInfo then_val_2 = then_val.add(then_val, "double_then"))
    //     CODEGEN_LINE(then_val_2.push("test_value_fwd_2"_cs))
    // });
    // if_else_branch.else_branch([] {
    //     CODEGEN_LINE(ValueInfo arg_a = CodeSectionContext::pop("a"_cs))
    //     CODEGEN_LINE(ValueInfo else_val = arg_a.add(ValueInfo::from_constant(8080), "else_add_tmp"))
    //     CODEGEN_LINE(else_val.push("test_value_fwd"_cs))
    //     CODEGEN_LINE(ValueInfo else_val_2 = else_val.add(else_val, "double_then"))
    //     CODEGEN_LINE(else_val_2.push("test_value_fwd_2"_cs))
    // });
    CODEGEN_LINE(if_else_branch.bind())
    CODEGEN_LINE(ValueInfo l_test_value_fwd = CodeSectionContext::pop("test_value_fwd_2"_cs))
    CODEGEN_LINE(CodeSectionContext::set_return_value(l_test_value_fwd))
    std::cout << "------------- Function END ------------------------- " << std::endl;
    // Next function
    CODEGEN_LINE(l_fn_body = fn_loop.mk_section("loop_fn_begin"))
    CODEGEN_LINE(l_fn_body.enter())
    CODEGEN_LINE(ValueInfo loop_ctx = fn_loop.context().value())
    CODEGEN_LINE(arg_a = loop_ctx.field("a").load())
    // ValueInfo arg_b = l_fn_body.pop("b"_cs);
    CODEGEN_LINE(ValueInfo c_1 = ValueInfo::from_constant(1))
    CODEGEN_LINE(c_5 = ValueInfo::from_constant(5))
    CODEGEN_LINE(compare_1 = c_1.less_than(c_5, "comp_test"))
    CODEGEN_LINE(ValueInfo test_val = ValueInfo::mk_pointer(bool_type))
    CODEGEN_LINE(test_val.store(compare_1))
    // {
    //     CODEGEN_LINE(DynamicLoopCond loop_cond{"loop_cond_block", test_val})
    //     CODEGEN_LINE(CodeSectionContext::mk_ptr("test_value_fwd"_cs, int32_type, ValueInfo::from_constant(0)))
    //     {
    //         CODEGEN_LINE(DynamicLoopCond::BranchSection& loop_body = loop_cond.loop_body())
    //         CODEGEN_LINE(loop_body.enter())
    //         CODEGEN_LINE(ValueInfo then_val = arg_a.add(ValueInfo::from_constant(100), "add_value"))
    //         CODEGEN_LINE(then_val.push("test_value_fwd"_cs))
    //         CODEGEN_LINE(loop_body.exit())
    //     }
    //     CODEGEN_LINE(loop_cond.bind())
    //     CODEGEN_LINE(ValueInfo l_test_value_fwd = CodeSectionContext::pop("test_value_fwd"_cs))
    //     CODEGEN_LINE(CodeSectionContext::set_return_value(l_test_value_fwd))
    // }
    // Next function
    CODEGEN_LINE(l_fn_body = fn_load.mk_section("fn_load_begin"))
    CODEGEN_LINE(l_fn_body.enter())
    CODEGEN_LINE(ValueInfo load_ctx = fn_load.context().value())
    CODEGEN_LINE(arg_a = load_ctx.field("a").load())
    CODEGEN_LINE(arg_b = load_ctx.field("b").load())
    CODEGEN_LINE(ValueInfo arr_entry_value = arg_b.entry(2).load())
    CODEGEN_LINE(arg_b.entry(0).store(ValueInfo::from_constant(1987)))
    CODEGEN_LINE(CodeSectionContext::set_return_value(arr_entry_value))
#endif

    CODEGEN_LINE(std::cout << " Module m : " << l_module.name() << std::endl)
    CODEGEN_LINE(std::cout << " Function fn : " << fn.name() << std::endl)
    CODEGEN_LINE(std::cout << " CodeSection: " << fn_section.name() << std::endl)
    CODEGEN_LINE(std::cout << " Type name: " << int32_type.short_name() << std::endl)
    ErrorContext::print(std::cout, 5);
    LLVM_BUILDER_ASSERT(not fn.has_error())
    LLVM_BUILDER_ASSERT(not l_module.has_error())
    LLVM_BUILDER_ASSERT(not int32_type.has_error())
    LLVM_BUILDER_ASSERT(not ErrorContext::has_error());
    CODEGEN_LINE(l_module.write_to_file())

    LLVM_BUILDER_ALWAYS_ASSERT(l_module.is_init());
    CODEGEN_LINE(jit_runner.add_module(l_cursor))
    std::cout << "-----------------------------" << std::endl;
    if (ErrorContext::has_error()) {
        ErrorContext::print(std::cout, 5);
    }
    CODEGEN_LINE(jit_runner.bind())

    LLVM_BUILDER_ASSERT(not ErrorContext::has_error());
#if 0
    {

        {
            struct fn1_args_struct { int32_t arg1; int32_t arg2; int32_t arg3; };
            using raw_fn_t = int32_t(fn1_args_struct*);
            raw_fn_t* fn = jit_runner.get_fn<raw_fn_t>("if_else_test_sample_fn_name");
            for (int32_t i = 0; i != 10; ++i) {
                // NOTE{vibhanshu}: this is the replica of computation done above
                int32_t arg1 = i;
                int32_t arg2 = i + 1;
                int32_t arg3 = i + 2;
                int32_t c1 = 101;
                int32_t c2 = 999;
                int32_t res = arg1 + arg2;
                int32_t res2 = res + c1;
                int32_t res3 = res2 + c2;
                int32_t res4 = res2 + arg3;
                int32_t res5 = res3 + res4;
                fn1_args_struct args{arg1, arg2, arg3};
                LLVM_BUILDER_ALWAYS_ASSERT(res5 == fn(&args));
            }
            std::cout << "sample_fn_name passed all tests" << std::endl;
        }
        {
            struct if_else_args_struct { int32_t a; int32_t b; int32_t c; };
            using raw_fn_t = int32_t(if_else_args_struct*);
            raw_fn_t* fn = jit_runner.get_fn<raw_fn_t>("if_else_test_foo_if_else");
            for (int32_t i = 0; i != 10; ++i) {
                // NOTE{vibhanshu}: this is the replica of computation done above
                int32_t arg_a = i;
                int32_t arg_b = i + 1;
                int32_t c_5 = 5;
                bool compare_1 = arg_b < c_5;
                bool compare_2 = compare_1 != false;
                int32_t test_value_fwd;
                int32_t test_value_fwd_2;
                if (compare_2) {
                    test_value_fwd = arg_a + 999;
                    test_value_fwd_2 = test_value_fwd + test_value_fwd;
                } else {
                    test_value_fwd = arg_a + 8080;
                    test_value_fwd_2 = test_value_fwd + test_value_fwd;
                }
                if_else_args_struct args{arg_a, arg_b, i+2};
                LLVM_BUILDER_ALWAYS_ASSERT(test_value_fwd_2 == fn(&args));
            }
            std::cout << "foo_if_else passed all tests" << std::endl;
        }
        // TODO{vibhanshu}: add unit-test for foo-lop also
        {
            struct fn_load_args_struct { int32_t a; int32_t* b; };
            using raw_fn_t = int32_t(fn_load_args_struct*);
            raw_fn_t* fn = jit_runner.get_fn<raw_fn_t>("if_else_test_foo_loop_v2");
            LLVM_BUILDER_ALWAYS_ASSERT(fn != nullptr);
            for (int32_t i = 0; i != 10; ++i) {
                // NOTE{vibhanshu}: this is the replica of computation done above
                int arr[5] = {i+100, i + 101, i+102, i+103, i+104};
                int32_t arg_a = i;
                int32_t* arg_b = arr;
                int32_t entry = arg_b[2];
                LLVM_BUILDER_ALWAYS_ASSERT(arr[0] == (i + 100));
                fn_load_args_struct args{arg_a, arg_b};
                LLVM_BUILDER_ALWAYS_ASSERT(entry == fn(&args));
                LLVM_BUILDER_ALWAYS_ASSERT(arr[0] == 1987);
            }
            std::cout << "foo_loop_v2 passed all tests" << std::endl;
        }
    }
#endif


    
    std::cout << " ======= APP END ======= " << std::endl;
    return 0;
}

