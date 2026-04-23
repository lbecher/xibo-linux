project(cache)

add_library(${PROJECT_NAME}
    UnsafeFaultCodes.hpp
    UnsafeItemStore.cpp
    UnsafeItemStore.hpp
)

target_link_libraries(${PROJECT_NAME}
    common
)
