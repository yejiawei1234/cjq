cmake_minimum_required(VERSION 3.28)
project(cjq C)

set(CMAKE_C_STANDARD 17)

add_subdirectory(
        third_party/argtable3
)

add_executable(
        cjq

        src/main.c
        src/cli.c
        src/rule.c
        src/traversal.c
        src/extractor.c
        src/progress.c
        src/writer.c

        third_party/yyjson/src/yyjson.c
        src/writer.h
        src/time_field.c
        src/time_field.h
)

target_include_directories(
        cjq
        PRIVATE
        third_party/yyjson/src
)

target_link_libraries(
        cjq
        PRIVATE
        argtable3
)

target_link_options(cjq PRIVATE -static)