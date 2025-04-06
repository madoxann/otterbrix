#include "distributed_plan/consistent_hashing/hash_ring.hpp"

#include <catch2/catch.hpp>

//#define private public
#include "distributed_plan/distributed_planner.hpp"

using namespace components;

namespace {
    std::mt19937_64 rng(1093293723341ULL);
    std::vector<consistent_hashing::physical_node> single = {consistent_hashing::physical_node(1)};
    std::vector<consistent_hashing::physical_node> four = {
        consistent_hashing::physical_node(1),
        consistent_hashing::physical_node(2),
        consistent_hashing::physical_node(3),
        consistent_hashing::physical_node(4),
    };

    auto resource = std::pmr::synchronized_pool_resource();
    sql::transform::transformer transformer(&resource);

    auto graph = consistent_hashing::cluster_graph_builder().add_edge(1, 2).add_edge(1, 3).add_edge(3, 4).finish();
} // namespace

std::string query_to_node(const char* str) {
    components::logical_plan::parameter_node_t agg(&resource);
    auto lst = raw_parser(str);
    auto node = transformer.transform(sql::transform::pg_cell_to_node_cast(lst->lst.back().data), &agg);
    return node->to_string();
}

TEST_CASE("distributed_plan::simple_relay") {
    auto copy = single;
    distributed_plan::distributed_planner planner(std::move(copy), rng);

    static auto check_single = [&](const char* query) {
        auto plans = planner.create_plan(query);
        auto& plan = plans.first;
        REQUIRE(plan[1].size() == 1);
        REQUIRE(plan[1].back().data() == query_to_node(query));
    };

    SECTION("simple DATABASE & TABLE") {
        check_single("CREATE DATABASE db_name;");
        check_single("CREATE TABLE db_name.test();");
        check_single("DROP TABLE db_name.test;");
        check_single("DROP DATABASE db_name;");
    }

    check_single("CREATE DATABASE db_name;");
    check_single("CREATE TABLE db_name.test();");

    SECTION("index") {
        check_single("CREATE INDEX base ON db_name.test (count);");
        check_single("DROP INDEX db_name.test.base");
    }

    SECTION("aggregate & join") {
        check_single("CREATE TABLE db_name.test1();");
        check_single("SELECT * FROM db_name.test;");
        check_single(R"_(SELECT * FROM db_name.test WHERE ((number = 10 AND name = 'doc 10') OR "count" = 2) AND )_"
                     R"_(((number = 10 AND name = 'doc 10') OR "count" = 2) AND )_"
                     R"_(((number = 10 AND name = 'doc 10') OR "count" = 2);)_");
        check_single(
            R"_(SELECT * from db_name.test JOIN db_name.test1 ON db_name.test.id = db_name.test1.id_col1 AND )_"
            R"_(db_name.test.name = db_name.test1.name;)_");
    }

    SECTION("insert, delete, update") {
        check_single("INSERT INTO db_name.test (id, name, count) VALUES (1, 'Name', 1), (2, 'Name', 2);");
        check_single("DELETE FROM db_name.test WHERE NOT (count = 10) AND NOT(name = 'doc 10' OR count = 2)");
        check_single("UPDATE db_name.test SET count = 10, name = 'new name', is_doc = true;");
    }
}

/* node graph configuration for test
 *       2
 *     /
 *   1
 *     \
 *      3 -- 4
 */
