#include "distributed_plan/catalog/analyze.hpp"
#include "distributed_plan/consistent_hashing/hash_ring.hpp"
#include "distributed_plan/consistent_hashing/murmur3.hpp"

#include <catch2/catch.hpp>

#define private public
#include "distributed_plan/distributed_planner.hpp"

namespace {
    std::mt19937_64 rng(1093293723341ULL);
    std::vector<consistent_hashing::physical_node> three = {
        consistent_hashing::physical_node(1),
        consistent_hashing::physical_node(2),
        consistent_hashing::physical_node(3),
    };
} // namespace

/* For seed 1093293723341ULL
 * node_id: 1 token: 5770386015840707575
 * node_id: 1 token: 6422660004058998051
 * node_id: 1 token: 7030320816992882670
 * node_id: 1 token: 7356521015032985418
 * node_id: 1 token: 7792326736390277133
 * node_id: 1 token: 12064007693831419901
 * node_id: 1 token: 16138848840512006224
 * node_id: 1 token: 16966380823601387770
 * node_id: 2 token: 3903822373903432379
 * node_id: 2 token: 4509841717462846954
 * node_id: 2 token: 4984256627559806196
 * node_id: 2 token: 11686985704135817893
 * node_id: 2 token: 13748782938838777012
 * node_id: 2 token: 14436308164755715793
 * node_id: 2 token: 15872075756726496906
 * node_id: 2 token: 18369341617200210043
 * node_id: 3 token: 863855232017240397
 * node_id: 3 token: 5087907520992001626
 * node_id: 3 token: 5361418089789069148
 * node_id: 3 token: 10019579519824023916
 * node_id: 3 token: 12963329397572884451
 * node_id: 3 token: 15417010361367667962
 * node_id: 3 token: 15506056353808981266
 * node_id: 3 token: 17111219714179906144
 * */

TEST_CASE("distributed_plan::record_distribution") {
    auto copy = three;
    distributed_plan::distributed_planner planner(std::move(copy), rng);
    auto& stats = planner.meta;
    planner.create_plan("CREATE DATABASE db_name; CREATE TABLE db_name.test();");
    stats.make_sharded("db_name", "test", "id", {});

    SECTION("Hash ring node selection") {
        // it's a RING after all...
        REQUIRE(stats.hash_ring.get_node_by_hash(0) == 3);
        REQUIRE(stats.hash_ring.get_node_by_hash(std::numeric_limits<uint64_t>::max()) == 3);

        REQUIRE(stats.hash_ring.get_node_by_hash(3903822373903432379) == 2);
        REQUIRE(stats.hash_ring.get_node_by_hash(7030320816992882670) == 1);
    }

    SECTION("Loads are calculated correctly") {
        node_load_t expected_loads = {{1, 0}, {2, 0}, {3, 0}};

        for (int64_t val = 1; val < 11; val++) {
            const uint64_t m = 0xc6a4a7935bd1e995ULL;
            uint64_t h = 0x8445d61a4e774912ULL ^ (sizeof(int64_t) * m);
            expected_loads[stats.hash_ring.get_node_by_hash(
                consistent_hashing::murmurhash3_x64_128(&val, sizeof(int64_t), h))]++;
        }

        planner.create_plan("INSERT INTO db_name.test (id) values (1), (2), (3), (4), (5), (6), (7), (8), (9), (10);");
        for (auto node : {1, 2, 3}) {
            REQUIRE(expected_loads[node] == stats.node_record_loads[node]);
        }
    }

    SECTION("Stats make sense") {
        planner.create_plan("INSERT INTO db_name.test (id) values (1), (2), (3), (4), (5), (6), (7), (8), (9), (10);");
        planner.create_plan("INSERT INTO db_name.test (id) values (11), (12), (13);");

        REQUIRE(stats.node_record_loads[1] > 0);
        REQUIRE(stats.node_record_loads[2] > 0);
        REQUIRE(stats.node_record_loads[3] > 0);

        REQUIRE(stats.node_record_loads[1] + stats.node_record_loads[2] + stats.node_record_loads[3] == 13);
        REQUIRE(stats.try_get_collection_info("db_name", "test")->get().records == 13);
    }
}
