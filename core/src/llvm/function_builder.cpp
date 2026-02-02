//
// Created by vibhanshu on 2025-11-21
// 

#include "core/llvm/function.h"
#include "core/meta/noncopyable.h"
#include "util/debug.h"
#include "core/llvm/module.h"
#include "llvm/context_impl.h"

LLVM_NS_BEGIN

//
// FunctionBuilder::Impl
//
class FunctionBuilder::Impl : meta::noncopyable {
    TypeInfo m_return_type;
    FnContext m_context;
    std::string m_name;
    Module m_module;
    bool m_is_external = false;
    Function m_fn;
public:
    explicit Impl() = default;
    ~Impl() = default;
public:
    bool is_frozen() const {
        return not m_fn.has_error();
    }
    void set_context(const FnContext& context) {
        CORE_ASSERT(not context.has_error());
        m_context = context;
    }
    void set_name(FunctionBuilder& parent, const std::string& name) {
        CODEGEN_FN
        if (not m_name.empty()) {
            CODEGEN_PUSH_ERROR(FUNCTION, "Trying to rename function from:" << m_name << "  to:" << name);
            parent.M_mark_error();
            return;
        }
        CORE_ASSERT(not name.empty());
        m_name = name;
    }
    void set_module(FunctionBuilder& parent, Module& mod) {
        CODEGEN_FN
        if (not m_module.has_error()) {
            CODEGEN_PUSH_ERROR(FUNCTION, "Module already set:" << m_module.name() << " can't change it to :" << mod.name());
            parent.M_mark_error();
            return;
        }
        CORE_ASSERT(not mod.has_error());
        m_module = mod;
    }
    void mark_external() {
        m_is_external = true;
    }
    Function compile(FunctionBuilder& parent) {
        CODEGEN_FN
        if (not CursorContextImpl::has_value()) {
            CODEGEN_PUSH_ERROR(CONTEXT, "function can't be compiled as context not found");
            parent.M_mark_error();
            return Function::null();
        }
        if (not is_frozen()) {
            m_return_type = TypeInfo::mk_int32();
            if (m_name.empty()) {
                CODEGEN_PUSH_ERROR(FUNCTION, "Function name not set in function builder");
                parent.M_mark_error();
                return Function{};
            }
            if (not m_return_type.is_valid() or m_return_type.has_error()) {
                CODEGEN_PUSH_ERROR(FUNCTION, "Return type not defined of function:" << m_name);
                parent.M_mark_error();
                return Function{};
            }
            if (m_context.has_error() or not m_context.is_valid()) {
                CODEGEN_PUSH_ERROR(FUNCTION, "invalid function argument");
                parent.M_mark_error();
                return Function{};
            }
            if (m_module.has_error()) {
                FunctionImpl impl(m_name, m_return_type, m_context, Function::c_construct{});
                m_fn = CursorContextImpl::mk_function(std::move(impl));
                CORE_ASSERT(not impl.is_valid());
            } else {
                FunctionImpl impl(m_module, m_name, m_return_type, m_context, Function::c_construct{});
                m_fn = CursorContextImpl::mk_function(std::move(impl));
                CORE_ASSERT(not impl.is_valid());
            }
        }
        if (is_frozen()) {
            return m_fn;
        } else {
            CODEGEN_PUSH_ERROR(FUNCTION, "function can't be compiled");
            parent.M_mark_error();
            return Function::null();
        }
    }
public:
    static FunctionBuilder M_gen_func_builder(Module& m, const std::string& name, const TypeInfo& return_type) {
        CODEGEN_FN
        FunctionBuilder fn_builder;
        if (m.has_error() or return_type.has_error() or name.empty()) {
            return fn_builder;
        }
        fn_builder.set_module(m)
            .set_name(name);
        return fn_builder;
    }
    static FunctionBuilder M_gen_func_builder_external(const std::string& name, const TypeInfo& return_type) {
        CODEGEN_FN
        FunctionBuilder fn_builder;
        if (return_type.has_error() or name.empty()) {
            return fn_builder;
        }
        fn_builder.set_name(name)
            .mark_external();
        return fn_builder;
    }
};

//
// FunctionBuilder
//
FunctionBuilder::FunctionBuilder() : BaseT{State::VALID} {
    m_impl = std::make_shared<Impl>();
}

FunctionBuilder::~FunctionBuilder() = default;

bool FunctionBuilder::is_frozen() const {
    if (has_error()) {
        return true;
    }
    return m_impl->is_frozen();
}

FunctionBuilder& FunctionBuilder::set_context(const FnContext& context) {
    CODEGEN_FN
    if (has_error()) {
        return *this;
    }
    if (context.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Can't set context to an invalid value");
        M_mark_error();
        return *this;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "FunctionBuilder is already used");
        M_mark_error();
        return *this;
    }
    m_impl->set_context(context);
    return *this;
}

FunctionBuilder& FunctionBuilder::set_name(const std::string& name) {
    CODEGEN_FN
    if (has_error()) {
        return *this;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "FunctionBuilder is already used");
        M_mark_error();
        return *this;
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Can't set function name to be empty");
        M_mark_error();
        return *this;
    }
    m_impl->set_name(*this, name);
    return *this;
}

FunctionBuilder& FunctionBuilder::set_module(Module& mod) {
    CODEGEN_FN
    if (has_error()) {
        return *this;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "FunctionBuilder is already used");
        M_mark_error();
        return *this;
    }
    if (mod.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "module not valid");
        M_mark_error();
        return *this;
    }
    m_impl->set_module(*this, mod);
    return *this;
}

FunctionBuilder& FunctionBuilder::mark_external() {
    CODEGEN_FN
    if (has_error()) {
        M_mark_error();
        return *this;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "FunctionBuilder is already used");
        M_mark_error();
        return *this;
    }
    m_impl->mark_external();
    return *this;
}

Function FunctionBuilder::compile() {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Trying to compile invalid function object");
        return Function::null();
    }
    Function res = m_impl->compile(*this);
    m_impl.reset();
    M_mark_error();
    return res;
}

void FunctionBuilder::print(std::ostream& os) const {
    os << "{FunctionBuilder}";
}

bool FunctionBuilder::operator == (const FunctionBuilder& rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_impl.get() == rhs.m_impl.get();
}

FunctionBuilder FunctionBuilder::null() {
    static FunctionBuilder s_null{};
    s_null.M_mark_error();
    return s_null;
}

LLVM_NS_END
