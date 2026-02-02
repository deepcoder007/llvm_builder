//
// Created by vibhanshu on 2024-02-08
//

#include "gtest/gtest.h"
#include "util/debug.h"
#include "core/util/dtype_util.h"
#include "core/linker/symbol.h"
#include "core/linker/object.h"
#include "core/linker/interface.h"

#include "util/logger.h"

using namespace core;
using namespace linker;

std::vector<int32_t> res_vec;
void cb_fn() {
    static int32_t v = 0;
    res_vec.emplace_back(v++);
}

TEST(INTERFACE_LINKER, fn_link) {

    ExportSymbol sym1{"NS1"_cs, "V1"_cs};
    ExportSymbol sym2{"NS1"_cs, "V2"_cs};
    ExportSymbol sym3{"NS1"_cs, "V1"_cs};
    CORE_ALWAYS_ASSERT(not(sym1 == sym2));
    CORE_ALWAYS_ASSERT(sym1 == sym3);
    ObjectUtil::singleton().clear();
    Producer& producer = Producer::mk_function(ExportSymbol{"N1"_cs, "V1"_cs});
    Consumer& consumer = Consumer::mk_function(ExportSymbol{"N1"_cs, "V1"_cs}, cb_fn);

    LinkedObject linked_object;
    linked_object.set_producer(producer);
    linked_object.set_consumer(consumer);

    CORE_ALWAYS_ASSERT(not producer.has_callback());
    linked_object.bind();
    CORE_ALWAYS_ASSERT(producer.has_callback());
    producer.callback()();
    CORE_ALWAYS_ASSERT(res_vec.size() == 1);
    CORE_ALWAYS_ASSERT(res_vec.back() == 0);
    producer.callback()();
    CORE_ALWAYS_ASSERT(res_vec.size() == 2);
    CORE_ALWAYS_ASSERT(res_vec.back() == 1);
    producer.callback()();
    CORE_ALWAYS_ASSERT(res_vec.size() == 3);
    CORE_ALWAYS_ASSERT(res_vec.back() == 2);
}

TEST(INTERFACE_LINKER, int_data_link) {

    ObjectUtil::singleton().clear();
    base_dtype_t dtype = DtypeUtil::singleton().from_name("int"_cs);
    int32_t l_val;

    Producer& producer = Producer::mk_data_ptr(ExportSymbol{"N1"_cs, "V1"_cs}, dtype, nullptr);
    Consumer& consumer = Consumer::mk_data_ptr(ExportSymbol{"N1"_cs, "V1"_cs}, dtype, &l_val);

    LinkedObject linked_object;
    linked_object.set_producer(producer);
    linked_object.set_consumer(consumer);

    CORE_ALWAYS_ASSERT(not producer.has_data_ptr());
    linked_object.bind();
    CORE_ALWAYS_ASSERT(producer.has_data_ptr());

    int32_t* l_ref = (int32_t*)producer.data_ptr();
    CORE_ALWAYS_ASSERT(*l_ref == l_val);
    *l_ref = 123;
    CORE_ALWAYS_ASSERT(*l_ref == l_val);
    *l_ref = 987;
    CORE_ALWAYS_ASSERT(*l_ref == l_val);
}

bool merge_cb_fn1_called = false;
bool merge_cb_fn2_called = false;

void merge_cb_fn1() {
    CORE_INFO << " Call back function called";
    merge_cb_fn1_called = true;
}

void merge_cb_fn2() {
    CORE_INFO << " Call back function called version 2";
    merge_cb_fn2_called = true;
}

struct CBObject {
   uint32_t m_num_cb_called = 0;
};

void object_cb_f1(void* obj) {
    ((CBObject*)obj)->m_num_cb_called++;
}