TEST_CASE("distributed_plan::plan_distribution") {
    auto copy = four;
    auto copy_graph = graph;
    distributed_plan::distributed_planner planner(std::move(copy), rng, std::move(copy_graph));

    planner.create_plan("CREATE DATABASE db_name; CREATE TABLE db_name.test();");
    REQUIRE(planner.meta.try_get_collection_info("db_name", "test")->get().primary_node == 1);
    planner.meta.make_sharded("db_name", "test", "id", shard_distribution()); // empty distributed collection

    // insert
    meta::node_load_t plan_loads = {{1, 0}, {2, 0}, {3, 0}, {4, 0}};
    auto plans =
        planner.create_plan("INSERT INTO db_name.test (id) values (1), (2), (3), (4), (5), (6), (7), (8), (9), (10);");

    {
        auto& plan = plans.first;
        REQUIRE(plan.size() == 3); // same distribution as in test_distribution

        // temporary tests, no serialization :(
        int cnt = 0;
        for (auto& [_, nodes] : plan) {
            for (auto str : nodes) {
                std::string str_(str);
                cnt +=
                    *std::find_if(str_.begin(), str_.end(), [&](const auto& item) { return std::isdigit(item); }) - '0';
            }
        }
        REQUIRE(cnt == 10);
    }

    plans = planner.create_plan("SELECT * FROM db_name.test;");
    REQUIRE(plans.first.size() == 3);

    plans = planner.create_plan("DELETE FROM db_name.test;");
    REQUIRE(plans.first.size() == 3);

    plans = planner.create_plan("CREATE INDEX base ON db_name.test (id);");
    REQUIRE(plans.first.size() == 3);

    plans = planner.create_plan("DROP INDEX db_name.test.base");
    REQUIRE(plans.first.size() == 3);
}

/* node graph configuration for test
 *       2
 *     /
 *   1
 *     \
 *      3 -- 4
 */
TEST_CASE("distributed_plan::distributed_join") {
    auto copy = four;
    auto copy_graph = graph;
    distributed_plan::distributed_planner planner(std::move(copy), rng, std::move(copy_graph));

    planner.create_plan(
        "CREATE DATABASE db_name; CREATE TABLE db_name.test();CREATE DATABASE db_name1; CREATE TABLE db_name1.test();");
    REQUIRE(planner.meta.try_get_collection_info("db_name", "test")->get().primary_node == 1);
    REQUIRE(planner.meta.try_get_collection_info("db_name1", "test")->get().primary_node == 2);

    planner.meta.make_sharded("db_name", "test", "id", shard_distribution()); // distributed to nodes 1, 2, 3
    // insert values, as join will ignore unpopulated nodes
    planner.create_plan("INSERT INTO db_name.test (id) values (1), (2), (3), (4), (5), (6), (7), (8), (9), (10);");

    // distributed and not
    {
        auto plans =
            planner.create_plan("SELECT * from db_name.test JOIN db_name1.test ON db_name.test.id = db_name1.test.id");
        REQUIRE(plans.second.has_value());

        auto& distrib_plan = plans.first;
        auto& local_join = plans.second.value();

        REQUIRE(local_join.left.index() == 0); // just node with 2 children
        auto& left = std::get<0>(local_join.left);
        auto& right = local_join.right;
        REQUIRE(left.size() == 3);
        REQUIRE(right.size() == 1);

        // no serialization :( just check existence
        REQUIRE(distrib_plan[right[0].first][right[0].second].size());
        for (auto& left_agg : left) {
            REQUIRE(distrib_plan[left_agg.first][left_agg.second].size());
        }
    }

    // both distributed
    {
        planner.create_plan("CREATE TABLE db_name1.test1();");
        planner.meta.make_sharded("db_name1", "test1", "id", shard_distribution()); // distributed to nodes 1, 2
        REQUIRE(planner.meta.try_get_collection_info("db_name1", "test1")->get().primary_node == 2);
        planner.create_plan(
            "INSERT INTO db_name1.test1 (id) values (1), (2), (3), (4), (5), (6), (7), (8), (9), (10);");

        auto plans = planner.create_plan(
            "SELECT * from db_name.test JOIN db_name1.test1 ON db_name.test.id = db_name1.test1.id");

        auto& local_join = plans.second.value();
        REQUIRE(std::get<0>(local_join.left).size() == 3);
        REQUIRE(local_join.right.size() == 2);
    }

    // distributed + not distributed + not distributed
    {
        planner.create_plan("CREATE TABLE db_name1.test2();");
        auto plans = planner.create_plan("SELECT * from db_name1.test JOIN db_name1.test2 ON db_name1.test.id = "
                                         "db_name1.test2.id JOIN db_name.test ON db_name1.test2.id = db_name.test.id");

        auto& local_join = plans.second.value();
        // three tables in join, however, two nodes, as db_name1.test and db_name1.test2 are joined locally
        REQUIRE(std::get<0>(local_join.left).size() == 1);
        REQUIRE(local_join.right.size() == 3);
    }
}
