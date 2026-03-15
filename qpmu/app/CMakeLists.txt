set(PROJECT_SOURCES
    src/app.h
    src/app.cpp
    src/flowlayout.h
    src/flowlayout.cpp
    src/main_window.h
    src/main_window.cpp
    src/phasor_monitor.h
    src/phasor_monitor.cpp
    src/main_page_interface.h
    src/data_observer.h
    src/data_observer.cpp
    src/data_processor.h
    src/data_processor.cpp
    src/settings_models.h
    src/settings_models.cpp
    src/settings_widget.h
    src/settings_widget.cpp
    src/main.cpp
    src/equally_scaled_axes_chart.h
    src/equally_scaled_axes_chart.cpp
    src/util.cpp
    img/.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${PROJECT_NAME}-app MANUAL_FINALIZATION ${PROJECT_SOURCES})
  # Define target properties for Android with Qt 6 as: set_property(TARGET
  # ${PROJECT_NAME}-app APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
  # ${CMAKE_CURRENT_SOURCE_DIR}/android) For more information, see
  # https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
  if(ANDROID)
    add_library(${PROJECT_NAME}-app SHARED ${PROJECT_SOURCES})
    # Define properties for Android with Qt 5 after find_package() calls as:
    # set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
  else()
    add_executable(${PROJECT_NAME}-app ${PROJECT_SOURCES})
  endif()
endif()

target_compile_definitions(
  ${PROJECT_NAME}-app
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(
  ${PROJECT_NAME}-app
  PRIVATE Qt${QT_VERSION_MAJOR}::Core
          Qt${QT_VERSION_MAJOR}::Widgets
          Qt${QT_VERSION_MAJOR}::Charts
          Qt${QT_VERSION_MAJOR}::Network
          fftw3
          ${PROJECT_NAME}-common
          ${PROJECT_NAME}-estimation
          StrongridDLL
          )

set_target_properties(
  ${PROJECT_NAME}-app
  PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME}-app)
endif()
