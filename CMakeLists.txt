cmake_minimum_required(VERSION 3.20.0)

project(
    QmlOffscreenRenderer
    VERSION 1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(
    Qt6
    COMPONENTS Quick
               Widgets
               Gui
               Concurrent
    REQUIRED)


set(SOURCES
    # cmake-format: sort
    movierenderer.cpp
    animationdriver.cpp)

set(HEADER
    # cmake-format: sort    
    movierenderer.h 
    animationdriver.h )

set(QML
    # cmake-format: sort
    main.qml)

qt_add_library(${PROJECT_NAME} STATIC)
qt_add_qml_module(
    ${PROJECT_NAME}
    URI
    ${PROJECT_NAME}
    VERSION
    1.0
    QML_FILES
    ${QML}
    SOURCES
    ${SOURCES}
    ${HEADER})

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC 
    Qt6::Core
    Qt6::Quick
    Qt6::Widgets
    Qt6::Gui
    Qt6::Concurrent)
    
add_executable(${PROJECT_NAME}Test main.cpp)
target_link_libraries(
    ${PROJECT_NAME}Test
    PRIVATE 
    ${PROJECT_NAME}plugin 
    Qt6::Gui 
    Qt6::Core
    Qt6::Quick
    Qt6::Widgets)
