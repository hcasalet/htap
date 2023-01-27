set(ycsb_srcs
    ycsbc.cc
    db/db_factory.cc
    db/cabindb_db.cc
    core/core_workload.cc)
add_executable(ycsb_test ${ycsb_srcs})
target_include_directories(ycsb_test PUBLIC ${CMAKE_SOURCE_DIR}/src/test/ycsb)
target_include_directories(ycsb_test PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(
    ycsb_test
    cabindb
)