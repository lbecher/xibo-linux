project(xmr)

pkg_check_modules(ZMQ REQUIRED libzmq>=4.3.2)
pkg_check_modules(OPENSSL REQUIRED openssl)

add_library(${PROJECT_NAME}
    XmrManager.cpp
    XmrManager.hpp
    XmrStatus.hpp
    XmrChannel.hpp
    XmrChannel.cpp
    zmq/Context.cpp
    zmq/Exception.cpp
    zmq/Exception.hpp
    zmq/Message.cpp
    zmq/Message.hpp
    zmq/Socket.cpp
    zmq/Socket.hpp
    zmq/Subscriber.cpp
    zmq/Subscriber.hpp
    ws/Subscriber.cpp
    ws/Subscriber.hpp
)

target_link_libraries(${PROJECT_NAME}
    common
    ${ZMQ_LINK_LIBRARIES}
    ${OPENSSL_LINK_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${GTKMM_INCLUDE_DIRS} # TODO remove this workaround
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${ZMQ_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)
