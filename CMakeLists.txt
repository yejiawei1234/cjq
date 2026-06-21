cmake_minimum_required(VERSION 3.28)
project(cjq C)
option(STATIC_BUILD "Build static executable" OFF)

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
        src/tracker_split.c
        src/tracker_split.h
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

if(STATIC_BUILD)
    target_link_options(cjq PRIVATE -static)
endif ()