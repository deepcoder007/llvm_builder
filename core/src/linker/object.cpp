//
// Created by vibhanshu on 2024-02-08
//

#include "core/linker/object.h"
#include "util/debug.h"
#include "meta/basic_meta.h"
#include "meta/simple.h"
#include "util/string_util.h"

CORE_LINKER_NS_BEGIN

// BaseRef
//
BaseRef::BaseRef(const ExportSymbol& symbol, type_t type, bool is_mandatory)
    : m_symbol{symbol}, m_type{type}, m_is_mandatory{is_mandatory} {
    if (is_function()) {
       M_init_fn();
    } else {
        CORE_ALWAYS_ASSERT(is_object_function());
        M_init_object_fn();
    }
}

BaseRef::BaseRef(const ExportSymbol& symbol, type_t type, callback_t* callback, bool is_mandatory)
    : m_symbol{symbol}, m_type{type}, m_callback{callback}, m_is_mandatory{is_mandatory} {
    CORE_ALWAYS_ASSERT(m_callback != nullptr);
    M_init_fn();
}

BaseRef::BaseRef(const ExportSymbol& symbol, type_t type, object_callback_t* callback, bool is_mandatory)
    : m_symbol{symbol}, m_type{type}, m_object_callback{callback}, m_is_mandatory{is_mandatory} {
    CORE_ALWAYS_ASSERT(m_object_callback != nullptr);
    M_init_object_fn();
}

BaseRef::BaseRef(const ExportSymbol& symbol, type_t type, base_dtype_t& dtype, void* data_ptr, bool is_mandatory)
    : m_symbol{symbol}, m_type{type}, m_data_type{dtype}, m_data_ptr{data_ptr}, m_is_mandatory{is_mandatory} {
    M_init_var();
}

void BaseRef::M_init_var() {
    CORE_ALWAYS_ASSERT(is_data());
    CORE_ALWAYS_ASSERT(m_callback == nullptr);
    CORE_ALWAYS_ASSERT(m_object_callback == nullptr);
}

void BaseRef::M_init_fn() {
    CORE_ALWAYS_ASSERT(is_function());
    CORE_ALWAYS_ASSERT(not m_data_type.is_valid());
    if (m_type == type_t::producer_fn) {
        CORE_ALWAYS_ASSERT(m_callback == nullptr);
    } else {
        CORE_ALWAYS_ASSERT(m_type == type_t::consumer_fn);
    }
}

void BaseRef::M_init_object_fn() {
    CORE_ALWAYS_ASSERT(is_object_function());
    CORE_ALWAYS_ASSERT(not m_data_type.is_valid());
    if (m_type == type_t::producer_object_fn) {
        CORE_ALWAYS_ASSERT(m_object_callback == nullptr);
    } else {
        CORE_ALWAYS_ASSERT(m_type == type_t::consumer_object_fn);
    }
}

bool BaseRef::operator==(const BaseRef& o) const {
    const bool r = m_symbol == o.m_symbol;
    if (r) {
        CORE_ALWAYS_ASSERT(m_type == o.m_type);
        CORE_ALWAYS_ASSERT(m_callback == o.m_callback);
        CORE_ALWAYS_ASSERT(m_object_callback == o.m_object_callback);
        CORE_ALWAYS_ASSERT(m_data_type == o.m_data_type);
        if (has_data_ptr() and o.has_data_ptr()) {
            CORE_ALWAYS_ASSERT(data_ptr() == o.data_ptr());
        } else {
            CORE_ALWAYS_ASSERT(not has_data_ptr());
            CORE_ALWAYS_ASSERT(not has_data_ptr());
        }
    }
    return r;
}

bool BaseRef::is_function() const {
    return m_type == type_t::producer_fn or m_type == type_t::consumer_fn;
}

bool BaseRef::is_object_function() const {
    return m_type == type_t::producer_object_fn or m_type == type_t::consumer_object_fn;
}

bool BaseRef::is_data() const {
    bool v = m_type == type_t::producer_data or m_type == type_t::consumer_data;
    CORE_ASSERT(meta::bool_if(v, m_data_type.is_valid()));
    return v;
}

void BaseRef::set_callback(callback_t* cb) {
    CORE_ALWAYS_ASSERT(cb != nullptr);
    CORE_ALWAYS_ASSERT(not has_callback());
    m_callback = cb;
}

void BaseRef::set_object_callback(object_callback_t* cb) {
    CORE_ALWAYS_ASSERT(cb != nullptr);
    CORE_ALWAYS_ASSERT(not has_object_callback());
    m_object_callback = cb;
}

void BaseRef::set_data_ptr(void* ptr) {
    CORE_ALWAYS_ASSERT(ptr != nullptr);
    CORE_ALWAYS_ASSERT(not has_data_ptr());
    m_data_ptr = ptr;
}