TEST(INTERFACE_LINKER, merged_interface) {

    ObjectUtil::singleton().clear();
    [[maybe_unused]] int vi1 = 0;
    [[maybe_unused]] int vi2 = 0;
    [[maybe_unused]] float vf1 = 0.0;
    base_dtype_t dtype_int = DtypeUtil::singleton().from_name("int"_cs);
    base_dtype_t dtype_float = DtypeUtil::singleton().from_name("float"_cs);

    Producer& producer_fn1 = Producer::mk_function(ExportSymbol{"N1"_cs, "F1"_cs});
    Producer& producer_fn2 = Producer::mk_function(ExportSymbol{"N1"_cs, "F2"_cs});
    Producer& producer_ob_fn1 = Producer::mk_object_function(ExportSymbol{"N1"_cs, "OF1"_cs});
    Producer& producer_data1 = Producer::mk_data_ptr(ExportSymbol{"N2"_cs, "D1"_cs}, dtype_int, &vi1);
    Producer& producer_data2 = Producer::mk_data_ptr(ExportSymbol{"N2"_cs, "D2"_cs}, dtype_float, &vf1);
    Producer& producer_data3 = Producer::mk_data_ptr(ExportSymbol{"N2"_cs, "D3"_cs}, dtype_int, nullptr);
    Consumer& consumer_fn1 = Consumer::mk_function(ExportSymbol{"N1"_cs, "F1"_cs}, merge_cb_fn1);
    Consumer& consumer_fn2 = Consumer::mk_function(ExportSymbol{"N1"_cs, "F2"_cs}, merge_cb_fn2);
    Consumer& consumer_ob_fn1 = Consumer::mk_object_function(ExportSymbol{"N1"_cs, "OF1"_cs}, object_cb_f1);
    Consumer& consumer_data1 = Consumer::mk_data_ptr(ExportSymbol{"N2"_cs, "D1"_cs}, dtype_int, nullptr);
    Consumer& consumer_data2 = Consumer::mk_data_ptr(ExportSymbol{"N2"_cs, "D2"_cs}, dtype_float, nullptr);
    Consumer& consumer_data3 = Consumer::mk_data_ptr(ExportSymbol{"N2"_cs, "D3"_cs}, dtype_int, &vi2);

    Interface foreign_iface{"TEST_iface"};
    Interface local_iface{"TEST_iface"};

    local_iface.add_producer(producer_fn1);
    local_iface.add_producer(producer_fn2);
    local_iface.add_producer(producer_ob_fn1);
    local_iface.add_producer(producer_data1);
    local_iface.add_producer(producer_data2);
    local_iface.add_consumer(consumer_data3);

    foreign_iface.add_consumer(consumer_fn1);
    foreign_iface.add_consumer(consumer_fn2);
    foreign_iface.add_consumer(consumer_ob_fn1);
    foreign_iface.add_consumer(consumer_data1);
    foreign_iface.add_consumer(consumer_data2);
    foreign_iface.add_producer(producer_data3);

    local_iface.freeze();
    foreign_iface.freeze();

    MergedInterface merged_interface{"TEST_iface"};
    merged_interface.set_local_interface(local_iface);
    merged_interface.set_foreign_interface(foreign_iface);

    CORE_ALWAYS_ASSERT(not producer_fn1.has_callback());
    CORE_ALWAYS_ASSERT(not producer_fn2.has_callback());
    CORE_ALWAYS_ASSERT(not consumer_data1.has_data_ptr());
    CORE_ALWAYS_ASSERT(not consumer_data2.has_data_ptr());
    CORE_ALWAYS_ASSERT(not producer_data3.has_data_ptr());
    merged_interface.bind();
    CORE_ALWAYS_ASSERT(producer_fn1.has_callback());
    CORE_ALWAYS_ASSERT(producer_fn2.has_callback());
    CORE_ALWAYS_ASSERT(consumer_data1.has_data_ptr());
    CORE_ALWAYS_ASSERT(consumer_data2.has_data_ptr());
    CORE_ALWAYS_ASSERT(producer_data3.has_data_ptr());

    CORE_ALWAYS_ASSERT(vi1 == 0);
    CORE_ALWAYS_ASSERT(vi2 == 0);
    CORE_ALWAYS_ASSERT(vf1 == 0.0);

    *(int*)consumer_data1.data_ptr() = 123;
    *(float*)consumer_data2.data_ptr() = 456;
    *(int*)producer_data3.data_ptr() = 789;

    CORE_ALWAYS_ASSERT(vi1 == 123);
    CORE_ALWAYS_ASSERT(vi2 == 789);
    CORE_ALWAYS_ASSERT(vf1 == 456.0f);

    merge_cb_fn1_called = false;
    merge_cb_fn2_called = false;
    producer_fn1.callback()();
    producer_fn2.callback()();
    CORE_ALWAYS_ASSERT(merge_cb_fn1_called);
    CORE_ALWAYS_ASSERT(merge_cb_fn2_called);

    CBObject l_obj1{}, l_obj2{};
    CORE_ALWAYS_ASSERT_EQ(l_obj1.m_num_cb_called, 0);
    CORE_ALWAYS_ASSERT_EQ(l_obj2.m_num_cb_called, 0);
    producer_ob_fn1.object_callback()(&l_obj1);
    producer_ob_fn1.object_callback()(&l_obj1);
    CORE_ALWAYS_ASSERT_EQ(l_obj1.m_num_cb_called, 2);
    CORE_ALWAYS_ASSERT_EQ(l_obj2.m_num_cb_called, 0);
    producer_ob_fn1.object_callback()(&l_obj2);
    producer_ob_fn1.object_callback()(&l_obj1);
    CORE_ALWAYS_ASSERT_EQ(l_obj1.m_num_cb_called, 3);
    CORE_ALWAYS_ASSERT_EQ(l_obj2.m_num_cb_called, 1);
}
