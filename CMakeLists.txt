cmake_minimum_required(VERSION 3.16)
project(numeric-engine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# GMP manual include and link (since find_package doesn't work on Windows)
include_directories("C:/vcpkg/installed/x64-windows/include")
link_directories("C:/vcpkg/installed/x64-windows/lib")

# --- Executable definition (cross-platform) ---
if (WIN32)
    # Windows: suppress console window in GUI apps
    qt_add_executable(${PROJECT_NAME} WIN32
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        converter.cpp
        converter.h
        converter.ui
        settings.cpp
        settings.h
        settings.ui
        help.cpp
        help.h
        help.ui
    )
elseif (APPLE)
    # macOS: make a proper .app bundle
    qt_add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        converter.cpp
        converter.h
        converter.ui
        settings.cpp
        settings.h
        settings.ui
        help.cpp
        help.h
        help.ui
    )
else ()
    # Linux / other platforms
    qt_add_executable(${PROJECT_NAME}
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        converter.cpp
        converter.h
        converter.ui
        settings.cpp
        settings.h
        settings.ui
        help.cpp
        help.h
        help.ui
    )
endif ()

# --- Link libraries AFTER the target is created
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    gmp
    gmpxx
)
