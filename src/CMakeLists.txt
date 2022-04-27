add_subdirectory(../third_party/argparse ${CMAKE_CURRENT_BINARY_DIR}/third_party/argparse)

set(TARGET k2-example)
add_executable(${TARGET})

target_sources(${TARGET}
    PRIVATE
        k2-example-app.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        k2
)


set(TARGET k2-register-task)
add_executable(${TARGET})

target_sources(${TARGET}
    PRIVATE
        k2-register-task.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        k2
        argparse::argparse
)

set(TARGET dev_t-to-internal)
add_executable(${TARGET})

target_sources(${TARGET}
        PRIVATE
        dev_t-to-internal.cpp
)
