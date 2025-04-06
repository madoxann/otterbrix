project(distributed_plan CXX)

set(SOURCE_${PROJECT_NAME}
        meta/analyze.cpp
        meta/catalog.cpp
        meta/collection_info.cpp

        consistent_hashing/hash_ring.cpp
        consistent_hashing/cluster_hash_ring.cpp
        consistent_hashing/murmur3.cpp

        distributed_planner.cpp
        impl/plan_common_distribute.cpp
        impl/plan_collection.cpp
        impl/plan_database.cpp
        impl/plan_insert.cpp
        impl/plan_join.cpp
)

include_directories(${CMAKE_SOURCE_DIR})

add_library(otterbrix_${PROJECT_NAME} ${SOURCE_${PROJECT_NAME}})
add_library(otterbrix::${PROJECT_NAME} ALIAS otterbrix_${PROJECT_NAME})

set_property(TARGET otterbrix_${PROJECT_NAME} PROPERTY EXPORT_NAME ${PROJECT_NAME})

target_link_libraries(
    otterbrix_${PROJECT_NAME} PUBLIC
    otterbrix::sql
    magic_enum::magic_enum
    msgpackc-cxx
    absl::int128
)

target_include_directories(
    otterbrix_${PROJECT_NAME}
    PUBLIC
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-all")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-all")

#if (DEV_MODE)
    add_subdirectory(test)
#endif ()
