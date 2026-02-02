//
// Created by vibhanshu on 2024-08-10
//

#ifndef CORE_UTIL_CALLBACKS_H_
#define CORE_UTIL_CALLBACKS_H_

#include "core/util/defines.h"
#include "core/ds/fixed_string.h"
#include "core/meta/noncopyable.h"

#include <functional>
#include <string>

CORE_NS_BEGIN

class BaseCallback : meta::noncopyable {
    fixed_string m_name;
    bool m_is_active = true;
protected:
    explicit BaseCallback(fixed_string name);
public:
    ~BaseCallback();
public:
    fixed_string name() const {
        return m_name;
    }
    bool is_active() const {
        return m_is_active;
    }
    void mark_active() {
        m_is_active = true;
    }
    void mark_inactive() {
        m_is_active = false;
    }
};

class NamedCallback : public BaseCallback {
    using BaseT = BaseCallback;
public:
    using callback_t = std::function<void()>;
private:
    callback_t m_callback;
public:
    explicit NamedCallback(fixed_string name, callback_t&& callback);
    ~NamedCallback();
public:
    void operator()() {
        if (is_active()) {
            m_callback();
        }
    }
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(NamedCallback)
};

template <typename T>
class NamedStateCallback : public BaseCallback {
    using BaseT = BaseCallback;
public:
    using callback_t = std::function<void(const T&)>;
private:
    callback_t m_callback;
public:
    explicit NamedStateCallback(fixed_string name, callback_t&& callback)
        : BaseT{name}, m_callback{callback} {
    }
    ~NamedStateCallback() = default;
public:
    void operator()(const T& v) {
        if (is_active()) {
            m_callback(v);
        }
    }
    void print(std::ostream& os) const {
        os << "{NamedStateCallack:name=" << BaseT::name() << "}";
    }
    OSTREAM_FRIEND(NamedStateCallback)
};

template <typename named_cb_t>
class _CallbackManager : meta::noncopyable {
    using callback_t = typename named_cb_t::callback_t;
private:
    fixed_string_map<named_cb_t> m_callbacks;
public:
    explicit _CallbackManager() = default;
    ~_CallbackManager() = default;
public:
    bool emplace_callback(fixed_string name, callback_t&& callback) {
        auto it = m_callbacks.try_emplace(name, name, std::move(callback));
        return it.second;
    }
    void mark_active(fixed_string name) {
        auto it = m_callbacks.find(name);
        if (it != m_callbacks.end()) {
            it->second.mark_active();
        }
    }
    void mark_inactive(fixed_string name) {
        auto it = m_callbacks.find(name);
        if (it != m_callbacks.end()) {
            it->second.mark_inactive();
        }
    }
    template <typename... Args>
    void operator()(Args... args) {
        for (std::pair<const fixed_string, named_cb_t>& cb : m_callbacks) {
            cb.second(std::forward<Args>(args)...);
        }
    }
};

extern template class _CallbackManager<NamedCallback>;

using SimpleCallbackManager = _CallbackManager<NamedCallback>;

template <typename StateT>
using StateCallbackManager = _CallbackManager<NamedStateCallback<StateT>>;

CORE_NS_END

#endif // CORE_UTIL_CALLBACKS_H_
