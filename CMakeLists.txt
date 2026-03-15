cmake_minimum_required(VERSION 3.10)

set(QPMU_FLOAT_TYPE double)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if(${QPMU_FLOAT_TYPE} STREQUAL "float")
  message(STATUS "Using float as the floating point type")
  set(QPMU_PROJECT_NAME qpmuf)
elseif(${QPMU_FLOAT_TYPE} STREQUAL "long double")
  message(STATUS "Using long double as the floating point type")
  set(QPMU_PROJECT_NAME qpmul)
else()
  message(STATUS "Using double as the floating point type")
  set(QPMU_PROJECT_NAME qpmu)
endif()

project(
  ${QPMU_PROJECT_NAME}
  VERSION 0.1
  LANGUAGES CXX)

# Enable compiler warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find Qt
find_package(
  QT NAMES Qt6 Qt5
  COMPONENTS Core Widgets Charts Network
  REQUIRED)
find_package(
  Qt${QT_VERSION_MAJOR}
  COMPONENTS Core Widgets Charts Network
  REQUIRED)

# Find Boost
find_package(
  Boost 1.70.0
  COMPONENTS program_options
  REQUIRED)

# Find FFTW
find_library(FFTW3_LIB fftw3)

# Add third-party libraries
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external)

# Add the "common" library and link it to the project
set(COMMON_LIB ${PROJECT_NAME}-common)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qpmu/common)
target_compile_definitions(
  ${COMMON_LIB}
  PUBLIC FLOAT_TYPE=${QPMU_FLOAT_TYPE}
  PUBLIC PROJECT_ORG_NAME="CPS Lab - North South University, Bangladesh"
  PUBLIC PROJECT_APP_NAME="${PROJECT_NAME}"
  PUBLIC PROJECT_APP_DISPLAY_NAME="QPMU"
  PUBLIC PROJECT_ADC_BITS=12)
link_libraries(${COMMON_LIB})

# Add the remaining subdirectories
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qpmu/sampling)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qpmu/estimation)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qpmu/app)
