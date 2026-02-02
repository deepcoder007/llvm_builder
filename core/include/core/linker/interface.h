//
// Created by vibhanshu on 2024-02-08
//

#ifndef CORE_LINKER_INTERFACE_H_
#define CORE_LINKER_INTERFACE_H_

#include "defines.h"
#include "symbol.h"
#include "object.h"

CORE_LINKER_NS_BEGIN

class Interface : meta::noncopyable {

    using producer_vec_t = std::vector<std::reference_wrapper<Producer>>;
    using consumer_vec_t = std::vector<std::reference_wrapper<Consumer>>;

  private:
    const std::string m_name;
    std::unordered_map<ExportSymbol, std::reference_wrapper<Producer>> m_producer_map;
    std::unordered_map<ExportSymbol, std::reference_wrapper<Consumer>> m_consumer_map;
    producer_vec_t m_producers;
    consumer_vec_t m_consumers;
    bool m_frozen = false;

  public:
    explicit Interface(const std::string& name) : m_name{name}, m_producer_map{}, m_consumer_map{} {
    }
    ~Interface() = default;

  public:
    bool is_frozen() const {
        return m_frozen;
    }
    void freeze();
    const std::string& name() const {
        return m_name;
    }
    void add_producer(Producer& producer);
    void add_consumer(Consumer& consumer);
    const producer_vec_t& producer() const {
        return m_producers;
    }
    const consumer_vec_t& consumer() const {
        return m_consumers;
    }
    Producer* try_get_producer(const ExportSymbol& symbol) const;
    Consumer* try_get_consumer(const ExportSymbol& symbol) const;
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(Interface)
};

class MergedInterface {
    const std::string m_name;
    Interface* m_local_interface = nullptr;
    Interface* m_foreign_interface = nullptr;
    std::vector<LinkedObject> m_linked_objects;
    std::unordered_map<ExportSymbol, std::reference_wrapper<BaseRef>> m_symbols;
    bool m_frozen = false;

  public:
    explicit MergedInterface(const std::string& name) : m_name{name} {
    }
    ~MergedInterface() = default;

  public:
    bool is_frozen() const {
        return m_frozen;
    }
    const std::string& name() const {
        return m_name;
    }
    void set_local_interface(Interface& interface);
    void set_foreign_interface(Interface& interface);
    void bind();
};

CORE_LINKER_NS_END

#endif // CORE_LINKER_INTERFACE_H_
