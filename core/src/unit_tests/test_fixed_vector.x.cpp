//
// Created by vibhanshu on 2024-06-07
//

#include "gtest/gtest.h"
#include "util/debug.h"
#include "core/ds/fixed_vector.h"

using namespace core;

TEST(FIXED_VECTOR_LIB, fixed_vector) {

    fixed_vector<uint32_t> v{10};
    CORE_ALWAYS_ASSERT(v.size() == 0);
    CORE_ALWAYS_ASSERT(v.capacity() == 10);
    CORE_ALWAYS_ASSERT(v.empty());
    CORE_ALWAYS_ASSERT(v.begin() == v.end());

    v.emplace_back(10u);
    v.emplace_back(30u);
    v.emplace_back(50u);
    CORE_ALWAYS_ASSERT(v.size() == 3);
    CORE_ALWAYS_ASSERT(v.remaining_size() == 7);
    CORE_ALWAYS_ASSERT(not v.empty());
    CORE_ALWAYS_ASSERT(v.begin() != v.end());

    CORE_ALWAYS_ASSERT(v.front() == 10);
    CORE_ALWAYS_ASSERT(v.back() == 50);

    CORE_ALWAYS_ASSERT(v[1] == 30);
    CORE_ALWAYS_ASSERT(v.at(2) == 50);
}

TEST(FIXED_VECTOR_LIB, tiny_fixed_v) {
    tiny_fixed_vector<uint32_t, 5u> v;
    v.emplace_back(1u);
    v.emplace_back(2u);
    CORE_ALWAYS_ASSERT(v.size() == 2u);
    CORE_ALWAYS_ASSERT(v[0] == 1);
    CORE_ALWAYS_ASSERT(v[1] == 2);
    v.emplace_front(3u);
    v.emplace_back(4u);
    v.emplace_front(5u);
    CORE_ALWAYS_ASSERT(v.size() == 5u);
    CORE_ALWAYS_ASSERT(v.full());
    CORE_ALWAYS_ASSERT(v[0] == 5);
    CORE_ALWAYS_ASSERT(v[1] == 3);
    CORE_ALWAYS_ASSERT(v[2] == 1);
    CORE_ALWAYS_ASSERT(v[3] == 2);
    CORE_ALWAYS_ASSERT(v[4] == 4);

    v.pop_back();
    v.pop_front();
    CORE_ALWAYS_ASSERT(v[0] == 3);
    CORE_ALWAYS_ASSERT(v[1] == 1);
    CORE_ALWAYS_ASSERT(v[2] == 2);

    v.erase(1);
    CORE_ALWAYS_ASSERT(v[0] == 3);
    CORE_ALWAYS_ASSERT(v[1] == 2);
    v.insert(1, 100);
    v.insert(2, 200);
    CORE_ALWAYS_ASSERT(v[0] == 3);
    CORE_ALWAYS_ASSERT(v[1] == 100);
    CORE_ALWAYS_ASSERT(v[2] == 200);
    CORE_ALWAYS_ASSERT(v[3] == 2);
}
