//
// Created by vibhanshu on 2024-02-08
//

#ifndef CORE_LINKER_OBJECT_H_
#define CORE_LINKER_OBJECT_H_

#include "defines.h"
#include "symbol.h"
#include "core/meta/noncopyable.h"
#include "core/util/dtype_util.h"
#include "core/ds/fixed_vector.h"

#include <ostream>

CORE_LINKER_NS_BEGIN

class LinkedObject;

class BaseRef : meta::noncopyable {
    friend class LinkedObject;
    friend class ObjectUtil;
protected:
    struct construct_t {
    };
public:
    // TODO{vibhanshu}: add support for ptr variable sharing also
    using callback_t = void();
    using object_callback_t = void(void*);
    enum class type_t : uint8_t {
        none,
        producer_fn,
        producer_object_fn,
        producer_data,
        consumer_fn,
        consumer_object_fn,
        consumer_data
    };
private:
    ExportSymbol m_symbol;
    type_t m_type = type_t::none;
    callback_t* m_callback = nullptr;
    object_callback_t* m_object_callback = nullptr;
    base_dtype_t m_data_type;
    void *m_data_ptr = nullptr;
    bool m_is_mandatory = true;
protected:
    explicit BaseRef() = default;
    explicit BaseRef(const ExportSymbol& symbol, type_t type, bool is_mandatory);
    explicit BaseRef(const ExportSymbol& symbol, type_t type, callback_t* callback, bool is_mandatory);
    explicit BaseRef(const ExportSymbol& symbol, type_t type, object_callback_t* callback, bool is_mandatory);
    explicit BaseRef(const ExportSymbol& symbol, type_t type, base_dtype_t &dtype, void *data_ptr, bool is_mandatory);
public:
    ~BaseRef() = default;
public:
    bool is_valid() const {
        return m_type != type_t::none;
    }
    const ExportSymbol &symbol() const {
        return m_symbol;
    }
    bool is_mandatory() const {
        return m_is_mandatory;
    }
    bool operator==(const BaseRef &o) const;
    bool is_function() const;
    bool is_object_function() const;
    bool is_data() const;
    bool is_producer() const {
      return m_type == type_t::producer_data
          or m_type == type_t::producer_fn
          or m_type == type_t::producer_object_fn;
    }
    bool is_consumer() const {
      return m_type == type_t::consumer_data
          or m_type == type_t::consumer_fn
          or m_type == type_t::consumer_object_fn;
    }
    bool has_callback() const {
      return m_callback != nullptr;
    }
    bool has_object_callback() const {
      return m_object_callback != nullptr;
    }
    bool has_data_ptr() const {
      return m_data_ptr != nullptr;
    }
    void set_callback(callback_t* cb);
    void set_object_callback(object_callback_t* cb);
    void set_data_ptr(void *ptr);
    callback_t *callback() const;
    object_callback_t* object_callback() const;
    void invoke_callback() const;
    void invoke_object_callback(void* object) const;
    const base_dtype_t& data_type() const;
    void *data_ptr() const {
      return m_data_ptr;
    }
private:
    void M_init_var();
    void M_init_fn();
    void M_init_object_fn();
};

class Producer : public BaseRef {
    using BaseT = BaseRef;
public:
    explicit Producer() = default;
public:
    explicit Producer(const ExportSymbol &symbol, type_t type, bool is_mandatory, construct_t);
    explicit Producer(const ExportSymbol &symbol, base_dtype_t &dtype, void *data_ptr, bool is_mandatory, construct_t);
    ~Producer() = default;
public:
    void print(std::ostream &os) const;
    OSTREAM_FRIEND(Producer)
public:
    static Producer& mk_function(const ExportSymbol& symbol, bool is_mandatory = true);
    static Producer& mk_object_function(const ExportSymbol& symbol, bool is_mandatory = true);
    static Producer& mk_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory = true);
};

class Consumer : public BaseRef {
    using BaseT = BaseRef;
public:
    explicit Consumer() = default;
public:
    explicit Consumer(const ExportSymbol &symbol, callback_t *callback, bool is_mandatory, construct_t);
    explicit Consumer(const ExportSymbol &symbol, object_callback_t *callback, bool is_mandatory, construct_t);
    explicit Consumer(const ExportSymbol &symbol, base_dtype_t &dtype, void *data_ptr, bool is_mandatory, construct_t);
    ~Consumer() = default;
public:
    void print(std::ostream &os) const;
    OSTREAM_FRIEND(Consumer)
public:
    static Consumer& mk_function(const ExportSymbol& symbol, callback_t* callback, bool is_mandatory = true);
    static Consumer& mk_object_function(const ExportSymbol& symbol, object_callback_t* callback, bool is_mandatory = true);
    static Consumer& mk_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory = true);
};

class LinkedObject {
    Producer* m_producer = nullptr;
    Consumer* m_consumer = nullptr;
public:
    explicit LinkedObject() = default;
    ~LinkedObject() = default;
public:
    void set_producer(Producer &producer);
    void set_consumer(Consumer &consumer);
    void bind();
};

class ObjectUtil {
    // TODO{vibhanshu}: make this configurable at startup
    enum : uint32_t {
        c_max_link_count = 1 << 15
    };
    using callback_t = typename BaseRef::callback_t;
    using object_callback_t = typename BaseRef::object_callback_t;
    using type_t = typename BaseRef::type_t;
private:
    fixed_vector<Producer> m_producers;
    fixed_vector<Consumer> m_consumers;
    std::unordered_map<ExportSymbol, std::reference_wrapper<Producer>> m_producer_map;
    std::unordered_map<ExportSymbol, std::reference_wrapper<Consumer>> m_consumer_map;
private:
    explicit ObjectUtil();
public:
    ~ObjectUtil();
public:
    void clear();
    Producer& mk_producer_function(const ExportSymbol& symbol, bool is_mandatory);
    Producer& mk_producer_object_function(const ExportSymbol& symbol, bool is_mandatory);
    Producer& mk_producer_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory);
    Consumer& mk_consumer_function(const ExportSymbol& symbol, callback_t* callback, bool is_mandatory);
    Consumer& mk_consumer_object_function(const ExportSymbol& symbol, object_callback_t* callback, bool is_mandatory);
    Consumer& mk_consumer_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory);
public:
    static ObjectUtil& singleton();
};

//
CORE_LINKER_NS_END

namespace std {

CORE_INLINE_ART
static bool
operator==(const std::reference_wrapper<core::linker::Producer> lhs,
           const std::reference_wrapper<core::linker::Producer> rhs) {
  return lhs.get() == rhs.get();
}

CORE_INLINE_ART
static bool
operator==(const std::reference_wrapper<core::linker::Consumer> lhs,
           const std::reference_wrapper<core::linker::Consumer> rhs) {
  return lhs.get() == rhs.get();
}

} // namespace std

#endif // CORE_LINKER_OBJECT_H_
