cmake_minimum_required(VERSION 3.16.3)

project(HTAP)

SET(CMAKE_CXX_FLAGS "-std=c++17")

add_subdirectory(lib/rocksdb)
add_subdirectory(src/cabindb)
add_subdirectory(src/test/ycsb)

include_directories(${CMAKE_SOURCE_DIR}/lib/rocksdb/include)
include_directories(/usr/include)

#set(ROCKSDB_INCLUDE_DIR /lib/rocksdb/include/rocksdb/)
#set(ROCKSDB_LIB_DIR /lib/rocksdb)

#target_include_directories(insert PUBLIC "${ROCKSDB_INCLUDE_DIR}")
#target_link_directories(insert PUBLIC "${ROCKSDB_LIB_DIR}")
