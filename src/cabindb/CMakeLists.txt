set(cabindb_srcs
    Dstore.cc
)

list(APPEND cabindb_srcs rocksdb-rados-env/env_librados.cc)

add_library(cabindb STATIC ${cabindb_srcs})
target_include_directories(cabindb PUBLIC ${CMAKE_SOURCE_DIR}/lib/rocksdb/include)
target_include_directories(cabindb PUBLIC ${CMAKE_SOURCE_DIR}/lib/rocksdb)
target_include_directories(cabindb PUBLIC /usr/include)

target_link_libraries(cabindb
                      rocksdb
                      rados)
