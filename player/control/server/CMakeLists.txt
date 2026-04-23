project(server)

add_library(${PROJECT_NAME}
    CriteriaController.cpp
    CriteriaController.hpp
    CriteriaRequest.hpp
    DurationController.cpp
    DurationController.hpp
    DurationRequest.hpp
    EmbeddedServer.cpp
    EmbeddedServer.hpp
    FaultController.cpp
    FaultController.hpp
    FaultRequest.hpp
    HookController.cpp
    HookController.hpp
    HttpTypes.hpp
    InfoController.cpp
    InfoController.hpp
    RestrictiveFileModule.cpp
    RestrictiveFileModule.hpp
    TriggerRequest.hpp
)

target_link_libraries(${PROJECT_NAME}
    common
)
