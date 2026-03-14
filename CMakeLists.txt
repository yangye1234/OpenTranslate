cmake_minimum_required(VERSION 3.16)

project(OpenTranslate VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "/Users/yy/Applications/qt/6.10.1/macos")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network LinguistTools)

include(FetchContent)
set(QT_DEFAULT_MAJOR_VERSION ${QT_VERSION_MAJOR} CACHE STRING "" FORCE)
FetchContent_Declare(
    qhotkey
    GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
    GIT_TAG 1.5.0
)
FetchContent_MakeAvailable(qhotkey)

set(TS_FILES OpenTranslate_zh_CN.ts)
set(APP_ICON_ICO "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app.ico")
set(APP_ICON_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app.icns")

set(PROJECT_SOURCES
        appconfig.h
        baidutranslatorservice.h
        baidutranslatorservice.cpp
        configstore.h
        configstore.cpp
        l10n.h
        l10n.cpp
        main.cpp
        settingswidget.cpp
        settingswidget.h
        settingswidget.ui
        translationcachestore.cpp
        translationcachestore.h
        translatorservice.h
        translate.cpp
        translate.h
        translate.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(OpenTranslate
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET OpenTranslate APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(OpenTranslate SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(OpenTranslate
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(OpenTranslate PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network QHotkey::QHotkey)

if(WIN32)
    if(EXISTS "${APP_ICON_ICO}")
        set(APP_ICON_ICO_RC "${APP_ICON_ICO}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/OpenTranslate.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/OpenTranslate.rc"
            @ONLY
        )
        target_sources(OpenTranslate PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/OpenTranslate.rc")
    else()
        message(WARNING "Windows icon not found: ${APP_ICON_ICO}. Run scripts/generate_app_icons.sh first.")
    endif()
endif()

if(APPLE)
    if(EXISTS "${APP_ICON_ICNS}")
        set_source_files_properties("${APP_ICON_ICNS}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(OpenTranslate PRIVATE "${APP_ICON_ICNS}")
        set_target_properties(OpenTranslate PROPERTIES MACOSX_BUNDLE_ICON_FILE "app.icns")
    else()
        message(WARNING "macOS icon not found: ${APP_ICON_ICNS}. Run scripts/generate_app_icons.sh first.")
    endif()
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.OpenTranslate)
endif()
set_target_properties(OpenTranslate PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS OpenTranslate
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    if(COMMAND qt_generate_deploy_app_script)
        qt_generate_deploy_app_script(
            TARGET OpenTranslate
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
        )
        install(SCRIPT ${deploy_script})
    endif()
endif()

set(CPACK_PACKAGE_NAME "OpenTranslate")
set(CPACK_PACKAGE_VENDOR "OpenTranslate")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "OpenTranslate")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "OpenTranslate")
    set(CPACK_NSIS_PACKAGE_NAME "OpenTranslate")
    if(EXISTS "${APP_ICON_ICO}")
        set(CPACK_NSIS_MUI_ICON "${APP_ICON_ICO}")
        set(CPACK_NSIS_MUI_UNIICON "${APP_ICON_ICO}")
    endif()
endif()

include(CPack)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(OpenTranslate)
endif()
