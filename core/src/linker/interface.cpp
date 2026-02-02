//
// Created by vibhanshu on 2024-02-08
//

#include "core/linker/interface.h"
#include "util/logger.h"

CORE_LINKER_NS_BEGIN

//
// Interface
//
void Interface::freeze() {
    CORE_ALWAYS_ASSERT(not is_frozen());

    m_frozen = true;
}

void Interface::add_producer(Producer& producer) {
    CORE_ALWAYS_ASSERT(not is_frozen());
    auto it = m_producer_map.try_emplace(producer.symbol(), producer);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Duplicate producer:" << producer.symbol());
    Producer& l_mem_producer = it.first->second;
    for (const Producer& l_producer: m_producers) {
        CORE_ALWAYS_ASSERT_NOT_EQ(l_producer, l_mem_producer);
    }
    m_producers.emplace_back(l_mem_producer);
}
void Interface::add_consumer(Consumer& consumer) {
    CORE_ALWAYS_ASSERT(not is_frozen());
    auto it = m_consumer_map.try_emplace(consumer.symbol(), consumer);
    CORE_ALWAYS_ASSERT_INFO(it.second, CORE_CONCAT << "Duplicate consumer:" << consumer.symbol());
    Consumer& l_mem_consumer = it.first->second;
    for (const Consumer& l_consumer: m_consumers) {
        CORE_ALWAYS_ASSERT_NOT_EQ(l_consumer, l_mem_consumer);
    }
    m_consumers.emplace_back(l_mem_consumer);
}

auto Interface::try_get_producer(const ExportSymbol& symbol) const -> Producer* {
    CORE_ALWAYS_ASSERT(is_frozen());
    if (m_producer_map.contains(symbol)) {
        return &(m_producer_map.at(symbol).get());
    }
    return nullptr;
}

auto Interface::try_get_consumer(const ExportSymbol& symbol) const -> Consumer* {
    CORE_ALWAYS_ASSERT(is_frozen());
    if (m_consumer_map.contains(symbol)) {
        return &(m_consumer_map.at(symbol).get());
    }
    return nullptr;
}

void Interface::print(std::ostream& os) const {
    os << "{Interface: name=[" << m_name << "], frozen=" << m_frozen << "\n"
       << "    Producers:[";
    for (const auto& kv : m_producer_map) {
        os << ", " << kv.first << std::endl;
    }
    os << "],\n"
       << "     Consumers:[";
    for (const auto& kv : m_consumer_map) {
        os << ", " << kv.first << std::endl;
    }
    os << "]\n";
}

//
// MergedInterface
//
void MergedInterface::set_local_interface(Interface& interface) {
    CORE_ALWAYS_ASSERT(m_local_interface == nullptr);
    m_local_interface = &interface;
}
void MergedInterface::set_foreign_interface(Interface& interface) {
    CORE_ALWAYS_ASSERT(m_foreign_interface == nullptr);
    m_foreign_interface = &interface;
}

void MergedInterface::bind() {
    CORE_ALWAYS_ASSERT(not is_frozen());
    CORE_ALWAYS_ASSERT_INFO(m_local_interface != nullptr, CORE_CONCAT << "Local interface not found in:[" << name() << "]");
    CORE_ALWAYS_ASSERT_INFO(m_foreign_interface != nullptr,
                          CORE_CONCAT << "Foregin interface not found in:[" << name() << "]");
    CORE_ALWAYS_ASSERT(m_local_interface->is_frozen());
    CORE_ALWAYS_ASSERT(m_foreign_interface->is_frozen());
    CORE_ALWAYS_ASSERT_INFO(name() == m_local_interface->name(),
                          CORE_CONCAT << "Local interface name not expected:" << name() << " : "
                                    << m_local_interface->name());
    CORE_ALWAYS_ASSERT_INFO(m_local_interface->name() == m_foreign_interface->name(),
                          CORE_CONCAT << "Interface name does not match:" << m_local_interface->name() << "-"
                                    << m_foreign_interface->name());
    CORE_INFO << "Local interface:" << *m_local_interface;
    CORE_INFO << "Foregin interface:" << *m_foreign_interface;

    // TODO{vibhanshu}: probably for some producers we can skip consumer check
    //                    if not available
    for (Producer& producer : m_local_interface->producer()) {
        const ExportSymbol& l_symbol = producer.symbol();
        auto it = m_symbols.try_emplace(l_symbol, producer);
        if (it.second) {
            Consumer* l_consumer = m_foreign_interface->try_get_consumer(l_symbol);
            CORE_ALWAYS_ASSERT_INFO(l_consumer != nullptr,
                                  CORE_CONCAT << "Consumer not found for Symbol:[" << l_symbol << "]");
            LinkedObject& linked_object = m_linked_objects.emplace_back();
            linked_object.set_producer(producer);
            linked_object.set_consumer(*l_consumer);
            linked_object.bind();
        } else {
            CORE_ABORT(CORE_CONCAT << "Duplicate entry for symbol in producers:" << l_symbol);
        }
    }
    for (Consumer& consumer : m_local_interface->consumer()) {
        const ExportSymbol& l_symbol = consumer.symbol();
        auto it = m_symbols.try_emplace(l_symbol, consumer);
        if (it.second) {
            Producer* l_producer = m_foreign_interface->try_get_producer(l_symbol);
            CORE_ALWAYS_ASSERT_INFO(l_producer != nullptr,
                                  CORE_CONCAT << "Producer not found for Symbol:[" << l_symbol << "]");
            LinkedObject& linked_object = m_linked_objects.emplace_back();
            linked_object.set_producer(*l_producer);
            linked_object.set_consumer(consumer);
            linked_object.bind();
        } else {
            CORE_ABORT(CORE_CONCAT << "Duplicate entry for symbol in consumers:" << l_symbol);
        }
    }
    for (Producer& producer : m_foreign_interface->producer()) {
        const ExportSymbol& l_symbol = producer.symbol();
        if (producer.is_mandatory()) {
            CORE_ALWAYS_ASSERT_INFO(m_symbols.contains(l_symbol), CORE_CONCAT << "Consumer not found for symbol:" << l_symbol);
            CORE_ALWAYS_ASSERT_INFO(m_symbols.at(l_symbol).get().is_consumer(),
                                CORE_CONCAT << "Mis-match for symbol:" << l_symbol);
        }
    }
    for (Consumer& consumer : m_foreign_interface->consumer()) {
        const ExportSymbol& l_symbol = consumer.symbol();
        if (consumer.is_mandatory()) {
            CORE_ALWAYS_ASSERT_INFO(m_symbols.contains(l_symbol), CORE_CONCAT << "Producer not found for symbol:" << l_symbol);
            CORE_ALWAYS_ASSERT_INFO(m_symbols.at(l_symbol).get().is_producer(),
                                CORE_CONCAT << "Mis-match for symbol:" << l_symbol);
        }
    }
    m_frozen = true;
}

CORE_LINKER_NS_END
