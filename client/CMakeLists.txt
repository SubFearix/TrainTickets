cmake_minimum_required(VERSION 3.16)
project(TrainTicketsClient VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "/home/subfear/qthttpserver/build")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets
    Network
)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Qt6 Core found: ${Qt6Core_FOUND}")
message(STATUS "Qt6 Widgets found: ${Qt6Widgets_FOUND}")
message(STATUS "Qt6 Network found: ${Qt6Network_FOUND}")

set(CLIENT_SOURCES
    main.cpp
    registration.cpp
    registration.h
    registration.ui
    verification.cpp
    verification.h
    verification.ui
    apiclient.cpp
    apiclient.h
)

add_executable(TrainTicketsClient ${CLIENT_SOURCES}
    config.h
    trainlistwidget.h trainlistwidget.cpp
    seatselectionwidget.h seatselectionwidget.cpp
    mainmenuwidget.h mainmenuwidget.cpp
    bookingwidget.h bookingwidget.cpp
    mainwindow.h mainwindow.cpp mainwindow.ui
    loginwidget.h loginwidget.cpp
    myticketswidget.h myticketswidget.cpp
    carriagewidget.h carriagewidget.cpp
    profilewidget.h profilewidget.cpp
)

target_link_libraries(TrainTicketsClient PRIVATE 
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)
target_link_libraries(TrainTicketsClient PRIVATE Qt6::Widgets)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icons)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/icons 
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    message(STATUS "Icons copied to build directory")
endif()

install(TARGETS TrainTicketsClient
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

