project(screenshot_gtk)

add_library(${PROJECT_NAME}
    GdkScreenShoter.cpp
    GdkScreenShoter.hpp
)

target_link_libraries(${PROJECT_NAME}
    widgets_gtk
    ${GTKMM_LIBRARIES}
)
