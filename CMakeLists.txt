cmake_minimum_required(VERSION 3.5)

project(pg_ulid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(PostgreSQL REQUIRED)
find_package(Postgres REQUIRED)

include_directories(SYSTEM ${PostgreSQL_INCLUDE_SERVERDIRS})

add_library(ulid SHARED src/ulid.cpp)
target_link_libraries(ulid PostgreSQL::PostgreSQL)

string(REPLACE "/usr/" "" PostgreSQL_SHAREDIR ${PostgreSQL_SHAREDIRS})
string(REPLACE "/usr/" "" PostgreSQL_PKGLIBDIR ${PostgreSQL_PKGLIBDIRS})

install(TARGETS ulid
        LIBRARY DESTINATION ${PostgreSQL_PKGLIBDIRS})

install(FILES
        sql/ulid--1.0--1.1.sql
        sql/ulid--1.1.sql
        sql/ulid.control
        DESTINATION ${PostgreSQL_SHAREDIR}/extension)
