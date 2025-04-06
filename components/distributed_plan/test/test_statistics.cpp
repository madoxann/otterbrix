#include "distributed_plan/consistent_hashing/hash_ring.hpp"

#include <catch2/catch.hpp>

//#define private public
#include "distributed_plan/distributed_planner.hpp"

namespace {
    std::mt19937_64 rng(1093293723341ULL);
    std::vector<consistent_hashing::physical_node> single = {consistent_hashing::physical_node(1)};
    std::vector<consistent_hashing::physical_node> three = {
        consistent_hashing::physical_node(1),
        consistent_hashing::physical_node(2),
        consistent_hashing::physical_node(3),
    };
} // namespace

TEST_CASE("distributed_plan::db_count") {
    auto copy = single;
    distributed_plan::distributed_planner planner(std::move(copy), rng);
    auto& stats = planner.meta;
    planner.create_plan("CREATE DATABASE db_name;");
    REQUIRE(stats.databases.size() == 1);

    planner.create_plan("CREATE DATABASE db_name1; CREATE DATABASE db_name2; CREATE DATABASE db_name3;");
    REQUIRE(stats.databases.size() == 4);
    REQUIRE(stats.node_db_loads[1] == 4);

    planner.create_plan("DROP DATABASE db_name;");
    REQUIRE(stats.databases.size() == 3);

    planner.create_plan("DROP DATABASE db_name1; DROP DATABASE db_name2; DROP DATABASE db_name3;");
    REQUIRE(stats.databases.size() == 0);
    REQUIRE(stats.node_db_loads[1] == 0);
}

TEST_CASE("distributed_plan::collection_stats") {
    auto copy = single;
    distributed_plan::distributed_planner planner(std::move(copy), rng);
    auto& stats = planner.meta;
    planner.create_plan("CREATE DATABASE db_name;");
    REQUIRE(stats.databases.size() == 1);

    auto& db_name = stats.try_get_db_info("db_name")->get();
    planner.create_plan("CREATE TABLE db_name.test();");
    REQUIRE(db_name.collections.size() == 1);

    planner.create_plan("CREATE TABLE db_name.test1(); CREATE TABLE db_name.test2(); CREATE TABLE db_name.test3();");
    REQUIRE(db_name.collections.size() == 4);

    planner.create_plan("DROP TABLE db_name.test1; DROP TABLE db_name.test2; DROP TABLE db_name.test3;");
    REQUIRE(db_name.collections.size() == 1);

    planner.create_plan("CREATE INDEX base ON db_name.test (count);");
    REQUIRE(db_name.collections.at("test").indexes.size() == 1);
    REQUIRE(db_name.collections.at("test").indexes.at("base").back().as_string() == "count");

    planner.create_plan("DROP INDEX db_name.test.base;");
    REQUIRE(db_name.collections.at("test").indexes.size() == 0);
}

TEST_CASE("distributed_plan::record_count") {
    auto copy = three;
    distributed_plan::distributed_planner planner(std::move(copy), rng);
    auto& stats = planner.meta;
    planner.create_plan("CREATE DATABASE db_name; CREATE TABLE db_name.test();");
    planner.create_plan("INSERT INTO db_name.test (id) values (1), (2), (3);");

    auto& db_name_test = stats.try_get_collection_info("db_name", "test")->get();
    REQUIRE(db_name_test.records == 3);
    REQUIRE(stats.node_record_loads[1] == 3);

    planner.create_plan("CREATE DATABASE db_name1; CREATE TABLE db_name1.test();");
    planner.create_plan("INSERT INTO db_name1.test (id) values (1), (2), (3);");
    planner.create_plan("INSERT INTO db_name.test (id) values (1), (2), (3);");

    // least loaded node must be chosen
    REQUIRE(stats.try_get_db_info("db_name1")->get().primary_node == 2);

    REQUIRE(db_name_test.records == 6);
    REQUIRE(stats.node_record_loads[1] == 6);
    REQUIRE(stats.node_record_loads[2] == 3);

    planner.create_plan("CREATE DATABASE db_name2;");
    // least loaded node must be chosen
    REQUIRE(stats.try_get_db_info("db_name2")->get().primary_node == 3);

    db_name_test.erase_documents_unsharded(6);
    REQUIRE(db_name_test.records == 0);
    REQUIRE(stats.node_record_loads[1] == 0);

    planner.create_plan("CREATE DATABASE db_name3; CREATE TABLE db_name3.test();");
    // node 1 is least loaded again
    planner.create_plan("INSERT INTO db_name3.test (id) values (1), (2), (3);");
    REQUIRE(stats.try_get_collection_info("db_name3", "test")->get().records == 3);
    REQUIRE(stats.node_record_loads[1] == 3);
}

TEST_CASE("distributed_plan::db_round_robin") {
    auto copy = three;
    distributed_plan::distributed_planner planner(std::move(copy), rng);
    auto& stats = planner.meta;
    planner.create_plan("CREATE DATABASE db_name1; CREATE DATABASE db_name2; CREATE DATABASE db_name3;");
    REQUIRE(stats.node_db_loads[1] == 1);
    REQUIRE(stats.node_db_loads[2] == 1);
    REQUIRE(stats.node_db_loads[3] == 1);

    REQUIRE(stats.try_get_db_info("db_name1")->get().primary_node == 1);
    REQUIRE(stats.try_get_db_info("db_name2")->get().primary_node == 2);
    REQUIRE(stats.try_get_db_info("db_name3")->get().primary_node == 3);

    planner.create_plan("CREATE DATABASE db_name4; CREATE DATABASE db_name5; CREATE DATABASE db_name6;");
    REQUIRE(stats.node_db_loads[1] == 2);
    REQUIRE(stats.node_db_loads[2] == 2);
    REQUIRE(stats.node_db_loads[3] == 2);

    REQUIRE(stats.try_get_db_info("db_name4")->get().primary_node == 1);
    REQUIRE(stats.try_get_db_info("db_name5")->get().primary_node == 2);
    REQUIRE(stats.try_get_db_info("db_name6")->get().primary_node == 3);
}
