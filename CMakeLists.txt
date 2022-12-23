cmake_minimum_required(VERSION 3.16.3)

SET(CMAKE_CXX_FLAGS "-std=c++17")
set(EXTERNAL_LIB_HEADERS lib/rocksdb)

project(HTAP)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")
find_package(RocksDB 5.14 REQUIRED)

# Rocksdb
option(WITH_SYSTEM_ROCKSDB "require and build with system rocksdb" OFF)
if (WITH_SYSTEM_ROCKSDB)
  find_package(RocksDB 5.14 REQUIRED)
endif()

add_executable(insert src/test/insert.cc)
target_include_directories(insert PUBLIC /usr/include/rocksdb-7.8.3/include)
target_link_libraries(insert rocksdb)
