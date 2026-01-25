cmake_minimum_required(VERSION 3.16)

project(RideSharingSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RideSharingSystem
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        riderwindow.h riderwindow.cpp
        citymapview.h citymapview.cpp
        core/city.h core/city.cpp
        core/driver.h core/driver.cpp
        core/rider.h core/rider.cpp
        core/trip.h core/trip.cpp
        core/dispatchengine.h core/dispatchengine.cpp
        core/rollbackmanager.h core/rollbackmanager.cpp
        core/ridesharesystem.h core/ridesharesystem.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET RideSharingSystem APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(RideSharingSystem SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(RideSharingSystem
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(RideSharingSystem PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia Qt${QT_VERSION_MAJOR}::MultimediaWidgets)

# ============ RESOURCE FILES COPYING ============
# Copy all resource files (images and videos) to build output directory
# This makes the app portable - resources are located relative to the executable

set(RESOURCE_FILES
    "${CMAKE_SOURCE_DIR}/user1.png"
    "${CMAKE_SOURCE_DIR}/Mask_group.png"
    "${CMAKE_SOURCE_DIR}/Group23.png"
    "${CMAKE_SOURCE_DIR}/preloader_ride_sharing_system.mp4"
)

# Copy resources to the build directory for development/testing
foreach(RESOURCE_FILE ${RESOURCE_FILES})
    if(EXISTS ${RESOURCE_FILE})
        add_custom_command(TARGET RideSharingSystem POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${RESOURCE_FILE}
            $<TARGET_FILE_DIR:RideSharingSystem>/
            COMMENT "Copying resource: ${RESOURCE_FILE}"
        )
    endif()
endforeach()

# ============ END RESOURCE FILES COPYING ============

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.RideSharingSystem)
endif()
set_target_properties(RideSharingSystem PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS RideSharingSystem
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RideSharingSystem)
endif()
