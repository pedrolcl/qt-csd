cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0092 NEW)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version" FORCE)
    find_program(BREW_EXECUTABLE brew HINTS /usr/local/bin)
    if ("${BREW_EXECUTABLE}" STREQUAL "BREW_EXECUTABLE-NOTFOUND")
        message(STATUS "macOS: Homebrew not found.")
    else ()
        message(STATUS "macOS: Homebrew found at ${BREW_EXECUTABLE}.")
        execute_process(COMMAND ${BREW_EXECUTABLE} --prefix qt OUTPUT_VARIABLE QT_PREFIX_PATH)
        string(REGEX REPLACE "\n$" "" QT_PREFIX_PATH_STRIPPED "${QT_PREFIX_PATH}")
        if ("${QT_PREFIX_PATH}" STREQUAL "")
            message(STATUS "macOS: Homebrew: Qt not installed.")
        else ()
            list(APPEND CMAKE_PREFIX_PATH "${QT_PREFIX_PATH_STRIPPED}")
            message(STATUS "macOS: Homebrew Qt at ${QT_PREFIX_PATH_STRIPPED}.")
        endif ()
    endif ()
endif ()

project ( qt-csd
    LANGUAGES CXX
    VERSION 0.1.0
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)
find_package(QT NAMES Qt5 Qt6 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets REQUIRED)

add_executable(${PROJECT_NAME} WIN32
    captionbuttonstyle.h
    csd.qrc
    csdmetatype.h
    csdtitlebar.cpp
    csdtitlebar.h
    csdtitlebarbutton.cpp
    csdtitlebarbutton.h
    main.cpp
)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" AND NOT MSVC)
    list(APPEND COMPILER_WARNINGS
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-compat-pedantic"
        "-Wno-padded"
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list(APPEND COMPILER_WARNINGS
        "-Wall"
        "-Wcast-align"
        "-Wcast-qual"
        "-Wconversion"
        "-Wctor-dtor-privacy"
        "-Wdisabled-optimization"
        "-Wdouble-promotion"
        "-Wduplicated-branches"
        "-Wduplicated-cond"
        "-Wextra"
        "-Wformat=2"
        "-Winvalid-pch"
        "-Wlogical-op"
        "-Wmisleading-indentation"
        "-Wmissing-include-dirs"
        "-Wnoexcept"
        "-Wnon-virtual-dtor"
        "-Wnull-dereference"
        "-Wold-style-cast"
        "-Woverloaded-virtual"
        "-Wpedantic"
        "-Wpointer-arith"
        "-Wredundant-decls"
        "-Wshadow"
        "-Wshift-overflow=2"
        "-Wsign-conversion"
        "-Wsized-deallocation"
        "-Wtrampolines"
        "-Wundef"
        "-Wunused"
        "-Wuseless-cast"
        "-Wvector-operation-performance"
        "-Wwrite-strings"
        "-Wzero-as-null-pointer-constant"
        "-pedantic-errors"
        )
else ()
    foreach (flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
    endforeach()
    list(APPEND COMPILER_WARNINGS
        "/W4"
    )
endif ()
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILER_WARNINGS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

if (APPLE)
    find_library(LIBOBJC "objc")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${LIBOBJC}
    )
endif()

if (UNIX)

    target_sources( ${PROJECT_NAME} PRIVATE
        linuxcsd.cpp
        linuxcsd.h
    )

elseif (WIN32) # Windows

    target_sources(${PROJECT_NAME} PRIVATE
        qregistrywatcher.cpp
        qregistrywatcher.h
        qtwinbackports.cpp
        qtwinbackports.h
        win32csd.cpp
        win32csd.h
    )

    #find_library(DWMAPI "dwmapi" REQUIRED)
    
    find_package(Qt${QT_VERSION_MAJOR}Gui)
    include_directories(${Qt${QT_VERSION_MAJOR}Gui_PRIVATE_INCLUDE_DIRS})
    
    target_compile_definitions(${PROJECT_NAME} PUBLIC WIN32_LEAN_AND_MEAN)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC UNICODE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dwmapi )

endif ()
