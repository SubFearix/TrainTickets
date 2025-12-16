cmake_minimum_required(VERSION 3.16)
project(TrainTicketsServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Network
    Sql
    PrintSupport
    Gui
)
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Qt6 Core found: ${Qt6Core_FOUND}")
message(STATUS "Qt6 Network found: ${Qt6Network_FOUND}")
message(STATUS "Qt6 Sql found: ${Qt6Sql_FOUND}")

set(SERVER_SOURCES
    main.cpp
    apiserver.cpp
    apiserver.h
    database.cpp
    database.h
    config.h
)

add_executable(TrainTicketsServer ${SERVER_SOURCES}
    emailconfig.h
    pdfgenerator.h pdfgenerator.cpp)

file(GLOB SMTP_SOURCES
    ../libs/SmtpClient-for-Qt/src/*.cpp
    ../libs/SmtpClient-for-Qt/src/*.h
)

target_sources(TrainTicketsServer PRIVATE ${SMTP_SOURCES})

target_include_directories(TrainTicketsServer PRIVATE
    ../libs/SmtpClient-for-Qt/src
)

target_link_libraries(TrainTicketsServer PRIVATE 
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::PrintSupport
    Qt6::Gui
)


if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config 
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    message(STATUS "Config files copied to build directory")
endif()

install(TARGETS TrainTicketsServer
    RUNTIME DESTINATION bin
)
