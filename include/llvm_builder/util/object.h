//
// Created by vibhanshu on 2025-11-13
// 

#ifndef LLVM_BUILDER_API_OBJECT_H_
#define LLVM_BUILDER_API_OBJECT_H_

#include "llvm_builder/defines.h"
#include "llvm_builder/meta/noncopyable.h"
#include "error.h"

LLVM_BUILDER_NS_BEGIN

// static_assert(std::is_base_of_v<_BaseObject<NAME>, NAME>);  
#define CONTEXT_DECL(NAME)                    \
public:                                       \
    class Context {                           \
    public:                                   \
        explicit Context(NAME value);         \
        ~Context();                           \
    public:                                   \
        static bool has_value();              \
        static NAME& value();                 \
        static void set_value(NAME& value);   \
        static NAME clear_value();            \
        static bool is_value(NAME& value);    \
    private:                                  \
        static NonNullPtr<NAME>& M_value();   \
    };                                        \
    friend class Context;                     \
    bool is_in_context();                     \
/**/                                          \

#define CONTEXT_DEF(NAME)                            \
NAME::Context::Context(NAME value) {                 \
    set_value(value);                                \
}                                                    \
NAME::Context::~Context() {                          \
    clear_value();                                   \
}                                                    \
bool NAME::Context::has_value() {                    \
    return M_value().has_value();                    \
}                                                    \
NAME& NAME::Context::value() {                       \
    return *M_value();                               \
}                                                    \
void NAME::Context::set_value(NAME& value) {         \
    if (not has_value()) {                           \
        M_value().set_value(value);                  \
    } else {                                         \
        CODEGEN_PUSH_ERROR(CONTEXT, "re-setting context not allowed");    \
    }                                                \
}                                                    \
NAME NAME::Context::clear_value() {                  \
    if (has_value()) {                               \
        return M_value().clear_value();              \
    } else {                                         \
        CODEGEN_PUSH_ERROR(CONTEXT, "clearing empty context not allowed");    \
        return *M_value();                           \
    }                                                \
}                                                    \
bool NAME::Context::is_value(NAME& value) {          \
    if (has_value()) {                               \
        return *M_value() == value;                  \
    } else {                                         \
        return false;                                \
    }                                                \
}                                                    \
NonNullPtr<NAME>& NAME::Context::M_value()  {        \
    static thread_local NonNullPtr<NAME> s_value;    \
    return s_value;                                  \
}                                                    \
bool NAME::is_in_context() {                         \
    return Context::is_value(*this);                 \
}                                                    \
/**/

template <typename T>
class NonNullPtr : meta::noncopyable {
    static_assert(std::is_copy_constructible_v<T>);
    static_assert(std::is_copy_assignable_v<T>);
private:
    T m_null;
    T m_ptr;
public:
    explicit NonNullPtr()
      : m_null{T::null()}
      , m_ptr{m_null} {
    }
    ~NonNullPtr() = default;
public:
    bool is_null() const {
        return m_ptr == m_null;
    }
    bool has_value() const {
        return not is_null();
    }
    T& value() {
        return m_ptr;
    }
    const T& value() const {
        return m_ptr;
    }
    T& operator*() {
        return m_ptr;
    }
    const T& operator*() const {
        return m_ptr;
    }
    T set_value(T& v) {
        T l_ret = m_ptr;
        m_ptr = v;
        return l_ret;
    }
    T clear_value() {
        return set_value(m_null);
    }
};

// NOTE{vibhanshu}: common interface of API public interface
template <typename DerivT>
class _BaseObject {
public:
    enum class State : uint32_t {
        VALID,
        ERROR
    };
private:
    mutable State m_state = State::VALID;
protected:
    _BaseObject(State state) : m_state{state} {
    }
    _BaseObject(_BaseObject&& o) : m_state{o.m_state} {
    }
    _BaseObject(const _BaseObject& o) : m_state{o.m_state} {
    }
    _BaseObject& operator = (const _BaseObject& o) {
        m_state = o.m_state;
        return *this;
    }
    _BaseObject& operator = (_BaseObject&& o) {
        m_state = o.m_state;
        return *this;
    }
public:
    void M_mark_error() const {
        m_state = State::ERROR;
    }
    bool has_error() const {
        return m_state == State::ERROR or ErrorContext::has_error();
    }
    const Error& error() const;
    bool is_null() const {
        return static_cast<const DerivT&>(*this) == DerivT::null();
    }
};

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_API_OBJECT_H_
