//
// Created by vibhanshu on 2024-08-10
//

#include "core/util/callbacks.h"
#include "core/ds/fixed_string.h"
#include "util/debug.h"

#include "gtest/gtest.h"
#include <cstdint>

using namespace core;

TEST(CORE_CALLBACKS, basic_test) {
    SimpleCallbackManager cb_mgr;
    int32_t state1 = 0;
    int32_t state2 = 0;
    cb_mgr.emplace_callback(fixed_string{"state_1_add"}, [&state1] {
        ++state1;
    });
    cb_mgr.emplace_callback(fixed_string{"state_2_add_2"}, [&state2] {
        state2 += 2;
    });
    CORE_ALWAYS_ASSERT(state1 == 0);
    CORE_ALWAYS_ASSERT(state2 == 0);
    cb_mgr();
    CORE_ALWAYS_ASSERT(state1 == 1);
    CORE_ALWAYS_ASSERT(state2 == 2);
    cb_mgr.mark_inactive(fixed_string{"state_1_add"});
    cb_mgr();
    CORE_ALWAYS_ASSERT(state1 == 1);
    CORE_ALWAYS_ASSERT(state2 == 4);
    cb_mgr.mark_inactive(fixed_string{"state_2_add_2"});
    cb_mgr();
    cb_mgr();
    CORE_ALWAYS_ASSERT(state1 == 1);
    CORE_ALWAYS_ASSERT(state2 == 4);
    cb_mgr.mark_active(fixed_string{"state_1_add"});
    cb_mgr();
    cb_mgr();
    CORE_ALWAYS_ASSERT(state1 == 3);
    CORE_ALWAYS_ASSERT(state2 == 4);
}


TEST(CORE_CALLBACKS, state_full_test) {
    StateCallbackManager<int32_t> cb_mgr;
    int32_t state1 = 0;
    int32_t state2 = 0;
    std::vector<int32_t> r1;
    std::vector<int32_t> r2;
    cb_mgr.emplace_callback(fixed_string{"state_1_add"}, [&state1, &r1] (int32_t v){
        ++state1;
        r1.emplace_back(v);
    });
    cb_mgr.emplace_callback(fixed_string{"state_2_add_2"}, [&state2, &r2] (int32_t v) {
        state2 += 2;
        r2.emplace_back(v);
    });
    CORE_ALWAYS_ASSERT(state1 == 0);
    CORE_ALWAYS_ASSERT(state2 == 0);
    cb_mgr(10);
    CORE_ALWAYS_ASSERT(state1 == 1);
    CORE_ALWAYS_ASSERT(state2 == 2);
    cb_mgr.mark_inactive(fixed_string{"state_1_add"});
    cb_mgr(100);
    CORE_ALWAYS_ASSERT(state1 == 1);
    CORE_ALWAYS_ASSERT(state2 == 4);
    cb_mgr.mark_inactive(fixed_string{"state_2_add_2"});
    cb_mgr(1000);
    cb_mgr(5000);
    CORE_ALWAYS_ASSERT(state1 == 1);
    CORE_ALWAYS_ASSERT(state2 == 4);
    cb_mgr.mark_active(fixed_string{"state_1_add"});
    cb_mgr(10000);
    cb_mgr(50000);
    CORE_ALWAYS_ASSERT(state1 == 3);
    CORE_ALWAYS_ASSERT(state2 == 4);

    CORE_ALWAYS_ASSERT(r1.size() == 3u);
    CORE_ALWAYS_ASSERT(r2.size() == 2u);
    CORE_ALWAYS_ASSERT(r1[0] == 10);
    CORE_ALWAYS_ASSERT(r1[1] == 10000);
    CORE_ALWAYS_ASSERT(r1[2] == 50000);
    CORE_ALWAYS_ASSERT(r2[0] == 10);
    CORE_ALWAYS_ASSERT(r2[1] == 100);
}
