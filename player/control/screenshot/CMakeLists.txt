project(screenshot)

pkg_check_modules(DBUS REQUIRED dbus-1)

add_library(${PROJECT_NAME}
    PortalScreenShoter.cpp
    PortalScreenShoter.hpp
    ScreenShoter.cpp
    ScreenShoter.hpp
    ScreenShotInterval.cpp
    ScreenShotInterval.hpp
    ScreeShoterFactory.hpp
)

target_link_libraries(${PROJECT_NAME}
    common
    ${DBUS_LINK_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${DBUS_INCLUDE_DIRS}
)

if(GTK_UI)
    target_link_libraries(${PROJECT_NAME} screenshot_gtk)
    add_subdirectory(gtk)
endif(GTK_UI)
