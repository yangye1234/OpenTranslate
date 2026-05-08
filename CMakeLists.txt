cmake_minimum_required(VERSION 3.16)

# CMake 4 defaults can reject old dependency policy compatibility in subprojects
# (e.g. fetched qhotkey). Keep compatibility floor explicitly for dependency configure.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
endif()

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

set(PROJECT_INCLUDE_DIRS
        src/audio
        src/config
        src/core
        src/ocr
        src/providers
        src/storage
        src/ui
)

set(PROJECT_SOURCES
        src/main.cpp
        src/audio/speechplayer.cpp
        src/audio/speechplayer.h
        src/config/appconfig.h
        src/config/configstore.cpp
        src/config/configstore.h
        src/core/l10n.cpp
        src/core/l10n.h
        src/core/languagedetector.cpp
        src/core/languagedetector.h
        src/core/translationresult.h
        src/ocr/ocrservice.cpp
        src/ocr/ocrservice.h
        src/ocr/screenshotoverlay.cpp
        src/ocr/screenshotoverlay.h
        src/providers/baidutranslatorservice.cpp
        src/providers/baidutranslatorservice.h
        src/providers/deepltranslatorservice.cpp
        src/providers/deepltranslatorservice.h
        src/providers/dictionarytranslatorservice.cpp
        src/providers/dictionarytranslatorservice.h
        src/providers/openaitranslatorservice.cpp
        src/providers/openaitranslatorservice.h
        src/providers/translatorservice.h
        src/storage/translationcachestore.cpp
        src/storage/translationcachestore.h
        src/storage/translationhistorystore.cpp
        src/storage/translationhistorystore.h
        src/ui/settingswidget.cpp
        src/ui/settingswidget.h
        src/ui/settingswidget.ui
        src/ui/translate.cpp
        src/ui/translate.h
        src/ui/translate.ui
        resources.qrc
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
target_include_directories(OpenTranslate PRIVATE ${PROJECT_INCLUDE_DIRS})

if(WIN32)
    target_sources(OpenTranslate PRIVATE src/ocr/windows/windowsocrservice.cpp)
    set(OPENTRANSLATE_CPPWINRT_INCLUDE_DIR "")
    set(_opentranslate_cppwinrt_candidates)
    if(DEFINED ENV{WindowsSdkDir})
        file(TO_CMAKE_PATH "$ENV{WindowsSdkDir}" _opentranslate_windows_sdk_dir)
        file(GLOB _opentranslate_env_cppwinrt "${_opentranslate_windows_sdk_dir}/Include/*/cppwinrt")
        list(APPEND _opentranslate_cppwinrt_candidates ${_opentranslate_env_cppwinrt})
    endif()
    file(GLOB _opentranslate_program_cppwinrt "C:/Program Files (x86)/Windows Kits/10/Include/*/cppwinrt")
    list(APPEND _opentranslate_cppwinrt_candidates ${_opentranslate_program_cppwinrt})
    if(_opentranslate_cppwinrt_candidates)
        list(SORT _opentranslate_cppwinrt_candidates COMPARE NATURAL ORDER DESCENDING)
        list(GET _opentranslate_cppwinrt_candidates 0 OPENTRANSLATE_CPPWINRT_INCLUDE_DIR)
        target_include_directories(OpenTranslate PRIVATE "${OPENTRANSLATE_CPPWINRT_INCLUDE_DIR}")
    endif()

    include(CheckIncludeFileCXX)
    set(_opentranslate_saved_required_includes "${CMAKE_REQUIRED_INCLUDES}")
    if(OPENTRANSLATE_CPPWINRT_INCLUDE_DIR)
        set(CMAKE_REQUIRED_INCLUDES "${OPENTRANSLATE_CPPWINRT_INCLUDE_DIR}")
    endif()
    check_include_file_cxx("winrt/Windows.Foundation.h" OPENTRANSLATE_HAS_CPPWINRT)
    set(CMAKE_REQUIRED_INCLUDES "${_opentranslate_saved_required_includes}")
    if(OPENTRANSLATE_HAS_CPPWINRT)
        target_compile_definitions(OpenTranslate PRIVATE OPENTRANSLATE_HAS_CPPWINRT=1)
        target_link_libraries(OpenTranslate PRIVATE windowsapp)
    else()
        message(WARNING "C++/WinRT headers were not found. Windows screenshot OCR will be disabled, but the app will still build.")
    endif()
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
    target_sources(OpenTranslate PRIVATE src/ocr/macos/macvisionocrservice.mm)
    set_source_files_properties(src/ocr/macos/macvisionocrservice.mm PROPERTIES
        LANGUAGE CXX
        COMPILE_FLAGS "-x objective-c++"
    )
    if(EXISTS "${APP_ICON_ICNS}")
        set_source_files_properties("${APP_ICON_ICNS}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(OpenTranslate PRIVATE "${APP_ICON_ICNS}")
        set_target_properties(OpenTranslate PROPERTIES MACOSX_BUNDLE_ICON_FILE "app.icns")
    else()
        message(WARNING "macOS icon not found: ${APP_ICON_ICNS}. Run scripts/generate_app_icons.sh first.")
    endif()
    target_link_libraries(OpenTranslate PRIVATE
        "-framework ApplicationServices"
        "-framework AppKit"
        "-framework CoreGraphics"
        "-framework Foundation"
        "-framework Vision"
    )
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