auto BaseRef::callback() const -> callback_t* {
    CORE_ASSERT(is_function());
    return m_callback;
}

auto BaseRef::object_callback() const -> object_callback_t* {
    CORE_ASSERT(is_object_function());
    return m_object_callback;
}

void BaseRef::invoke_callback() const {
    CORE_ASSERT(has_callback());
    m_callback();
}

void BaseRef::invoke_object_callback(void* object) const {
    CORE_ASSERT(has_object_callback());
    CORE_ASSERT(object != nullptr);
    m_object_callback(object);
}

auto BaseRef::data_type() const -> const base_dtype_t& {
    CORE_ASSERT(is_data());
    return m_data_type;
}

//
// Producer
//
Producer::Producer(const ExportSymbol& symbol, type_t type, bool is_mandatory, construct_t) : BaseT{symbol, type, is_mandatory} {
}

Producer::Producer(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory, construct_t)
    : BaseT{symbol, type_t::producer_data, dtype, data_ptr, is_mandatory} {
}

void Producer::print(std::ostream& os) const {
    os << "{Producer: name=" << symbol()
       << ", is_function=" << is_function()
       << ", is_data=" << is_data()
       << ", is_producer=" << is_producer()
       << ", is_consumer=" << is_consumer()
       << ", has_callback=" << has_callback()
       << ", has_data=" << has_data_ptr()
       << "}";
}

auto Producer::mk_function(const ExportSymbol& symbol, bool is_mandatory) -> Producer& {
    return ObjectUtil::singleton().mk_producer_function(symbol, is_mandatory);
}

auto Producer::mk_object_function(const ExportSymbol& symbol, bool is_mandatory) -> Producer& {
    return ObjectUtil::singleton().mk_producer_object_function(symbol, is_mandatory);
}

auto Producer::mk_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory) -> Producer& {
    return ObjectUtil::singleton().mk_producer_data_ptr(symbol, dtype, data_ptr, is_mandatory);
}

//
// Consumer
//
Consumer::Consumer(const ExportSymbol& symbol, callback_t* callback, bool is_mandatory, construct_t)
    : BaseT{symbol, type_t::consumer_fn, callback, is_mandatory} {
}

Consumer::Consumer(const ExportSymbol &symbol, object_callback_t *callback, bool is_mandatory, construct_t)
    : BaseT{symbol, type_t::consumer_object_fn, callback, is_mandatory} {
}

Consumer::Consumer(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory, construct_t)
    : BaseT{symbol, type_t::consumer_data, dtype, data_ptr, is_mandatory} {
}

void Consumer::print(std::ostream& os) const {
    os << "{Consumer: name=" << symbol()
       << ", is_function=" << is_function()
       << ", is_data=" << is_data()
       << ", is_producer=" << is_producer()
       << ", is_consumer=" << is_consumer()
       << ", has_callback=" << has_callback()
       << ", has_data=" << has_data_ptr() << "}";
}

auto Consumer::mk_function(const ExportSymbol& symbol, callback_t* callback, bool is_mandatory) -> Consumer& {
    return ObjectUtil::singleton().mk_consumer_function(symbol, callback, is_mandatory);
}

auto Consumer::mk_object_function(const ExportSymbol& symbol, object_callback_t* callback, bool is_mandatory) -> Consumer& {
    return ObjectUtil::singleton().mk_consumer_object_function(symbol, callback, is_mandatory);
}

auto Consumer::mk_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory) -> Consumer& {
    return ObjectUtil::singleton().mk_consumer_data_ptr(symbol, dtype, data_ptr, is_mandatory);
}


//
// LinkedObject
//
void LinkedObject::set_producer(Producer& producer) {
    CORE_ALWAYS_ASSERT(m_producer == nullptr)
    CORE_ALWAYS_ASSERT(producer.is_valid());
    m_producer = &producer;
}

void LinkedObject::set_consumer(Consumer& consumer) {
    CORE_ALWAYS_ASSERT(m_consumer == nullptr);
    CORE_ALWAYS_ASSERT(consumer.is_valid());
    m_consumer = &consumer;
}

