#include <array>
#include <atomic>
#include <type_traits>
#include <memory>
#include <thread>
#include <vector>
#include <unordered_set>
#include "gtest.h"
#include "observable/detail/collection.hpp"

namespace observable { namespace detail { namespace test {

TEST(collection_test, can_default_construct)
{
    collection<int> col;
}

TEST(collection_test, default_constructed_collection_is_empty)
{
    collection<int> col;
    ASSERT_TRUE(col.empty());
}

TEST(collection_test, can_insert)
{
    collection<int> col;
    col.insert(5);
}

TEST(collection_test, can_apply_to_elements)
{
    collection<int> col;
    auto call_count = 0;

    col.insert(5);
    col.insert(6);
    col.apply([&](auto) { ++call_count; });

    ASSERT_EQ(call_count, 2);
}

TEST(collection_test, apply_does_nothing_for_empty_collection)
{
    collection<int> col;
    auto call_count = 0;

    col.apply([&](auto) { ++call_count; });

    ASSERT_EQ(call_count, 0);
}

TEST(collection_test, can_remove)
{
    collection<int> col;

    auto call_count = 0;
    auto id = col.insert(5);

    auto success = col.remove(id);

    ASSERT_TRUE(success);
    ASSERT_TRUE(col.empty());

    col.apply([&](auto) { ++call_count; });

    ASSERT_EQ(call_count, 0);
}

TEST(collection_test, elements_are_passed_to_the_apply_functor)
{
    collection<int> col;
    auto result = 0;

    col.insert(11);
    col.insert(7);
    col.apply([&](auto v) { result += v; });

    ASSERT_EQ(result, 11 + 7);
}

TEST(collection_test, removed_element_during_apply_is_not_applied)
{
    collection<unsigned int> col;

    std::array<collection<unsigned int>::id, 3> ids;
    for(auto i = 0u; i < ids.size(); ++i)
        ids[i] = col.insert(i);

    auto call_count = 0;
    col.apply([&](auto j) {
        for(auto i = 0u; i < ids.size(); ++i)
            if(i != j)
                col.remove(ids[i]);
        ++call_count;
    });

    ASSERT_EQ(call_count, 1);
}

TEST(collection_test, can_remove_applied_element)
{
    collection<unsigned int> col;

    std::array<collection<unsigned int>::id, 3> ids;
    for(auto i = 0u; i < ids.size(); ++i)
        ids[i] = col.insert(i);

    col.apply([&](auto i) {
        col.remove(ids[i]);
    });

    ASSERT_TRUE(col.empty());
}

TEST(collection_test, can_insert_while_applying)
{
    collection<int> col;

    col.insert(3);

    auto insert_count = 0;
    col.apply([&](auto) {
        if(insert_count == 0)
            col.insert(7);
        ++insert_count;
    });

    auto sum = 0;
    col.apply([&](auto v) { sum += v; });

    ASSERT_EQ(sum, 3 + 7);
}

TEST(collection_test, can_insert_elements_in_parallel)
{
    // This test seems evil, but it fails with a decent probability.
    collection<int> col;

    std::vector<std::thread> ts;
    auto ref_sum = 0;
    std::atomic<bool> wait { true };

    for(auto i = 1; i <= 8; ++i)
    {
        ref_sum += i;
        ts.emplace_back([&, i]() {
            while(wait)
                ;
            col.insert(i);
        });
    }

    wait = false;
    for(auto && t : ts)
        t.join();

    std::unordered_set<int> ref_els { 1, 2, 3, 4, 5, 6, 7, 8 };
    std::unordered_set<int> els;
    col.apply([&](auto i) { els.insert(i); });

    ASSERT_EQ(ref_els, els);
}

TEST(collection_test, can_remove_elements_in_parallel)
{
    // This test seems evil, but it fails with a decent probability.
    collection<unsigned int> col;

    std::vector<std::thread> ts;
    std::array<std::atomic<collection<unsigned int>::id>, 8> ids;
    std::atomic<bool> wait { true };

    for(auto i = 0u; i < ids.size(); ++i)
    {
        ids[i] = col.insert(i);
        ts.emplace_back([&, i]() {
            while(wait)
                ;
            col.remove(ids[i]);
        });
    }

    wait = false;
    for(auto && t : ts)
        t.join();

    ASSERT_TRUE(col.empty());
}

TEST(collection_test, can_insert_and_remove_in_parallel)
{
    // This test seems evil, but it fails with a decent probability.
    collection<unsigned int> col;

    std::vector<std::thread> ts;
    std::array<
        std::array<std::atomic<collection<unsigned int>::id>, 8>,
        4> ids;
    std::atomic<bool> wait { true };

    for(auto j = 0u; j < 4u; ++j)
    {
        ts.emplace_back([&, j]() {
            while(wait)
                ;
            for(auto i = 0u; i < 8u; ++i)
                ids[j][i] = col.insert(i);

            for(auto i = 0u; i < 8u; ++i)
                col.remove(ids[j][i]);
        });
    }

    wait = false;
    for(auto && t : ts)
        t.join();

    ASSERT_TRUE(col.empty());
}

TEST(collection_test, can_remove_same_node_in_parallel)
{
    collection<unsigned int> col;

    std::array<std::atomic<collection<unsigned int>::id>, 3> ids;
    for(auto i = 0u; i < ids.size(); ++i)
        ids[i] = col.insert(i);

    std::atomic<bool> wait { true };
    std::vector<std::thread> ts;
    for(auto j = 0; j < 8; ++j)
        ts.emplace_back([&]() {
            while(wait)
                ;
            for(auto i = 0u; i < ids.size(); ++i)
                col.remove(ids[i]);
        });

    wait = false;
    for(auto && t : ts)
        t.join();

    ASSERT_TRUE(col.empty());
}

} } }
