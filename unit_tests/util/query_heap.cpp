#include "util/query_heap.hpp"
#include "util/typedefs.hpp"

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

BOOST_AUTO_TEST_SUITE(binary_heap)

using namespace osrm;
using namespace osrm::util;

struct TestData
{
    unsigned value;
};

using TestNodeID = NodeID;
using TestKey = int;
using TestWeight = int;
using storage_types =
    boost::mpl::list<ArrayStorage<TestNodeID, TestKey>, UnorderedMapStorage<TestNodeID, TestKey>>;

template <unsigned NUM_ELEM> struct RandomDataFixture
{
    RandomDataFixture()
    {
        for (unsigned i = 0; i < NUM_ELEM; i++)
        {
            data.push_back(TestData{i * 3});
            weights.push_back((i + 1) * 100);
            ids.push_back(i);
            order.push_back(i);
        }

        // Choosen by a fair W20 dice roll
        std::mt19937 g(15);

        std::shuffle(order.begin(), order.end(), g);
    }

    std::vector<TestData> data;
    std::vector<TestWeight> weights;
    std::vector<TestNodeID> ids;
    std::vector<unsigned> order;
};

constexpr unsigned NUM_NODES = 100;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(insert_test, T, storage_types, RandomDataFixture<NUM_NODES>)
{
    QueryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(NUM_NODES);

    TestWeight min_weight = std::numeric_limits<TestWeight>::max();
    TestNodeID min_id;

    for (unsigned idx : order)
    {
        BOOST_CHECK(!heap.WasInserted(ids[idx]));

        heap.Insert(ids[idx], weights[idx], data[idx]);

        BOOST_CHECK(heap.WasInserted(ids[idx]));

        if (weights[idx] < min_weight)
        {
            min_weight = weights[idx];
            min_id = ids[idx];
        }
        BOOST_CHECK_EQUAL(min_id, heap.Min());
    }

    for (auto id : ids)
    {
        const auto &d = heap.GetData(id);
        BOOST_CHECK_EQUAL(d.value, data[id].value);

        const auto &w = heap.GetKey(id);
        BOOST_CHECK_EQUAL(w, weights[id]);
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(delete_min_test, T, storage_types, RandomDataFixture<NUM_NODES>)
{
    QueryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(NUM_NODES);

    for (unsigned idx : order)
    {
        heap.Insert(ids[idx], weights[idx], data[idx]);
    }

    for (auto id : ids)
    {
        BOOST_CHECK(!heap.WasRemoved(id));

        BOOST_CHECK_EQUAL(heap.Min(), id);
        BOOST_CHECK_EQUAL(id, heap.DeleteMin());
        if (id + 1 < NUM_NODES)
            BOOST_CHECK_EQUAL(heap.Min(), id + 1);

        BOOST_CHECK(heap.WasRemoved(id));
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(delete_all_test, T, storage_types, RandomDataFixture<NUM_NODES>)
{
    QueryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(NUM_NODES);

    for (unsigned idx : order)
    {
        heap.Insert(ids[idx], weights[idx], data[idx]);
    }

    heap.DeleteAll();

    BOOST_CHECK(heap.Empty());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(smoke_test, T, storage_types, RandomDataFixture<NUM_NODES>)
{
    QueryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(NUM_NODES);

    for (unsigned idx : order)
    {
        heap.Insert(ids[idx], weights[idx], data[idx]);
    }

    while (!heap.Empty())
    {
        auto old_weight = heap.MinKey();
        auto node = heap.GetHeapNodeIfWasInserted(heap.Min());
        BOOST_CHECK(old_weight == node->weight);
        BOOST_CHECK(node);
        node->weight = node->weight - 1;
        heap.DecreaseKey(*node);
        BOOST_CHECK(heap.MinKey() == node->weight);
        heap.DeleteMin();
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(decrease_key_test, T, storage_types, RandomDataFixture<10>)
{
    QueryHeap<TestNodeID, TestKey, TestWeight, TestData, T> heap(10);

    for (unsigned idx : order)
    {
        heap.Insert(ids[idx], weights[idx], data[idx]);
    }

    std::vector<TestNodeID> rids(ids);
    std::reverse(rids.begin(), rids.end());

    for (auto id : rids)
    {
        TestNodeID min_id = heap.Min();
        TestWeight min_weight = heap.GetKey(min_id);

        // decrease weight until we reach min weight
        while (weights[id] > min_weight)
        {
            heap.DecreaseKey(id, weights[id]);
            BOOST_CHECK_EQUAL(heap.Min(), min_id);
            BOOST_CHECK_EQUAL(heap.MinKey(), min_weight);
            weights[id]--;
        }

        // make weight smaller than min
        weights[id] -= 2;
        heap.DecreaseKey(id, weights[id]);
        BOOST_CHECK_EQUAL(heap.Min(), id);
        BOOST_CHECK_EQUAL(heap.MinKey(), weights[id]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