void LinkedObject::bind() {
    CORE_ALWAYS_ASSERT_INFO(m_producer != nullptr, CORE_CONCAT << "Producer not found.");
    CORE_ALWAYS_ASSERT_INFO(m_consumer != nullptr, CORE_CONCAT << "Consumer not found.");
    CORE_ALWAYS_ASSERT_INFO(m_producer->symbol() == m_consumer->symbol(),
                          CORE_CONCAT << "Producer-Consumer key mismatch: "
                                    << "Producer[" << m_producer->symbol() << "] "
                                    << "Consumer[" << m_consumer->symbol() << "]");
    CORE_ALWAYS_ASSERT(m_producer->is_valid());
    CORE_ALWAYS_ASSERT(m_consumer->is_valid());
    if (m_consumer->is_function()) {
        CORE_ALWAYS_ASSERT(m_producer->is_function());
        CORE_ALWAYS_ASSERT(m_consumer->has_callback());
        CORE_ALWAYS_ASSERT(not m_producer->has_callback());
        m_producer->set_callback(m_consumer->callback());
    } else if (m_consumer->is_object_function()) {
        CORE_ALWAYS_ASSERT(m_producer->is_object_function());
        CORE_ALWAYS_ASSERT(m_consumer->has_object_callback());
        CORE_ALWAYS_ASSERT(not m_producer->has_object_callback());
        m_producer->set_object_callback(m_consumer->object_callback());
    } else {
        CORE_ALWAYS_ASSERT(m_consumer->is_data());
        CORE_ALWAYS_ASSERT(m_producer->is_data());
        const base_dtype_t& producer_dtype = m_producer->data_type();
        const base_dtype_t& consumer_dtype = m_consumer->data_type();
        CORE_ALWAYS_ASSERT_INFO(producer_dtype == consumer_dtype,
                              CORE_CONCAT << "Producer-Consumer dtype mismatch: "
                                        << "Producer:[" << producer_dtype << "] "
                                        << "Consumer:[" << consumer_dtype << "]"
                                        << "for symbol:" << m_producer->symbol());
        if (m_producer->has_data_ptr()) {
            CORE_ALWAYS_ASSERT_INFO(not m_consumer->has_data_ptr(),
                                  CORE_CONCAT << "Producer already provided data storage for:[" << m_producer->symbol()
                                            << "]");
            m_consumer->set_data_ptr(m_producer->data_ptr());
        } else {
            CORE_ALWAYS_ASSERT_INFO(m_consumer->has_data_ptr(),
                                  CORE_CONCAT << "Atleat producer or consumer should provide data storage:["
                                            << m_producer->symbol() << "]");
            m_producer->set_data_ptr(m_consumer->data_ptr());
        }
    }
}

//
// ObjectUtil
//
ObjectUtil::ObjectUtil()
    : m_producers{c_max_link_count}
    , m_consumers{c_max_link_count} {
}

ObjectUtil::~ObjectUtil() = default;

ObjectUtil& ObjectUtil::singleton() {
    static ObjectUtil s_singleton{};
    return s_singleton;
}

void ObjectUtil::clear() {
    m_producers.clear();
    m_consumers.clear();
    m_producer_map.clear();
    m_consumer_map.clear();
}

auto ObjectUtil::mk_producer_function(const ExportSymbol& symbol, bool is_mandatory) -> Producer& {
    Producer& r = m_producers.emplace_back(symbol, type_t::producer_fn, is_mandatory, BaseRef::construct_t{});
    auto it = m_producer_map.try_emplace(symbol, r);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Producer symbol already exists:" << symbol);
    return r;
}

auto ObjectUtil::mk_producer_object_function(const ExportSymbol& symbol, bool is_mandatory) -> Producer& {
    Producer& r = m_producers.emplace_back(symbol, type_t::producer_object_fn, is_mandatory, BaseRef::construct_t{});
    auto it = m_producer_map.try_emplace(symbol, r);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Producer symbol already exists:" << symbol);
    return r;
}

auto ObjectUtil::mk_producer_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory) -> Producer& {
    Producer& r = m_producers.emplace_back(symbol, dtype, data_ptr, is_mandatory, BaseRef::construct_t{});
    auto it = m_producer_map.try_emplace(symbol, r);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Producer symbol already exists:" << symbol);
    return r;
}

auto ObjectUtil::mk_consumer_function(const ExportSymbol& symbol, callback_t* callback, bool is_mandatory) -> Consumer& {
    Consumer& r = m_consumers.emplace_back(symbol, callback, is_mandatory, BaseRef::construct_t{});
    auto it = m_consumer_map.try_emplace(symbol, r);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Consumer symbol already exists:" << symbol);
    return r;
}

auto ObjectUtil::mk_consumer_object_function(const ExportSymbol& symbol, object_callback_t* callback, bool is_mandatory) -> Consumer& {
    Consumer& r = m_consumers.emplace_back(symbol, callback, is_mandatory, BaseRef::construct_t{});
    auto it = m_consumer_map.try_emplace(symbol, r);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Consumer symbol already exists:" << symbol);
    return r;
}

auto ObjectUtil::mk_consumer_data_ptr(const ExportSymbol& symbol, base_dtype_t& dtype, void* data_ptr, bool is_mandatory) -> Consumer& {
    Consumer& r = m_consumers.emplace_back(symbol, dtype, data_ptr, is_mandatory, BaseRef::construct_t{});
    auto it = m_consumer_map.try_emplace(symbol, r);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Consumer symbol already exists:" << symbol);
    return r;
}

CORE_LINKER_NS_END
