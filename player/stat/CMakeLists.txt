project(stat)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
find_path(SQLITECPP_INCLUDE_DIR SQLiteCpp/SQLiteCpp.h)
find_library(SQLITECPP_LIBRARY SQLiteCpp)

add_library(${PROJECT_NAME}
    PlayingTime.hpp
    Recorder.cpp
    Recorder.hpp
    records/XmlFormatter.cpp
    records/XmlFormatter.hpp
    records/Record.hpp
    records/Record.cpp
    records/LayoutRecord.hpp
    records/LayoutRecord.cpp
    records/MediaRecord.hpp
    records/MediaRecord.cpp
    records/Records.hpp
    records/Container.hpp
    records/RecordType.cpp
    records/RecordType.hpp
    records/RecordVisitor.hpp
    storage/RecordDto.hpp
    storage/DtoConverter.cpp
    storage/DtoConverter.hpp
    storage/DataProvider.hpp
    storage/DatabaseProvider.cpp
    storage/DatabaseProvider.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE dt ${SQLITE3_LIBRARIES} SQLiteCpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${SQLITE3_INCLUDE_DIRS} ${SQLITECPP_INCLUDE_DIR})
