set(TARGET k2)
add_library(${TARGET} STATIC)

target_sources(${TARGET}
    PRIVATE
        libk2.cpp
        ionice.cpp
)

target_include_directories(${TARGET}
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../k2-scheduler>
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)